//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// PoolAlloc.cpp:
//    Implements the class methods for PoolAllocator and Allocation classes.
//

#include "common/PoolAlloc.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "common/angleutils.h"
#include "common/debug.h"
#include "common/platform.h"
#include "common/tls.h"

namespace angle
{

//
// Implement the functionality of the PoolAllocator class, which
// is documented in PoolAlloc.h.
//
PoolAllocator::PoolAllocator(int growthIncrement, int allocationAlignment)
    : alignment(allocationAlignment),
#if !defined(ANGLE_DISABLE_POOL_ALLOC)
      pageSize(growthIncrement),
      freeList(0),
      inUseList(0),
      numCalls(0),
      totalBytes(0),
#endif
      mLocked(false)
{
    //
    // Adjust alignment to be at least pointer aligned and
    // power of 2.
    //
    size_t minAlign = sizeof(void *);
    alignment &= ~(minAlign - 1);
    if (alignment < minAlign)
        alignment = minAlign;
    size_t a = 1;
    while (a < alignment)
        a <<= 1;
    alignment     = a;
    alignmentMask = a - 1;

#if !defined(ANGLE_DISABLE_POOL_ALLOC)
    //
    // Don't allow page sizes we know are smaller than all common
    // OS page sizes.
    //
    if (pageSize < 4 * 1024)
        pageSize = 4 * 1024;

    //
    // A large currentPageOffset indicates a new page needs to
    // be obtained to allocate memory.
    //
    currentPageOffset = pageSize;

    //
    // Align header skip
    //
    headerSkip = minAlign;
    if (headerSkip < sizeof(tHeader))
    {
        headerSkip = (sizeof(tHeader) + alignmentMask) & ~alignmentMask;
    }
#else  // !defined(ANGLE_DISABLE_POOL_ALLOC)
    mStack.push_back({});
#endif
}

PoolAllocator::PoolAllocator(PoolAllocator &&rhs) noexcept
    : alignment(std::exchange(rhs.alignment, 0)),
      alignmentMask(std::exchange(rhs.alignmentMask, 0)),
#if !defined(ANGLE_DISABLE_POOL_ALLOC)
      pageSize(std::exchange(rhs.pageSize, 0)),
      headerSkip(std::exchange(rhs.headerSkip, 0)),
      currentPageOffset(std::exchange(rhs.currentPageOffset, 0)),
      freeList(std::exchange(rhs.freeList, nullptr)),
      inUseList(std::exchange(rhs.inUseList, nullptr)),
      mStack(std::move(rhs.mStack)),
      numCalls(std::exchange(rhs.numCalls, 0)),
      totalBytes(std::exchange(rhs.totalBytes, 0)),
#else
      mStack(std::move(rhs.mStack)),
#endif
      mLocked(std::exchange(rhs.mLocked, false))
{}

PoolAllocator &PoolAllocator::operator=(PoolAllocator &&rhs)
{
    if (this != &rhs)
    {
        std::swap(alignment, rhs.alignment);
        std::swap(alignmentMask, rhs.alignmentMask);
#if !defined(ANGLE_DISABLE_POOL_ALLOC)
        std::swap(pageSize, rhs.pageSize);
        std::swap(headerSkip, rhs.headerSkip);
        std::swap(currentPageOffset, rhs.currentPageOffset);
        std::swap(freeList, rhs.freeList);
        std::swap(inUseList, rhs.inUseList);
        std::swap(numCalls, rhs.numCalls);
        std::swap(totalBytes, rhs.totalBytes);
#endif
        std::swap(mStack, rhs.mStack);
        std::swap(mLocked, rhs.mLocked);
    }
    return *this;
}

PoolAllocator::~PoolAllocator()
{
#if !defined(ANGLE_DISABLE_POOL_ALLOC)
    while (inUseList)
    {
        tHeader *next = inUseList->nextPage;
        inUseList->~tHeader();
        delete[] reinterpret_cast<char *>(inUseList);
        inUseList = next;
    }

    // We should not check the guard blocks
    // here, because we did it already when the block was
    // placed into the free list.
    //
    while (freeList)
    {
        tHeader *next = freeList->nextPage;
        delete[] reinterpret_cast<char *>(freeList);
        freeList = next;
    }
#else  // !defined(ANGLE_DISABLE_POOL_ALLOC)
    for (auto &allocs : mStack)
    {
        for (auto alloc : allocs)
        {
            free(alloc);
        }
    }
    mStack.clear();
#endif
}

//
// Check a single guard block for damage
//
void Allocation::checkGuardBlock(unsigned char *blockMem,
                                 unsigned char val,
                                 const char *locText) const
{
#ifdef GUARD_BLOCKS
    for (size_t x = 0; x < kGuardBlockSize; x++)
    {
        if (blockMem[x] != val)
        {
            char assertMsg[80];
            // We don't print the assert message.  It's here just to be helpful.
            snprintf(assertMsg, sizeof(assertMsg),
                     "PoolAlloc: Damage %s %zu byte allocation at 0x%p\n", locText, size, data());

            assert(0 && "PoolAlloc: Damage in guard block");
        }
    }
#endif
}

void PoolAllocator::push()
{
#if !defined(ANGLE_DISABLE_POOL_ALLOC)
    tAllocState state = {currentPageOffset, inUseList};

    mStack.push_back(state);

    //
    // Indicate there is no current page to allocate from.
    //
    currentPageOffset = pageSize;
#else  // !defined(ANGLE_DISABLE_POOL_ALLOC)
    mStack.push_back({});
#endif
}

//
// Do a mass-deallocation of all the individual allocations
// that have occurred since the last push(), or since the
// last pop(), or since the object's creation.
//
// The deallocated pages are saved for future allocations.
//
void PoolAllocator::pop()
{
    if (mStack.size() < 1)
        return;

#if !defined(ANGLE_DISABLE_POOL_ALLOC)
    tHeader *page     = mStack.back().page;
    currentPageOffset = mStack.back().offset;

    while (inUseList != page)
    {
        // invoke destructor to free allocation list
        inUseList->~tHeader();

        tHeader *nextInUse = inUseList->nextPage;
        if (inUseList->pageCount > 1)
            delete[] reinterpret_cast<char *>(inUseList);
        else
        {
            inUseList->nextPage = freeList;
            freeList            = inUseList;
        }
        inUseList = nextInUse;
    }

    mStack.pop_back();
#else  // !defined(ANGLE_DISABLE_POOL_ALLOC)
    for (auto &alloc : mStack.back())
    {
        free(alloc);
    }
    mStack.pop_back();
#endif
}

//
// Do a mass-deallocation of all the individual allocations
// that have occurred.
//
void PoolAllocator::popAll()
{
    while (mStack.size() > 0)
        pop();
}

//
// Return a pointer to the Allocation Header for an existing memAllocation.
// Pre-condition: memAllocation must be non-null
//
Allocation *PoolAllocator::getAllocationHeader(void *memAllocation) const
{
    ASSERT(memAllocation != nullptr);
    uint8_t *origAllocAddress = static_cast<uint8_t *>(memAllocation);
    return reinterpret_cast<Allocation *>(origAllocAddress - sizeof(Allocation));
}

//
// Do a reallocation, resizing the the given originalAllocation to numBytes while
// preserving the contents of originalAllocation.
//
void *PoolAllocator::reallocate(void *originalAllocation, size_t numBytes)
{
    if (originalAllocation == nullptr)
    {
        return allocate(numBytes);
    }
    if (numBytes == 0)
    {
        // this is a no-op given the current way we use new pool allocators. Memory will be freed
        // when allocator is destroyed.
        return nullptr;
    }

    // Compare the original allocation size to requested realloc size
    Allocation *origAllocationHeader = getAllocationHeader(originalAllocation);
    size_t origSize                  = origAllocationHeader->getSize();
    if (origSize > numBytes)
    {
        // For growing allocation, create new allocation and copy over original contents
        void *newAlloc = allocate(numBytes);
        memcpy(newAlloc, originalAllocation, origSize);
        return newAlloc;
    }
    // For shrinking allocation, shrink size and return original alloc ptr
    origAllocationHeader->setSize(numBytes);
    return originalAllocation;
}

void *PoolAllocator::allocate(size_t numBytes)
{
    ASSERT(!mLocked);

#if !defined(ANGLE_DISABLE_POOL_ALLOC)
    //
    // Just keep some interesting statistics.
    //
    ++numCalls;
    totalBytes += numBytes;

    // If we are using guard blocks, all allocations are bracketed by
    // them: [guardblock][allocation][guardblock].  numBytes is how
    // much memory the caller asked for.  allocationSize is the total
    // size including guard blocks.  In release build,
    // kGuardBlockSize=0 and this all gets optimized away.
    size_t allocationSize = Allocation::allocationSize(numBytes);
    // Detect integer overflow.
    if (allocationSize < numBytes)
        return 0;

    //
    // Do the allocation, most likely case first, for efficiency.
    // This step could be moved to be inline sometime.
    //
    if (allocationSize <= pageSize - currentPageOffset)
    {
        //
        // Safe to allocate from currentPageOffset.
        //
        unsigned char *memory = reinterpret_cast<unsigned char *>(inUseList) + currentPageOffset;
        currentPageOffset += allocationSize;
        currentPageOffset = (currentPageOffset + alignmentMask) & ~alignmentMask;

        return initializeAllocation(inUseList, memory, numBytes);
    }

    if (allocationSize > pageSize - headerSkip)
    {
        //
        // Do a multi-page allocation.  Don't mix these with the others.
        // The OS is efficient in allocating and freeing multiple pages.
        //
        size_t numBytesToAlloc = allocationSize + headerSkip;
        // Detect integer overflow.
        if (numBytesToAlloc < allocationSize)
            return 0;

        tHeader *memory = reinterpret_cast<tHeader *>(::new char[numBytesToAlloc]);
        if (memory == 0)
            return 0;

        // Use placement-new to initialize header
        new (memory) tHeader(inUseList, (numBytesToAlloc + pageSize - 1) / pageSize);
        inUseList = memory;

        currentPageOffset = pageSize;  // make next allocation come from a new page

        // No guard blocks for multi-page allocations (yet)
        return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(memory) + headerSkip);
    }

    //
    // Need a simple page to allocate from.
    //
    tHeader *memory;
    if (freeList)
    {
        memory   = freeList;
        freeList = freeList->nextPage;
    }
    else
    {
        memory = reinterpret_cast<tHeader *>(::new char[pageSize]);
        if (memory == 0)
            return 0;
    }

    // Use placement-new to initialize header
    new (memory) tHeader(inUseList, 1);
    inUseList = memory;

    unsigned char *ret = reinterpret_cast<unsigned char *>(inUseList) + headerSkip;
    currentPageOffset  = (headerSkip + allocationSize + alignmentMask) & ~alignmentMask;

    return initializeAllocation(inUseList, ret, numBytes);
#else  // !defined(ANGLE_DISABLE_POOL_ALLOC)
    void *alloc = malloc(numBytes + alignmentMask);
    mStack.back().push_back(alloc);

    intptr_t intAlloc = reinterpret_cast<intptr_t>(alloc);
    intAlloc = (intAlloc + alignmentMask) & ~alignmentMask;
    return reinterpret_cast<void *>(intAlloc);
#endif
}

void PoolAllocator::lock()
{
    ASSERT(!mLocked);
    mLocked = true;
}

void PoolAllocator::unlock()
{
    ASSERT(mLocked);
    mLocked = false;
}

//
// Check all allocations in a list for damage by calling check on each.
//
void Allocation::checkAllocList() const
{
    for (const Allocation *alloc = this; alloc != 0; alloc = alloc->prevAlloc)
        alloc->check();
}

}  // namespace angle