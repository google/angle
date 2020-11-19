//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationEGL.h: Validation functions for generic EGL entry point parameters

#ifndef LIBANGLE_VALIDATIONEGL_H_
#define LIBANGLE_VALIDATIONEGL_H_

#include "common/PackedEnums.h"
#include "libANGLE/Error.h"
#include "libANGLE/Texture.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace gl
{
class Context;
}

namespace egl
{

class AttributeMap;
struct ClientExtensions;
struct Config;
class Device;
class Display;
class Image;
class Stream;
class Surface;
class Sync;
class Thread;
class LabeledObject;

struct ValidationContext
{
    ValidationContext(Thread *threadIn, const char *entryPointIn, const LabeledObject *objectIn)
        : eglThread(threadIn), entryPoint(entryPointIn), labeledObject(objectIn)
    {}

    // We should remove the message-less overload once we have messages for all EGL errors.
    void setError(EGLint error) const;
    void setError(EGLint error, const char *message...) const;

    Thread *eglThread;
    const char *entryPoint;
    const LabeledObject *labeledObject;
};

// Object validation
bool ValidateDisplay(const ValidationContext *val, const Display *display);
bool ValidateSurface(const ValidationContext *val, const Display *display, const Surface *surface);
bool ValidateConfig(const ValidationContext *val, const Display *display, const Config *config);
bool ValidateContext(const ValidationContext *val,
                     const Display *display,
                     const gl::Context *context);
bool ValidateImage(const ValidationContext *val, const Display *display, const Image *image);
bool ValidateDevice(const ValidationContext *val, const Device *device);
bool ValidateSync(const ValidationContext *val, const Display *display, const Sync *sync);

// Return the requested object only if it is valid (otherwise nullptr)
const Thread *GetThreadIfValid(const Thread *thread);
const Display *GetDisplayIfValid(const Display *display);
const Surface *GetSurfaceIfValid(const Display *display, const Surface *surface);
const Image *GetImageIfValid(const Display *display, const Image *image);
const Stream *GetStreamIfValid(const Display *display, const Stream *stream);
const gl::Context *GetContextIfValid(const Display *display, const gl::Context *context);
const Device *GetDeviceIfValid(const Device *device);
const Sync *GetSyncIfValid(const Display *display, const Sync *sync);
LabeledObject *GetLabeledObjectIfValid(Thread *thread,
                                       const Display *display,
                                       ObjectType objectType,
                                       EGLObjectKHR object);

// Entry point validation
bool ValidateInitialize(const ValidationContext *val, const Display *display);

bool ValidateTerminate(const ValidationContext *val, const Display *display);

bool ValidateCreateContext(const ValidationContext *val,
                           Display *display,
                           Config *configuration,
                           gl::Context *shareContext,
                           const AttributeMap &attributes);

bool ValidateCreateWindowSurface(const ValidationContext *val,
                                 Display *display,
                                 Config *config,
                                 EGLNativeWindowType window,
                                 const AttributeMap &attributes);

bool ValidateCreatePbufferSurface(const ValidationContext *val,
                                  Display *display,
                                  Config *config,
                                  const AttributeMap &attributes);
bool ValidateCreatePbufferFromClientBuffer(const ValidationContext *val,
                                           Display *display,
                                           EGLenum buftype,
                                           EGLClientBuffer buffer,
                                           Config *config,
                                           const AttributeMap &attributes);

bool ValidateCreatePixmapSurface(const ValidationContext *val,
                                 Display *display,
                                 Config *config,
                                 EGLNativePixmapType pixmap,
                                 const AttributeMap &attributes);

bool ValidateMakeCurrent(const ValidationContext *val,
                         Display *display,
                         Surface *draw,
                         Surface *read,
                         gl::Context *context);

bool ValidateCreateImage(const ValidationContext *val,
                         const Display *display,
                         gl::Context *context,
                         EGLenum target,
                         EGLClientBuffer buffer,
                         const AttributeMap &attributes);
bool ValidateDestroyImage(const ValidationContext *val, const Display *display, const Image *image);
bool ValidateCreateImageKHR(const ValidationContext *val,
                            const Display *display,
                            gl::Context *context,
                            EGLenum target,
                            EGLClientBuffer buffer,
                            const AttributeMap &attributes);
bool ValidateDestroyImageKHR(const ValidationContext *val,
                             const Display *display,
                             const Image *image);

bool ValidateCreateDeviceANGLE(const ValidationContext *val,
                               EGLint device_type,
                               void *native_device,
                               const EGLAttrib *attrib_list);
bool ValidateReleaseDeviceANGLE(const ValidationContext *val, Device *device);

bool ValidateCreateSyncBase(const ValidationContext *val,
                            const Display *display,
                            EGLenum type,
                            const AttributeMap &attribs,
                            const Display *currentDisplay,
                            const gl::Context *currentContext,
                            bool isExt);
bool ValidateGetSyncAttribBase(const ValidationContext *val,
                               const Display *display,
                               const Sync *sync,
                               EGLint attribute);

bool ValidateCreateSyncKHR(const ValidationContext *val,
                           const Display *display,
                           EGLenum type,
                           const AttributeMap &attribs,
                           const Display *currentDisplay,
                           const gl::Context *currentContext);
bool ValidateCreateSync(const ValidationContext *val,
                        const Display *display,
                        EGLenum type,
                        const AttributeMap &attribs,
                        const Display *currentDisplay,
                        const gl::Context *currentContext);
bool ValidateDestroySync(const ValidationContext *val, const Display *display, const Sync *sync);
bool ValidateClientWaitSync(const ValidationContext *val,
                            const Display *display,
                            const Sync *sync,
                            EGLint flags,
                            EGLTime timeout);
bool ValidateWaitSync(const ValidationContext *val,
                      const Display *display,
                      const gl::Context *context,
                      const Sync *sync,
                      EGLint flags);
bool ValidateGetSyncAttribKHR(const ValidationContext *val,
                              const Display *display,
                              const Sync *sync,
                              EGLint attribute,
                              EGLint *value);
bool ValidateGetSyncAttrib(const ValidationContext *val,
                           const Display *display,
                           const Sync *sync,
                           EGLint attribute,
                           EGLAttrib *value);

bool ValidateCreateStreamKHR(const ValidationContext *val,
                             const Display *display,
                             const AttributeMap &attributes);
bool ValidateDestroyStreamKHR(const ValidationContext *val,
                              const Display *display,
                              const Stream *stream);
bool ValidateStreamAttribKHR(const ValidationContext *val,
                             const Display *display,
                             const Stream *stream,
                             EGLint attribute,
                             EGLint value);
bool ValidateQueryStreamKHR(const ValidationContext *val,
                            const Display *display,
                            const Stream *stream,
                            EGLenum attribute,
                            EGLint *value);
bool ValidateQueryStreamu64KHR(const ValidationContext *val,
                               const Display *display,
                               const Stream *stream,
                               EGLenum attribute,
                               EGLuint64KHR *value);
bool ValidateStreamConsumerGLTextureExternalKHR(const ValidationContext *val,
                                                const Display *display,
                                                gl::Context *context,
                                                const Stream *stream);
bool ValidateStreamConsumerAcquireKHR(const ValidationContext *val,
                                      const Display *display,
                                      gl::Context *context,
                                      const Stream *stream);
bool ValidateStreamConsumerReleaseKHR(const ValidationContext *val,
                                      const Display *display,
                                      gl::Context *context,
                                      const Stream *stream);
bool ValidateStreamConsumerGLTextureExternalAttribsNV(const ValidationContext *val,
                                                      const Display *display,
                                                      gl::Context *context,
                                                      const Stream *stream,
                                                      const AttributeMap &attribs);
bool ValidateCreateStreamProducerD3DTextureANGLE(const ValidationContext *val,
                                                 const Display *display,
                                                 const Stream *stream,
                                                 const AttributeMap &attribs);
bool ValidateStreamPostD3DTextureANGLE(const ValidationContext *val,
                                       const Display *display,
                                       const Stream *stream,
                                       void *texture,
                                       const AttributeMap &attribs);

bool ValidateGetMscRateANGLE(const ValidationContext *val,
                             const Display *display,
                             const Surface *surface,
                             const EGLint *numerator,
                             const EGLint *denominator);
bool ValidateGetSyncValuesCHROMIUM(const ValidationContext *val,
                                   const Display *display,
                                   const Surface *surface,
                                   const EGLuint64KHR *ust,
                                   const EGLuint64KHR *msc,
                                   const EGLuint64KHR *sbc);

bool ValidateDestroySurface(const ValidationContext *val,
                            const Display *display,
                            const Surface *surface,
                            const EGLSurface eglSurface);

bool ValidateDestroyContext(const ValidationContext *val,
                            const Display *display,
                            const gl::Context *glCtx,
                            const EGLContext eglCtx);

bool ValidateSwapBuffers(const ValidationContext *val,
                         Thread *thread,
                         const Display *display,
                         const Surface *surface);

bool ValidateWaitNative(const ValidationContext *val, const Display *display, const EGLint engine);

bool ValidateCopyBuffers(const ValidationContext *val, Display *display, const Surface *surface);

bool ValidateSwapBuffersWithDamageKHR(const ValidationContext *val,
                                      const Display *display,
                                      const Surface *surface,
                                      EGLint *rects,
                                      EGLint n_rects);

bool ValidateBindTexImage(const ValidationContext *val,
                          const Display *display,
                          const Surface *surface,
                          const EGLSurface eglSurface,
                          const EGLint buffer,
                          const gl::Context *context,
                          gl::Texture **textureObject);

bool ValidateReleaseTexImage(const ValidationContext *val,
                             const Display *display,
                             const Surface *surface,
                             const EGLSurface eglSurface,
                             const EGLint buffer);

bool ValidateSwapInterval(const ValidationContext *val,
                          const Display *display,
                          const Surface *draw_surface,
                          const gl::Context *context);

bool ValidateBindAPI(const ValidationContext *val, const EGLenum api);

bool ValidatePresentationTimeANDROID(const ValidationContext *val,
                                     const Display *display,
                                     const Surface *surface,
                                     EGLnsecsANDROID time);

bool ValidateSetBlobCacheANDROID(const ValidationContext *val,
                                 const Display *display,
                                 EGLSetBlobFuncANDROID set,
                                 EGLGetBlobFuncANDROID get);

bool ValidateGetConfigAttrib(const ValidationContext *val,
                             const Display *display,
                             const Config *config,
                             EGLint attribute);
bool ValidateChooseConfig(const ValidationContext *val,
                          const Display *display,
                          const AttributeMap &attribs,
                          EGLint configSize,
                          EGLint *numConfig);
bool ValidateGetConfigs(const ValidationContext *val,
                        const Display *display,
                        EGLint configSize,
                        EGLint *numConfig);

// Other validation
bool ValidateCompatibleSurface(const ValidationContext *val,
                               const Display *display,
                               gl::Context *context,
                               const Surface *surface);

bool ValidateGetPlatformDisplay(const ValidationContext *val,
                                EGLenum platform,
                                void *native_display,
                                const EGLAttrib *attrib_list);
bool ValidateGetPlatformDisplayEXT(const ValidationContext *val,
                                   EGLenum platform,
                                   void *native_display,
                                   const EGLint *attrib_list);
bool ValidateCreatePlatformWindowSurfaceEXT(const ValidationContext *val,
                                            const Display *display,
                                            const Config *configuration,
                                            void *nativeWindow,
                                            const AttributeMap &attributes);
bool ValidateCreatePlatformPixmapSurfaceEXT(const ValidationContext *val,
                                            const Display *display,
                                            const Config *configuration,
                                            void *nativePixmap,
                                            const AttributeMap &attributes);

bool ValidateProgramCacheGetAttribANGLE(const ValidationContext *val,
                                        const Display *display,
                                        EGLenum attrib);

bool ValidateProgramCacheQueryANGLE(const ValidationContext *val,
                                    const Display *display,
                                    EGLint index,
                                    void *key,
                                    EGLint *keysize,
                                    void *binary,
                                    EGLint *binarysize);

bool ValidateProgramCachePopulateANGLE(const ValidationContext *val,
                                       const Display *display,
                                       const void *key,
                                       EGLint keysize,
                                       const void *binary,
                                       EGLint binarysize);

bool ValidateProgramCacheResizeANGLE(const ValidationContext *val,
                                     const Display *display,
                                     EGLint limit,
                                     EGLenum mode);

bool ValidateSurfaceAttrib(const ValidationContext *val,
                           const Display *display,
                           const Surface *surface,
                           EGLint attribute,
                           EGLint value);
bool ValidateQuerySurface(const ValidationContext *val,
                          const Display *display,
                          const Surface *surface,
                          EGLint attribute,
                          EGLint *value);
bool ValidateQueryContext(const ValidationContext *val,
                          const Display *display,
                          const gl::Context *context,
                          EGLint attribute,
                          EGLint *value);

// EGL_KHR_debug
bool ValidateDebugMessageControlKHR(const ValidationContext *val,
                                    EGLDEBUGPROCKHR callback,
                                    const AttributeMap &attribs);

bool ValidateQueryDebugKHR(const ValidationContext *val, EGLint attribute, EGLAttrib *value);

bool ValidateLabelObjectKHR(const ValidationContext *val,
                            Thread *thread,
                            const Display *display,
                            ObjectType objectType,
                            EGLObjectKHR object,
                            EGLLabelKHR label);

// ANDROID_get_frame_timestamps
bool ValidateGetCompositorTimingSupportedANDROID(const ValidationContext *val,
                                                 const Display *display,
                                                 const Surface *surface,
                                                 CompositorTiming name);

bool ValidateGetCompositorTimingANDROID(const ValidationContext *val,
                                        const Display *display,
                                        const Surface *surface,
                                        EGLint numTimestamps,
                                        const EGLint *names,
                                        EGLnsecsANDROID *values);

bool ValidateGetNextFrameIdANDROID(const ValidationContext *val,
                                   const Display *display,
                                   const Surface *surface,
                                   EGLuint64KHR *frameId);

bool ValidateGetFrameTimestampSupportedANDROID(const ValidationContext *val,
                                               const Display *display,
                                               const Surface *surface,
                                               Timestamp timestamp);

bool ValidateGetFrameTimestampsANDROID(const ValidationContext *val,
                                       const Display *display,
                                       const Surface *surface,
                                       EGLuint64KHR frameId,
                                       EGLint numTimestamps,
                                       const EGLint *timestamps,
                                       EGLnsecsANDROID *values);

bool ValidateQueryStringiANGLE(const ValidationContext *val,
                               const Display *display,
                               EGLint name,
                               EGLint index);

bool ValidateQueryDisplayAttribEXT(const ValidationContext *val,
                                   const Display *display,
                                   const EGLint attribute);
bool ValidateQueryDisplayAttribANGLE(const ValidationContext *val,
                                     const Display *display,
                                     const EGLint attribute);

// EGL_ANDROID_get_native_client_buffer
bool ValidateGetNativeClientBufferANDROID(const ValidationContext *val,
                                          const struct AHardwareBuffer *buffer);

// EGL_ANDROID_create_native_client_buffer
bool ValidateCreateNativeClientBufferANDROID(const ValidationContext *val,
                                             const egl::AttributeMap &attribMap);

// EGL_ANDROID_native_fence_sync
bool ValidateDupNativeFenceFDANDROID(const ValidationContext *val,
                                     const Display *display,
                                     const Sync *sync);

// EGL_ANGLE_swap_with_frame_token
bool ValidateSwapBuffersWithFrameTokenANGLE(const ValidationContext *val,
                                            const Display *display,
                                            const Surface *surface,
                                            EGLFrameTokenANGLE frametoken);

// EGL_KHR_reusable_sync
bool ValidateSignalSyncKHR(const ValidationContext *val,
                           const Display *display,
                           const Sync *sync,
                           EGLint mode);

// EGL_ANGLE_query_surface_pointer
bool ValidateQuerySurfacePointerANGLE(const ValidationContext *val,
                                      const Display *display,
                                      Surface *eglSurface,
                                      EGLint attribute,
                                      void **value);

// EGL_NV_post_sub_buffer
bool ValidatePostSubBufferNV(const ValidationContext *val,
                             Display *display,
                             Surface *eglSurface,
                             EGLint x,
                             EGLint y,
                             EGLint width,
                             EGLint height);

// EGL_EXT_device_query
bool ValidateQueryDeviceAttribEXT(const ValidationContext *val,
                                  Device *device,
                                  EGLint attribute,
                                  EGLAttrib *value);
bool ValidateQueryDeviceStringEXT(const ValidationContext *val, Device *device, EGLint name);

bool ValidateReleaseHighPowerGPUANGLE(const ValidationContext *val,
                                      Display *display,
                                      gl::Context *context);
bool ValidateReacquireHighPowerGPUANGLE(const ValidationContext *val,
                                        Display *display,
                                        gl::Context *context);
bool ValidateHandleGPUSwitchANGLE(const ValidationContext *val, Display *display);

}  // namespace egl

#define ANGLE_EGL_VALIDATE(THREAD, EP, OBJ, RETVAL, ...)           \
    do                                                             \
    {                                                              \
        const char *epname = "egl" #EP;                            \
        ValidationContext vctx(THREAD, epname, OBJ);               \
        auto ANGLE_LOCAL_VAR = (Validate##EP(&vctx, __VA_ARGS__)); \
        if (!ANGLE_LOCAL_VAR)                                      \
        {                                                          \
            return RETVAL;                                         \
        }                                                          \
    } while (0)

#define ANGLE_EGL_VALIDATE_VOID(THREAD, EP, OBJ, ...)              \
    do                                                             \
    {                                                              \
        const char *epname = "egl" #EP;                            \
        ValidationContext vctx(THREAD, epname, OBJ);               \
        auto ANGLE_LOCAL_VAR = (Validate##EP(&vctx, __VA_ARGS__)); \
        if (!ANGLE_LOCAL_VAR)                                      \
        {                                                          \
            return;                                                \
        }                                                          \
    } while (0)

#define ANGLE_EGL_TRY(THREAD, EXPR, FUNCNAME, LABELOBJECT)                   \
    do                                                                       \
    {                                                                        \
        auto ANGLE_LOCAL_VAR = (EXPR);                                       \
        if (ANGLE_LOCAL_VAR.isError())                                       \
            return THREAD->setError(ANGLE_LOCAL_VAR, FUNCNAME, LABELOBJECT); \
    } while (0)

#define ANGLE_EGL_TRY_RETURN(THREAD, EXPR, FUNCNAME, LABELOBJECT, RETVAL) \
    do                                                                    \
    {                                                                     \
        auto ANGLE_LOCAL_VAR = (EXPR);                                    \
        if (ANGLE_LOCAL_VAR.isError())                                    \
        {                                                                 \
            THREAD->setError(ANGLE_LOCAL_VAR, FUNCNAME, LABELOBJECT);     \
            return RETVAL;                                                \
        }                                                                 \
    } while (0)

#endif  // LIBANGLE_VALIDATIONEGL_H_
