//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SyncVk:
//    Defines the class interface for SyncVk, implementing SyncImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_FENCESYNCVK_H_
#define LIBANGLE_RENDERER_VULKAN_FENCESYNCVK_H_

#include "libANGLE/renderer/EGLSyncImpl.h"
#include "libANGLE/renderer/SyncImpl.h"

#include "libANGLE/renderer/vulkan/vk_utils.h"

namespace egl
{
class AttributeMap;
}

namespace rx
{
// The behaviors of SyncImpl and EGLSyncImpl as fence syncs (only supported type) are currently
// identical for the Vulkan backend, and this class implements both interfaces.
class FenceSyncVk
{
  public:
    FenceSyncVk();
    ~FenceSyncVk();

    void onDestroy(RendererVk *renderer);

    angle::Result initialize(vk::Context *context);
    angle::Result clientWait(vk::Context *context,
                             bool flushCommands,
                             uint64_t timeout,
                             VkResult *outResult);
    angle::Result serverWait(vk::Context *context);
    angle::Result getStatus(vk::Context *context, bool *signaled);

  private:
    bool hasPendingWork(RendererVk *renderer);

    // The vkEvent that's signaled on `init` and can be waited on in `serverWait`, or queried with
    // `getStatus`.
    vk::Event mEvent;
    // The serial in which the event was inserted.  Used in `clientWait` to know whether flush is
    // necessary, and to be able to wait on the vkFence that's automatically inserted at the end of
    // each submissions.
    Serial mSignalSerial;
};

class SyncVk final : public SyncImpl
{
  public:
    SyncVk();
    ~SyncVk() override;

    void onDestroy(const gl::Context *context) override;

    angle::Result set(const gl::Context *context, GLenum condition, GLbitfield flags) override;
    angle::Result clientWait(const gl::Context *context,
                             GLbitfield flags,
                             GLuint64 timeout,
                             GLenum *outResult) override;
    angle::Result serverWait(const gl::Context *context,
                             GLbitfield flags,
                             GLuint64 timeout) override;
    angle::Result getStatus(const gl::Context *context, GLint *outResult) override;

  private:
    FenceSyncVk mFenceSync;
};

class EGLSyncVk final : public EGLSyncImpl
{
  public:
    EGLSyncVk(const egl::AttributeMap &attribs);
    ~EGLSyncVk() override;

    void onDestroy(const egl::Display *display) override;

    egl::Error initialize(const egl::Display *display, EGLenum type) override;
    egl::Error clientWait(const egl::Display *display,
                          EGLint flags,
                          EGLTime timeout,
                          EGLint *outResult) override;
    egl::Error serverWait(const egl::Display *display, EGLint flags) override;
    egl::Error getStatus(const egl::Display *display, EGLint *outStatus) override;

  private:
    FenceSyncVk mFenceSync;
};
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_FENCESYNCVK_H_
