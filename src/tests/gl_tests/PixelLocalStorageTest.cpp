//
// Copyright 2022 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Rive
//

#include <regex>
#include <sstream>
#include <string>
#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

////////////////////////////////////////////////////////////////////////////////////////////////////
// GL_ANGLE_shader_pixel_local_storage prototype.
//
// NOTE: the hope is for this to eventually move into ANGLE.

#define GL_DISABLED_ANGLE 0xbaadbeef

constexpr static int MAX_LOCAL_STORAGE_PLANES                = 3;
constexpr static int MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE = 1;

// ES 3.1 unfortunately requires most image formats to be either readonly or writeonly. To work
// around this limitation, we bind the same image unit to both a readonly and a writeonly image2D.
// We mark the images as volatile since they are aliases of the same memory.
//
// The ANGLE GLSL compiler doesn't appear to support macro concatenation (e.g., NAME ## _R). For
// now, the client code is responsible to know there are two image2D variables, append "_R" for
// pixelLocalLoadImpl, and append "_W" for pixelLocalStoreImpl.
//
// NOTE: PixelLocalStorageTest::useProgram appends "_R"/"_W" for you automatically if you use
// PIXEL_LOCAL_DECL / pixelLocalLoad / pixelLocalStore.
constexpr static const char kLocalStorageGLSLDefines[] = R"(
#define PIXEL_LOCAL_DECL_IMPL(NAME_R, NAME_W, BINDING, FORMAT) \
    layout(BINDING, FORMAT) coherent volatile readonly highp uniform image2D NAME_R; \
    layout(BINDING, FORMAT) coherent volatile writeonly highp uniform image2D NAME_W
#define PIXEL_I_COORD \
    ivec2(floor(gl_FragCoord.xy))
#define pixelLocalLoadImpl(NAME_R) \
    imageLoad(NAME_R, PIXEL_I_COORD)
#define pixelLocalStoreImpl(NAME_W, VALUE) \
    { \
        memoryBarrierImage(); \
        imageStore(NAME_W, PIXEL_I_COORD, VALUE); \
        memoryBarrierImage(); \
    }
// Don't execute pixelLocalStore when depth/stencil fails.
layout(early_fragment_tests) in;
)";

class PixelLocalStoragePrototype
{
  public:
    void framebufferPixelLocalStorage(GLuint unit,
                                      GLuint backingtexture,
                                      GLint level,
                                      GLint layer,
                                      GLint width,
                                      GLint height,
                                      GLenum internalformat);
    void beginPixelLocalStorage(GLsizei n, const GLenum *loadOps);
    void pixelLocalStorageBarrier();
    void endPixelLocalStorage();

  private:
    struct LocalStoragePlane
    {
        GLuint tex;
        GLsizei width;
        GLsizei height;
        GLenum internalformat;
    };
    std::array<LocalStoragePlane, MAX_LOCAL_STORAGE_PLANES> &boundLocalStoragePlanes()
    {
        GLint drawFBO;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO);
        ASSERT(drawFBO != 0);  // GL_INVALID_OPERATION!
        return mLocalStoragePlanes[drawFBO];
    }
    std::map<GLuint, std::array<LocalStoragePlane, MAX_LOCAL_STORAGE_PLANES>> mLocalStoragePlanes;
    bool mLocalStorageEnabled = false;
    std::vector<int> mEnabledLocalStoragePlanes;
    GLint mFramebufferPreviousDefaultWidth  = 0;
    GLint mFramebufferPreviousDefaultHeight = 0;
};

// Bootstrap the draft extension assuming an in-scope PixelLocalStoragePrototype object named "pls".
#define glFramebufferPixelLocalStorageANGLE pls.framebufferPixelLocalStorage
#define glBeginPixelLocalStorageANGLE pls.beginPixelLocalStorage
#define glPixelLocalStorageBarrierANGLE pls.pixelLocalStorageBarrier
#define glEndPixelLocalStorageANGLE pls.endPixelLocalStorage

void PixelLocalStoragePrototype::framebufferPixelLocalStorage(GLuint unit,
                                                              GLuint backingtexture,
                                                              GLint level,
                                                              GLint layer,
                                                              GLsizei width,
                                                              GLsizei height,
                                                              GLenum internalformat)
{
    ASSERT(0 <= unit && unit < MAX_LOCAL_STORAGE_PLANES);  // GL_INVALID_VALUE!
    ASSERT(backingtexture != 0);                           // NOT IMPLEMENTED!
    ASSERT(level == 0);                                    // NOT IMPLEMENTED!
    ASSERT(layer == 0);                                    // NOT IMPLEMENTED!
    ASSERT(width > 0 && height > 0);                       // NOT IMPLEMENTED!
    boundLocalStoragePlanes()[unit] = {backingtexture, width, height, internalformat};
}

class AutoRestoreDrawBuffers
{
  public:
    AutoRestoreDrawBuffers()
    {
        GLint MAX_COLOR_ATTACHMENTS;
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &MAX_COLOR_ATTACHMENTS);

        GLint MAX_DRAW_BUFFERS;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &MAX_DRAW_BUFFERS);

        for (int i = 0; i < MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE; ++i)
        {
            GLint drawBuffer;
            glGetIntegerv(GL_DRAW_BUFFER0 + i, &drawBuffer);
            // glDrawBuffers must not reference an attachment at or beyond
            // MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE.
            if (GL_COLOR_ATTACHMENT0 <= drawBuffer &&
                drawBuffer < GL_COLOR_ATTACHMENT0 + MAX_COLOR_ATTACHMENTS)
            {
                // GL_INVALID_OPERATION!
                ASSERT(drawBuffer < GL_COLOR_ATTACHMENT0 + MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE);
            }
            mDrawBuffers[i] = drawBuffer;
        }
        // glDrawBuffers must be GL_NONE at or beyond MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE.
        for (int i = MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE; i < MAX_DRAW_BUFFERS; ++i)
        {
            GLint drawBuffer;
            glGetIntegerv(GL_DRAW_BUFFER0 + i, &drawBuffer);
            ASSERT(drawBuffer == GL_NONE);  // GL_INVALID_OPERATION!
        }
    }

    ~AutoRestoreDrawBuffers()
    {
        glDrawBuffers(MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE, mDrawBuffers);
    }

  private:
    GLenum mDrawBuffers[MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE];
};

class AutoRestoreClearColor
{
  public:
    AutoRestoreClearColor() { glGetFloatv(GL_COLOR_CLEAR_VALUE, mClearColor); }

    ~AutoRestoreClearColor()
    {
        glClearColor(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);
    }

  private:
    float mClearColor[4];
};

class AutoDisableScissor
{
  public:
    AutoDisableScissor()
    {
        glGetIntegerv(GL_SCISSOR_TEST, &mScissorTestEnabled);
        if (mScissorTestEnabled)
        {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    ~AutoDisableScissor()
    {
        if (mScissorTestEnabled)
        {
            glEnable(GL_SCISSOR_TEST);
        }
    }

  private:
    GLint mScissorTestEnabled;
};

void PixelLocalStoragePrototype::beginPixelLocalStorage(GLsizei n, const GLenum *loadOps)
{
    ASSERT(1 <= n && n <= MAX_LOCAL_STORAGE_PLANES);  // GL_INVALID_VALUE!
    ASSERT(!mLocalStorageEnabled);                    // GL_INVALID_OPERATION!
    ASSERT(mEnabledLocalStoragePlanes.empty());

    mLocalStorageEnabled = true;

    const auto &planes = boundLocalStoragePlanes();

    // A framebuffer must have no attachments at or beyond MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE.
    GLint MAX_COLOR_ATTACHMENTS;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &MAX_COLOR_ATTACHMENTS);
    for (int i = MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE; i < MAX_COLOR_ATTACHMENTS; ++i)
    {
        GLint type;
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
        ASSERT(type == GL_NONE);  // GL_INVALID_OPERATION!
    }

    int framebufferWidth  = 0;
    int framebufferHeight = 0;
    bool needsClear       = false;
    GLenum attachmentsToClear[MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE + MAX_LOCAL_STORAGE_PLANES];
    memset(attachmentsToClear, 0, sizeof(attachmentsToClear));

    for (int i = 0; i < n; ++i)
    {
        GLuint tex            = 0;
        GLenum internalformat = GL_RGBA8;
        if (loadOps[i] != GL_DISABLED_ANGLE)
        {
            ASSERT(planes[i].tex);  // GL_INVALID_FRAMEBUFFER_OPERATION!
            tex            = planes[i].tex;
            internalformat = planes[i].internalformat;

            // GL_INVALID_FRAMEBUFFER_OPERATION!
            ASSERT(!framebufferWidth || framebufferWidth == planes[i].width);
            ASSERT(!framebufferHeight || framebufferHeight == planes[i].height);
            framebufferWidth  = planes[i].width;
            framebufferHeight = planes[i].height;

            mEnabledLocalStoragePlanes.push_back(i);
        }
        if (loadOps[i] == GL_ZERO)
        {
            // Attach all textures that need clearing to the framebuffer.
            GLenum attachmentPoint =
                GL_COLOR_ATTACHMENT0 + MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE + i;
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, GL_TEXTURE_2D, tex, 0);
            // If the GL is bound to a draw framebuffer object, the ith buffer listed in bufs must
            // be GL_COLOR_ATTACHMENTi or GL_NONE.
            needsClear                                                      = true;
            attachmentsToClear[MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE + i] = attachmentPoint;
        }
        // Bind local storage textures to their corresponding image unit. Use GL_READ_WRITE since
        // this binding will be referenced by two image2Ds -- one readeonly and one writeonly.
        glBindImageTexture(i, tex, 0, GL_FALSE, 0, GL_READ_WRITE, internalformat);
    }
    if (needsClear)
    {
        AutoRestoreDrawBuffers autoRestoreDrawBuffers;
        AutoRestoreClearColor autoRestoreClearColor;
        AutoDisableScissor autoDisableScissor;

        glDrawBuffers(MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE + n, attachmentsToClear);
        glClearColor(0, 0, 0, 0);  // TODO: We should use glClearBuffer here.
        glClear(GL_COLOR_BUFFER_BIT);

        // Detach the textures that needed clearing.
        for (int i = 0; i < n; ++i)
        {
            if (loadOps[i] == GL_ZERO)
            {
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0 + MAX_FRAGMENT_OUTPUTS_WITH_LOCAL_STORAGE + i,
                    GL_TEXTURE_2D, 0, 0);
            }
        }
    }

    glGetFramebufferParameteriv(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH,
                                &mFramebufferPreviousDefaultWidth);
    glGetFramebufferParameteriv(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT,
                                &mFramebufferPreviousDefaultHeight);
    glFramebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, framebufferWidth);
    glFramebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, framebufferHeight);

    // Do *ALL* barriers since we don't know what the client did with memory before this point.
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void PixelLocalStoragePrototype::pixelLocalStorageBarrier()
{
    // In an ideal world we would only need GL_SHADER_IMAGE_ACCESS_BARRIER_BIT, but some drivers
    // need a bit more persuasion to get this right.
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void PixelLocalStoragePrototype::endPixelLocalStorage()
{
    ASSERT(mLocalStorageEnabled);  // GL_INVALID_OPERATION!

    // Do *ALL* barriers since we don't know what the client will do with memory after this point.
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Restore framebuffer default dimensions.
    glFramebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH,
                            mFramebufferPreviousDefaultWidth);
    glFramebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT,
                            mFramebufferPreviousDefaultHeight);

    // Unbind local storage image textures.
    for (int i : mEnabledLocalStoragePlanes)
    {
        glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    }
    mEnabledLocalStoragePlanes.clear();

    mLocalStorageEnabled = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr static int W = 128, H = 128;

template <typename T>
struct Array
{
    Array(const std::initializer_list<T> &list) : mVec(list) {}
    operator const T *() const { return mVec.data(); }
    std::vector<T> mVec;
};
template <typename T>
static Array<T> MakeArray(const std::initializer_list<T> &list)
{
    return Array<T>(list);
}
static Array<GLenum> GLenumArray(const std::initializer_list<GLenum> &list)
{
    return Array<GLenum>(list);
}

class PixelLocalStorageTest : public ANGLETest
{
  protected:
    PixelLocalStorageTest()
    {
        setWindowWidth(1);
        setWindowHeight(1);
    }

    ~PixelLocalStorageTest()
    {
        if (mScratchFBO)
        {
            glDeleteFramebuffers(1, &mScratchFBO);
        }
    }

    bool supportsPixelLocalStorage()
    {
        ASSERT(getClientMajorVersion() == 3);
        ASSERT(getClientMinorVersion() == 1);

        if (isD3D11Renderer())
        {
            // We can't implement pixel local storage via shader images on top of D3D11:
            //
            //   * D3D UAVs don't support aliasing: https://anglebug.com/3032
            //   * But ES 3.1 doesn't allow most image2D formats to be readwrite
            //   * And we can't use texelFetch because ps_5_0 does not support thread
            //     synchronization operations in shaders (aka memoryBarrier()).
            //
            // We will need to do a custom local storage implementation in D3D11 that uses
            // RWTexture2D<> or, more ideally, the coherent RasterizerOrderedTexture2D<>.
            return false;
        }

        return true;
    }

    void useProgram(std::string fsMain)
    {
        // Replace: PIXEL_LOCAL_DECL(name, ...) -> PIXEL_LOCAL_DECL_IMPL(name_R, name_W, ...)
        static std::regex kDeclPattern("PIXEL_LOCAL_DECL\\s*\\(\\s*([a-zA-Z_][a-zA-Z0-9_]*)");
        fsMain = std::regex_replace(fsMain, kDeclPattern, "PIXEL_LOCAL_DECL_IMPL($1_R, $1_W");

        // Replace: pixelLocalLoad(name) -> pixelLocalLoadImpl(name_R)
        static std::regex kLoadPattern("pixelLocalLoad\\s*\\(\\s*([a-zA-Z_][a-zA-Z0-9_]*)");
        fsMain = std::regex_replace(fsMain, kLoadPattern, "pixelLocalLoadImpl($1_R");

        // Replace: pixelLocalStore(name, ...) -> pixelLocalStoreImpl(name_W, ...)
        static std::regex kStorePattern("pixelLocalStore\\s*\\(\\s*([a-zA-Z_][a-zA-Z0-9_]*)");
        fsMain = std::regex_replace(fsMain, kStorePattern, "pixelLocalStoreImpl($1_W");

        mProgram.makeRaster(
            R"(#version 310 es
            precision highp float;

            uniform float W, H;
            uniform vec4 rect;

            void main()
            {
                gl_Position.x = ((gl_VertexID & 1) == 0 ? rect.x : rect.z) * 2.0 / W - 1.0;
                gl_Position.y = ((gl_VertexID & 2) == 0 ? rect.y : rect.w) * 2.0 / H - 1.0;
                gl_Position.zw = vec2(0, 1);
            })",

            std::string(R"(#version 310 es
            precision highp float;
            uniform vec4 color;)")
                .append(kLocalStorageGLSLDefines)
                .append(fsMain)
                .c_str());

        ASSERT_TRUE(mProgram.valid());

        glUseProgram(mProgram);

        glUniform1f(glGetUniformLocation(mProgram, "W"), W);
        glUniform1f(glGetUniformLocation(mProgram, "H"), H);

        mRectUniform = glGetUniformLocation(mProgram, "rect");
        ASSERT_TRUE(mRectUniform >= 0);

        mColorUniform = glGetUniformLocation(mProgram, "color");
        ASSERT_TRUE(mColorUniform >= 0);
    }

    void attachTextureToScratchFBO(GLuint tex)
    {
        if (!mScratchFBO)
        {
            glGenFramebuffers(1, &mScratchFBO);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, mScratchFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
    }

    GLProgram mProgram;
    GLint mRectUniform  = -1;
    GLint mColorUniform = -1;

    GLuint mScratchFBO = 0;
};

// Verify that R,G,B values from separate draw calls persist in pixel local storage.
TEST_P(PixelLocalStorageTest, RGB)
{
    ANGLE_SKIP_TEST_IF(!supportsPixelLocalStorage());

    PixelLocalStoragePrototype pls;

    GLTexture tex;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, W, H);

    GLFramebuffer fbo;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferPixelLocalStorageANGLE(0, tex, 0, 0, W, H, GL_RGBA8);
    glViewport(0, 0, W, H);
    glDrawBuffers(0, nullptr);

    glBeginPixelLocalStorageANGLE(1, GLenumArray({GL_ZERO}));

    // Accumulate R,G,B in 3 separate passes.
    useProgram(R"(
    PIXEL_LOCAL_DECL(framebuffer, binding=0, rgba8);
    void main()
    {
        vec4 dst = pixelLocalLoad(framebuffer);
        pixelLocalStore(framebuffer, color + dst);
    })");

    // Draw fullscreen rects.
    glUniform4f(mRectUniform, 0, 0, W, H);

    glUniform4f(mColorUniform, 1, 0, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glPixelLocalStorageBarrierANGLE();

    glUniform4f(mColorUniform, 0, 1, 0, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glPixelLocalStorageBarrierANGLE();

    glUniform4f(mColorUniform, 0, 0, 1, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glEndPixelLocalStorageANGLE();

    attachTextureToScratchFBO(tex);
    EXPECT_PIXEL_RECT_EQ(0, 0, W, H, GLColor(255, 255, 255, 0));

    ASSERT_GL_NO_ERROR();
}

ANGLE_INSTANTIATE_TEST_ES31(PixelLocalStorageTest);
