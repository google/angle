#include "precompiled.h"
//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer.cpp: Implements EGL dependencies for creating and destroying Renderer instances.

#include <EGL/eglext.h>
#include "libGLESv2/main.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/d3d9/Renderer9.h"
#include "libGLESv2/renderer/d3d11/Renderer11.h"
#include "libGLESv2/utilities.h"
#include "third_party/trace_event/trace_event.h"

#if !defined(ANGLE_ENABLE_D3D11)
// Enables use of the Direct3D 11 API for a default display, when available
#define ANGLE_ENABLE_D3D11 0
#endif

namespace rx
{

Renderer::Renderer(egl::Display *display) : mDisplay(display)
{
    mD3dCompilerModule = NULL;
    mD3DCompileFunc = NULL;
}

Renderer::~Renderer()
{
    if (mD3dCompilerModule)
    {
        FreeLibrary(mD3dCompilerModule);
        mD3dCompilerModule = NULL;
    }
}

bool Renderer::initializeCompiler()
{
    TRACE_EVENT0("gpu", "initializeCompiler");
#if defined(ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES)
    // Find a D3DCompiler module that had already been loaded based on a predefined list of versions.
    static TCHAR* d3dCompilerNames[] = ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES;

    for (size_t i = 0; i < ArraySize(d3dCompilerNames); ++i)
    {
        if (GetModuleHandleEx(0, d3dCompilerNames[i], &mD3dCompilerModule))
        {
            break;
        }
    }
#endif  // ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES

    if (!mD3dCompilerModule)
    {
        // Load the version of the D3DCompiler DLL associated with the Direct3D version ANGLE was built with.
        mD3dCompilerModule = LoadLibrary(D3DCOMPILER_DLL);
    }

    if (!mD3dCompilerModule)
    {
        ERR("No D3D compiler module found - aborting!\n");
        return false;
    }

    mD3DCompileFunc = reinterpret_cast<pCompileFunc>(GetProcAddress(mD3dCompilerModule, "D3DCompile"));
    ASSERT(mD3DCompileFunc);

    return mD3DCompileFunc != NULL;
}

// Compiles HLSL code into executable binaries
ShaderBlob *Renderer::compileToBinary(gl::InfoLog &infoLog, const char *hlsl, const char *profile, const UINT optimizationFlags[], const char *flagNames[], int attempts)
{
    if (!hlsl)
    {
        return NULL;
    }

    pD3DCompile compileFunc = reinterpret_cast<pD3DCompile>(mD3DCompileFunc);
    for (int i = 0; i < attempts; ++i)
    {
        ID3DBlob *errorMessage = NULL;
        ID3DBlob *binary = NULL;

        HRESULT result = compileFunc(hlsl, strlen(hlsl), gl::g_fakepath, NULL, NULL,
                                     "main", profile, optimizationFlags[i], 0, &binary, &errorMessage);
        if (errorMessage)
        {
            const char *message = (const char*)errorMessage->GetBufferPointer();

            infoLog.appendSanitized(message);
            TRACE("\n%s", hlsl);
            TRACE("\n%s", message);

            errorMessage->Release();
            errorMessage = NULL;
        }

        if (SUCCEEDED(result))
        {
            return (ShaderBlob*)binary;
        }
        else
        {
            if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
            {
                return gl::error(GL_OUT_OF_MEMORY, (ShaderBlob*) NULL);
            }

            infoLog.append("Warning: D3D shader compilation failed with ");
            infoLog.append(flagNames[i]);
            infoLog.append(" flags.");
            if (i + 1 < attempts)
            {
                infoLog.append(" Retrying with ");
                infoLog.append(flagNames[i + 1]);
                infoLog.append(".\n");
            }
        }
    }

    return NULL;
}

}

extern "C"
{

rx::Renderer *glCreateRenderer(egl::Display *display, HDC hDc, EGLNativeDisplayType displayId)
{
    rx::Renderer *renderer = NULL;
    EGLint status = EGL_BAD_ALLOC;
    
    if (ANGLE_ENABLE_D3D11 ||
        displayId == EGL_D3D11_ELSE_D3D9_DISPLAY_ANGLE ||
        displayId == EGL_D3D11_ONLY_DISPLAY_ANGLE)
    {
        renderer = new rx::Renderer11(display, hDc);
    
        if (renderer)
        {
            status = renderer->initialize();
        }

        if (status == EGL_SUCCESS)
        {
            return renderer;
        }
        else if (displayId == EGL_D3D11_ONLY_DISPLAY_ANGLE)
        {
            return NULL;
        }

        // Failed to create a D3D11 renderer, try creating a D3D9 renderer
        delete renderer;
    }

    bool softwareDevice = (displayId == EGL_SOFTWARE_DISPLAY_ANGLE);
    renderer = new rx::Renderer9(display, hDc, softwareDevice);
    
    if (renderer)
    {
        status = renderer->initialize();
    }

    if (status == EGL_SUCCESS)
    {
        return renderer;
    }

    return NULL;
}

void glDestroyRenderer(rx::Renderer *renderer)
{
    delete renderer;
}

}