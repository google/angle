//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// PoolAlloc.h:
//    Defines the class interface for PoolAllocator and the Allocation
//    class that it uses internally.
//

#ifndef POOLALLOC_H_
#define POOLALLOC_H_

#ifdef _DEBUG
#    define GUARD_BLOCKS  // define to enable guard block sanity checking
#endif

//
// This header defines an allocator that can be used to efficiently
// allocate a large number of small requests for heap memory, with the
// intention that they are not individually deallocated, but rather
// collectively deallocated at one time.
//
// This simultaneously
//
// * Makes each individual allocation much more efficient; the
//     typical allocation is trivial.
// * Completely avoids the cost of doing individual deallocation.
// * Saves the trouble of tracking down and plugging a large class of leaks.
//
// Individual classes can use this allocator by supplying their own
// new and delete methods.
//

#include <stddef.h>
#include <string.h>
#include <vector>

#include "angleutils.h"

namespace angle
{
// If we are using guard blocks, we must track each individual
// allocation.  If we aren't using guard blocks, these
// never get instantiated, so won't have any impact.
//

class Allocation
{
  public:
    Allocation(size_t size, unsigned char *mem, Allocation *prev = 0)
        : size(size), mem(mem), prevAlloc(prev)
    {
// Allocations are bracketed:
//    [allocationHeader][initialGuardBlock][userData][finalGuardBlock]
// This would be cleaner with if (kGuardBlockSize)..., but that
// makes the compiler print warnings about 0 length memsets,
// even with the if() protecting them.
#ifdef GUARD_BLOCKS
        memset(preGuard(), kGuardBlockBeginVal, kGuardBlockSize);
        memset(data(), kUserDataFill, size);
        memset(postGuard(), kGuardBlockEndVal, kGuardBlockSize);
#endif
    }

    void check() const
    {
        checkGuardBlock(preGuard(), kGuardBlockBeginVal, "before");
        checkGuardBlock(postGuard(), kGuardBlockEndVal, "after");
    }

    void checkAllocList() const;

    // Return total size needed to accomodate user buffer of 'size',
    // plus our tracking data.
    inline static size_t allocationSize(size_t size)
    {
        return size + 2 * kGuardBlockSize + headerSize();
    }

    // Offset from surrounding buffer to get to user data buffer.
    inline static unsigned char *offsetAllocation(unsigned char *m)
    {
        return m + kGuardBlockSize + headerSize();
    }

    // Return size of allocation.
    size_t getSize() const { return size; }
    // Set size of allocation
    void setSize(size_t newSize)
    {
        size = newSize;
#ifdef GUARD_BLOCKS
        // Push post-guard block back now that size is updated
        memset(postGuard(), kGuardBlockEndVal, kGuardBlockSize);
#endif
    }

  private:
    void checkGuardBlock(unsigned char *blockMem, unsigned char val, const char *locText) const;

    // Find offsets to pre and post guard blocks, and user data buffer
    unsigned char *preGuard() const { return mem + headerSize(); }
    unsigned char *data() const { return preGuard() + kGuardBlockSize; }
    unsigned char *postGuard() const { return data() + size; }

    size_t size;            // size of the user data area
    unsigned char *mem;     // beginning of our allocation (pts to header)
    Allocation *prevAlloc;  // prior allocation in the chain

    static constexpr unsigned char kGuardBlockBeginVal = 0xfb;
    static constexpr unsigned char kGuardBlockEndVal   = 0xfe;
    static constexpr unsigned char kUserDataFill       = 0xcd;
#ifdef GUARD_BLOCKS
    static constexpr size_t kGuardBlockSize = 16;
#else
    static constexpr size_t kGuardBlockSize = 0;
#endif

    inline static size_t headerSize() { return sizeof(Allocation); }
};

//
// There are several stacks.  One is to track the pushing and popping
// of the user, and not yet implemented.  The others are simply a
// repositories of free pages or used pages.
//
// Page stacks are linked together with a simple header at the beginning
// of each allocation obtained from the underlying OS.  Multi-page allocations
// are returned to the OS.  Individual page allocations are kept for future
// re-use.
//
// The "page size" used is not, nor must it match, the underlying OS
// page size.  But, having it be about that size or equal to a set of
// pages is likely most optimal.
//
class PoolAllocator : angle::NonCopyable
{
  public:
    static const int kDefaultAlignment = 16;
    PoolAllocator(int growthIncrement = 8 * 1024, int allocationAlignment = kDefaultAlignment);
    PoolAllocator(PoolAllocator &&rhs) noexcept;
    PoolAllocator &operator=(PoolAllocator &&);

    //
    // Don't call the destructor just to free up the memory, call pop()
    //
    ~PoolAllocator();

    //
    // Call push() to establish a new place to pop memory to.  Does not
    // have to be called to get things started.
    //
    void push();

    //
    // Call pop() to free all memory allocated since the last call to push(),
    // or if no last call to push, frees all memory since first allocation.
    //
    void pop();

    //
    // Call popAll() to free all memory allocated.
    //
    void popAll();

    //
    // Call allocate() to actually acquire memory.  Returns 0 if no memory
    // available, otherwise a properly aligned pointer to 'numBytes' of memory.
    //
    void *allocate(size_t numBytes);

    //
    // Call reallocate() to resize a previous allocation.  Returns 0 if no memory
    // available, otherwise a properly aligned pointer to 'numBytes' of memory
    // where any contents from the original allocation will be preserved.
    //
    void *reallocate(void *originalAllocation, size_t numBytes);

    //
    // There is no deallocate.  The point of this class is that
    // deallocation can be skipped by the user of it, as the model
    // of use is to simultaneously deallocate everything at once
    // by calling pop(), and to not have to solve memory leak problems.
    //

    // Catch unwanted allocations.
    // TODO(jmadill): Remove this when we remove the global allocator.
    void lock();
    void unlock();

  private:
    size_t alignment;  // all returned allocations will be aligned at
                       // this granularity, which will be a power of 2
    size_t alignmentMask;

#if !defined(ANGLE_DISABLE_POOL_ALLOC)
    friend struct tHeader;

    struct tHeader
    {
        tHeader(tHeader *nextPage, size_t pageCount)
            : nextPage(nextPage),
              pageCount(pageCount)
#    ifdef GUARD_BLOCKS
              ,
              lastAllocation(0)
#    endif
        {}

        ~tHeader()
        {
#    ifdef GUARD_BLOCKS
            if (lastAllocation)
                lastAllocation->checkAllocList();
#    endif
        }

        tHeader *nextPage;
        size_t pageCount;
        Allocation *lastAllocation;
    };

    struct tAllocState
    {
        size_t offset;
        tHeader *page;
    };
    typedef std::vector<tAllocState> tAllocStack;

    // Track allocations if and only if we're using guard blocks
    void *initializeAllocation(tHeader *block, unsigned char *memory, size_t numBytes)
    {
        // Init Allocation by default for reallocation support.
        new (memory) Allocation(numBytes, memory, block->lastAllocation);
        block->lastAllocation = reinterpret_cast<Allocation *>(memory);
        return Allocation::offsetAllocation(memory);
    }

    Allocation *getAllocationHeader(void *memAllocation) const;

    size_t pageSize;           // granularity of allocation from the OS
    size_t headerSkip;         // amount of memory to skip to make room for the
                               //      header (basically, size of header, rounded
                               //      up to make it aligned
    size_t currentPageOffset;  // next offset in top of inUseList to allocate from
    tHeader *freeList;         // list of popped memory
    tHeader *inUseList;        // list of all memory currently being used
    tAllocStack mStack;        // stack of where to allocate from, to partition pool

    int numCalls;       // just an interesting statistic
    size_t totalBytes;  // just an interesting statistic

#else  // !defined(ANGLE_DISABLE_POOL_ALLOC)
    std::vector<std::vector<void *>> mStack;
#endif

    bool mLocked;
};

}  // namespace angle

#endif  // POOLALLOC_H_
