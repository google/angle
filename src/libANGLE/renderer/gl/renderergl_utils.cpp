//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer11_utils.cpp: Conversion functions and other utility routines
// specific to the OpenGL renderer.

#include "libANGLE/renderer/gl/renderergl_utils.h"

#include <limits>

#include "libANGLE/Caps.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/formatutilsgl.h"

#include <algorithm>

namespace rx
{

namespace nativegl
{

void GetGLVersion(PFNGLGETSTRINGPROC getStringFunction, GLuint *outMajorVersion, GLuint *outMinorVersion,
                  bool *outIsES)
{
    const std::string version = reinterpret_cast<const char*>(getStringFunction(GL_VERSION));
    if (version.find("OpenGL ES") == std::string::npos)
    {
        // ES spec states that the GL_VERSION string will be in the following format:
        // "OpenGL ES N.M vendor-specific information"
        *outIsES = false;
        *outMajorVersion = version[0] - '0';
        *outMinorVersion = version[2] - '0';
    }
    else
    {
        // OpenGL spec states the GL_VERSION string will be in the following format:
        // <version number><space><vendor-specific information>
        // The version number is either of the form major number.minor number or major
        // number.minor number.release number, where the numbers all have one or more
        // digits
        *outIsES = true;
        *outMajorVersion = version[10] - '0';
        *outMinorVersion = version[12] - '0';
    }
}

std::vector<std::string> GetGLExtensions(PFNGLGETSTRINGPROC getStringFunction)
{
    std::vector<std::string> result;

    std::istringstream stream(reinterpret_cast<const char*>(getStringFunction(GL_EXTENSIONS)));
    std::string extension;
    while (std::getline(stream, extension, ' '))
    {
        result.push_back(extension);
    }

    return result;
}

}

namespace nativegl_gl
{

static gl::TextureCaps GenerateTextureFormatCaps(const FunctionsGL *functions, GLenum internalFormat, GLuint majorVersion, GLuint minorVersion,
                                                 const std::vector<std::string> &extensions)
{
    gl::TextureCaps textureCaps;

    const nativegl::InternalFormat &formatInfo = nativegl::GetInternalFormatInfo(internalFormat);
    textureCaps.texturable = formatInfo.textureSupport(majorVersion, minorVersion, extensions);
    textureCaps.renderable = formatInfo.renderSupport(majorVersion, minorVersion, extensions);
    textureCaps.filterable = formatInfo.filterSupport(majorVersion, minorVersion, extensions);

    return textureCaps;
}

static GLint QuerySingleGLInt(const FunctionsGL *functions, GLenum name)
{
    GLint result;
    functions->getIntegerv(name, &result);
    return result;
}

void GenerateCaps(const FunctionsGL *functions, gl::Caps *caps, gl::TextureCapsMap *textureCapsMap,
                  gl::Extensions *extensions)
{
    GLuint majorVersion = 0;
    GLuint minorVersion = 0;
    bool isES = false;
    nativegl::GetGLVersion(functions->getString, &majorVersion, &minorVersion, &isES);

    std::vector<std::string> nativeExtensions = nativegl::GetGLExtensions(functions->getString);

    // Texture format support checks
    GLuint maxSamples = 0;
    const gl::FormatSet &allFormats = gl::GetAllSizedInternalFormats();
    for (GLenum internalFormat : allFormats)
    {
        gl::TextureCaps textureCaps = GenerateTextureFormatCaps(functions, internalFormat, majorVersion, minorVersion, nativeExtensions);
        textureCapsMap->insert(internalFormat, textureCaps);

        maxSamples = std::max(maxSamples, textureCaps.getMaxSamples());

        if (gl::GetInternalFormatInfo(internalFormat).compressed)
        {
            caps->compressedTextureFormats.push_back(internalFormat);
        }
    }

    // Set some minimum GLES2 caps, TODO: query for real GL caps

    // Table 6.28, implementation dependent values
    caps->maxElementIndex = static_cast<GLint64>(std::numeric_limits<unsigned int>::max());
    caps->max3DTextureSize = QuerySingleGLInt(functions, GL_MAX_3D_TEXTURE_SIZE);
    caps->max2DTextureSize = QuerySingleGLInt(functions, GL_MAX_TEXTURE_SIZE);
    caps->maxCubeMapTextureSize = QuerySingleGLInt(functions, GL_MAX_CUBE_MAP_TEXTURE_SIZE);
    caps->maxArrayTextureLayers = QuerySingleGLInt(functions, GL_MAX_ARRAY_TEXTURE_LAYERS);
    caps->maxLODBias = 2.0f;
    caps->maxRenderbufferSize = QuerySingleGLInt(functions, GL_MAX_RENDERBUFFER_SIZE);
    caps->maxDrawBuffers = QuerySingleGLInt(functions, GL_MAX_DRAW_BUFFERS);
    caps->maxColorAttachments = QuerySingleGLInt(functions, GL_MAX_COLOR_ATTACHMENTS);
    caps->maxViewportWidth = caps->max2DTextureSize;
    caps->maxViewportHeight = caps->maxViewportWidth;
    caps->minAliasedPointSize = 1.0f;
    caps->maxAliasedPointSize = 1.0f;
    caps->minAliasedLineWidth = 1.0f;
    caps->maxAliasedLineWidth = 1.0f;

    // Table 6.29, implementation dependent values (cont.)
    caps->maxElementsIndices = 0;
    caps->maxElementsVertices = 0;
    caps->vertexHighpFloat.setIEEEFloat();
    caps->vertexMediumpFloat.setIEEEFloat();
    caps->vertexLowpFloat.setIEEEFloat();
    caps->fragmentHighpFloat.setIEEEFloat();
    caps->fragmentMediumpFloat.setIEEEFloat();
    caps->fragmentLowpFloat.setIEEEFloat();
    caps->vertexHighpInt.setTwosComplementInt(32);
    caps->vertexMediumpInt.setTwosComplementInt(32);
    caps->vertexLowpInt.setTwosComplementInt(32);
    caps->fragmentHighpInt.setTwosComplementInt(32);
    caps->fragmentMediumpInt.setTwosComplementInt(32);
    caps->fragmentLowpInt.setTwosComplementInt(32);
    caps->maxServerWaitTimeout = 0;

    // Table 6.31, implementation dependent vertex shader limits
    caps->maxVertexAttributes = 16;
    caps->maxVertexUniformComponents = 1024;
    caps->maxVertexUniformVectors = 256;
    caps->maxVertexUniformBlocks = 12;
    caps->maxVertexOutputComponents = 64;
    caps->maxVertexTextureImageUnits = QuerySingleGLInt(functions, GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);

    // Table 6.32, implementation dependent fragment shader limits
    caps->maxFragmentUniformComponents = 896;
    caps->maxFragmentUniformVectors = 224;
    caps->maxFragmentUniformBlocks = 12;
    caps->maxFragmentInputComponents = 60;
    caps->maxTextureImageUnits = QuerySingleGLInt(functions, GL_MAX_TEXTURE_IMAGE_UNITS);
    caps->minProgramTexelOffset = -8;
    caps->maxProgramTexelOffset = 7;

    // Table 6.33, implementation dependent aggregate shader limits
    caps->maxUniformBufferBindings = 24;
    caps->maxUniformBlockSize = 16384;
    caps->uniformBufferOffsetAlignment = 1;
    caps->maxCombinedUniformBlocks = 24;
    caps->maxCombinedVertexUniformComponents = (caps->maxVertexUniformBlocks * (caps->maxUniformBlockSize / 4)) + caps->maxVertexUniformComponents;
    caps->maxCombinedFragmentUniformComponents = (caps->maxFragmentUniformBlocks * (caps->maxUniformBlockSize / 4)) + caps->maxFragmentUniformComponents;
    caps->maxVaryingComponents = 60;
    caps->maxVaryingVectors = 15;
    caps->maxCombinedTextureImageUnits = caps->maxVertexTextureImageUnits + caps->maxTextureImageUnits;

    // Table 6.34, implementation dependent transform feedback limits
    caps->maxTransformFeedbackInterleavedComponents = 64;
    caps->maxTransformFeedbackSeparateAttributes = 4;
    caps->maxTransformFeedbackSeparateComponents = 4;

    // Extension support
    extensions->setTextureExtensionSupport(*textureCapsMap);
    extensions->textureNPOT = true;
    extensions->textureStorage = true;
}

}

}
