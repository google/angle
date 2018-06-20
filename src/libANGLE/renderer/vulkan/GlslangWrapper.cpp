//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// GlslangWrapper: Wrapper for Vulkan's glslang compiler.
//

#include "libANGLE/renderer/vulkan/GlslangWrapper.h"

// glslang's version of ShaderLang.h, not to be confused with ANGLE's.
// Our function defs conflict with theirs, but we carefully manage our includes to prevent this.
#include <ShaderLang.h>

// Other glslang includes.
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>

#include <array>

#include "common/string_utils.h"
#include "common/utilities.h"
#include "libANGLE/Caps.h"
#include "libANGLE/ProgramLinkedResources.h"

namespace rx
{

namespace
{

constexpr char kQualifierMarkerBegin[] = "@@ QUALIFIER-";
constexpr char kLayoutMarkerBegin[]    = "@@ LAYOUT-";
constexpr char kMarkerEnd[]            = " @@";
constexpr char kUniformQualifier[]     = "uniform";

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

void InsertLayoutSpecifierString(std::string *shaderString,
                                 const std::string &variableName,
                                 const std::string &layoutString)
{
    std::stringstream searchStringBuilder;
    searchStringBuilder << kLayoutMarkerBegin << variableName << kMarkerEnd;
    std::string searchString = searchStringBuilder.str();

    if (!layoutString.empty())
    {
        angle::ReplaceSubstring(shaderString, searchString, "layout(" + layoutString + ")");
    }
    else
    {
        angle::ReplaceSubstring(shaderString, searchString, layoutString);
    }
}

void InsertQualifierSpecifierString(std::string *shaderString,
                                    const std::string &variableName,
                                    const std::string &replacementString)
{
    std::stringstream searchStringBuilder;
    searchStringBuilder << kQualifierMarkerBegin << variableName << kMarkerEnd;
    std::string searchString = searchStringBuilder.str();
    angle::ReplaceSubstring(shaderString, searchString, replacementString);
}

}  // anonymous namespace

// static
GlslangWrapper *GlslangWrapper::mInstance = nullptr;

// static
GlslangWrapper *GlslangWrapper::GetReference()
{
    if (!mInstance)
    {
        mInstance = new GlslangWrapper();
    }

    mInstance->addRef();

    return mInstance;
}

// static
void GlslangWrapper::ReleaseReference()
{
    if (mInstance->getRefCount() == 1)
    {
        mInstance->release();
        mInstance = nullptr;
    }
    else
    {
        mInstance->release();
    }
}

GlslangWrapper::GlslangWrapper()
{
    int result = ShInitialize();
    ASSERT(result != 0);
}

GlslangWrapper::~GlslangWrapper()
{
    int result = ShFinalize();
    ASSERT(result != 0);
}

gl::LinkResult GlslangWrapper::linkProgram(const gl::Context *glContext,
                                           const gl::ProgramState &programState,
                                           const gl::ProgramLinkedResources &resources,
                                           const gl::Caps &glCaps,
                                           std::vector<uint32_t> *vertexCodeOut,
                                           std::vector<uint32_t> *fragmentCodeOut)
{
    gl::Shader *glVertexShader   = programState.getAttachedShader(gl::ShaderType::Vertex);
    gl::Shader *glFragmentShader = programState.getAttachedShader(gl::ShaderType::Fragment);

    std::string vertexSource   = glVertexShader->getTranslatedSource(glContext);
    std::string fragmentSource = glFragmentShader->getTranslatedSource(glContext);

    // Parse attribute locations and replace them in the vertex shader.
    // See corresponding code in OutputVulkanGLSL.cpp.
    // TODO(jmadill): Also do the same for ESSL 3 fragment outputs.
    for (const sh::Attribute &attribute : programState.getAttributes())
    {
        // Warning: If we endup supporting ES 3.0 shaders and up, Program::linkAttributes is going
        // to bring us all attributes in this list instead of only the active ones.
        ASSERT(attribute.active);

        std::string locationString = "location = " + Str(attribute.location);
        InsertLayoutSpecifierString(&vertexSource, attribute.name, locationString);
        InsertQualifierSpecifierString(&vertexSource, attribute.name, "in");
    }

    // The attributes in the programState could have been filled with active attributes only
    // depending on the shader version. If there is inactive attributes left, we have to remove
    // their @@ QUALIFIER and @@ LAYOUT markers.
    for (const sh::Attribute &attribute : glVertexShader->getAllAttributes(glContext))
    {
        if (attribute.active)
        {
            continue;
        }

        InsertLayoutSpecifierString(&vertexSource, attribute.name, "");
        InsertQualifierSpecifierString(&vertexSource, attribute.name, "");
    }

    // Assign varying locations.
    for (const gl::PackedVaryingRegister &varyingReg : resources.varyingPacking.getRegisterList())
    {
        const auto &varying = *varyingReg.packedVarying;

        std::string locationString = "location = " + Str(varyingReg.registerRow);
        if (varyingReg.registerColumn > 0)
        {
            ASSERT(!varying.varying->isStruct());
            ASSERT(!gl::IsMatrixType(varying.varying->type));
            locationString += ", component = " + Str(varyingReg.registerColumn);
        }

        InsertLayoutSpecifierString(&vertexSource, varying.varying->name, locationString);
        InsertLayoutSpecifierString(&fragmentSource, varying.varying->name, locationString);

        ASSERT(varying.interpolation == sh::INTERPOLATION_SMOOTH);
        InsertQualifierSpecifierString(&vertexSource, varying.varying->name, "out");
        InsertQualifierSpecifierString(&fragmentSource, varying.varying->name, "in");
    }

    // Remove all the markers for unused varyings.
    for (const std::string &varyingName : resources.varyingPacking.getInactiveVaryingNames())
    {
        InsertLayoutSpecifierString(&vertexSource, varyingName, "");
        InsertLayoutSpecifierString(&fragmentSource, varyingName, "");
        InsertQualifierSpecifierString(&vertexSource, varyingName, "");
        InsertQualifierSpecifierString(&fragmentSource, varyingName, "");
    }

    // Bind the default uniforms for vertex and fragment shaders.
    // See corresponding code in OutputVulkanGLSL.cpp.
    std::stringstream searchStringBuilder;
    searchStringBuilder << "@@ DEFAULT-UNIFORMS-SET-BINDING @@";
    std::string searchString = searchStringBuilder.str();

    std::string vertexDefaultUniformsBinding   = "set = 0, binding = 0";
    std::string fragmentDefaultUniformsBinding = "set = 0, binding = 1";

    angle::ReplaceSubstring(&vertexSource, searchString, vertexDefaultUniformsBinding);
    angle::ReplaceSubstring(&fragmentSource, searchString, fragmentDefaultUniformsBinding);

    // Assign textures to a descriptor set and binding.
    int textureCount     = 0;
    const auto &uniforms = programState.getUniforms();
    for (unsigned int uniformIndex : programState.getSamplerUniformRange())
    {
        const gl::LinkedUniform &samplerUniform = uniforms[uniformIndex];
        std::string setBindingString            = "set = 1, binding = " + Str(textureCount);

        std::string samplerName = gl::ParseResourceName(samplerUniform.name, nullptr);

        // Samplers in structs are extracted.
        std::replace(samplerName.begin(), samplerName.end(), '.', '_');

        ASSERT(samplerUniform.isActive(gl::ShaderType::Vertex) ||
               samplerUniform.isActive(gl::ShaderType::Fragment));
        if (samplerUniform.isActive(gl::ShaderType::Vertex))
        {
            InsertLayoutSpecifierString(&vertexSource, samplerName, setBindingString);
        }
        InsertQualifierSpecifierString(&vertexSource, samplerName, kUniformQualifier);

        if (samplerUniform.isActive(gl::ShaderType::Fragment))
        {
            InsertLayoutSpecifierString(&fragmentSource, samplerName, setBindingString);
        }
        InsertQualifierSpecifierString(&fragmentSource, samplerName, kUniformQualifier);

        textureCount++;
    }

    for (const gl::UnusedUniform &unusedUniform : resources.unusedUniforms)
    {
        InsertLayoutSpecifierString(&vertexSource, unusedUniform.name, "");
        InsertLayoutSpecifierString(&fragmentSource, unusedUniform.name, "");

        std::string qualifierToUse = unusedUniform.isSampler ? kUniformQualifier : "";
        InsertQualifierSpecifierString(&vertexSource, unusedUniform.name, qualifierToUse);
        InsertQualifierSpecifierString(&fragmentSource, unusedUniform.name, qualifierToUse);
    }

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
        return gl::InternalError() << "Internal error parsing Vulkan vertex shader:\n"
                                   << vertexShader.getInfoLog() << "\n"
                                   << vertexShader.getInfoDebugLog() << "\n";
    }

    glslang::TShader fragmentShader(EShLangFragment);
    fragmentShader.setStringsWithLengths(&strings[1], &lengths[1], 1);
    fragmentShader.setEntryPoint("main");
    bool fragmentResult =
        fragmentShader.parse(&builtInResources, 450, ECoreProfile, false, false, messages);
    if (!fragmentResult)
    {
        return gl::InternalError() << "Internal error parsing Vulkan fragment shader:\n"
                                   << fragmentShader.getInfoLog() << "\n"
                                   << fragmentShader.getInfoDebugLog() << "\n";
    }

    glslang::TProgram program;
    program.addShader(&vertexShader);
    program.addShader(&fragmentShader);
    bool linkResult = program.link(messages);
    if (!linkResult)
    {
        return gl::InternalError() << "Internal error linking Vulkan shaders:\n"
                                   << program.getInfoLog() << "\n";
    }

    glslang::TIntermediate *vertexStage   = program.getIntermediate(EShLangVertex);
    glslang::TIntermediate *fragmentStage = program.getIntermediate(EShLangFragment);
    glslang::GlslangToSpv(*vertexStage, *vertexCodeOut);
    glslang::GlslangToSpv(*fragmentStage, *fragmentCodeOut);

    return true;
}

}  // namespace rx
