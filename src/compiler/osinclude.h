//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef __OSINCLUDE_H
#define __OSINCLUDE_H

//
// This file contains contains the window's specific datatypes and
// declares any windows specific functions.
//

#if !(defined(_WIN32) || defined(_WIN64))
#error Trying to include a windows specific file in a non windows build.
#endif

#define STRICT
#define VC_EXTRALEAN 1
#include <windows.h>
#include <assert.h>


//
// Thread Local Storage Operations
//
typedef DWORD OS_TLSIndex;
#define OS_INVALID_TLS_INDEX (TLS_OUT_OF_INDEXES)

OS_TLSIndex OS_AllocTLSIndex();
bool        OS_SetTLSValue(OS_TLSIndex nIndex, void *lpvValue);
bool        OS_FreeTLSIndex(OS_TLSIndex nIndex);

inline void* OS_GetTLSValue(OS_TLSIndex nIndex)
{
	assert(nIndex != OS_INVALID_TLS_INDEX);
	return TlsGetValue(nIndex);
}

#endif // __OSINCLUDE_H
