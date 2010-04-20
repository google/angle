//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _INFOSINK_INCLUDED_
#define _INFOSINK_INCLUDED_

#include <math.h>

#include "compiler/Common.h"

//
// TPrefixType is used to centralize how info log messages start.
// See below.
//
enum TPrefixType {
    EPrefixNone,
    EPrefixWarning,
    EPrefixError,
    EPrefixInternalError,
    EPrefixUnimplemented,
    EPrefixNote
};

enum TOutputStream {
    ENull = 0,
    EStdOut = 0x01,
    EString = 0x02,
};
//
// Encapsulate info logs for all objects that have them.
//
// The methods are a general set of tools for getting a variety of
// messages and types inserted into the log.
//
class TInfoSinkBase {
public:
    TInfoSinkBase() : outputStream(EString) {}
    void erase() { sink.erase(); }
    TInfoSinkBase& operator<<(const TPersistString& t) { append(t); return *this; }
    TInfoSinkBase& operator<<(char c)                  { append(1, c); return *this; }
    TInfoSinkBase& operator<<(const char* s)           { append(s); return *this; }
    TInfoSinkBase& operator<<(int n)                   { append(String(n)); return *this; }
    TInfoSinkBase& operator<<(const unsigned int n)    { append(String(n)); return *this; }
    TInfoSinkBase& operator<<(float n)                 { char buf[40]; 
                                                     sprintf(buf, "%.8g", n);
                                                     append(buf); 
                                                     return *this; }
    TInfoSinkBase& operator+(const TPersistString& t)  { append(t); return *this; }
    TInfoSinkBase& operator+(const TString& t)         { append(t); return *this; }
    TInfoSinkBase& operator<<(const TString& t)        { append(t); return *this; }
    TInfoSinkBase& operator+(const char* s)            { append(s); return *this; }
    const char* c_str() const { return sink.c_str(); }
    void prefix(TPrefixType message) {
        switch(message) {
        case EPrefixNone:                                      break;
        case EPrefixWarning:       append("WARNING: ");        break;
        case EPrefixError:         append("ERROR: ");          break;
        case EPrefixInternalError: append("INTERNAL ERROR: "); break;
        case EPrefixUnimplemented: append("UNIMPLEMENTED: ");  break;
        case EPrefixNote:          append("NOTE: ");           break;
        default:                   append("UNKOWN ERROR: ");   break;
        }
    }
    void location(TSourceLoc loc) {
        append(FormatSourceLoc(loc).c_str());
        append(": ");
    }
    void message(TPrefixType message, const char* s) {
        prefix(message);
        append(s);
        append("\n");
    }
    void message(TPrefixType message, const char* s, TSourceLoc loc) {
        prefix(message);
        location(loc);
        append(s);
        append("\n");
    }
    
    void setOutputStream(int output = 4)
    {
        outputStream = output;
    }

protected:
    void append(const char *s); 

    void append(int count, char c);
    void append(const TPersistString& t);
    void append(const TString& t);

    void checkMem(size_t growth) { if (sink.capacity() < sink.size() + growth + 2)  
                                       sink.reserve(sink.capacity() +  sink.capacity() / 2); }
    void appendToStream(const char* s);
    TPersistString sink;
    int outputStream;
};

class TInfoSink {
public:
    TInfoSinkBase info;
    TInfoSinkBase debug;
    TInfoSinkBase obj;
};

#endif // _INFOSINK_INCLUDED_
