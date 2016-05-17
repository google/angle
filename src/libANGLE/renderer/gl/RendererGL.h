//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RendererGL.h: Defines the class interface for RendererGL.

#ifndef LIBANGLE_RENDERER_GL_RENDERERGL_H_
#define LIBANGLE_RENDERER_GL_RENDERERGL_H_

#include "libANGLE/Caps.h"
#include "libANGLE/Error.h"
#include "libANGLE/Version.h"
#include "libANGLE/renderer/gl/WorkaroundsGL.h"

namespace gl
{
struct ContextState;
struct IndexRange;
}

namespace egl
{
class AttributeMap;
}

namespace sh
{
struct BlockMemberInfo;
}

namespace rx
{
class BlitGL;
class ContextImpl;
class FunctionsGL;
class StateManagerGL;

class RendererGL : angle::NonCopyable
{
  public:
    RendererGL(const FunctionsGL *functions, const egl::AttributeMap &attribMap);
    ~RendererGL();

    ContextImpl *createContext(const gl::ContextState &state);

    gl::Error flush();
    gl::Error finish();

    gl::Error drawArrays(const gl::ContextState &data, GLenum mode, GLint first, GLsizei count);
    gl::Error drawArraysInstanced(const gl::ContextState &data,
                                  GLenum mode,
                                  GLint first,
                                  GLsizei count,
                                  GLsizei instanceCount);

    gl::Error drawElements(const gl::ContextState &data,
                           GLenum mode,
                           GLsizei count,
                           GLenum type,
                           const GLvoid *indices,
                           const gl::IndexRange &indexRange);
    gl::Error drawElementsInstanced(const gl::ContextState &data,
                                    GLenum mode,
                                    GLsizei count,
                                    GLenum type,
                                    const GLvoid *indices,
                                    GLsizei instances,
                                    const gl::IndexRange &indexRange);
    gl::Error drawRangeElements(const gl::ContextState &data,
                                GLenum mode,
                                GLuint start,
                                GLuint end,
                                GLsizei count,
                                GLenum type,
                                const GLvoid *indices,
                                const gl::IndexRange &indexRange);

    // EXT_debug_marker
    void insertEventMarker(GLsizei length, const char *marker);
    void pushGroupMarker(GLsizei length, const char *marker);
    void popGroupMarker();

    // lost device
    void notifyDeviceLost();
    bool isDeviceLost() const;
    bool testDeviceLost();
    bool testDeviceResettable();

    std::string getVendorString() const;
    std::string getRendererDescription() const;

    GLint getGPUDisjoint();
    GLint64 getTimestamp();

    const gl::Version &getMaxSupportedESVersion() const;
    const FunctionsGL *getFunctions() const { return mFunctions; }
    StateManagerGL *getStateManager() const { return mStateManager; }
    const WorkaroundsGL &getWorkarounds() const { return mWorkarounds; }
    BlitGL *getBlitter() const { return mBlitter; }

    const gl::Caps &getNativeCaps() const;
    const gl::TextureCapsMap &getNativeTextureCaps() const;
    const gl::Extensions &getNativeExtensions() const;
    const gl::Limitations &getNativeLimitations() const;

  private:
    void ensureCapsInitialized() const;
    void generateCaps(gl::Caps *outCaps,
                      gl::TextureCapsMap *outTextureCaps,
                      gl::Extensions *outExtensions,
                      gl::Limitations *outLimitations) const;

    mutable gl::Version mMaxSupportedESVersion;

    const FunctionsGL *mFunctions;
    StateManagerGL *mStateManager;

    BlitGL *mBlitter;

    WorkaroundsGL mWorkarounds;

    bool mHasDebugOutput;

    // For performance debugging
    bool mSkipDrawCalls;

    mutable bool mCapsInitialized;
    mutable gl::Caps mNativeCaps;
    mutable gl::TextureCapsMap mNativeTextureCaps;
    mutable gl::Extensions mNativeExtensions;
    mutable gl::Limitations mNativeLimitations;
};

}  // namespace rx

#endif // LIBANGLE_RENDERER_GL_RENDERERGL_H_
