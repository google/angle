//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// GlslangWrapper: Wrapper for Vulkan's glslang compiler.
//

#include "libANGLE/renderer/vulkan/GlslangWrapper.h"

// glslang has issues with some specific warnings.
ANGLE_DISABLE_EXTRA_SEMI_WARNING

// glslang's version of ShaderLang.h, not to be confused with ANGLE's.
#include <glslang/Public/ShaderLang.h>

// Other glslang includes.
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>

ANGLE_REENABLE_EXTRA_SEMI_WARNING

#include <array>

#include "common/FixedVector.h"
#include "common/string_utils.h"
#include "common/utilities.h"
#include "libANGLE/Caps.h"
#include "libANGLE/ProgramLinkedResources.h"
#include "libANGLE/renderer/vulkan/vk_cache_utils.h"

namespace rx
{
namespace
{
constexpr char kMarkerStart[]          = "@@ ";
constexpr char kQualifierMarkerBegin[] = "@@ QUALIFIER-";
constexpr char kLayoutMarkerBegin[]    = "@@ LAYOUT-";
constexpr char kMarkerEnd[]            = " @@";
constexpr char kLayoutParamsBegin      = '(';
constexpr char kLayoutParamsEnd        = ')';
constexpr char kUniformQualifier[]     = "uniform";
constexpr char kVersionDefine[]        = "#version 450 core\n";
constexpr char kLineRasterDefine[]     = R"(#version 450 core

#define ANGLE_ENABLE_LINE_SEGMENT_RASTERIZATION
)";

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

class IntermediateShaderSource final : angle::NonCopyable
{
  public:
    IntermediateShaderSource(const std::string &source);

    // Find @@ LAYOUT-name(extra, args) @@ and replace it with:
    //
    //     layout(specifier, extra, args)
    //
    // or if specifier is empty,
    //
    //     layout(extra, args)
    //
    void insertLayoutSpecifier(const std::string &name, const std::string &specifier);

    // Find @@ QUALIFIER-name @@ and replace it with |specifier|.
    void insertQualifierSpecifier(const std::string &name, const std::string &specifier);

    // Remove @@ LAYOUT-name(*) @@ and @@ QUALIFIER-name @@ altogether.
    void eraseLayoutAndQualifierSpecifiers(const std::string &name);

    // Replace @@ DEFAULT-UNIFORMS-SET-BINDING @@ with |specifier|.
    void insertDefaultUniformsSpecifier(std::string &&specifier);

    // Get the transformed shader source as one string.
    std::string getShaderSource();

  private:
    enum class TokenType
    {
        // A piece of shader source code.
        Text,
        // Block corresponding to @@ QUALIFIER-abc @@
        Qualifier,
        // Block corresponding to @@ LAYOUT-abc(extra, args) @@
        Layout,
    };

    struct Token
    {
        TokenType type;
        // |text| contains some shader code if Text, or the id of macro ("abc" in examples above)
        // being replaced if Qualifier or Layout.
        std::string text;
        // If Layout, this contains extra parameters passed in parentheses, if any.
        std::string args;
    };

    void addTextBlock(std::string &&text);
    void addLayoutBlock(std::string &&name, std::string &&args);
    void addQualifierBlock(std::string &&name);

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

void IntermediateShaderSource::addQualifierBlock(std::string &&name)
{
    ASSERT(!name.empty());
    Token token = {TokenType::Qualifier, std::move(name), ""};
    mTokens.emplace_back(std::move(token));
}

IntermediateShaderSource::IntermediateShaderSource(const std::string &source)
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

            // Get the id of the macro and add a qualifier block.
            std::string name = angle::GetPrefix(source, cur, kMarkerEnd);
            cur += name.length();
            addQualifierBlock(std::move(name));
        }
        else if (source.compare(cur, ConstStrLen(kLayoutMarkerBegin), kLayoutMarkerBegin) == 0)
        {
            cur += ConstStrLen(kLayoutMarkerBegin);

            // Get the id and arguments of the macro and add a layout block.
            // There should always be an extra args list (even if empty, for simplicity).
            std::string name = angle::GetPrefix(source, cur, kLayoutParamsBegin);
            cur += name.length() + 1;
            std::string args = angle::GetPrefix(source, cur, kLayoutParamsEnd);
            cur += args.length() + 1;
            addLayoutBlock(std::move(name), std::move(args));
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
            const char *separator = specifier.empty() || block.args.empty() ? "" : ", ";

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
            break;
        }
    }
}

void IntermediateShaderSource::eraseLayoutAndQualifierSpecifiers(const std::string &name)
{
    for (Token &block : mTokens)
    {
        if ((block.type == TokenType::Layout || block.type == TokenType::Qualifier) &&
            block.text == name)
        {
            block.type = TokenType::Text;
            block.text = "";
        }
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

std::string GetMappedSamplerName(const std::string &originalName)
{
    std::string samplerName = gl::ParseResourceName(originalName, nullptr);

    // Samplers in structs are extracted.
    std::replace(samplerName.begin(), samplerName.end(), '.', '_');

    // Samplers in arrays of structs are also extracted.
    std::replace(samplerName.begin(), samplerName.end(), '[', '_');
    samplerName.erase(std::remove(samplerName.begin(), samplerName.end(), ']'), samplerName.end());
    return samplerName;
}
}  // anonymous namespace

// static
void GlslangWrapper::Initialize()
{
    int result = ShInitialize();
    ASSERT(result != 0);
}

// static
void GlslangWrapper::Release()
{
    int result = ShFinalize();
    ASSERT(result != 0);
}

// static
void GlslangWrapper::GetShaderSource(const gl::ProgramState &programState,
                                     const gl::ProgramLinkedResources &resources,
                                     std::string *vertexSourceOut,
                                     std::string *fragmentSourceOut)
{
    gl::Shader *glVertexShader   = programState.getAttachedShader(gl::ShaderType::Vertex);
    gl::Shader *glFragmentShader = programState.getAttachedShader(gl::ShaderType::Fragment);

    IntermediateShaderSource vertexSource(glVertexShader->getTranslatedSource());
    IntermediateShaderSource fragmentSource(glFragmentShader->getTranslatedSource());

    // Parse attribute locations and replace them in the vertex shader.
    // See corresponding code in OutputVulkanGLSL.cpp.
    for (const sh::Attribute &attribute : programState.getAttributes())
    {
        // Warning: If we endup supporting ES 3.0 shaders and up, Program::linkAttributes is going
        // to bring us all attributes in this list instead of only the active ones.
        ASSERT(attribute.active);

        std::string locationString = "location = " + Str(attribute.location);
        vertexSource.insertLayoutSpecifier(attribute.name, locationString);
        vertexSource.insertQualifierSpecifier(attribute.name, "in");
    }

    // The attributes in the programState could have been filled with active attributes only
    // depending on the shader version. If there is inactive attributes left, we have to remove
    // their @@ QUALIFIER and @@ LAYOUT markers.
    for (const sh::Attribute &attribute : glVertexShader->getAllAttributes())
    {
        if (attribute.active)
        {
            continue;
        }

        vertexSource.eraseLayoutAndQualifierSpecifiers(attribute.name);
    }

    // Parse output locations and replace them in the fragment shader.
    // See corresponding code in OutputVulkanGLSL.cpp.
    // TODO(syoussefi): Add support for EXT_blend_func_extended.  http://anglebug.com/3385
    const auto &outputLocations = programState.getOutputLocations();
    const auto &outputVariables = programState.getOutputVariables();
    for (const gl::VariableLocation &outputLocation : outputLocations)
    {
        if (outputLocation.arrayIndex == 0 && outputLocation.used() && !outputLocation.ignored)
        {
            const sh::OutputVariable &outputVar = outputVariables[outputLocation.index];

            std::string locationString;
            if (outputVar.location != -1)
            {
                locationString = "location = " + Str(outputVar.location);
            }
            else
            {
                // If there is only one output, it is allowed not to have a location qualifier, in
                // which case it defaults to 0.  GLSL ES 3.00 spec, section 4.3.8.2.
                ASSERT(outputVariables.size() == 1);
                locationString = "location = 0";
            }

            fragmentSource.insertLayoutSpecifier(outputVar.name, locationString);
        }
    }

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

        std::string locationString = "location = " + Str(varyingReg.registerRow);
        if (varyingReg.registerColumn > 0)
        {
            ASSERT(!varying.varying->isStruct());
            ASSERT(!gl::IsMatrixType(varying.varying->type));
            locationString += ", component = " + Str(varyingReg.registerColumn);
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

        vertexSource.insertLayoutSpecifier(name, locationString);
        fragmentSource.insertLayoutSpecifier(name, locationString);

        ASSERT(varying.interpolation == sh::INTERPOLATION_SMOOTH);
        vertexSource.insertQualifierSpecifier(name, "out");
        fragmentSource.insertQualifierSpecifier(name, "in");
    }

    // Remove all the markers for unused varyings.
    for (const std::string &varyingName : resources.varyingPacking.getInactiveVaryingNames())
    {
        vertexSource.eraseLayoutAndQualifierSpecifiers(varyingName);
        fragmentSource.eraseLayoutAndQualifierSpecifiers(varyingName);
    }

    // Assign uniform locations

    // Bind the default uniforms for vertex and fragment shaders.
    // See corresponding code in OutputVulkanGLSL.cpp.
    const std::string uniformsSearchString("@@ DEFAULT-UNIFORMS-SET-BINDING @@");

    const std::string driverUniformsDescriptorSet =
        "set = " + Str(kDriverUniformsDescriptorSetIndex);
    const std::string uniformsDescriptorSet      = "set = " + Str(kUniformsDescriptorSetIndex);
    const std::string uniformBlocksDescriptorSet = "set = " + Str(kUniformBlockDescriptorSetIndex);
    const std::string texturesDescriptorSet      = "set = " + Str(kTextureDescriptorSetIndex);

    std::string vertexDefaultUniformsBinding =
        uniformsDescriptorSet + ", binding = " + Str(kVertexUniformsBindingIndex);
    std::string fragmentDefaultUniformsBinding =
        uniformsDescriptorSet + ", binding = " + Str(kFragmentUniformsBindingIndex);

    constexpr char kDefaultUniformsBlockName[] = "defaultUniforms";
    vertexSource.insertLayoutSpecifier(kDefaultUniformsBlockName, vertexDefaultUniformsBinding);
    fragmentSource.insertLayoutSpecifier(kDefaultUniformsBlockName, fragmentDefaultUniformsBinding);

    // Assign uniform blocks to a descriptor set and binding.
    const auto &uniformBlocks    = programState.getUniformBlocks();
    uint32_t uniformBlockBinding = 0;
    for (const gl::InterfaceBlock &uniformBlock : uniformBlocks)
    {
        const std::string setBindingString =
            uniformBlocksDescriptorSet + ", binding = " + Str(uniformBlockBinding);

        vertexSource.insertLayoutSpecifier(uniformBlock.name, setBindingString);
        fragmentSource.insertLayoutSpecifier(uniformBlock.name, setBindingString);

        vertexSource.insertQualifierSpecifier(uniformBlock.name, kUniformQualifier);
        fragmentSource.insertQualifierSpecifier(uniformBlock.name, kUniformQualifier);

        ++uniformBlockBinding;
    }

    // Assign textures to a descriptor set and binding.
    uint32_t textureBinding = 0;
    const auto &uniforms = programState.getUniforms();
    for (unsigned int uniformIndex : programState.getSamplerUniformRange())
    {
        const gl::LinkedUniform &samplerUniform = uniforms[uniformIndex];
        const std::string setBindingString =
            texturesDescriptorSet + ", binding = " + Str(textureBinding);

        // Samplers in structs are extracted and renamed.
        const std::string samplerName = GetMappedSamplerName(samplerUniform.name);

        ASSERT(samplerUniform.isActive(gl::ShaderType::Vertex) ||
               samplerUniform.isActive(gl::ShaderType::Fragment));
        if (samplerUniform.isActive(gl::ShaderType::Vertex))
        {
            vertexSource.insertLayoutSpecifier(samplerName, setBindingString);
        }
        vertexSource.insertQualifierSpecifier(samplerName, kUniformQualifier);

        if (samplerUniform.isActive(gl::ShaderType::Fragment))
        {
            fragmentSource.insertLayoutSpecifier(samplerName, setBindingString);
        }
        fragmentSource.insertQualifierSpecifier(samplerName, kUniformQualifier);

        textureBinding++;
    }

    // Place the unused uniforms in the driver uniforms descriptor set, which has a fixed number of
    // bindings.  This avoids any possible index collision between uniform bindings set in the
    // shader and the ones assigned here to the unused ones.
    constexpr int kBaseUnusedSamplerBinding = kReservedDriverUniformBindingCount;
    int unusedSamplerBinding                = kBaseUnusedSamplerBinding;

    for (const gl::UnusedUniform &unusedUniform : resources.unusedUniforms)
    {
        if (unusedUniform.isSampler)
        {
            // Samplers in structs are extracted and renamed.
            std::string uniformName = GetMappedSamplerName(unusedUniform.name);

            std::stringstream layoutStringStream;

            layoutStringStream << driverUniformsDescriptorSet + ", binding = "
                               << unusedSamplerBinding++;

            std::string layoutString = layoutStringStream.str();

            vertexSource.insertLayoutSpecifier(uniformName, layoutString);
            fragmentSource.insertLayoutSpecifier(uniformName, layoutString);

            vertexSource.insertQualifierSpecifier(uniformName, kUniformQualifier);
            fragmentSource.insertQualifierSpecifier(uniformName, kUniformQualifier);
        }
        else
        {
            vertexSource.eraseLayoutAndQualifierSpecifiers(unusedUniform.name);
            fragmentSource.eraseLayoutAndQualifierSpecifiers(unusedUniform.name);
        }
    }

    // Substitute layout and qualifier strings for the driver uniforms block.
    const std::string driverBlockLayoutString = driverUniformsDescriptorSet + ", binding = 0";
    constexpr char kDriverBlockName[]         = "ANGLEUniformBlock";
    vertexSource.insertLayoutSpecifier(kDriverBlockName, driverBlockLayoutString);
    fragmentSource.insertLayoutSpecifier(kDriverBlockName, driverBlockLayoutString);

    vertexSource.insertQualifierSpecifier(kDriverBlockName, kUniformQualifier);
    fragmentSource.insertQualifierSpecifier(kDriverBlockName, kUniformQualifier);

    // Substitute layout and qualifier strings for the position varying. Use the first free
    // varying register after the packed varyings.
    constexpr char kVaryingName[] = "ANGLEPosition";
    std::stringstream layoutStream;
    layoutStream << "location = " << (resources.varyingPacking.getMaxSemanticIndex() + 1);
    const std::string layout = layoutStream.str();
    vertexSource.insertLayoutSpecifier(kVaryingName, layout);
    fragmentSource.insertLayoutSpecifier(kVaryingName, layout);

    vertexSource.insertQualifierSpecifier(kVaryingName, "out");
    fragmentSource.insertQualifierSpecifier(kVaryingName, "in");

    *vertexSourceOut   = vertexSource.getShaderSource();
    *fragmentSourceOut = fragmentSource.getShaderSource();
}

// static
angle::Result GlslangWrapper::GetShaderCode(vk::Context *context,
                                            const gl::Caps &glCaps,
                                            bool enableLineRasterEmulation,
                                            const std::string &vertexSource,
                                            const std::string &fragmentSource,
                                            std::vector<uint32_t> *vertexCodeOut,
                                            std::vector<uint32_t> *fragmentCodeOut)
{
    if (enableLineRasterEmulation)
    {
        std::string patchedVertexSource   = vertexSource;
        std::string patchedFragmentSource = fragmentSource;

        // #defines must come after the #version directive.
        ANGLE_VK_CHECK(
            context,
            angle::ReplaceSubstring(&patchedVertexSource, kVersionDefine, kLineRasterDefine),
            VK_ERROR_INVALID_SHADER_NV);
        ANGLE_VK_CHECK(
            context,
            angle::ReplaceSubstring(&patchedFragmentSource, kVersionDefine, kLineRasterDefine),
            VK_ERROR_INVALID_SHADER_NV);

        return GetShaderCodeImpl(context, glCaps, patchedVertexSource, patchedFragmentSource,
                                 vertexCodeOut, fragmentCodeOut);
    }
    else
    {
        return GetShaderCodeImpl(context, glCaps, vertexSource, fragmentSource, vertexCodeOut,
                                 fragmentCodeOut);
    }
}

// static
angle::Result GlslangWrapper::GetShaderCodeImpl(vk::Context *context,
                                                const gl::Caps &glCaps,
                                                const std::string &vertexSource,
                                                const std::string &fragmentSource,
                                                std::vector<uint32_t> *vertexCodeOut,
                                                std::vector<uint32_t> *fragmentCodeOut)
{
    std::array<const char *, 2> strings = {{vertexSource.c_str(), fragmentSource.c_str()}};
    std::array<int, 2> lengths          = {
        {static_cast<int>(vertexSource.length()), static_cast<int>(fragmentSource.length())}};

    // Enable SPIR-V and Vulkan rules when parsing GLSL
    EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    glslang::TShader vertexShader(EShLangVertex);
    vertexShader.setStringsWithLengths(&strings[0], &lengths[0], 1);
    vertexShader.setEntryPoint("main");

    TBuiltInResource builtInResources(glslang::DefaultTBuiltInResource);
    GetBuiltInResourcesFromCaps(glCaps, &builtInResources);

    bool vertexResult =
        vertexShader.parse(&builtInResources, 450, ECoreProfile, false, false, messages);
    if (!vertexResult)
    {
        ERR() << "Internal error parsing Vulkan vertex shader:\n"
              << vertexShader.getInfoLog() << "\n"
              << vertexShader.getInfoDebugLog() << "\n";
        ANGLE_VK_CHECK(context, false, VK_ERROR_INVALID_SHADER_NV);
    }

    glslang::TShader fragmentShader(EShLangFragment);
    fragmentShader.setStringsWithLengths(&strings[1], &lengths[1], 1);
    fragmentShader.setEntryPoint("main");
    bool fragmentResult =
        fragmentShader.parse(&builtInResources, 450, ECoreProfile, false, false, messages);
    if (!fragmentResult)
    {
        ERR() << "Internal error parsing Vulkan fragment shader:\n"
              << fragmentShader.getInfoLog() << "\n"
              << fragmentShader.getInfoDebugLog() << "\n";
        ANGLE_VK_CHECK(context, false, VK_ERROR_INVALID_SHADER_NV);
    }

    glslang::TProgram program;
    program.addShader(&vertexShader);
    program.addShader(&fragmentShader);
    bool linkResult = program.link(messages);
    if (!linkResult)
    {
        ERR() << "Internal error linking Vulkan shaders:\n" << program.getInfoLog() << "\n";
        ANGLE_VK_CHECK(context, false, VK_ERROR_INVALID_SHADER_NV);
    }

    glslang::TIntermediate *vertexStage   = program.getIntermediate(EShLangVertex);
    glslang::TIntermediate *fragmentStage = program.getIntermediate(EShLangFragment);
    glslang::GlslangToSpv(*vertexStage, *vertexCodeOut);
    glslang::GlslangToSpv(*fragmentStage, *fragmentCodeOut);

    return angle::Result::Continue;
}
}  // namespace rx
