//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/InfoSink.h"

#ifdef _WIN32
    #include <windows.h>
#endif

void TInfoSinkBase::append(const char *s)           
{
    if (outputStream & EString) {
        checkMem(strlen(s)); 
        sink.append(s); 
    }

    if (outputStream & EStdOut)
        fprintf(stdout, "%s", s);
}

void TInfoSinkBase::append(int count, char c)       
{ 
    if (outputStream & EString) {
        checkMem(count);         
        sink.append(count, c); 
    }

    if (outputStream & EStdOut)
        fprintf(stdout, "%c", c);
}

void TInfoSinkBase::append(const TPersistString& t) 
{ 
    if (outputStream & EString) {
        checkMem(t.size());  
        sink.append(t); 
    }

    if (outputStream & EStdOut)
        fprintf(stdout, "%s", t.c_str());
}

void TInfoSinkBase::append(const TString& t)
{ 
    if (outputStream & EString) {
        checkMem(t.size());  
        sink.append(t.c_str()); 
    }

    if (outputStream & EStdOut)
        fprintf(stdout, "%s", t.c_str());
}
