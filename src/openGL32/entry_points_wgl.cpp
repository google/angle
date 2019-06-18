//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// entry_points_wgl.cpp: Implements the exported WGL functions.

#include "entry_points_wgl.h"

#include "common/debug.h"
#include "common/event_tracer.h"
#include "common/utilities.h"
#include "common/version.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/EGLSync.h"
#include "libANGLE/Surface.h"
#include "libANGLE/Texture.h"
#include "libANGLE/Thread.h"
#include "libANGLE/queryutils.h"
#include "libANGLE/validationEGL.h"
#include "libGLESv2/global_state.h"
#include "openGL32/proc_table_wgl.h"

using namespace wgl;
using namespace egl;

namespace
{

bool CompareProc(const ProcEntry &a, const char *b)
{
    return strcmp(a.first, b) < 0;
}

void ClipConfigs(const std::vector<const Config *> &filteredConfigs,
                 EGLConfig *output_configs,
                 EGLint config_size,
                 EGLint *num_config)
{
    EGLint result_size = static_cast<EGLint>(filteredConfigs.size());
    if (output_configs)
    {
        result_size = std::max(std::min(result_size, config_size), 0);
        for (EGLint i = 0; i < result_size; i++)
        {
            output_configs[i] = const_cast<Config *>(filteredConfigs[i]);
        }
    }
    *num_config = result_size;
}
}  // anonymous namespace

extern "C" {

// WGL 1.0
int GL_APIENTRY wglChoosePixelFormat(HDC hDc, const PIXELFORMATDESCRIPTOR *pPfd)
{
    UNIMPLEMENTED();
    return 1;
}

int GL_APIENTRY wglDescribePixelFormat(HDC hdc, int ipfd, UINT cjpfd, PIXELFORMATDESCRIPTOR *ppfd)
{
    UNIMPLEMENTED();
    if (ppfd)
    {
        ppfd->dwFlags      = ppfd->dwFlags | PFD_DRAW_TO_WINDOW;
        ppfd->dwFlags      = ppfd->dwFlags | PFD_SUPPORT_OPENGL;
        ppfd->dwFlags      = ppfd->dwFlags | PFD_GENERIC_ACCELERATED;
        ppfd->dwFlags      = ppfd->dwFlags | PFD_DOUBLEBUFFER;
        ppfd->iPixelType   = PFD_TYPE_RGBA;
        ppfd->cRedBits     = 8;
        ppfd->cGreenBits   = 8;
        ppfd->cBlueBits    = 8;
        ppfd->cAlphaBits   = 8;
        ppfd->cDepthBits   = 24;
        ppfd->cStencilBits = 8;
        ppfd->nVersion     = 3;
    }
    return 1;
}

UINT GL_APIENTRY wglGetEnhMetaFilePixelFormat(HENHMETAFILE hemf,
                                              UINT cbBuffer,
                                              PIXELFORMATDESCRIPTOR *ppfd)
{
    UNIMPLEMENTED();
    return 1u;
}

int GL_APIENTRY wglGetPixelFormat(HDC hdc)
{
    UNIMPLEMENTED();
    return 1;
}

BOOL GL_APIENTRY wglSetPixelFormat(HDC hdc, int ipfd, const PIXELFORMATDESCRIPTOR *ppfd)
{
    UNIMPLEMENTED();
    return TRUE;
}

BOOL GL_APIENTRY wglSwapBuffers(HDC hdc)
{
    UNIMPLEMENTED();
    return TRUE;
}

BOOL GL_APIENTRY wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
    UNIMPLEMENTED();
    return TRUE;
}

HGLRC GL_APIENTRY wglCreateContext(HDC hDc)
{

    GLenum platformType = EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE;

    std::vector<EGLint> displayAttributes;
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
    displayAttributes.push_back(platformType);
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE);
    displayAttributes.push_back(EGL_DONT_CARE);
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE);
    displayAttributes.push_back(EGL_DONT_CARE);
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    displayAttributes.push_back(EGL_NONE);

    const auto &attribMapDisplay =
        AttributeMap::CreateFromAttribArray((const EGLAttrib *)&displayAttributes[0]);

    EGLDisplay mDisplay = egl::Display::GetDisplayFromNativeDisplay(hDc, attribMapDisplay);

    egl::Display *display = static_cast<egl::Display *>(mDisplay);
    auto error            = display->initialize();

    // Don't have a thread to bind API to, so just use this API
    // eglBindAPI(EGL_OPENGL_ES_API);

    // Default config
    const EGLint configAttributes[] = {EGL_RED_SIZE,
                                       EGL_DONT_CARE,
                                       EGL_GREEN_SIZE,
                                       EGL_DONT_CARE,
                                       EGL_BLUE_SIZE,
                                       EGL_DONT_CARE,
                                       EGL_ALPHA_SIZE,
                                       EGL_DONT_CARE,
                                       EGL_DEPTH_SIZE,
                                       EGL_DONT_CARE,
                                       EGL_STENCIL_SIZE,
                                       EGL_DONT_CARE,
                                       EGL_SAMPLE_BUFFERS,
                                       0,
                                       EGL_NONE};

    // Choose config
    EGLint configCount;
    EGLConfig mConfig;
    AttributeMap attribMapConfig = AttributeMap::CreateFromIntArray(configAttributes);
    ClipConfigs(display->chooseConfig(attribMapConfig), &mConfig, 1, &configCount);

    // Initialize surface
    std::vector<EGLint> surfaceAttributes;
    surfaceAttributes.push_back(EGL_NONE);
    surfaceAttributes.push_back(EGL_NONE);

    // Create first window surface
    // EGLSurface mWindowSurface = eglCreateWindowSurface(mDisplay, mConfig,
    // (EGLNativeWindowType)hDc, &surfaceAttributes[0]);

    // Initialize context
    EGLint contextAttibutes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    Config *configuration        = static_cast<Config *>(mConfig);
    gl::Context *sharedGLContext = static_cast<gl::Context *>(nullptr);
    AttributeMap attributes      = AttributeMap::CreateFromIntArray(contextAttibutes);

    gl::Context *context = nullptr;
    auto error1 = display->createContext(configuration, sharedGLContext, attributes, &context);

    EGLContext mContext = static_cast<EGLContext>(context);

    return (HGLRC)mContext;
}

HGLRC GL_APIENTRY wglCreateLayerContext(HDC hDc, int level)
{
    UNIMPLEMENTED();
    return nullptr;
}

BOOL GL_APIENTRY wglDeleteContext(HGLRC oldContext)
{
    UNIMPLEMENTED();
    return FALSE;
}

BOOL GL_APIENTRY wglDescribeLayerPlane(HDC hDc,
                                       int pixelFormat,
                                       int layerPlane,
                                       UINT nBytes,
                                       LAYERPLANEDESCRIPTOR *plpd)
{
    UNIMPLEMENTED();
    return FALSE;
}

HGLRC GL_APIENTRY wglGetCurrentContext()
{
    UNIMPLEMENTED();
    return nullptr;
}

HDC GL_APIENTRY wglGetCurrentDC()
{
    UNIMPLEMENTED();
    return nullptr;
}

int GL_APIENTRY
wglGetLayerPaletteEntries(HDC hdc, int iLayerPlane, int iStart, int cEntries, COLORREF *pcr)
{
    UNIMPLEMENTED();
    return 0;
}

PROC GL_APIENTRY wglGetProcAddress(LPCSTR lpszProc)
{
    ANGLE_SCOPED_GLOBAL_LOCK();
    EVENT("(const char *procname = \"%s\")", lpszProc);
    egl::Thread *thread = egl::GetCurrentThread();

    ProcEntry *entry =
        std::lower_bound(&g_procTable[0], &g_procTable[g_numProcs], lpszProc, CompareProc);

    thread->setSuccess();

    if (entry == &g_procTable[g_numProcs] || strcmp(entry->first, lpszProc) != 0)
    {
        return nullptr;
    }

    return entry->second;
}

BOOL GL_APIENTRY wglMakeCurrent(HDC hDc, HGLRC newContext)
{
    UNIMPLEMENTED();
    return FALSE;
}

BOOL GL_APIENTRY wglRealizeLayerPalette(HDC hdc, int iLayerPlane, BOOL bRealize)
{
    UNIMPLEMENTED();
    return FALSE;
}

int GL_APIENTRY
wglSetLayerPaletteEntries(HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr)
{
    UNIMPLEMENTED();
    return 0;
}

BOOL GL_APIENTRY wglShareLists(HGLRC hrcSrvShare, HGLRC hrcSrvSource)
{
    UNIMPLEMENTED();
    return FALSE;
}

BOOL GL_APIENTRY wglSwapLayerBuffers(HDC hdc, UINT fuFlags)
{
    UNIMPLEMENTED();
    return FALSE;
}

BOOL GL_APIENTRY wglUseFontBitmapsA(HDC hDC, DWORD first, DWORD count, DWORD listBase)
{
    UNIMPLEMENTED();
    return FALSE;
}

BOOL GL_APIENTRY wglUseFontBitmapsW(HDC hDC, DWORD first, DWORD count, DWORD listBase)
{
    UNIMPLEMENTED();
    return FALSE;
}

BOOL GL_APIENTRY wglUseFontOutlinesA(HDC hDC,
                                     DWORD first,
                                     DWORD count,
                                     DWORD listBase,
                                     FLOAT deviation,
                                     FLOAT extrusion,
                                     int format,
                                     LPGLYPHMETRICSFLOAT lpgmf)
{
    UNIMPLEMENTED();
    return FALSE;
}

BOOL GL_APIENTRY wglUseFontOutlinesW(HDC hDC,
                                     DWORD first,
                                     DWORD count,
                                     DWORD listBase,
                                     FLOAT deviation,
                                     FLOAT extrusion,
                                     int format,
                                     LPGLYPHMETRICSFLOAT lpgmf)
{
    UNIMPLEMENTED();
    return FALSE;
}

}  // extern "C"
