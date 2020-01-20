//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Wrapper for Khronos glslang compiler.
//

#include "libANGLE/renderer/glslang_wrapper_utils.h"

// glslang has issues with some specific warnings.
ANGLE_DISABLE_EXTRA_SEMI_WARNING
ANGLE_DISABLE_SHADOWING_WARNING

// glslang's version of ShaderLang.h, not to be confused with ANGLE's.
#include <glslang/Public/ShaderLang.h>

// Other glslang includes.
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>

ANGLE_REENABLE_SHADOWING_WARNING
ANGLE_REENABLE_EXTRA_SEMI_WARNING

// SPIR-V headers include for AST transformation.
#include <spirv/unified1/spirv.hpp>

// SPIR-V tools include for AST validation.
#include <spirv-tools/libspirv.hpp>

#include <array>
#include <numeric>

#include "common/FixedVector.h"
#include "common/string_utils.h"
#include "common/utilities.h"
#include "libANGLE/Caps.h"
#include "libANGLE/ProgramLinkedResources.h"

#define ANGLE_GLSLANG_CHECK(CALLBACK, TEST, ERR) \
    do                                           \
    {                                            \
        if (ANGLE_UNLIKELY(!(TEST)))             \
        {                                        \
            return CALLBACK(ERR);                \
        }                                        \
                                                 \
    } while (0)

namespace rx
{
namespace
{
constexpr char kMarkerStart[]                      = "@@ ";
constexpr char kQualifierMarkerBegin[]             = "@@ QUALIFIER-";
constexpr char kLayoutMarkerBegin[]                = "@@ LAYOUT-";
constexpr char kXfbDeclMarkerBegin[]               = "@@ XFB-DECL";
constexpr char kXfbOutMarkerBegin[]                = "@@ XFB-OUT";
constexpr char kMarkerEnd[]                        = " @@";
constexpr char kParamsBegin                        = '(';
constexpr char kParamsEnd                          = ')';
constexpr uint32_t kANGLEPositionLocationOffset    = 1;
constexpr uint32_t kXfbANGLEPositionLocationOffset = 2;

constexpr gl::ShaderMap<const char *> kDefaultUniformNames = {
    {gl::ShaderType::Vertex, sh::vk::kDefaultUniformsNameVS},
    {gl::ShaderType::Geometry, sh::vk::kDefaultUniformsNameGS},
    {gl::ShaderType::Fragment, sh::vk::kDefaultUniformsNameFS},
    {gl::ShaderType::Compute, sh::vk::kDefaultUniformsNameCS},
};

template <size_t N>
constexpr size_t ConstStrLen(const char (&)[N])
{
    static_assert(N > 0, "C++ shouldn't allow N to be zero");

    // The length of a string defined as a char array is the size of the array minus 1 (the
    // terminating '\0').
    return N - 1;
}

void GetBuiltInResourcesFromCaps(const gl::Caps &caps, TBuiltInResource *outBuiltInResources)
{
    outBuiltInResources->maxDrawBuffers                   = caps.maxDrawBuffers;
    outBuiltInResources->maxAtomicCounterBindings         = caps.maxAtomicCounterBufferBindings;
    outBuiltInResources->maxAtomicCounterBufferSize       = caps.maxAtomicCounterBufferSize;
    outBuiltInResources->maxClipPlanes                    = caps.maxClipPlanes;
    outBuiltInResources->maxCombinedAtomicCounterBuffers  = caps.maxCombinedAtomicCounterBuffers;
    outBuiltInResources->maxCombinedAtomicCounters        = caps.maxCombinedAtomicCounters;
    outBuiltInResources->maxCombinedImageUniforms         = caps.maxCombinedImageUniforms;
    outBuiltInResources->maxCombinedTextureImageUnits     = caps.maxCombinedTextureImageUnits;
    outBuiltInResources->maxCombinedShaderOutputResources = caps.maxCombinedShaderOutputResources;
    outBuiltInResources->maxComputeWorkGroupCountX        = caps.maxComputeWorkGroupCount[0];
    outBuiltInResources->maxComputeWorkGroupCountY        = caps.maxComputeWorkGroupCount[1];
    outBuiltInResources->maxComputeWorkGroupCountZ        = caps.maxComputeWorkGroupCount[2];
    outBuiltInResources->maxComputeWorkGroupSizeX         = caps.maxComputeWorkGroupSize[0];
    outBuiltInResources->maxComputeWorkGroupSizeY         = caps.maxComputeWorkGroupSize[1];
    outBuiltInResources->maxComputeWorkGroupSizeZ         = caps.maxComputeWorkGroupSize[2];
    outBuiltInResources->minProgramTexelOffset            = caps.minProgramTexelOffset;
    outBuiltInResources->maxFragmentUniformVectors        = caps.maxFragmentUniformVectors;
    outBuiltInResources->maxFragmentInputComponents       = caps.maxFragmentInputComponents;
    outBuiltInResources->maxGeometryInputComponents       = caps.maxGeometryInputComponents;
    outBuiltInResources->maxGeometryOutputComponents      = caps.maxGeometryOutputComponents;
    outBuiltInResources->maxGeometryOutputVertices        = caps.maxGeometryOutputVertices;
    outBuiltInResources->maxGeometryTotalOutputComponents = caps.maxGeometryTotalOutputComponents;
    outBuiltInResources->maxLights                        = caps.maxLights;
    outBuiltInResources->maxProgramTexelOffset            = caps.maxProgramTexelOffset;
    outBuiltInResources->maxVaryingComponents             = caps.maxVaryingComponents;
    outBuiltInResources->maxVaryingVectors                = caps.maxVaryingVectors;
    outBuiltInResources->maxVertexAttribs                 = caps.maxVertexAttributes;
    outBuiltInResources->maxVertexOutputComponents        = caps.maxVertexOutputComponents;
    outBuiltInResources->maxVertexUniformVectors          = caps.maxVertexUniformVectors;
}

// Information used for Xfb layout qualifier
struct XFBBufferInfo
{
    GLuint index;
    GLuint offset;
    GLuint stride;
};

struct VaryingNameEquals
{
    VaryingNameEquals(const std::string &name_) : name(name_) {}
    bool operator()(const gl::TransformFeedbackVarying &var) const { return var.name == name; }

    std::string name;
};

using XfbBufferMap = std::map<std::string, XFBBufferInfo>;

class IntermediateShaderSource final : angle::NonCopyable
{
  public:
    void init(const std::string &source);
    bool empty() const { return mTokens.empty(); }

    // Find @@ LAYOUT-name(extra, args) @@ and replace it with:
    //
    //     layout(specifier, extra, args)
    //
    // or if |specifier| is empty:
    //
    //     layout(extra, args)
    //
    void insertLayoutSpecifier(const std::string &name, const std::string &specifier);

    // Find @@ QUALIFIER-name(other qualifiers) @@ and replace it with:
    //
    //      specifier other qualifiers
    //
    // or if |specifier| is empty, with nothing.
    //
    void insertQualifierSpecifier(const std::string &name, const std::string &specifier);

    // Replace @@ XFB-DECL @@ with |decl|.
    void insertTransformFeedbackDeclaration(const std::string &&decl);

    // Replace @@ XFB-OUT @@ with |output| code block.
    void insertTransformFeedbackOutput(const std::string &&output);

    // Remove @@ LAYOUT-name(*) @@ and @@ QUALIFIER-name(*) @@ altogether, optionally replacing them
    // with something to make sure the shader still compiles.
    void eraseLayoutAndQualifierSpecifiers(const std::string &name, const std::string &replacement);

    // Get the transformed shader source as one string.
    std::string getShaderSource();

  private:
    enum class TokenType
    {
        // A piece of shader source code.
        Text,
        // Block corresponding to @@ QUALIFIER-abc(other qualifiers) @@
        Qualifier,
        // Block corresponding to @@ LAYOUT-abc(extra, args) @@
        Layout,
        // Block corresponding to @@ XFB-DECL @@
        TransformFeedbackDeclaration,
        // Block corresponding to @@ XFB-OUT @@
        TransformFeedbackOutput,
    };

    struct Token
    {
        TokenType type;
        // |text| contains some shader code if Text, or the id of macro ("abc" in examples above)
        // being replaced if Qualifier or Layout.
        std::string text;
        // If Qualifier or Layout, this contains extra parameters passed in parentheses, if any.
        std::string args;
    };

    void addTextBlock(std::string &&text);
    void addLayoutBlock(std::string &&name, std::string &&args);
    void addQualifierBlock(std::string &&name, std::string &&args);
    void addTransformFeedbackDeclarationBlock();
    void addTransformFeedbackOutputBlock();

    void replaceSingleMacro(TokenType type, const std::string &&text);

    std::vector<Token> mTokens;
};

void IntermediateShaderSource::addTextBlock(std::string &&text)
{
    if (!text.empty())
    {
        Token token = {TokenType::Text, std::move(text), ""};
        mTokens.emplace_back(std::move(token));
    }
}

void IntermediateShaderSource::addLayoutBlock(std::string &&name, std::string &&args)
{
    ASSERT(!name.empty());
    Token token = {TokenType::Layout, std::move(name), std::move(args)};
    mTokens.emplace_back(std::move(token));
}

void IntermediateShaderSource::addQualifierBlock(std::string &&name, std::string &&args)
{
    ASSERT(!name.empty());
    Token token = {TokenType::Qualifier, std::move(name), std::move(args)};
    mTokens.emplace_back(std::move(token));
}

void IntermediateShaderSource::addTransformFeedbackDeclarationBlock()
{
    Token token = {TokenType::TransformFeedbackDeclaration, "", ""};
    mTokens.emplace_back(std::move(token));
}

void IntermediateShaderSource::addTransformFeedbackOutputBlock()
{
    Token token = {TokenType::TransformFeedbackOutput, "", ""};
    mTokens.emplace_back(std::move(token));
}

size_t ExtractNameAndArgs(const std::string &source,
                          size_t cur,
                          std::string *nameOut,
                          std::string *argsOut)
{
    *nameOut = angle::GetPrefix(source, cur, kParamsBegin);

    // There should always be an extra args list (even if empty, for simplicity).
    size_t readCount = nameOut->length() + 1;
    *argsOut         = angle::GetPrefix(source, cur + readCount, kParamsEnd);
    readCount += argsOut->length() + 1;

    return readCount;
}

void IntermediateShaderSource::init(const std::string &source)
{
    size_t cur = 0;

    // Split the source into Text, Layout and Qualifier blocks for efficient macro expansion.
    while (cur < source.length())
    {
        // Create a Text block for the code up to the first marker.
        std::string text = angle::GetPrefix(source, cur, kMarkerStart);
        cur += text.length();

        addTextBlock(std::move(text));

        if (cur >= source.length())
        {
            break;
        }

        if (source.compare(cur, ConstStrLen(kQualifierMarkerBegin), kQualifierMarkerBegin) == 0)
        {
            cur += ConstStrLen(kQualifierMarkerBegin);

            // Get the id and arguments of the macro and add a qualifier block.
            std::string name, args;
            cur += ExtractNameAndArgs(source, cur, &name, &args);
            addQualifierBlock(std::move(name), std::move(args));
        }
        else if (source.compare(cur, ConstStrLen(kLayoutMarkerBegin), kLayoutMarkerBegin) == 0)
        {
            cur += ConstStrLen(kLayoutMarkerBegin);

            // Get the id and arguments of the macro and add a layout block.
            std::string name, args;
            cur += ExtractNameAndArgs(source, cur, &name, &args);
            addLayoutBlock(std::move(name), std::move(args));
        }
        else if (source.compare(cur, ConstStrLen(kXfbDeclMarkerBegin), kXfbDeclMarkerBegin) == 0)
        {
            cur += ConstStrLen(kXfbDeclMarkerBegin);
            addTransformFeedbackDeclarationBlock();
        }
        else if (source.compare(cur, ConstStrLen(kXfbOutMarkerBegin), kXfbOutMarkerBegin) == 0)
        {
            cur += ConstStrLen(kXfbOutMarkerBegin);
            addTransformFeedbackOutputBlock();
        }
        else
        {
            // If reached here, @@ was met in the shader source itself which would have been a
            // compile error.
            UNREACHABLE();
        }

        // There should always be a closing marker at this point.
        ASSERT(source.compare(cur, ConstStrLen(kMarkerEnd), kMarkerEnd) == 0);

        // Continue from after the closing of this macro.
        cur += ConstStrLen(kMarkerEnd);
    }
}

void IntermediateShaderSource::insertLayoutSpecifier(const std::string &name,
                                                     const std::string &specifier)
{
    for (Token &block : mTokens)
    {
        if (block.type == TokenType::Layout && block.text == name)
        {
            ASSERT(!specifier.empty());
            const char *separator = block.args.empty() ? "" : ", ";

            block.type = TokenType::Text;
            block.text = "layout(" + block.args + separator + specifier + ")";
            break;
        }
    }
}

void IntermediateShaderSource::insertQualifierSpecifier(const std::string &name,
                                                        const std::string &specifier)
{
    for (Token &block : mTokens)
    {
        if (block.type == TokenType::Qualifier && block.text == name)
        {
            block.type = TokenType::Text;
            block.text = specifier;
            if (!block.args.empty())
            {
                block.text += " " + block.args;
            }
            break;
        }
    }
}

void IntermediateShaderSource::replaceSingleMacro(TokenType type, const std::string &&text)
{
    for (Token &block : mTokens)
    {
        if (block.type == type)
        {
            block.type = TokenType::Text;
            block.text = std::move(text);
            break;
        }
    }
}

void IntermediateShaderSource::insertTransformFeedbackDeclaration(const std::string &&decl)
{
    replaceSingleMacro(TokenType::TransformFeedbackDeclaration, std::move(decl));
}

void IntermediateShaderSource::insertTransformFeedbackOutput(const std::string &&output)
{
    replaceSingleMacro(TokenType::TransformFeedbackOutput, std::move(output));
}

void IntermediateShaderSource::eraseLayoutAndQualifierSpecifiers(const std::string &name,
                                                                 const std::string &replacement)
{
    for (Token &block : mTokens)
    {
        if (block.type == TokenType::Text || block.text != name)
        {
            continue;
        }

        block.text = block.type == TokenType::Layout ? replacement : "";
        block.type = TokenType::Text;
    }
}

std::string IntermediateShaderSource::getShaderSource()
{
    std::string shaderSource;

    for (Token &block : mTokens)
    {
        // All blocks should have been replaced.
        ASSERT(block.type == TokenType::Text);
        shaderSource += block.text;
    }

    return shaderSource;
}

// Test if there are non-zero indices in the uniform name, returning false in that case.  This
// happens for multi-dimensional arrays, where a uniform is created for every possible index of the
// array (except for the innermost dimension).  When assigning decorations (set/binding/etc), only
// the indices corresponding to the first element of the array should be specified.  This function
// is used to skip the other indices.
//
// If useOldRewriteStructSamplers, there are multiple samplers extracted out of struct arrays
// though, so the above only applies to the sampler array defined in the struct.
bool UniformNameIsIndexZero(const std::string &name, bool excludeCheckForOwningStructArrays)
{
    size_t lastBracketClose = 0;

    if (excludeCheckForOwningStructArrays)
    {
        size_t lastDot = name.find_last_of('.');
        if (lastDot != std::string::npos)
        {
            lastBracketClose = lastDot;
        }
    }

    while (true)
    {
        size_t openBracket = name.find('[', lastBracketClose);
        if (openBracket == std::string::npos)
        {
            break;
        }
        size_t closeBracket = name.find(']', openBracket);

        // If the index between the brackets is not zero, ignore this uniform.
        if (name.substr(openBracket + 1, closeBracket - openBracket - 1) != "0")
        {
            return false;
        }
        lastBracketClose = closeBracket;
    }

    return true;
}

// Strip indices from the name.  If there are non-zero indices, return false to indicate that this
// image uniform doesn't require set/binding.  That is done on index 0.
bool GetImageNameWithoutIndices(std::string *name)
{
    if (name->back() != ']')
    {
        return true;
    }

    if (!UniformNameIsIndexZero(*name, false))
    {
        return false;
    }

    // Strip all indices
    *name = name->substr(0, name->find('['));
    return true;
}

bool MappedSamplerNameNeedsUserDefinedPrefix(const std::string &originalName)
{
    return originalName.find('.') == std::string::npos;
}

std::string GetMappedSamplerNameOld(const std::string &originalName)
{
    std::string samplerName = gl::ParseResourceName(originalName, nullptr);

    // Samplers in structs are extracted.
    std::replace(samplerName.begin(), samplerName.end(), '.', '_');

    // Samplers in arrays of structs are also extracted.
    std::replace(samplerName.begin(), samplerName.end(), '[', '_');
    samplerName.erase(std::remove(samplerName.begin(), samplerName.end(), ']'), samplerName.end());

    if (MappedSamplerNameNeedsUserDefinedPrefix(originalName))
    {
        samplerName = sh::kUserDefinedNamePrefix + samplerName;
    }

    return samplerName;
}

template <typename OutputIter, typename ImplicitIter>
uint32_t CountExplicitOutputs(OutputIter outputsBegin,
                              OutputIter outputsEnd,
                              ImplicitIter implicitsBegin,
                              ImplicitIter implicitsEnd)
{
    auto reduce = [implicitsBegin, implicitsEnd](uint32_t count, const sh::ShaderVariable &var) {
        bool isExplicit = std::find(implicitsBegin, implicitsEnd, var.name) == implicitsEnd;
        return count + isExplicit;
    };

    return std::accumulate(outputsBegin, outputsEnd, 0, reduce);
}

ShaderInterfaceVariableInfo *AddShaderInterfaceVariable(ShaderInterfaceVariableInfoMap *infoMap,
                                                        const std::string &varName)
{
    ASSERT(infoMap->find(varName) == infoMap->end());
    return &(*infoMap)[varName];
}

ShaderInterfaceVariableInfo *AddResourceInfo(ShaderInterfaceVariableInfoMap *infoMap,
                                             const std::string &varName,
                                             uint32_t descriptorSet,
                                             uint32_t binding)
{
    ShaderInterfaceVariableInfo *info = AddShaderInterfaceVariable(infoMap, varName);
    info->descriptorSet               = descriptorSet;
    info->binding                     = binding;
    return info;
}

// Add location information for an in/out variable.
ShaderInterfaceVariableInfo *AddLocationInfo(ShaderInterfaceVariableInfoMap *infoMap,
                                             const std::string &varName,
                                             gl::ShaderType shaderType,
                                             uint32_t location,
                                             uint32_t component)
{
    // The info map for this name may or may not exist already.  This function merges the
    // location/component information.
    ShaderInterfaceVariableInfo *info = &(*infoMap)[varName];

    ASSERT(info->descriptorSet == ShaderInterfaceVariableInfo::kInvalid);
    ASSERT(info->binding == ShaderInterfaceVariableInfo::kInvalid);
    ASSERT(info->location[shaderType] == ShaderInterfaceVariableInfo::kInvalid);
    ASSERT(info->component[shaderType] == ShaderInterfaceVariableInfo::kInvalid);

    info->location[shaderType]  = location;
    info->component[shaderType] = component;
    return info;
}

std::string GenerateTransformFeedbackVaryingOutput(const gl::TransformFeedbackVarying &varying,
                                                   const gl::UniformTypeInfo &info,
                                                   size_t strideBytes,
                                                   size_t offset,
                                                   const std::string &bufferIndex)
{
    std::ostringstream result;

    ASSERT(strideBytes % 4 == 0);
    size_t stride = strideBytes / 4;

    const size_t arrayIndexStart = varying.arrayIndex == GL_INVALID_INDEX ? 0 : varying.arrayIndex;
    const size_t arrayIndexEnd   = arrayIndexStart + varying.size();

    for (size_t arrayIndex = arrayIndexStart; arrayIndex < arrayIndexEnd; ++arrayIndex)
    {
        for (int col = 0; col < info.columnCount; ++col)
        {
            for (int row = 0; row < info.rowCount; ++row)
            {
                result << "xfbOut" << bufferIndex << "[" << sh::vk::kDriverUniformsVarName
                       << ".xfbBufferOffsets[" << bufferIndex
                       << "] + (gl_VertexIndex + gl_InstanceIndex * "
                       << sh::vk::kDriverUniformsVarName << ".xfbVerticesPerDraw) * " << stride
                       << " + " << offset << "] = " << info.glslAsFloat << "("
                       << varying.mappedName;

                if (varying.isArray())
                {
                    result << "[" << arrayIndex << "]";
                }

                if (info.columnCount > 1)
                {
                    result << "[" << col << "]";
                }

                if (info.rowCount > 1)
                {
                    result << "[" << row << "]";
                }

                result << ");\n";
                ++offset;
            }
        }
    }

    return result.str();
}

void GenerateTransformFeedbackEmulationOutputs(const GlslangSourceOptions &options,
                                               const gl::ProgramState &programState,
                                               IntermediateShaderSource *vertexShader,
                                               ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    const std::vector<gl::TransformFeedbackVarying> &varyings =
        programState.getLinkedTransformFeedbackVaryings();
    const std::vector<GLsizei> &bufferStrides = programState.getTransformFeedbackStrides();
    const bool isInterleaved =
        programState.getTransformFeedbackBufferMode() == GL_INTERLEAVED_ATTRIBS;
    const size_t bufferCount = isInterleaved ? 1 : varyings.size();

    const std::string xfbSet = Str(options.uniformsAndXfbDescriptorSetIndex);
    std::vector<std::string> xfbIndices(bufferCount);

    std::string xfbDecl;

    for (uint32_t bufferIndex = 0; bufferIndex < bufferCount; ++bufferIndex)
    {
        const std::string xfbBinding = Str(options.xfbBindingIndexStart + bufferIndex);
        xfbIndices[bufferIndex]      = Str(bufferIndex);

        std::string bufferName = "xfbBuffer" + xfbIndices[bufferIndex];

        xfbDecl += "layout(set = " + xfbSet + ", binding = " + xfbBinding + ") buffer " +
                   bufferName + " { float xfbOut" + xfbIndices[bufferIndex] + "[]; };\n";

        // Add this entry to the info map, so we can easily assert that every resource has an entry
        // in this map.
        AddResourceInfo(variableInfoMapOut, bufferName, options.uniformsAndXfbDescriptorSetIndex,
                        options.xfbBindingIndexStart + bufferIndex);
    }

    std::string xfbOut =
        "if (" + std::string(sh::vk::kDriverUniformsVarName) + ".xfbActiveUnpaused != 0)\n{\n";
    size_t outputOffset = 0;
    for (size_t varyingIndex = 0; varyingIndex < varyings.size(); ++varyingIndex)
    {
        const size_t bufferIndex                    = isInterleaved ? 0 : varyingIndex;
        const gl::TransformFeedbackVarying &varying = varyings[varyingIndex];

        // For every varying, output to the respective buffer packed.  If interleaved, the output is
        // always to the same buffer, but at different offsets.
        const gl::UniformTypeInfo &info = gl::GetUniformTypeInfo(varying.type);
        xfbOut += GenerateTransformFeedbackVaryingOutput(varying, info, bufferStrides[bufferIndex],
                                                         outputOffset, xfbIndices[bufferIndex]);

        if (isInterleaved)
        {
            outputOffset += info.columnCount * info.rowCount * varying.size();
        }
    }
    xfbOut += "}\n";

    vertexShader->insertTransformFeedbackDeclaration(std::move(xfbDecl));
    vertexShader->insertTransformFeedbackOutput(std::move(xfbOut));
}

// Calculates XFB layout qualifier arguments for each tranform feedback varying, inserts
// layout quailifier for built-in varyings here and gathers calculated arguments for non built-in
// varyings for later use.
void GenerateTransformFeedbackExtensionOutputs(const gl::ProgramState &programState,
                                               IntermediateShaderSource *vertexShader,
                                               XfbBufferMap *xfbBufferMap,
                                               const gl::ProgramLinkedResources &resources)
{
    const std::vector<gl::TransformFeedbackVarying> &tfVaryings =
        programState.getLinkedTransformFeedbackVaryings();
    const std::vector<GLsizei> &varyingStrides = programState.getTransformFeedbackStrides();
    const bool isInterleaved =
        programState.getTransformFeedbackBufferMode() == GL_INTERLEAVED_ATTRIBS;

    std::string xfbDecl;
    bool hasBuiltInVaryings     = false;
    bool replacePositionVarying = false;
    uint32_t currentOffset      = 0;
    uint32_t currentStride      = 0;
    uint32_t bufferIndex        = 0;
    std::string varyingType;
    std::string xfbIndices;
    std::string xfbOffsets;
    std::string xfbStrides;
    std::string replacedPositionLayout;

    for (uint32_t varyingIndex = 0; varyingIndex < tfVaryings.size(); ++varyingIndex)
    {
        if (isInterleaved)
        {
            bufferIndex = 0;
            if (varyingIndex > 0)
            {
                const gl::TransformFeedbackVarying &prev = tfVaryings[varyingIndex - 1];
                currentOffset += prev.size() * gl::VariableExternalSize(prev.type);
            }
            currentStride = varyingStrides[0];
        }
        else
        {
            bufferIndex   = varyingIndex;
            currentOffset = 0;
            currentStride = varyingStrides[varyingIndex];
        }

        if (tfVaryings[varyingIndex].isBuiltIn())
        {
            xfbIndices  = Str(bufferIndex);
            xfbOffsets  = Str(currentOffset);
            xfbStrides  = Str(currentStride);
            varyingType = gl::GetGLSLTypeString(tfVaryings[varyingIndex].type);

            if (tfVaryings[varyingIndex].name.compare("gl_Position") == 0)
            {
                replacePositionVarying = true;

                std::string xfbReplacedPositionLocation =
                    Str(resources.varyingPacking.getMaxSemanticIndex() +
                        kXfbANGLEPositionLocationOffset);

                replacedPositionLayout = "layout(location = " + xfbReplacedPositionLocation +
                                         ", xfb_buffer = " + xfbIndices +
                                         ", xfb_offset = " + xfbOffsets +
                                         ", xfb_stride = " + xfbStrides + ") out " + varyingType +
                                         " xfbANGLEPosition;\n";
            }
            else
            {
                // Since built-in varyings are not in RegisterList, we can add layout qualifier
                // here.
                if (!hasBuiltInVaryings)
                {
                    hasBuiltInVaryings = true;

                    xfbDecl += "out gl_PerVertex\n{\n";
                    for (uint32_t index = 0; index < tfVaryings.size(); ++index)
                    {
                        // need to add gl_Position to gl_Pervertex because we declared layout for
                        // replaced xfbANGLEPosition instead
                        if (tfVaryings[index].name.compare("gl_Position") == 0)
                        {
                            xfbDecl += "vec4 gl_Position;\n";
                            break;
                        }
                    }
                }
                xfbDecl += "layout(xfb_buffer = " + xfbIndices + ", xfb_offset = " + xfbOffsets +
                           ", xfb_stride = " + xfbStrides + ") " + varyingType + " " +
                           tfVaryings[varyingIndex].name + ";\n";
            }
        }
        else
        {
            // Layout qualifier for non built-in varying will be written later, so we just save
            // Xfb layout qualifier information into the xfbBufferMap.
            XFBBufferInfo bufferInfo;
            bufferInfo.index  = bufferIndex;
            bufferInfo.offset = currentOffset;
            bufferInfo.stride = currentStride;
            xfbBufferMap->insert(make_pair(tfVaryings[varyingIndex].name, bufferInfo));
        }
    }

    if (hasBuiltInVaryings)
    {
        // We should add non transform feedback built-in varyings to gl_PerVertex because once
        // we declare gl_PerVertex, all built-in varyings used in shaders should be included
        // in gl_PerVertex struct.
        for (const sh::ShaderVariable &varying : resources.varyingPacking.getInputVaryings())
        {
            if (varying.isBuiltIn())
            {
                auto iter = std::find_if(tfVaryings.begin(), tfVaryings.end(),
                                         VaryingNameEquals(varying.name));
                if (iter == tfVaryings.end())
                {
                    xfbDecl += gl::GetGLSLTypeString(varying.type) + " " + varying.name + ";\n";
                }
            }
        }
        xfbDecl += "\n};\n";
    }

    xfbDecl += replacedPositionLayout;

    vertexShader->insertTransformFeedbackDeclaration(std::move(xfbDecl));

    std::string xfbOut;
    if (replacePositionVarying)
    {
        xfbOut += "xfbANGLEPosition = gl_Position;\n";
    }

    vertexShader->insertTransformFeedbackOutput(std::move(xfbOut));
}

void AssignAttributeLocations(const gl::ProgramState &programState,
                              ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    // Assign attribute locations for the vertex shader.
    for (const sh::ShaderVariable &attribute : programState.getProgramInputs())
    {
        ASSERT(attribute.active);

        AddLocationInfo(variableInfoMapOut, attribute.mappedName, gl::ShaderType::Vertex,
                        attribute.location, ShaderInterfaceVariableInfo::kInvalid);
    }
}

void AssignOutputLocations(const gl::ProgramState &programState,
                           ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    // Assign output locations for the fragment shader.
    // TODO(syoussefi): Add support for EXT_blend_func_extended.  http://anglebug.com/3385
    const auto &outputLocations                      = programState.getOutputLocations();
    const auto &outputVariables                      = programState.getOutputVariables();
    const std::array<std::string, 3> implicitOutputs = {"gl_FragDepth", "gl_SampleMask",
                                                        "gl_FragStencilRefARB"};
    for (const gl::VariableLocation &outputLocation : outputLocations)
    {
        if (outputLocation.arrayIndex == 0 && outputLocation.used() && !outputLocation.ignored)
        {
            const sh::ShaderVariable &outputVar = outputVariables[outputLocation.index];

            uint32_t location = 0;
            if (outputVar.location != -1)
            {
                location = outputVar.location;
            }
            else if (std::find(implicitOutputs.begin(), implicitOutputs.end(), outputVar.name) ==
                     implicitOutputs.end())
            {
                // If there is only one output, it is allowed not to have a location qualifier, in
                // which case it defaults to 0.  GLSL ES 3.00 spec, section 4.3.8.2.
                ASSERT(CountExplicitOutputs(outputVariables.begin(), outputVariables.end(),
                                            implicitOutputs.begin(), implicitOutputs.end()) == 1);
            }

            AddLocationInfo(variableInfoMapOut, outputVar.mappedName, gl::ShaderType::Fragment,
                            location, ShaderInterfaceVariableInfo::kInvalid);
        }
    }
}

void AssignVaryingLocations(const gl::ProgramState &programState,
                            const gl::ProgramLinkedResources &resources,
                            gl::ShaderMap<IntermediateShaderSource> *shaderSources,
                            XfbBufferMap *xfbBufferMap)
{

    // Assign varying locations.
    for (const gl::PackedVaryingRegister &varyingReg : resources.varyingPacking.getRegisterList())
    {
        const auto &varying = *varyingReg.packedVarying;

        // In Vulkan GLSL, struct fields are not allowed to have location assignments.  The varying
        // of a struct type is thus given a location equal to the one assigned to its first field.
        if (varying.isStructField() && varying.fieldIndex > 0)
        {
            continue;
        }

        // Similarly, assign array varying locations to the assigned location of the first element.
        if (varying.isArrayElement() && varying.arrayIndex != 0)
        {
            continue;
        }

        // In the following:
        //
        //     struct S { vec4 field; };
        //     out S varStruct;
        //
        // "varStruct" is found through |parentStructName|, with |varying->name| being "field".  In
        // such a case, use |parentStructName|.
        const std::string &name =
            varying.isStructField() ? varying.parentStructName : varying.varying->name;

        std::string locationString = "location = " + Str(varyingReg.registerRow);
        if (varyingReg.registerColumn > 0)
        {
            ASSERT(!varying.varying->isStruct());
            ASSERT(!gl::IsMatrixType(varying.varying->type));
            locationString += ", component = " + Str(varyingReg.registerColumn);
        }

        std::string *layoutSpecifier = &locationString;

        std::string xfbSpecifier;
        XfbBufferMap::iterator iter;
        iter = xfbBufferMap->find(name);
        if (iter != xfbBufferMap->end())
        {
            XFBBufferInfo item = iter->second;
            xfbSpecifier       = "xfb_buffer = " + Str(item.index) +
                           ", xfb_offset = " + Str(item.offset) +
                           ", xfb_stride = " + Str(item.stride) + ", " + locationString;
            layoutSpecifier = &xfbSpecifier;
        }

        for (const gl::ShaderType stage : gl::kAllGraphicsShaderTypes)
        {
            IntermediateShaderSource *shaderSource = &(*shaderSources)[stage];
            if (shaderSource->empty())
            {
                ASSERT(!varying.shaderStages[stage]);
                continue;
            }

            if (!varying.shaderStages[stage])
            {
                // If not active in this stage, remove the varying declaration.  Imagine the
                // following scenario:
                //
                //  - VS: declare out varying used for transform feedback
                //  - FS: declare corresponding in varying which is not active
                //
                // Then varying.shaderStages would only contain Vertex, but the varying is not
                // present in the list of inactive varyings since it _is_ active in some stages.
                // As a result, we remove the varying from any stage that's not active.
                // CleanupUnusedEntities will remove the varyings that are inactive in all stages.
                shaderSource->eraseLayoutAndQualifierSpecifiers(name, "");
                continue;
            }

            shaderSource->insertLayoutSpecifier(
                name, stage == gl::ShaderType::Fragment ? locationString : *layoutSpecifier);
            shaderSource->insertQualifierSpecifier(name, "");
        }
    }

    // Substitute layout and qualifier strings for the position varying. Use the first free
    // varying register after the packed varyings.
    constexpr char kVaryingName[] = "ANGLEPosition";
    std::stringstream layoutStream;
    layoutStream << "location = "
                 << (resources.varyingPacking.getMaxSemanticIndex() + kANGLEPositionLocationOffset);
    const std::string layout = layoutStream.str();

    for (IntermediateShaderSource &shaderSource : *shaderSources)
    {
        shaderSource.insertLayoutSpecifier(kVaryingName, layout);
        shaderSource.insertQualifierSpecifier(kVaryingName, "");
    }
}

void AssignUniformBindings(const GlslangSourceOptions &options,
                           gl::ShaderMap<IntermediateShaderSource> *shaderSources,
                           ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    // Assign binding to the default uniforms block of each shader stage.
    uint32_t bindingIndex = 0;
    for (const gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        IntermediateShaderSource &shaderSource = (*shaderSources)[shaderType];
        if (!shaderSource.empty())
        {
            AddResourceInfo(variableInfoMapOut, kDefaultUniformNames[shaderType],
                            options.uniformsAndXfbDescriptorSetIndex, bindingIndex);
            ++bindingIndex;
        }
    }

    // Assign binding to the driver uniforms block
    AddResourceInfo(variableInfoMapOut, sh::vk::kDriverUniformsVarName,
                    options.driverUniformsDescriptorSetIndex, 0);
}

uint32_t AssignInterfaceBlockBindings(const GlslangSourceOptions &options,
                                      const std::vector<gl::InterfaceBlock> &blocks,
                                      uint32_t bindingStart,
                                      ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    uint32_t bindingIndex = bindingStart;
    for (const gl::InterfaceBlock &block : blocks)
    {
        if (!block.isArray || block.arrayElement == 0)
        {
            AddResourceInfo(variableInfoMapOut, block.mappedName,
                            options.shaderResourceDescriptorSetIndex, bindingIndex);
            ++bindingIndex;
        }
    }

    return bindingIndex;
}

uint32_t AssignAtomicCounterBufferBindings(const GlslangSourceOptions &options,
                                           const std::vector<gl::AtomicCounterBuffer> &buffers,
                                           uint32_t bindingStart,
                                           ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    if (buffers.size() == 0)
    {
        return bindingStart;
    }

    AddResourceInfo(variableInfoMapOut, sh::vk::kAtomicCountersVarName,
                    options.shaderResourceDescriptorSetIndex, bindingStart);

    return bindingStart + 1;
}

uint32_t AssignImageBindings(const GlslangSourceOptions &options,
                             const std::vector<gl::LinkedUniform> &uniforms,
                             const gl::RangeUI &imageUniformRange,
                             uint32_t bindingStart,
                             ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    uint32_t bindingIndex = bindingStart;
    for (unsigned int uniformIndex : imageUniformRange)
    {
        const gl::LinkedUniform &imageUniform = uniforms[uniformIndex];

        std::string name = imageUniform.mappedName;
        if (GetImageNameWithoutIndices(&name))
        {
            AddResourceInfo(variableInfoMapOut, name, options.shaderResourceDescriptorSetIndex,
                            bindingIndex);
        }
        ++bindingIndex;
    }

    return bindingIndex;
}

void AssignNonTextureBindings(const GlslangSourceOptions &options,
                              const gl::ProgramState &programState,
                              ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    uint32_t bindingStart = 0;

    const std::vector<gl::InterfaceBlock> &uniformBlocks = programState.getUniformBlocks();
    bindingStart =
        AssignInterfaceBlockBindings(options, uniformBlocks, bindingStart, variableInfoMapOut);

    const std::vector<gl::InterfaceBlock> &storageBlocks = programState.getShaderStorageBlocks();
    bindingStart =
        AssignInterfaceBlockBindings(options, storageBlocks, bindingStart, variableInfoMapOut);

    const std::vector<gl::AtomicCounterBuffer> &atomicCounterBuffers =
        programState.getAtomicCounterBuffers();
    bindingStart = AssignAtomicCounterBufferBindings(options, atomicCounterBuffers, bindingStart,
                                                     variableInfoMapOut);

    const std::vector<gl::LinkedUniform> &uniforms = programState.getUniforms();
    const gl::RangeUI &imageUniformRange           = programState.getImageUniformRange();
    bindingStart =
        AssignImageBindings(options, uniforms, imageUniformRange, bindingStart, variableInfoMapOut);
}

void AssignTextureBindings(const GlslangSourceOptions &options,
                           const gl::ProgramState &programState,
                           ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    // Assign textures to a descriptor set and binding.
    uint32_t bindingIndex                          = 0;
    const std::vector<gl::LinkedUniform> &uniforms = programState.getUniforms();

    for (unsigned int uniformIndex : programState.getSamplerUniformRange())
    {
        const gl::LinkedUniform &samplerUniform = uniforms[uniformIndex];

        if (!options.useOldRewriteStructSamplers &&
            gl::SamplerNameContainsNonZeroArrayElement(samplerUniform.name))
        {
            continue;
        }

        if (UniformNameIsIndexZero(samplerUniform.name, options.useOldRewriteStructSamplers))
        {
            // Samplers in structs are extracted and renamed.
            const std::string samplerName = options.useOldRewriteStructSamplers
                                                ? GetMappedSamplerNameOld(samplerUniform.name)
                                                : GlslangGetMappedSamplerName(samplerUniform.name);

            AddResourceInfo(variableInfoMapOut, samplerName, options.textureDescriptorSetIndex,
                            bindingIndex);
        }

        ++bindingIndex;
    }
}

void CleanupUnusedEntities(bool useOldRewriteStructSamplers,
                           const gl::ProgramState &programState,
                           const gl::ProgramLinkedResources &resources,
                           gl::ShaderMap<IntermediateShaderSource> *shaderSources)
{
    // Remove all the markers for unused varyings.
    for (const std::string &varyingName : resources.varyingPacking.getInactiveVaryingNames())
    {
        for (IntermediateShaderSource &shaderSource : *shaderSources)
        {
            shaderSource.eraseLayoutAndQualifierSpecifiers(varyingName, "");
        }
    }
}

constexpr gl::ShaderMap<EShLanguage> kShLanguageMap = {
    {gl::ShaderType::Vertex, EShLangVertex},
    {gl::ShaderType::Geometry, EShLangGeometry},
    {gl::ShaderType::Fragment, EShLangFragment},
    {gl::ShaderType::Compute, EShLangCompute},
};

angle::Result GetShaderSpirvCode(GlslangErrorCallback callback,
                                 const gl::Caps &glCaps,
                                 const gl::ShaderMap<std::string> &shaderSources,
                                 gl::ShaderMap<std::vector<uint32_t>> *spirvBlobsOut)
{
    // Enable SPIR-V and Vulkan rules when parsing GLSL
    EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    TBuiltInResource builtInResources(glslang::DefaultTBuiltInResource);
    GetBuiltInResourcesFromCaps(glCaps, &builtInResources);

    glslang::TShader vertexShader(EShLangVertex);
    glslang::TShader fragmentShader(EShLangFragment);
    glslang::TShader geometryShader(EShLangGeometry);
    glslang::TShader computeShader(EShLangCompute);

    gl::ShaderMap<glslang::TShader *> shaders = {
        {gl::ShaderType::Vertex, &vertexShader},
        {gl::ShaderType::Fragment, &fragmentShader},
        {gl::ShaderType::Geometry, &geometryShader},
        {gl::ShaderType::Compute, &computeShader},
    };
    glslang::TProgram program;

    for (const gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        if (shaderSources[shaderType].empty())
        {
            continue;
        }

        const char *shaderString = shaderSources[shaderType].c_str();
        int shaderLength         = static_cast<int>(shaderSources[shaderType].size());

        glslang::TShader *shader = shaders[shaderType];
        shader->setStringsWithLengths(&shaderString, &shaderLength, 1);
        shader->setEntryPoint("main");

        bool result = shader->parse(&builtInResources, 450, ECoreProfile, false, false, messages);
        if (!result)
        {
            ERR() << "Internal error parsing Vulkan shader corresponding to " << shaderType << ":\n"
                  << shader->getInfoLog() << "\n"
                  << shader->getInfoDebugLog() << "\n";
            ANGLE_GLSLANG_CHECK(callback, false, GlslangError::InvalidShader);
        }

        program.addShader(shader);
    }

    bool linkResult = program.link(messages);
    if (!linkResult)
    {
        ERR() << "Internal error linking Vulkan shaders:\n" << program.getInfoLog() << "\n";
        ANGLE_GLSLANG_CHECK(callback, false, GlslangError::InvalidShader);
    }

    for (const gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        if (shaderSources[shaderType].empty())
        {
            continue;
        }

        glslang::TIntermediate *intermediate = program.getIntermediate(kShLanguageMap[shaderType]);
        glslang::GlslangToSpv(*intermediate, (*spirvBlobsOut)[shaderType]);
    }

    return angle::Result::Continue;
}

void ValidateSpirvMessage(spv_message_level_t level,
                          const char *source,
                          const spv_position_t &position,
                          const char *message)
{
    WARN() << "Level" << level << ": " << message;
}

bool ValidateSpirv(const std::vector<uint32_t> &spirvBlob)
{
    spvtools::SpirvTools spirvTools(SPV_ENV_VULKAN_1_1);

    spirvTools.SetMessageConsumer(ValidateSpirvMessage);
    bool result = spirvTools.Validate(spirvBlob);

    if (!result)
    {
        std::string readableSpirv;
        result = spirvTools.Disassemble(spirvBlob, &readableSpirv,
                                        SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
        WARN() << "Invalid SPIR-V:\n" << readableSpirv;
    }

    return result;
}

// A SPIR-V transformer.  It walks the instructions and modifies them as necessary, for example to
// assign bindings or locations.
class SpirvTransformer final : angle::NonCopyable
{
  public:
    SpirvTransformer(const std::vector<uint32_t> &spirvBlobIn,
                     const ShaderInterfaceVariableInfoMap &variableInfoMap,
                     gl::ShaderType shaderType,
                     SpirvBlob *spirvBlobOut)
        : mSpirvBlobIn(spirvBlobIn),
          mShaderType(shaderType),
          mVariableInfoMap(variableInfoMap),
          mSpirvBlobOut(spirvBlobOut)
    {}

    bool transform();

  private:
    // SPIR-V 1.0 Table 1: First Words of Physical Layout
    enum HeaderIndex
    {
        kHeaderIndexMagic        = 0,
        kHeaderIndexVersion      = 1,
        kHeaderIndexGenerator    = 2,
        kHeaderIndexIndexBound   = 3,
        kHeaderIndexSchema       = 4,
        kHeaderIndexInstructions = 5,
    };

    // A prepass to resolve interesting ids:
    void resolveVariableIds();

    // Transform instructions:
    void transformInstruction();

    // Instructions that are purely informational:
    void visitName(const uint32_t *instruction);
    void visitTypeHelper(const uint32_t *instruction, size_t idIndex, size_t typeIdIndex);
    void visitTypeArray(const uint32_t *instruction);
    void visitTypePointer(const uint32_t *instruction);
    void visitVariable(const uint32_t *instruction);

    // Instructions that potentially need transformation.  They return true if the instruction is
    // transformed.  If false is returned, the instruction should be copied as-is.
    bool transformDecorate(const uint32_t *instruction, size_t wordCount);

    // Any other instructions:
    void copyInstruction(const uint32_t *instruction, size_t wordCount);
    size_t getCurrentOutputOffset() const;

    // SPIR-V to transform:
    const std::vector<uint32_t> &mSpirvBlobIn;
    const gl::ShaderType mShaderType;

    // Input shader variable info map:
    const ShaderInterfaceVariableInfoMap &mVariableInfoMap;

    // Transformed SPIR-V:
    SpirvBlob *mSpirvBlobOut;

    // Traversal state:
    size_t mCurrentWord = 0;

    // Transformation state:

    // Shader variable info per id, if id is a shader variable.
    std::vector<const ShaderInterfaceVariableInfo *> mVariableInfoById;
};

bool SpirvTransformer::transform()
{
    // Glslang succeeded in outputting SPIR-V, so we assume it's valid.
    ASSERT(mSpirvBlobIn.size() >= kHeaderIndexInstructions);
    // Since SPIR-V comes from a local call to glslang, it necessarily has the same endianness as
    // the running architecture, so no byte-swapping is necessary.
    ASSERT(mSpirvBlobIn[kHeaderIndexMagic] == spv::MagicNumber);

    // Make sure the transformer is not reused to avoid having to reinitialize it here.
    ASSERT(mCurrentWord == 0);

    // Make sure the SpirvBlob is not reused.
    ASSERT(mSpirvBlobOut->empty());

    // First, find all necessary ids and associate them with the information required to transform
    // their decorations.
    resolveVariableIds();

    // Copy the header to SpirvBlob
    mSpirvBlobOut->assign(mSpirvBlobIn.begin(), mSpirvBlobIn.begin() + kHeaderIndexInstructions);

    mCurrentWord = kHeaderIndexInstructions;
    while (mCurrentWord < mSpirvBlobIn.size())
    {
        transformInstruction();
    }

    return true;
}

// SPIR-V 1.0 Table 2: Instruction Physical Layout
uint32_t GetSpirvInstructionLength(const uint32_t *instruction)
{
    return instruction[0] >> 16;
}

uint32_t GetSpirvInstructionOp(const uint32_t *instruction)
{
    constexpr uint32_t kOpMask = 0xFFFFu;
    return instruction[0] & kOpMask;
}

void SpirvTransformer::resolveVariableIds()
{
    // Allocate storage for id-to-info map.  If %i is the id of a name in mVariableInfoMap, index i
    // in this vector will hold a pointer to the ShaderInterfaceVariableInfo object associated with
    // that name in mVariableInfoMap.
    mVariableInfoById.resize(mSpirvBlobIn[kHeaderIndexIndexBound] + 1, nullptr);

    size_t currentWord = kHeaderIndexInstructions;

    while (currentWord < mSpirvBlobIn.size())
    {
        const uint32_t *instruction = &mSpirvBlobIn[currentWord];

        const uint32_t wordCount = GetSpirvInstructionLength(instruction);
        const uint32_t opCode    = GetSpirvInstructionOp(instruction);

        switch (opCode)
        {
            case spv::OpName:
                visitName(instruction);
                break;
            case spv::OpTypeArray:
                visitTypeArray(instruction);
                break;
            case spv::OpTypePointer:
                visitTypePointer(instruction);
                break;
            case spv::OpVariable:
                visitVariable(instruction);
                break;
            case spv::OpFunction:
                // SPIR-V is structured in sections. Names appear before decorations, which are
                // followed by type+variables and finally functions.  We are only interested in name
                // and variable declarations (as well as type declarations for the sake of nameless
                // interface blocks).  Early out when the function declaration section is met.
                return;
            default:
                break;
        }

        currentWord += wordCount;
    }
}

void SpirvTransformer::transformInstruction()
{
    const uint32_t *instruction = &mSpirvBlobIn[mCurrentWord];

    const uint32_t wordCount = GetSpirvInstructionLength(instruction);
    const uint32_t opCode    = GetSpirvInstructionOp(instruction);

    // Since glslang succeeded in producing SPIR-V, we assume it to be valid.
    const size_t spirvBlobInSize = mSpirvBlobIn.size();
    ASSERT(mCurrentWord + wordCount <= spirvBlobInSize);

    // Only look at interesting instructions.
    bool transformed = false;
    switch (opCode)
    {
        case spv::OpDecorate:
            transformed = transformDecorate(instruction, wordCount);
            break;
        case spv::OpFunction:
            // SPIR-V is structured in sections.  Function declarations come last.  This
            // transformation will not modify functions, so we can copy the rest of the spir-v
            // as-is.
            copyInstruction(instruction, spirvBlobInSize - mCurrentWord);
            mCurrentWord = spirvBlobInSize;
            return;
        default:
            break;
    }

    // If the instruction was not transformed, copy it to output as is.
    if (!transformed)
    {
        copyInstruction(instruction, wordCount);
    }

    // Advance to next instruction.
    mCurrentWord += wordCount;
}

void SpirvTransformer::visitName(const uint32_t *instruction)
{
    // We currently don't have any big-endian devices in the list of supported platforms.  Literal
    // strings in SPIR-V are stored little-endian (SPIR-V 1.0 Section 2.2.1, Literal String), so if
    // a big-endian device is to be supported, the string matching here should be specialized.
    ASSERT(IsLittleEndian());

    // SPIR-V 1.0 Section 3.32 Instructions, OpName
    constexpr size_t kIdIndex   = 1;
    constexpr size_t kNameIndex = 2;

    const uint32_t id = instruction[kIdIndex];
    const char *name  = reinterpret_cast<const char *>(&instruction[kNameIndex]);

    auto infoIter = mVariableInfoMap.find(name);
    if (infoIter == mVariableInfoMap.end())
    {
        return;
    }

    // The names and ids are unique
    ASSERT(id < mVariableInfoById.size());
    ASSERT(mVariableInfoById[id] == nullptr);

    // Associate the id of this name with its info.
    mVariableInfoById[id] = &infoIter->second;
}

void SpirvTransformer::visitTypeHelper(const uint32_t *instruction,
                                       const size_t idIndex,
                                       const size_t typeIdIndex)
{
    const uint32_t id     = instruction[idIndex];
    const uint32_t typeId = instruction[typeIdIndex];

    // Every type id is declared only once.
    ASSERT(typeId < mVariableInfoById.size());

    if (mVariableInfoById[typeId] != nullptr)
    {
        // Carry the info forward from the base type.  This is only necessary for interface blocks,
        // as the variable info is associated with the block name instead of the variable name (to
        // support nameless interface blocks).  In that case, the variable itself doesn't yet have
        // an associated info.
        ASSERT(id < mVariableInfoById.size());
        ASSERT(mVariableInfoById[id] == nullptr);

        mVariableInfoById[id] = mVariableInfoById[typeId];
    }
}

void SpirvTransformer::visitTypeArray(const uint32_t *instruction)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpTypeArray
    constexpr size_t kIdIndex            = 1;
    constexpr size_t kElementTypeIdIndex = 2;

    visitTypeHelper(instruction, kIdIndex, kElementTypeIdIndex);
}

void SpirvTransformer::visitTypePointer(const uint32_t *instruction)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpTypePointer
    constexpr size_t kIdIndex     = 1;
    constexpr size_t kTypeIdIndex = 3;

    visitTypeHelper(instruction, kIdIndex, kTypeIdIndex);
}

void SpirvTransformer::visitVariable(const uint32_t *instruction)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpVariable
    constexpr size_t kTypeIdIndex       = 1;
    constexpr size_t kIdIndex           = 2;
    constexpr size_t kStorageClassIndex = 3;

    visitTypeHelper(instruction, kIdIndex, kTypeIdIndex);

    // All resources that take set/binding should be transformed.
    const uint32_t id           = instruction[kIdIndex];
    const uint32_t storageClass = instruction[kStorageClassIndex];

    ASSERT((storageClass != spv::StorageClassUniform && storageClass != spv::StorageClassImage &&
            storageClass != spv::StorageClassStorageBuffer) ||
           mVariableInfoById[id] != nullptr);
}

bool SpirvTransformer::transformDecorate(const uint32_t *instruction, size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpDecorate
    constexpr size_t kIdIndex              = 1;
    constexpr size_t kDecorationIndex      = 2;
    constexpr size_t kDecorationValueIndex = 3;

    uint32_t id         = instruction[kIdIndex];
    uint32_t decoration = instruction[kDecorationIndex];

    const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];

    // If variable is not a shader interface variable that needs modification, there's nothing to
    // do.
    if (info == nullptr)
    {
        return false;
    }

    uint32_t newDecorationValue = ShaderInterfaceVariableInfo::kInvalid;

    switch (decoration)
    {
        case spv::DecorationLocation:
            newDecorationValue = info->location[mShaderType];
            break;
        case spv::DecorationComponent:
            newDecorationValue = info->component[mShaderType];
            break;
        case spv::DecorationBinding:
            newDecorationValue = info->binding;
            break;
        case spv::DecorationDescriptorSet:
            newDecorationValue = info->descriptorSet;
            break;
        case spv::DecorationOffset:
            newDecorationValue = info->xfbOffset;
            break;
        case spv::DecorationXfbBuffer:
            newDecorationValue = info->xfbBuffer;
            break;
        case spv::DecorationXfbStride:
            newDecorationValue = info->xfbStride;
            break;
        default:
            break;
    }

    // If the decoration is not something we care about modifying, there's nothing to do.
    if (newDecorationValue == ShaderInterfaceVariableInfo::kInvalid)
    {
        return false;
    }

    // Copy the decoration declaration and modify it.
    const size_t currentOutputOffset = getCurrentOutputOffset();
    copyInstruction(instruction, wordCount);
    (*mSpirvBlobOut)[currentOutputOffset + kDecorationValueIndex] = newDecorationValue;

    return true;
}

void SpirvTransformer::copyInstruction(const uint32_t *instruction, size_t wordCount)
{
    mSpirvBlobOut->insert(mSpirvBlobOut->end(), instruction, instruction + wordCount);
}

size_t SpirvTransformer::getCurrentOutputOffset() const
{
    return mSpirvBlobOut->size();
}
}  // anonymous namespace

const uint32_t ShaderInterfaceVariableInfo::kInvalid;

ShaderInterfaceVariableInfo::ShaderInterfaceVariableInfo()
{
    location.fill(kInvalid);
    component.fill(kInvalid);
}

void GlslangInitialize()
{
    int result = ShInitialize();
    ASSERT(result != 0);
}

void GlslangRelease()
{
    int result = ShFinalize();
    ASSERT(result != 0);
}

std::string GlslangGetMappedSamplerName(const std::string &originalName)
{
    std::string samplerName = originalName;

    // Samplers in structs are extracted.
    std::replace(samplerName.begin(), samplerName.end(), '.', '_');

    // Remove array elements
    auto out = samplerName.begin();
    for (auto in = samplerName.begin(); in != samplerName.end(); in++)
    {
        if (*in == '[')
        {
            while (*in != ']')
            {
                in++;
                ASSERT(in != samplerName.end());
            }
        }
        else
        {
            *out++ = *in;
        }
    }

    samplerName.erase(out, samplerName.end());

    if (MappedSamplerNameNeedsUserDefinedPrefix(originalName))
    {
        samplerName = sh::kUserDefinedNamePrefix + samplerName;
    }

    return samplerName;
}

void GlslangGetShaderSource(const GlslangSourceOptions &options,
                            const gl::ProgramState &programState,
                            const gl::ProgramLinkedResources &resources,
                            gl::ShaderMap<std::string> *shaderSourcesOut,
                            ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    variableInfoMapOut->clear();

    gl::ShaderMap<IntermediateShaderSource> intermediateSources;

    for (const gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        gl::Shader *glShader = programState.getAttachedShader(shaderType);
        if (glShader)
        {
            intermediateSources[shaderType].init(glShader->getTranslatedSource());
        }
    }

    IntermediateShaderSource *vertexSource   = &intermediateSources[gl::ShaderType::Vertex];
    IntermediateShaderSource *fragmentSource = &intermediateSources[gl::ShaderType::Fragment];
    IntermediateShaderSource *computeSource  = &intermediateSources[gl::ShaderType::Compute];

    XfbBufferMap xfbBufferMap;

    // Write transform feedback output code.
    if (!vertexSource->empty())
    {
        if (programState.getLinkedTransformFeedbackVaryings().empty())
        {
            vertexSource->insertTransformFeedbackDeclaration("");
            vertexSource->insertTransformFeedbackOutput("");
        }
        else
        {
            if (options.supportsTransformFeedbackExtension)
            {
                GenerateTransformFeedbackExtensionOutputs(programState, vertexSource, &xfbBufferMap,
                                                          resources);
            }
            else if (options.emulateTransformFeedback)
            {
                GenerateTransformFeedbackEmulationOutputs(options, programState, vertexSource,
                                                          variableInfoMapOut);
            }
        }
    }

    // Assign outputs to the fragment shader, if any.
    if (!fragmentSource->empty())
    {
        AssignOutputLocations(programState, variableInfoMapOut);
    }

    // Assign attributes to the vertex shader, if any.
    if (!vertexSource->empty())
    {
        AssignAttributeLocations(programState, variableInfoMapOut);
    }

    if (computeSource->empty())
    {
        // Assign varying locations.
        AssignVaryingLocations(programState, resources, &intermediateSources, &xfbBufferMap);
    }

    AssignUniformBindings(options, &intermediateSources, variableInfoMapOut);
    AssignTextureBindings(options, programState, variableInfoMapOut);
    AssignNonTextureBindings(options, programState, variableInfoMapOut);

    CleanupUnusedEntities(options.useOldRewriteStructSamplers, programState, resources,
                          &intermediateSources);

    for (const gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        (*shaderSourcesOut)[shaderType] = intermediateSources[shaderType].getShaderSource();
    }
}

angle::Result GlslangGetShaderSpirvCode(GlslangErrorCallback callback,
                                        const gl::Caps &glCaps,
                                        const gl::ShaderMap<std::string> &shaderSources,
                                        const ShaderInterfaceVariableInfoMap &variableInfoMap,
                                        gl::ShaderMap<SpirvBlob> *spirvBlobsOut)
{
    gl::ShaderMap<std::vector<uint32_t>> initialSpirvBlobs;
    ANGLE_TRY(GetShaderSpirvCode(callback, glCaps, shaderSources, &initialSpirvBlobs));

    // Transform the SPIR-V code by assigning location/set/binding values.
    for (const gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        const std::vector<uint32_t> initialSpirvBlob = initialSpirvBlobs[shaderType];

        if (initialSpirvBlob.empty())
        {
            continue;
        }

        SpirvBlob *spirvBlob = &(*spirvBlobsOut)[shaderType];

        SpirvTransformer transformer(initialSpirvBlob, variableInfoMap, shaderType, spirvBlob);
        ANGLE_GLSLANG_CHECK(callback, transformer.transform(), GlslangError::InvalidSpirv);

        ASSERT(ValidateSpirv(*spirvBlob));
    }

    return angle::Result::Continue;
}
}  // namespace rx
