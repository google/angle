//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.cpp: DLL entry point and management of thread-local data.

#include "libEGL/main.h"

#include "common/debug.h"

static DWORD currentTLS = TLS_OUT_OF_INDEXES;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        {
            #ifndef NDEBUG
                FILE *debug = fopen("debug.txt", "rt");

                if (debug)
                {
                    fclose(debug);
                    debug = fopen("debug.txt", "wt");   // Erase
                    fclose(debug);
                }
            #endif

            currentTLS = TlsAlloc();

            if (currentTLS == TLS_OUT_OF_INDEXES)
            {
                return FALSE;
            }
        }
        // Fall throught to initialize index
      case DLL_THREAD_ATTACH:
        {
            egl::Current *current = (egl::Current*)LocalAlloc(LPTR, sizeof(egl::Current));

            if (current)
            {
                TlsSetValue(currentTLS, current);

                current->error = EGL_SUCCESS;
                current->API = EGL_OPENGL_ES_API;
            }
        }
        break;
      case DLL_THREAD_DETACH:
        {
            void *current = TlsGetValue(currentTLS);

            if (current)
            {
                LocalFree((HLOCAL)current);
            }
        }
        break;
      case DLL_PROCESS_DETACH:
        {
            void *current = TlsGetValue(currentTLS);

            if (current)
            {
                LocalFree((HLOCAL)current);
            }

            TlsFree(currentTLS);
        }
        break;
      default:
        break;
    }

    return TRUE;
}

namespace egl
{
void setCurrentError(EGLint error)
{
    Current *current = (Current*)TlsGetValue(currentTLS);

    current->error = error;
}

EGLint getCurrentError()
{
    Current *current = (Current*)TlsGetValue(currentTLS);

    return current->error;
}

void setCurrentAPI(EGLenum API)
{
    Current *current = (Current*)TlsGetValue(currentTLS);

    current->API = API;
}

EGLenum getCurrentAPI()
{
    Current *current = (Current*)TlsGetValue(currentTLS);

    return current->API;
}
}

void error(EGLint errorCode)
{
    egl::setCurrentError(errorCode);
}
