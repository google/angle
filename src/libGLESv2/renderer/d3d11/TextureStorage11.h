//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage11.h: Defines the abstract rx::TextureStorage11 class and its concrete derived
// classes TextureStorage11_2D and TextureStorage11_Cube, which act as the interface to the D3D11 texture.

#ifndef LIBGLESV2_RENDERER_TEXTURESTORAGE11_H_
#define LIBGLESV2_RENDERER_TEXTURESTORAGE11_H_

#include "libGLESv2/Texture.h"
#include "libGLESv2/renderer/TextureStorage.h"

namespace rx
{
class RenderTarget;
class RenderTarget11;
class Renderer;
class Renderer11;
class SwapChain11;

class TextureStorage11 : public TextureStorage
{
  public:
    virtual ~TextureStorage11();

    static TextureStorage11 *makeTextureStorage11(TextureStorage *storage);

    static DWORD GetTextureBindFlags(GLenum internalFormat, GLuint clientVersion, bool renderTarget);

    UINT getBindFlags() const;

    virtual ID3D11Resource *getBaseTexture() const = 0;
    virtual ID3D11ShaderResourceView *getSRV(const gl::SamplerState &samplerState) = 0;
    virtual RenderTarget *getRenderTarget(int level) { return NULL; }
    virtual RenderTarget *getRenderTargetFace(GLenum faceTarget, int level) { return NULL; }
    virtual RenderTarget *getRenderTargetLayer(int mipLevel, int layer) { return NULL; }

    virtual void generateMipmap(int level) {};
    virtual void generateMipmap(int face, int level) {};

    virtual int getTopLevel() const;
    virtual bool isRenderTarget() const;
    virtual bool isManaged() const;
    virtual int getBaseLevel() const;
    virtual int getMaxLevel() const;
    UINT getSubresourceIndex(int mipLevel, int layerTarget) const;

    void generateSwizzles(GLenum swizzleRed, GLenum swizzleGreen, GLenum swizzleBlue, GLenum swizzleAlpha);
    void invalidateSwizzleCacheLevel(int mipLevel);
    void invalidateSwizzleCache();

    bool updateSubresourceLevel(ID3D11Resource *texture, unsigned int sourceSubresource, int level,
                                int layerTarget, GLint xoffset, GLint yoffset, GLint zoffset,
                                GLsizei width, GLsizei height, GLsizei depth);

  protected:
    TextureStorage11(Renderer *renderer, int baseLevel, UINT bindFlags);
    void generateMipmapLayer(RenderTarget11 *source, RenderTarget11 *dest);
    int getLevelWidth(int mipLevel) const;
    int getLevelHeight(int mipLevel) const;
    int getLevelDepth(int mipLevel) const;

    virtual ID3D11RenderTargetView *getSwizzleRenderTarget(int mipLevel) = 0;
    virtual ID3D11ShaderResourceView *getSRVLevel(int mipLevel) = 0;

    void verifySwizzleExists(GLenum swizzleRed, GLenum swizzleGreen, GLenum swizzleBlue, GLenum swizzleAlpha);

    virtual unsigned int getTextureLevelDepth(int mipLevel) const = 0;

    Renderer11 *mRenderer;
    int mTopLevel;
    unsigned int mMipLevels;
    int mBaseLevel;

    DXGI_FORMAT mTextureFormat;
    DXGI_FORMAT mShaderResourceFormat;
    DXGI_FORMAT mRenderTargetFormat;
    DXGI_FORMAT mDepthStencilFormat;
    DXGI_FORMAT mSwizzleTextureFormat;
    DXGI_FORMAT mSwizzleShaderResourceFormat;
    DXGI_FORMAT mSwizzleRenderTargetFormat;
    unsigned int mTextureWidth;
    unsigned int mTextureHeight;
    unsigned int mTextureDepth;

    struct SwizzleCacheValue
    {
        GLenum swizzleRed;
        GLenum swizzleGreen;
        GLenum swizzleBlue;
        GLenum swizzleAlpha;

        SwizzleCacheValue();
        SwizzleCacheValue(GLenum red, GLenum green, GLenum blue, GLenum alpha);

        bool operator ==(const SwizzleCacheValue &other) const;
        bool operator !=(const SwizzleCacheValue &other) const;
    };
    SwizzleCacheValue mSwizzleCache[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage11);

    const UINT mBindFlags;
};

class TextureStorage11_2D : public TextureStorage11
{
  public:
    TextureStorage11_2D(Renderer *renderer, SwapChain11 *swapchain);
    TextureStorage11_2D(Renderer *renderer, int baseLevel, int maxLevel, GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height);
    virtual ~TextureStorage11_2D();

    static TextureStorage11_2D *makeTextureStorage11_2D(TextureStorage *storage);

    virtual ID3D11Resource *getBaseTexture() const;
    virtual ID3D11ShaderResourceView *getSRV(const gl::SamplerState &samplerState);
    virtual RenderTarget *getRenderTarget(int level);

    virtual void generateMipmap(int level);

  protected:
    ID3D11Texture2D *getSwizzleTexture();
    virtual ID3D11RenderTargetView *getSwizzleRenderTarget(int mipLevel);

    virtual ID3D11ShaderResourceView *getSRVLevel(int mipLevel);

    virtual unsigned int getTextureLevelDepth(int mipLevel) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage11_2D);

    ID3D11Texture2D *mTexture;
    RenderTarget11 *mRenderTarget[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    ID3D11Texture2D *mSwizzleTexture;
    ID3D11RenderTargetView *mSwizzleRenderTargets[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    ID3D11ShaderResourceView *mSRV[2][2];   // [swizzle][mipmapping]
    ID3D11ShaderResourceView *mLevelSRVs[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

class TextureStorage11_Cube : public TextureStorage11
{
  public:
    TextureStorage11_Cube(Renderer *renderer, int baseLevel, int maxLevel, GLenum internalformat, bool renderTarget, int size);
    virtual ~TextureStorage11_Cube();

    static TextureStorage11_Cube *makeTextureStorage11_Cube(TextureStorage *storage);

    virtual ID3D11Resource *getBaseTexture() const;
    virtual ID3D11ShaderResourceView *getSRV(const gl::SamplerState &samplerState);
    virtual RenderTarget *getRenderTargetFace(GLenum faceTarget, int level);

    virtual void generateMipmap(int faceIndex, int level);

  protected:
    ID3D11Texture2D *getSwizzleTexture();
    virtual ID3D11RenderTargetView *getSwizzleRenderTarget(int mipLevel);

    virtual ID3D11ShaderResourceView *getSRVLevel(int mipLevel);

    virtual unsigned int getTextureLevelDepth(int mipLevel) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage11_Cube);

    ID3D11Texture2D *mTexture;
    RenderTarget11 *mRenderTarget[6][gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    ID3D11Texture2D *mSwizzleTexture;
    ID3D11RenderTargetView *mSwizzleRenderTargets[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    ID3D11ShaderResourceView *mSRV[2][2];   // [swizzle][mipmapping]
    ID3D11ShaderResourceView *mLevelSRVs[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

class TextureStorage11_3D : public TextureStorage11
{
  public:
    TextureStorage11_3D(Renderer *renderer, int baseLevel, int maxLevel, GLenum internalformat, bool renderTarget,
                        GLsizei width, GLsizei height, GLsizei depth);
    virtual ~TextureStorage11_3D();

    static TextureStorage11_3D *makeTextureStorage11_3D(TextureStorage *storage);

    virtual ID3D11Resource *getBaseTexture() const;
    virtual ID3D11ShaderResourceView *getSRV(const gl::SamplerState &samplerState);
    virtual RenderTarget *getRenderTarget(int mipLevel);
    virtual RenderTarget *getRenderTargetLayer(int mipLevel, int layer);

    virtual void generateMipmap(int level);

  protected:
    ID3D11Texture3D *getSwizzleTexture();
    virtual ID3D11RenderTargetView *getSwizzleRenderTarget(int mipLevel);

    virtual ID3D11ShaderResourceView *getSRVLevel(int mipLevel);

    virtual unsigned int getTextureLevelDepth(int mipLevel) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage11_3D);

    typedef std::pair<int, int> LevelLayerKey;
    typedef std::map<LevelLayerKey, RenderTarget11*> RenderTargetMap;
    RenderTargetMap mLevelLayerRenderTargets;

    RenderTarget11 *mLevelRenderTargets[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    ID3D11Texture3D *mTexture;
    ID3D11Texture3D *mSwizzleTexture;
    ID3D11RenderTargetView *mSwizzleRenderTargets[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    ID3D11ShaderResourceView *mSRV[2][2];   // [swizzle][mipmapping]
    ID3D11ShaderResourceView *mLevelSRVs[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

class TextureStorage11_2DArray : public TextureStorage11
{
  public:
    TextureStorage11_2DArray(Renderer *renderer, int baseLevel, int maxLevel, GLenum internalformat, bool renderTarget,
                             GLsizei width, GLsizei height, GLsizei depth);
    virtual ~TextureStorage11_2DArray();

    static TextureStorage11_2DArray *makeTextureStorage11_2DArray(TextureStorage *storage);

    virtual ID3D11Resource *getBaseTexture() const;
    virtual ID3D11ShaderResourceView *getSRV(const gl::SamplerState &samplerState);
    virtual RenderTarget *getRenderTargetLayer(int mipLevel, int layer);

    virtual void generateMipmap(int level);

  protected:
    ID3D11Texture2D *getSwizzleTexture();
    virtual ID3D11RenderTargetView *getSwizzleRenderTarget(int mipLevel);

    virtual ID3D11ShaderResourceView *getSRVLevel(int mipLevel);

    virtual unsigned int getTextureLevelDepth(int mipLevel) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage11_2DArray);

    typedef std::pair<int, int> LevelLayerKey;
    typedef std::map<LevelLayerKey, RenderTarget11*> RenderTargetMap;
    RenderTargetMap mRenderTargets;

    ID3D11Texture2D *mTexture;

    ID3D11Texture2D *mSwizzleTexture;
    ID3D11RenderTargetView *mSwizzleRenderTargets[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    ID3D11ShaderResourceView *mSRV[2][2];   // [swizzle][mipmapping]
    ID3D11ShaderResourceView *mLevelSRVs[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

}

#endif // LIBGLESV2_RENDERER_TEXTURESTORAGE11_H_
