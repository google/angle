//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _COMMON_INCLUDED_
#define _COMMON_INCLUDED_

#include <assert.h>
#include <stdio.h>

#include <map>
#include <string>
#include <vector>

#include "compiler/PoolAlloc.h"

typedef int TSourceLoc;

//
// Put POOL_ALLOCATOR_NEW_DELETE in base classes to make them use this scheme.
//
#define POOL_ALLOCATOR_NEW_DELETE(A)                                  \
    void* operator new(size_t s) { return (A).allocate(s); }          \
    void* operator new(size_t, void *_Where) { return (_Where);	}     \
    void operator delete(void*) { }                                   \
    void operator delete(void *, void *) { }                          \
    void* operator new[](size_t s) { return (A).allocate(s); }        \
    void* operator new[](size_t, void *_Where) { return (_Where);	} \
    void operator delete[](void*) { }                                 \
    void operator delete[](void *, void *) { }

//
// Pool version of string.
//
typedef pool_allocator<char> TStringAllocator;
typedef std::basic_string <char, std::char_traits<char>, TStringAllocator > TString;
inline TString* NewPoolTString(const char* s)
{
	void* memory = GlobalPoolAllocator.allocate(sizeof(TString));
	return new(memory) TString(s);
}

//
// Pool allocator versions of vectors, lists, and maps
//
template <class T> class TVector : public std::vector<T, pool_allocator<T> > {
public:
    typedef typename std::vector<T, pool_allocator<T> >::size_type size_type;
    TVector() : std::vector<T, pool_allocator<T> >() {}
    TVector(const pool_allocator<T>& a) : std::vector<T, pool_allocator<T> >(a) {}
    TVector(size_type i): std::vector<T, pool_allocator<T> >(i) {}
};

template <class K, class D, class CMP = std::less<K> > 
class TMap : public std::map<K, D, CMP, pool_allocator<std::pair<const K, D> > > {
public:
    typedef pool_allocator<std::pair<const K, D> > tAllocator;

    TMap() : std::map<K, D, CMP, tAllocator>() {}
    // use correct two-stage name lookup supported in gcc 3.4 and above
    TMap(const tAllocator& a) : std::map<K, D, CMP, tAllocator>(std::map<K, D, CMP, tAllocator>::key_compare(), a) {}
};

//
// Persistent string memory.  Should only be used for strings that survive
// across compiles/links.
//
typedef std::basic_string<char> TPersistString;

//
// Create a TString object from an integer.
//
inline const TString String(const int i, const int base = 10)
{
    char text[16];     // 32 bit ints are at most 10 digits in base 10
    
    #ifdef _WIN32
        _itoa(i, text, base);
    #else
        // we assume base 10 for all cases
        sprintf(text, "%d", i);
    #endif

    return text;
}

const unsigned int SourceLocLineMask = 0xffff;
const unsigned int SourceLocStringShift = 16;

__inline TPersistString FormatSourceLoc(const TSourceLoc loc)
{
    char locText[64];

    int string = loc >> SourceLocStringShift;
    int line = loc & SourceLocLineMask;

    if (line)
        sprintf(locText, "%d:%d", string, line);
    else
        sprintf(locText, "%d:? ", string);

    return TPersistString(locText);
}


typedef TMap<TString, TString> TPragmaTable;
typedef TMap<TString, TString>::tAllocator TPragmaTableAllocator;

#endif // _COMMON_INCLUDED_
