#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Shader.cpp: Implements the gl::Shader class and its  derived classes
// VertexShader and FragmentShader. Implements GL shader objects and related
// functionality. [OpenGL ES 2.0.24] section 2.10 page 24 and section 3.8 page 84.

#include "libGLESv2/Shader.h"

#include "GLSLANG/ShaderLang.h"
#include "common/utilities.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Constants.h"
#include "libGLESv2/ResourceManager.h"

namespace gl
{
void *Shader::mFragmentCompiler = NULL;
void *Shader::mVertexCompiler = NULL;

Shader::Shader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle)
    : mHandle(handle), mRenderer(renderer), mResourceManager(manager)
{
    uncompile();
    initializeCompiler();

    mRefCount = 0;
    mDeleteStatus = false;
    mShaderVersion = 100;
}

Shader::~Shader()
{
}

GLuint Shader::getHandle() const
{
    return mHandle;
}

void Shader::setSource(GLsizei count, const char *const *string, const GLint *length)
{
    std::ostringstream stream;

    for (int i = 0; i < count; i++)
    {
        stream << string[i];
    }

    mSource = stream.str();
}

int Shader::getInfoLogLength() const
{
    return mInfoLog.empty() ? 0 : (mInfoLog.length() + 1);
}

void Shader::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog) const
{
    int index = 0;

    if (bufSize > 0)
    {
        index = std::min(bufSize - 1, static_cast<GLsizei>(mInfoLog.length()));
        memcpy(infoLog, mInfoLog.c_str(), index);

        infoLog[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

int Shader::getSourceLength() const
{
    return mSource.empty() ? 0 : (mSource.length() + 1);
}

int Shader::getTranslatedSourceLength() const
{
    return mHlsl.empty() ? 0 : (mHlsl.length() + 1);
}

void Shader::getSourceImpl(const std::string &source, GLsizei bufSize, GLsizei *length, char *buffer) const
{
    int index = 0;

    if (bufSize > 0)
    {
        index = std::min(bufSize - 1, static_cast<GLsizei>(source.length()));
        memcpy(buffer, source.c_str(), index);

        buffer[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

void Shader::getSource(GLsizei bufSize, GLsizei *length, char *buffer) const
{
    getSourceImpl(mSource, bufSize, length, buffer);
}

void Shader::getTranslatedSource(GLsizei bufSize, GLsizei *length, char *buffer) const
{
    getSourceImpl(mHlsl, bufSize, length, buffer);
}

const sh::ActiveUniforms &Shader::getUniforms() const
{
    return mActiveUniforms;
}

const sh::ActiveInterfaceBlocks &Shader::getInterfaceBlocks() const
{
    return mActiveInterfaceBlocks;
}

bool Shader::isCompiled() const
{
    return !mHlsl.empty();
}

const std::string &Shader::getHLSL() const
{
    return mHlsl;
}

void Shader::addRef()
{
    mRefCount++;
}

void Shader::release()
{
    mRefCount--;

    if (mRefCount == 0 && mDeleteStatus)
    {
        mResourceManager->deleteShader(mHandle);
    }
}

unsigned int Shader::getRefCount() const
{
    return mRefCount;
}

bool Shader::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}

void Shader::flagForDeletion()
{
    mDeleteStatus = true;
}

// Perform a one-time initialization of the shader compiler (or after being destructed by releaseCompiler)
void Shader::initializeCompiler()
{
    if (!mFragmentCompiler)
    {
        int result = ShInitialize();

        if (result)
        {
            ShShaderOutput hlslVersion = (mRenderer->getMajorShaderModel() >= 4) ? SH_HLSL11_OUTPUT : SH_HLSL9_OUTPUT;

            ShBuiltInResources resources;
            ShInitBuiltInResources(&resources);

            resources.MaxVertexAttribs = MAX_VERTEX_ATTRIBS;
            resources.MaxVertexUniformVectors = mRenderer->getMaxVertexUniformVectors();
            resources.MaxVaryingVectors = mRenderer->getMaxVaryingVectors();
            resources.MaxVertexTextureImageUnits = mRenderer->getMaxVertexTextureImageUnits();
            resources.MaxCombinedTextureImageUnits = mRenderer->getMaxCombinedTextureImageUnits();
            resources.MaxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
            resources.MaxFragmentUniformVectors = mRenderer->getMaxFragmentUniformVectors();
            resources.MaxDrawBuffers = mRenderer->getMaxRenderTargets();
            resources.OES_standard_derivatives = mRenderer->getDerivativeInstructionSupport();
            resources.EXT_draw_buffers = mRenderer->getMaxRenderTargets() > 1;
            // resources.OES_EGL_image_external = mRenderer->getShareHandleSupport() ? 1 : 0; // TODO: commented out until the extension is actually supported.
            resources.FragmentPrecisionHigh = 1;   // Shader Model 2+ always supports FP24 (s16e7) which corresponds to highp
            resources.EXT_frag_depth = 1; // Shader Model 2+ always supports explicit depth output
            // GLSL ES 3.0 constants
            resources.MaxVertexOutputVectors = mRenderer->getMaxVaryingVectors();
            resources.MaxFragmentInputVectors = mRenderer->getMaxVaryingVectors();
            resources.MinProgramTexelOffset = -8;   // D3D10_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE
            resources.MaxProgramTexelOffset = 7;    // D3D10_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE

            mFragmentCompiler = ShConstructCompiler(SH_FRAGMENT_SHADER, SH_GLES2_SPEC, hlslVersion, &resources);
            mVertexCompiler = ShConstructCompiler(SH_VERTEX_SHADER, SH_GLES2_SPEC, hlslVersion, &resources);
        }
    }
}

void Shader::releaseCompiler()
{
    ShDestruct(mFragmentCompiler);
    ShDestruct(mVertexCompiler);

    mFragmentCompiler = NULL;
    mVertexCompiler = NULL;

    ShFinalize();
}

void Shader::parseVaryings()
{
    if (!mHlsl.empty())
    {
        const std::string varyingsTitle("// Varyings");
        size_t input = mHlsl.find(varyingsTitle);
        if (input != std::string::npos)
        {
            input += varyingsTitle.length() + 1;
        }

        while(input != std::string::npos)
        {
            char string1[256];
            char string2[256];
            char string3[256];

            int matches = sscanf(mHlsl.c_str() + input, "static %255s %255s %255s", string1, string2, string3);

            char *interpolation = "linear";   // Default
            char *type = string1;
            char *name = string2;

            if (matches == 0)
            {
                break;
            }
            else if (matches == 3)
            {
                if (string3[0] != '=')   // Explicit interpolation qualifier
                {
                    type = string2;
                    name = string3;
                }
            }
            else UNREACHABLE();

            char *array = strstr(name, "[");
            int size = 1;

            if (array)
            {
                size = atoi(array + 1);
                *array = '\0';
            }

            mVaryings.push_back(Varying(parseInterpolation(interpolation), parseType(type), name, size, array != NULL));

            const std::string semiColon(";");
            input = mHlsl.find(semiColon, input);
            if (input != std::string::npos)
            {
                input += semiColon.length() + 1;
            }
        }

        mUsesMultipleRenderTargets = mHlsl.find("GL_USES_MRT")          != std::string::npos;
        mUsesFragColor             = mHlsl.find("GL_USES_FRAG_COLOR")   != std::string::npos;
        mUsesFragData              = mHlsl.find("GL_USES_FRAG_DATA")    != std::string::npos;
        mUsesFragCoord             = mHlsl.find("GL_USES_FRAG_COORD")   != std::string::npos;
        mUsesFrontFacing           = mHlsl.find("GL_USES_FRONT_FACING") != std::string::npos;
        mUsesPointSize             = mHlsl.find("GL_USES_POINT_SIZE")   != std::string::npos;
        mUsesPointCoord            = mHlsl.find("GL_USES_POINT_COORD")  != std::string::npos;
        mUsesDepthRange            = mHlsl.find("GL_USES_DEPTH_RANGE")  != std::string::npos;
        mUsesFragDepth             = mHlsl.find("GL_USES_FRAG_DEPTH")   != std::string::npos;
    }
}

void Shader::resetVaryingsRegisterAssignment()
{
    for (VaryingList::iterator var = mVaryings.begin(); var != mVaryings.end(); var++)
    {
        var->reg = -1;
        var->col = -1;
    }
}

// initialize/clean up previous state
void Shader::uncompile()
{
    // set by compileToHLSL
    mHlsl.clear();
    mInfoLog.clear();

    // set by parseVaryings
    mVaryings.clear();

    mUsesMultipleRenderTargets = false;
    mUsesFragColor = false;
    mUsesFragData = false;
    mUsesFragCoord = false;
    mUsesFrontFacing = false;
    mUsesPointSize = false;
    mUsesPointCoord = false;
    mUsesDepthRange = false;
    mUsesFragDepth = false;
    mShaderVersion = 100;

    mActiveUniforms.clear();
    mActiveInterfaceBlocks.clear();
}

void Shader::compileToHLSL(void *compiler)
{
    // ensure the compiler is loaded
    initializeCompiler();

    int compileOptions = SH_OBJECT_CODE;
    std::string sourcePath;
    if (perfActive())
    {
        sourcePath = getTempPath();
        writeFile(sourcePath.c_str(), mSource.c_str(), mSource.length());
        compileOptions |= SH_LINE_DIRECTIVES;
    }

    int result;
    if (sourcePath.empty())
    {
        const char* sourceStrings[] =
        {
            mSource.c_str(),
        };

        result = ShCompile(compiler, sourceStrings, ArraySize(sourceStrings), compileOptions);
    }
    else
    {
        const char* sourceStrings[] =
        {
            sourcePath.c_str(),
            mSource.c_str(),
        };

        result = ShCompile(compiler, sourceStrings, ArraySize(sourceStrings), compileOptions | SH_SOURCE_PATH);
    }

    size_t shaderVersion = 100;
    ShGetInfo(compiler, SH_SHADER_VERSION, &shaderVersion);

    mShaderVersion = static_cast<int>(shaderVersion);

    if (shaderVersion == 300 && mRenderer->getCurrentClientVersion() < 3)
    {
        mInfoLog = "GLSL ES 3.00 is not supported by OpenGL ES 2.0 contexts";
        TRACE("\n%s", mInfoLog.c_str());
    }
    else if (result)
    {
        size_t objCodeLen = 0;
        ShGetInfo(compiler, SH_OBJECT_CODE_LENGTH, &objCodeLen);

        char* outputHLSL = new char[objCodeLen];
        ShGetObjectCode(compiler, outputHLSL);

#ifdef _DEBUG
        std::ostringstream hlslStream;
        hlslStream << "// GLSL\n";
        hlslStream << "//\n";

        size_t curPos = 0;
        while (curPos != std::string::npos)
        {
            size_t nextLine = mSource.find("\n", curPos);
            size_t len = (nextLine == std::string::npos) ? std::string::npos : (nextLine - curPos + 1);

            hlslStream << "// " << mSource.substr(curPos, len);

            curPos = (nextLine == std::string::npos) ? std::string::npos : (nextLine + 1);
        }
        hlslStream << "\n\n";
        hlslStream << outputHLSL;
        mHlsl = hlslStream.str();
#else
        mHlsl = outputHLSL;
#endif

        delete[] outputHLSL;

        void *activeUniforms;
        ShGetInfoPointer(compiler, SH_ACTIVE_UNIFORMS_ARRAY, &activeUniforms);
        mActiveUniforms = *(sh::ActiveUniforms*)activeUniforms;

        void *activeInterfaceBlocks;
        ShGetInfoPointer(compiler, SH_ACTIVE_INTERFACE_BLOCKS_ARRAY, &activeInterfaceBlocks);
        mActiveInterfaceBlocks = *(sh::ActiveInterfaceBlocks*)activeInterfaceBlocks;
    }
    else
    {
        size_t infoLogLen = 0;
        ShGetInfo(compiler, SH_INFO_LOG_LENGTH, &infoLogLen);

        char* infoLog = new char[infoLogLen];
        ShGetInfoLog(compiler, infoLog);
        mInfoLog = infoLog;

        TRACE("\n%s", mInfoLog.c_str());
    }
}

Interpolation Shader::parseInterpolation(const std::string &type)
{
    if (type == "linear")
    {
        return Smooth;
    }
    else if (type == "centroid")
    {
        return Centroid;
    }
    else if (type == "nointerpolation")
    {
        return Flat;
    }
    else UNREACHABLE();

    return Smooth;
}

GLenum Shader::parseType(const std::string &type)
{
    if (type == "float")
    {
        return GL_FLOAT;
    }
    else if (type == "float2")
    {
        return GL_FLOAT_VEC2;
    }
    else if (type == "float3")
    {
        return GL_FLOAT_VEC3;
    }
    else if (type == "float4")
    {
        return GL_FLOAT_VEC4;
    }
    else if (type == "float2x2")
    {
        return GL_FLOAT_MAT2;
    }
    else if (type == "float3x3")
    {
        return GL_FLOAT_MAT3;
    }
    else if (type == "float4x4")
    {
        return GL_FLOAT_MAT4;
    }
    else if (type == "float2x3")
    {
        return GL_FLOAT_MAT2x3;
    }
    else if (type == "float3x2")
    {
        return GL_FLOAT_MAT3x2;
    }
    else if (type == "float2x4")
    {
        return GL_FLOAT_MAT2x4;
    }
    else if (type == "float4x2")
    {
        return GL_FLOAT_MAT4x2;
    }
    else if (type == "float3x4")
    {
        return GL_FLOAT_MAT3x4;
    }
    else if (type == "float4x3")
    {
        return GL_FLOAT_MAT4x3;
    }
    else if (type == "int")
    {
        return GL_INT;
    }
    else if (type == "int2")
    {
        return GL_INT_VEC2;
    }
    else if (type == "int3")
    {
        return GL_INT_VEC3;
    }
    else if (type == "int4")
    {
        return GL_INT_VEC4;
    }
    else if (type == "uint")
    {
        return GL_UNSIGNED_INT;
    }
    else if (type == "uint2")
    {
        return GL_UNSIGNED_INT_VEC2;
    }
    else if (type == "uint3")
    {
        return GL_UNSIGNED_INT_VEC3;
    }
    else if (type == "uint4")
    {
        return GL_UNSIGNED_INT_VEC4;
    }
    else UNREACHABLE();

    return GL_NONE;
}

typedef std::map<GLenum, int> VaryingPriorityMap;
static VaryingPriorityMap varyingPriorities;

static void makeVaryingPriorityMap()
{
    varyingPriorities[GL_FLOAT_MAT4]    = 0;
    varyingPriorities[GL_FLOAT_MAT3x4]  = 10;
    varyingPriorities[GL_FLOAT_MAT4x3]  = 20;
    varyingPriorities[GL_FLOAT_MAT2x4]  = 30;
    varyingPriorities[GL_FLOAT_MAT4x2]  = 40;
    varyingPriorities[GL_FLOAT_MAT2]    = 50;
    varyingPriorities[GL_FLOAT_VEC4]    = 60;
    varyingPriorities[GL_INT_VEC4]      = 61;
    varyingPriorities[GL_UNSIGNED_INT_VEC4] = 62;
    varyingPriorities[GL_FLOAT_MAT3]    = 70;
    varyingPriorities[GL_FLOAT_MAT2x3]  = 80;
    varyingPriorities[GL_FLOAT_MAT3x2]  = 90;
    varyingPriorities[GL_FLOAT_VEC3]    = 100;
    varyingPriorities[GL_INT_VEC3]      = 101;
    varyingPriorities[GL_UNSIGNED_INT_VEC3] = 102;
    varyingPriorities[GL_FLOAT_VEC2]    = 110;
    varyingPriorities[GL_INT_VEC2]      = 111;
    varyingPriorities[GL_UNSIGNED_INT_VEC2] = 112;
    varyingPriorities[GL_FLOAT]         = 120;
    varyingPriorities[GL_INT]           = 125;
    varyingPriorities[GL_UNSIGNED_INT]  = 130;
}

// true if varying x has a higher priority in packing than y
bool Shader::compareVarying(const Varying &x, const Varying &y)
{
    if (varyingPriorities.empty())
    {
        makeVaryingPriorityMap();
    }

    if(x.type == y.type)
    {
        return x.size > y.size;
    }

    VaryingPriorityMap::iterator xPriority = varyingPriorities.find(x.type);
    VaryingPriorityMap::iterator yPriority = varyingPriorities.find(y.type);

    ASSERT(xPriority != varyingPriorities.end());
    ASSERT(yPriority != varyingPriorities.end());

    return xPriority->second <= yPriority->second;
}

int Shader::getShaderVersion() const
{
    return mShaderVersion;
}

VertexShader::VertexShader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle)
    : Shader(manager, renderer, handle)
{
}

VertexShader::~VertexShader()
{
}

GLenum VertexShader::getType()
{
    return GL_VERTEX_SHADER;
}

void VertexShader::uncompile()
{
    Shader::uncompile();

    // set by ParseAttributes
    mActiveAttributes.clear();
}

void VertexShader::compile()
{
    uncompile();

    compileToHLSL(mVertexCompiler);
    parseAttributes();
    parseVaryings();
}

int VertexShader::getSemanticIndex(const std::string &attributeName)
{
    if (!attributeName.empty())
    {
        int semanticIndex = 0;
        for (unsigned int attributeIndex = 0; attributeIndex < mActiveAttributes.size(); attributeIndex++)
        {
            const sh::ShaderVariable &attribute = mActiveAttributes[attributeIndex];

            if (attribute.name == attributeName)
            {
                return semanticIndex;
            }

            semanticIndex += AttributeRegisterCount(attribute.type);
        }
    }

    return -1;
}

void VertexShader::parseAttributes()
{
    const std::string &hlsl = getHLSL();
    if (!hlsl.empty())
    {
        void *activeAttributes;
        ShGetInfoPointer(mVertexCompiler, SH_ACTIVE_ATTRIBUTES_ARRAY, &activeAttributes);
        mActiveAttributes = *(sh::ActiveShaderVariables*)activeAttributes;
    }
}

FragmentShader::FragmentShader(ResourceManager *manager, const rx::Renderer *renderer, GLuint handle)
    : Shader(manager, renderer, handle)
{
}

FragmentShader::~FragmentShader()
{
}

GLenum FragmentShader::getType()
{
    return GL_FRAGMENT_SHADER;
}

void FragmentShader::compile()
{
    uncompile();

    compileToHLSL(mFragmentCompiler);
    parseVaryings();
    mVaryings.sort(compareVarying);

    const std::string &hlsl = getHLSL();
    if (!hlsl.empty())
    {
        void *activeOutputVariables;
        ShGetInfoPointer(mFragmentCompiler, SH_ACTIVE_OUTPUT_VARIABLES_ARRAY, &activeOutputVariables);
        mActiveOutputVariables = *(sh::ActiveShaderVariables*)activeOutputVariables;
    }
}

void FragmentShader::uncompile()
{
    Shader::uncompile();

    mActiveOutputVariables.clear();
}

const sh::ActiveShaderVariables &FragmentShader::getOutputVariables() const
{
    return mActiveOutputVariables;
}

}
