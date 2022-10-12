//
// Copyright 2022 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// capture_egl.h:
//   EGL capture functions
//

#include "libANGLE/capture/capture_egl.h"
#include "libANGLE/Surface.h"
#include "libANGLE/capture/FrameCapture.h"
#include "libANGLE/capture/frame_capture_utils.h"
#include "libANGLE/capture/gl_enum_utils_autogen.h"

#define USE_SYSTEM_ZLIB
#include "compression_utils_portable.h"

#if !ANGLE_CAPTURE_ENABLED
#    error Frame capture must be enabled to include this file.
#endif  // !ANGLE_CAPTURE_ENABLED

namespace angle
{
template <>
void WriteParamValueReplay<ParamType::Tgl_ContextPointer>(std::ostream &os,
                                                          const CallCapture &call,
                                                          gl::Context *context)
{
    if (context == nullptr)
    {
        os << "EGL_NO_CONTEXT";
    }
    else
    {
        os << "gContextMap[" << context->id().value << "]";
    }
}

template <>
void WriteParamValueReplay<ParamType::Tegl_DisplayPointer>(std::ostream &os,
                                                           const CallCapture &call,
                                                           egl::Display *display)
{
    os << "EGL_NO_DISPLAY";
}

template <>
void WriteParamValueReplay<ParamType::Tegl_ConfigPointer>(std::ostream &os,
                                                          const CallCapture &call,
                                                          egl::Config *config)
{
    os << "EGL_NO_CONFIG_KHR";
}

template <>
void WriteParamValueReplay<ParamType::Tegl_SurfacePointer>(std::ostream &os,
                                                           const CallCapture &call,
                                                           egl::Surface *surface)
{
    if (surface == nullptr)
    {
        os << "EGL_NO_SURFACE";
    }
    else
    {
        os << "gSurfaceMap[" << reinterpret_cast<uintptr_t>(surface) << "]";
    }
}

template <>
void WriteParamValueReplay<ParamType::TEGLClientBuffer>(std::ostream &os,
                                                        const CallCapture &call,
                                                        EGLClientBuffer value)
{
    const auto &targetParam = call.params.getParam("target", ParamType::TEGLenum, 2);
    os << "GetClientBuffer(" << targetParam.value.EGLenumVal << ", " << value << ")";
}

}  // namespace angle

namespace egl
{

static angle::ParamCapture CaptureAttributeMap(const egl::AttributeMap &attribMap)
{
    std::vector<EGLAttrib> attribs;
    for (const auto &[key, value] : attribMap)
    {
        attribs.push_back(key);
        attribs.push_back(value);
    }
    attribs.push_back(EGL_NONE);

    angle::ParamCapture paramCapture("attrib_list", angle::ParamType::TGLint64Pointer);
    angle::CaptureMemory(attribs.data(), attribs.size() * sizeof(EGLAttrib), &paramCapture);
    return paramCapture;
}

static angle::ParamCapture CaptureAttributeMapInt(const egl::AttributeMap &attribMap)
{
    std::vector<EGLint> attribs;
    for (const auto &[key, value] : attribMap)
    {
        attribs.push_back(static_cast<EGLint>(key));
        attribs.push_back(static_cast<EGLint>(value));
    }
    attribs.push_back(EGL_NONE);

    angle::ParamCapture paramCapture("attrib_list", angle::ParamType::TGLintPointer);
    angle::CaptureMemory(attribs.data(), attribs.size() * sizeof(EGLint), &paramCapture);
    return paramCapture;
}

angle::CallCapture CaptureCreateNativeClientBufferANDROID(const egl::AttributeMap &attribMap,
                                                          EGLClientBuffer eglClientBuffer)
{
    angle::ParamBuffer paramBuffer;
    paramBuffer.addParam(CaptureAttributeMap(attribMap));

    angle::ParamCapture retval;
    angle::SetParamVal<angle::ParamType::TEGLClientBuffer, EGLClientBuffer>(eglClientBuffer,
                                                                            &retval.value);
    paramBuffer.addReturnValue(std::move(retval));

    return angle::CallCapture(angle::EntryPoint::EGLCreateNativeClientBufferANDROID,
                              std::move(paramBuffer));
}

angle::CallCapture CaptureEGLCreateImage(egl::Display *display,
                                         gl::Context *context,
                                         EGLenum target,
                                         EGLClientBuffer buffer,
                                         const egl::AttributeMap &attributes,
                                         egl::Image *image)
{
    angle::ParamBuffer paramBuffer;

    // The EGL display will be queried directly in the emitted code
    // so this is actually just a place holder
    paramBuffer.addValueParam("display", angle::ParamType::Tegl_DisplayPointer, display);

    // In CaptureMidExecutionSetup and FrameCaptureShared::captureCall
    // we capture the actual context ID (via CaptureMakeCurrent),
    // so we have to do the same here.
    paramBuffer.addValueParam("context", angle::ParamType::Tgl_ContextPointer, context);

    paramBuffer.addEnumParam("target", gl::GLESEnum::AllEnums, angle::ParamType::TEGLenum, target);

    angle::ParamCapture paramsClientBuffer("buffer", angle::ParamType::TEGLClientBuffer);
    uint64_t bufferID = reinterpret_cast<uintptr_t>(buffer);
    angle::SetParamVal<angle::ParamType::TGLuint64>(bufferID, &paramsClientBuffer.value);
    paramBuffer.addParam(std::move(paramsClientBuffer));

    angle::ParamCapture paramsAttr = CaptureAttributeMap(attributes);
    paramBuffer.addParam(std::move(paramsAttr));

    angle::ParamCapture retval;
    angle::SetParamVal<angle::ParamType::TGLeglImageOES, GLeglImageOES>(image, &retval.value);
    paramBuffer.addReturnValue(std::move(retval));

    return angle::CallCapture(angle::EntryPoint::EGLCreateImage, std::move(paramBuffer));
}

angle::CallCapture CaptureEGLDestroyImage(egl::Display *display, egl::Image *image)
{
    angle::ParamBuffer paramBuffer;
    paramBuffer.addValueParam("display", angle::ParamType::Tegl_DisplayPointer, display);

    angle::ParamCapture paramImage("image", angle::ParamType::TGLeglImageOES);
    angle::SetParamVal<angle::ParamType::TGLeglImageOES, GLeglImageOES>(image, &paramImage.value);
    paramBuffer.addParam(std::move(paramImage));

    return angle::CallCapture(angle::EntryPoint::EGLDestroyImage, std::move(paramBuffer));
}

angle::CallCapture CaptureEGLCreatePbufferSurface(egl::Display *display,
                                                  egl::Config *config,
                                                  const AttributeMap &attrib_list,
                                                  egl::Surface *surface)
{
    angle::ParamBuffer paramBuffer;
    paramBuffer.addValueParam("display", angle::ParamType::Tegl_DisplayPointer, display);
    paramBuffer.addValueParam("config", angle::ParamType::Tegl_ConfigPointer, config);

    angle::ParamCapture paramsAttr = CaptureAttributeMapInt(attrib_list);
    paramBuffer.addParam(std::move(paramsAttr));

    angle::ParamCapture retval;
    angle::SetParamVal<angle::ParamType::Tegl_SurfacePointer>(surface, &retval.value);
    paramBuffer.addReturnValue(std::move(retval));

    return angle::CallCapture(angle::EntryPoint::EGLCreatePbufferSurface, std::move(paramBuffer));
}
angle::CallCapture CaptureEGLDestroySurface(egl::Display *display, egl::Surface *surface)
{
    angle::ParamBuffer paramBuffer;
    paramBuffer.addValueParam("display", angle::ParamType::Tegl_DisplayPointer, display);
    paramBuffer.addValueParam("surface", angle::ParamType::Tegl_SurfacePointer, surface);

    return angle::CallCapture(angle::EntryPoint::EGLDestroySurface, std::move(paramBuffer));
}

static angle::CallCapture CaptureEGLBindOrReleaseImage(egl::Display *display,
                                                       egl::Surface *surface,
                                                       EGLint buffer,
                                                       angle::EntryPoint entryPoint)
{
    angle::ParamBuffer paramBuffer;
    paramBuffer.addValueParam("display", angle::ParamType::Tegl_DisplayPointer, display);
    paramBuffer.addValueParam("surface", angle::ParamType::Tegl_SurfacePointer, surface);
    paramBuffer.addValueParam("buffer", angle::ParamType::TEGLint, buffer);
    return angle::CallCapture(entryPoint, std::move(paramBuffer));
}

angle::CallCapture CaptureEGLBindTexImage(egl::Display *display,
                                          egl::Surface *surface,
                                          EGLint buffer)
{
    return CaptureEGLBindOrReleaseImage(display, surface, buffer,
                                        angle::EntryPoint::EGLBindTexImage);
}

angle::CallCapture CaptureEGLReleaseTexImage(egl::Display *display,
                                             egl::Surface *surface,
                                             EGLint buffer)
{
    return CaptureEGLBindOrReleaseImage(display, surface, buffer,
                                        angle::EntryPoint::EGLReleaseTexImage);
}

angle::CallCapture CaptureEGLMakeCurrent(egl::Display *display,
                                         Surface *drawSurface,
                                         Surface *readSurface,
                                         gl::Context *context)
{
    angle::ParamBuffer paramBuffer;
    paramBuffer.addValueParam("display", angle::ParamType::Tegl_DisplayPointer, display);
    paramBuffer.addValueParam("draw", angle::ParamType::Tegl_SurfacePointer, drawSurface);
    paramBuffer.addValueParam("read", angle::ParamType::Tegl_SurfacePointer, readSurface);
    paramBuffer.addValueParam("context", angle::ParamType::Tgl_ContextPointer, context);

    return angle::CallCapture(angle::EntryPoint::EGLMakeCurrent, std::move(paramBuffer));
}

}  // namespace egl
