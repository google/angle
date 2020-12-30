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
ANGLE_DISABLE_SUGGEST_OVERRIDE_WARNINGS

// glslang's version of ShaderLang.h, not to be confused with ANGLE's.
#include <glslang/Public/ShaderLang.h>

// Other glslang includes.
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/ResourceLimits.h>

ANGLE_REENABLE_SUGGEST_OVERRIDE_WARNINGS
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
#include "libANGLE/trace.h"

#define ANGLE_GLSLANG_CHECK(CALLBACK, TEST, ERR) \
    do                                           \
    {                                            \
        if (ANGLE_UNLIKELY(!(TEST)))             \
        {                                        \
            return CALLBACK(ERR);                \
        }                                        \
                                                 \
    } while (0)

// Enable this for debug logging of pre-transform SPIR-V:
#if !defined(ANGLE_DEBUG_SPIRV_TRANSFORMER)
#    define ANGLE_DEBUG_SPIRV_TRANSFORMER 0
#endif  // !defined(ANGLE_DEBUG_SPIRV_TRANSFORMER)

namespace rx
{
namespace
{
constexpr char kXfbDeclMarker[]    = "@@ XFB-DECL @@";
constexpr char kXfbOutMarker[]     = "@@ XFB-OUT @@;";
constexpr char kXfbBuiltInPrefix[] = "xfbANGLE";

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
    outBuiltInResources->maxClipDistances                 = caps.maxClipDistances;
    outBuiltInResources->maxSamples                       = caps.maxSamples;
}

// Run at startup to warm up glslang's internals to avoid hitches on first shader compile.
void GlslangWarmup()
{
    ANGLE_TRACE_EVENT0("gpu.angle,startup", "GlslangWarmup");

    EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    // EShMessages messages = EShMsgDefault;

    const TBuiltInResource builtInResources(glslang::DefaultTBuiltInResource);
    glslang::TShader warmUpShader(EShLangVertex);

    const char *kShaderString = R"(#version 450 core
        void main(){}
    )";
    const int kShaderLength   = static_cast<int>(strlen(kShaderString));

    warmUpShader.setStringsWithLengths(&kShaderString, &kShaderLength, 1);
    warmUpShader.setEntryPoint("main");

    bool result = warmUpShader.parse(&builtInResources, 450, ECoreProfile, false, false, messages);
    ASSERT(result);
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

bool MappedSamplerNameNeedsUserDefinedPrefix(const std::string &originalName)
{
    return originalName.find('.') == std::string::npos;
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

ShaderInterfaceVariableInfo *AddResourceInfoToAllStages(ShaderInterfaceVariableInfoMap *infoMap,
                                                        gl::ShaderType shaderType,
                                                        const std::string &varName,
                                                        uint32_t descriptorSet,
                                                        uint32_t binding)
{
    gl::ShaderBitSet allStages;
    allStages.set();

    ShaderInterfaceVariableInfo &info = infoMap->add(shaderType, varName);
    info.descriptorSet                = descriptorSet;
    info.binding                      = binding;
    info.activeStages                 = allStages;
    return &info;
}

ShaderInterfaceVariableInfo *AddResourceInfo(ShaderInterfaceVariableInfoMap *infoMap,
                                             gl::ShaderType shaderType,
                                             const std::string &varName,
                                             uint32_t descriptorSet,
                                             uint32_t binding)
{
    gl::ShaderBitSet stages;
    stages.set(shaderType);

    ShaderInterfaceVariableInfo &info = infoMap->add(shaderType, varName);
    info.descriptorSet                = descriptorSet;
    info.binding                      = binding;
    info.activeStages                 = stages;
    return &info;
}

// Add location information for an in/out variable.
ShaderInterfaceVariableInfo *AddLocationInfo(ShaderInterfaceVariableInfoMap *infoMap,
                                             gl::ShaderType shaderType,
                                             const std::string &varName,
                                             uint32_t location,
                                             uint32_t component,
                                             uint8_t attributeComponentCount,
                                             uint8_t attributeLocationCount)
{
    // The info map for this name may or may not exist already.  This function merges the
    // location/component information.
    ShaderInterfaceVariableInfo &info = infoMap->addOrGet(shaderType, varName);

    ASSERT(info.descriptorSet == ShaderInterfaceVariableInfo::kInvalid);
    ASSERT(info.binding == ShaderInterfaceVariableInfo::kInvalid);
    ASSERT(info.location == ShaderInterfaceVariableInfo::kInvalid);
    ASSERT(info.component == ShaderInterfaceVariableInfo::kInvalid);

    info.location  = location;
    info.component = component;
    info.activeStages.set(shaderType);
    info.attributeComponentCount = attributeComponentCount;
    info.attributeLocationCount  = attributeLocationCount;

    return &info;
}

// Add location information for an in/out variable
void AddVaryingLocationInfo(ShaderInterfaceVariableInfoMap *infoMap,
                            const gl::VaryingInShaderRef &ref,
                            const bool isStructField,
                            const uint32_t location,
                            const uint32_t component)
{
    const std::string &name = isStructField ? ref.parentStructMappedName : ref.varying->mappedName;
    AddLocationInfo(infoMap, ref.stage, name, location, component, 0, 0);
}

// Modify an existing out variable and add transform feedback information.
ShaderInterfaceVariableInfo *SetXfbInfo(ShaderInterfaceVariableInfoMap *infoMap,
                                        gl::ShaderType shaderType,
                                        const std::string &varName,
                                        int fieldIndex,
                                        uint32_t xfbBuffer,
                                        uint32_t xfbOffset,
                                        uint32_t xfbStride)
{
    ShaderInterfaceVariableInfo &info   = infoMap->get(shaderType, varName);
    ShaderInterfaceVariableXfbInfo *xfb = &info.xfb;

    if (fieldIndex >= 0)
    {
        if (info.fieldXfb.size() <= static_cast<size_t>(fieldIndex))
        {
            info.fieldXfb.resize(fieldIndex + 1);
        }
        xfb = &info.fieldXfb[fieldIndex];
    }

    ASSERT(xfb->buffer == ShaderInterfaceVariableXfbInfo::kInvalid);
    ASSERT(xfb->offset == ShaderInterfaceVariableXfbInfo::kInvalid);
    ASSERT(xfb->stride == ShaderInterfaceVariableXfbInfo::kInvalid);

    xfb->buffer = xfbBuffer;
    xfb->offset = xfbOffset;
    xfb->stride = xfbStride;
    return &info;
}

std::string SubstituteTransformFeedbackMarkers(const std::string &originalSource,
                                               const std::string &xfbDecl,
                                               const std::string &xfbOut)
{
    const size_t xfbDeclMarkerStart = originalSource.find(kXfbDeclMarker);
    const size_t xfbDeclMarkerEnd   = xfbDeclMarkerStart + ConstStrLen(kXfbDeclMarker);

    const size_t xfbOutMarkerStart = originalSource.find(kXfbOutMarker, xfbDeclMarkerStart);
    const size_t xfbOutMarkerEnd   = xfbOutMarkerStart + ConstStrLen(kXfbOutMarker);

    // The shader is the following form:
    //
    // ..part1..
    // @@ XFB-DECL @@
    // ..part2..
    // @@ XFB-OUT @@;
    // ..part3..
    //
    // Construct the string by concatenating these five pieces, replacing the markers with the given
    // values.
    std::string result;

    result.append(&originalSource[0], &originalSource[xfbDeclMarkerStart]);
    result.append(xfbDecl);
    result.append(&originalSource[xfbDeclMarkerEnd], &originalSource[xfbOutMarkerStart]);
    result.append(xfbOut);
    result.append(&originalSource[xfbOutMarkerEnd], &originalSource[originalSource.size()]);

    return result;
}

void GenerateTransformFeedbackVaryingOutput(const gl::TransformFeedbackVarying &varying,
                                            const gl::UniformTypeInfo &info,
                                            size_t offset,
                                            const std::string &bufferIndex,
                                            std::ostringstream *xfbOut)
{
    const size_t arrayIndexStart = varying.arrayIndex == GL_INVALID_INDEX ? 0 : varying.arrayIndex;
    const size_t arrayIndexEnd   = arrayIndexStart + varying.size();

    for (size_t arrayIndex = arrayIndexStart; arrayIndex < arrayIndexEnd; ++arrayIndex)
    {
        for (int col = 0; col < info.columnCount; ++col)
        {
            for (int row = 0; row < info.rowCount; ++row)
            {
                *xfbOut << "xfbOut" << bufferIndex << "[xfbOffsets[" << bufferIndex << "] + "
                        << offset << "] = " << info.glslAsFloat << "(" << varying.mappedName;

                if (varying.isArray())
                {
                    *xfbOut << "[" << arrayIndex << "]";
                }

                if (info.columnCount > 1)
                {
                    *xfbOut << "[" << col << "]";
                }

                if (info.rowCount > 1)
                {
                    *xfbOut << "[" << row << "]";
                }

                *xfbOut << ");\n";
                ++offset;
            }
        }
    }
}

void GenerateTransformFeedbackEmulationOutputs(const GlslangSourceOptions &options,
                                               gl::ShaderType shaderType,
                                               const gl::ProgramState &programState,
                                               GlslangProgramInterfaceInfo *programInterfaceInfo,
                                               std::string *vertexShader,
                                               ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    const std::vector<gl::TransformFeedbackVarying> &varyings =
        programState.getLinkedTransformFeedbackVaryings();
    const std::vector<GLsizei> &bufferStrides = programState.getTransformFeedbackStrides();
    const bool isInterleaved =
        programState.getTransformFeedbackBufferMode() == GL_INTERLEAVED_ATTRIBS;
    const size_t bufferCount = isInterleaved ? 1 : varyings.size();
    ASSERT(bufferCount > 0);

    const std::string xfbSet = Str(programInterfaceInfo->uniformsAndXfbDescriptorSetIndex);
    std::vector<std::string> xfbIndices(bufferCount);

    std::string xfbDecl;

    for (uint32_t bufferIndex = 0; bufferIndex < bufferCount; ++bufferIndex)
    {
        const std::string xfbBinding = Str(programInterfaceInfo->currentUniformBindingIndex);
        xfbIndices[bufferIndex]      = Str(bufferIndex);

        std::string bufferName = GetXfbBufferName(bufferIndex);

        xfbDecl += "layout(set = " + xfbSet + ", binding = " + xfbBinding + ") buffer " +
                   bufferName + " { float xfbOut" + Str(bufferIndex) + "[]; };\n";

        // Add this entry to the info map, so we can easily assert that every resource has an entry
        // in this map.
        AddResourceInfo(variableInfoMapOut, shaderType, bufferName,
                        programInterfaceInfo->uniformsAndXfbDescriptorSetIndex,
                        programInterfaceInfo->currentUniformBindingIndex);
        ++programInterfaceInfo->currentUniformBindingIndex;
    }

    const std::string driverUniforms = std::string(sh::vk::kDriverUniformsVarName);
    std::ostringstream xfbOut;

    xfbOut << "if (" << driverUniforms << ".xfbActiveUnpaused != 0)\n{\nivec4 xfbOffsets = "
           << sh::vk::kXfbEmulationGetOffsetsFunctionName << "(ivec4(";
    for (size_t bufferIndex = 0; bufferIndex < bufferCount; ++bufferIndex)
    {
        if (bufferIndex > 0)
        {
            xfbOut << ", ";
        }

        ASSERT(bufferStrides[bufferIndex] % 4 == 0);
        xfbOut << bufferStrides[bufferIndex] / 4;
    }
    for (size_t bufferIndex = bufferCount; bufferIndex < 4; ++bufferIndex)
    {
        xfbOut << ", 0";
    }
    xfbOut << "));\n";
    size_t outputOffset = 0;
    for (size_t varyingIndex = 0; varyingIndex < varyings.size(); ++varyingIndex)
    {
        const size_t bufferIndex                    = isInterleaved ? 0 : varyingIndex;
        const gl::TransformFeedbackVarying &varying = varyings[varyingIndex];

        // For every varying, output to the respective buffer packed.  If interleaved, the output is
        // always to the same buffer, but at different offsets.
        const gl::UniformTypeInfo &info = gl::GetUniformTypeInfo(varying.type);
        GenerateTransformFeedbackVaryingOutput(varying, info, outputOffset, xfbIndices[bufferIndex],
                                               &xfbOut);

        if (isInterleaved)
        {
            outputOffset += info.columnCount * info.rowCount * varying.size();
        }
    }
    xfbOut << "}\n";

    *vertexShader = SubstituteTransformFeedbackMarkers(*vertexShader, xfbDecl, xfbOut.str());
}

bool IsFirstRegisterOfVarying(const gl::PackedVaryingRegister &varyingReg, bool allowFields)
{
    const gl::PackedVarying &varying = *varyingReg.packedVarying;

    // In Vulkan GLSL, struct fields are not allowed to have location assignments.  The varying of a
    // struct type is thus given a location equal to the one assigned to its first field.  With I/O
    // blocks, transform feedback can capture an arbitrary field.  In that case, we need to look at
    // every field, not just the first one.
    if (!allowFields && varying.isStructField() &&
        (varying.fieldIndex > 0 || varying.secondaryFieldIndex > 0))
    {
        return false;
    }

    // Similarly, assign array varying locations to the assigned location of the first element.
    if (varyingReg.varyingArrayIndex != 0 ||
        (varying.arrayIndex != GL_INVALID_INDEX && varying.arrayIndex != 0))
    {
        return false;
    }

    // Similarly, assign matrix varying locations to the assigned location of the first row.
    if (varyingReg.varyingRowIndex != 0)
    {
        return false;
    }

    return true;
}

// Calculates XFB layout qualifier arguments for each tranform feedback varying.  Stores calculated
// values for the SPIR-V transformation.
void GenerateTransformFeedbackExtensionOutputs(const gl::ProgramState &programState,
                                               const gl::VaryingPacking &varyingPacking,
                                               std::string *xfbShaderSource,
                                               uint32_t *locationsUsedForXfbExtensionOut)
{
    const std::vector<gl::TransformFeedbackVarying> &tfVaryings =
        programState.getLinkedTransformFeedbackVaryings();

    std::string xfbDecl;
    std::string xfbOut;

    for (uint32_t varyingIndex = 0; varyingIndex < tfVaryings.size(); ++varyingIndex)
    {
        const gl::TransformFeedbackVarying &tfVarying = tfVaryings[varyingIndex];
        const std::string &tfVaryingName              = tfVarying.mappedName;

        if (tfVaryingName == "gl_Position")
        {
            ASSERT(tfVarying.isBuiltIn());

            // For gl_Position, create a copy of the builtin so xfb qualifiers could be added to
            // that instead.  gl_Position is modified by the shader (to account for Vulkan depth
            // clip space and prerotation), so it cannot be captured directly.
            //
            // The rest of the builtins are captured by decorating gl_PerVertex directly.
            uint32_t xfbVaryingLocation =
                varyingPacking.getMaxSemanticIndex() + ++(*locationsUsedForXfbExtensionOut);

            std::string xfbVaryingName = kXfbBuiltInPrefix + tfVaryingName;

            // Add declaration and initialization code for the new varying.
            std::string varyingType = gl::GetGLSLTypeString(tfVarying.type);
            xfbDecl += "layout(location = " + Str(xfbVaryingLocation) + ") out " + varyingType +
                       " " + xfbVaryingName + ";\n";
            xfbOut += xfbVaryingName + " = " + tfVaryingName + ";\n";
        }
    }

    *xfbShaderSource = SubstituteTransformFeedbackMarkers(*xfbShaderSource, xfbDecl, xfbOut);
}

void AssignAttributeLocations(const gl::ProgramExecutable &programExecutable,
                              gl::ShaderType shaderType,
                              ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    // Assign attribute locations for the vertex shader.
    for (const sh::ShaderVariable &attribute : programExecutable.getProgramInputs())
    {
        ASSERT(attribute.active);

        const uint8_t colCount = static_cast<uint8_t>(gl::VariableColumnCount(attribute.type));
        const uint8_t rowCount = static_cast<uint8_t>(gl::VariableRowCount(attribute.type));
        const bool isMatrix    = colCount > 1 && rowCount > 1;

        const uint8_t componentCount = isMatrix ? rowCount : colCount;
        const uint8_t locationCount  = isMatrix ? colCount : rowCount;

        AddLocationInfo(variableInfoMapOut, shaderType, attribute.mappedName, attribute.location,
                        ShaderInterfaceVariableInfo::kInvalid, componentCount, locationCount);
    }
}

void AssignOutputLocations(const gl::ProgramExecutable &programExecutable,
                           const gl::ShaderType shaderType,
                           ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    // Assign output locations for the fragment shader.
    ASSERT(shaderType == gl::ShaderType::Fragment);
    // TODO(syoussefi): Add support for EXT_blend_func_extended.  http://anglebug.com/3385
    const auto &outputLocations                      = programExecutable.getOutputLocations();
    const auto &outputVariables                      = programExecutable.getOutputVariables();
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

            AddLocationInfo(variableInfoMapOut, shaderType, outputVar.mappedName, location,
                            ShaderInterfaceVariableInfo::kInvalid, 0, 0);
        }
    }

    // When no fragment output is specified by the shader, the translator outputs webgl_FragColor or
    // webgl_FragData.  Add an entry for these.  Even though the translator is already assigning
    // location 0 to these entries, adding an entry for them here allows us to ASSERT that every
    // shader interface variable is processed during the SPIR-V transformation.  This is done when
    // iterating the ids provided by OpEntryPoint.
    AddLocationInfo(variableInfoMapOut, shaderType, "webgl_FragColor", 0, 0, 0, 0);
    AddLocationInfo(variableInfoMapOut, shaderType, "webgl_FragData", 0, 0, 0, 0);
}

void AssignVaryingLocations(const GlslangSourceOptions &options,
                            const gl::VaryingPacking &varyingPacking,
                            const gl::ShaderType shaderType,
                            const gl::ShaderType frontShaderType,
                            GlslangProgramInterfaceInfo *programInterfaceInfo,
                            ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    uint32_t locationsUsedForEmulation = programInterfaceInfo->locationsUsedForXfbExtension;

    // Substitute layout and qualifier strings for the position varying added for line raster
    // emulation.
    if (options.emulateBresenhamLines)
    {
        uint32_t lineRasterEmulationPositionLocation = locationsUsedForEmulation++;

        AddLocationInfo(variableInfoMapOut, shaderType, sh::vk::kLineRasterEmulationPosition,
                        lineRasterEmulationPositionLocation, ShaderInterfaceVariableInfo::kInvalid,
                        0, 0);
    }

    // Assign varying locations.
    for (const gl::PackedVaryingRegister &varyingReg : varyingPacking.getRegisterList())
    {
        if (!IsFirstRegisterOfVarying(varyingReg, false))
        {
            continue;
        }

        const gl::PackedVarying &varying = *varyingReg.packedVarying;

        uint32_t location  = varyingReg.registerRow + locationsUsedForEmulation;
        uint32_t component = ShaderInterfaceVariableInfo::kInvalid;
        if (varyingReg.registerColumn > 0)
        {
            ASSERT(!varying.varying().isStruct());
            ASSERT(!gl::IsMatrixType(varying.varying().type));
            component = varyingReg.registerColumn;
        }

        // In the following:
        //
        //     struct S { vec4 field; };
        //     out S varStruct;
        //
        // "_uvarStruct" is found through |parentStructMappedName|, with |varying->mappedName|
        // being "_ufield".  In such a case, use |parentStructMappedName|.
        if (varying.frontVarying.varying && (varying.frontVarying.stage == shaderType))
        {
            AddVaryingLocationInfo(variableInfoMapOut, varying.frontVarying,
                                   varying.isStructField(), location, component);
        }

        if (varying.backVarying.varying && (varying.backVarying.stage == shaderType))
        {
            AddVaryingLocationInfo(variableInfoMapOut, varying.backVarying, varying.isStructField(),
                                   location, component);
        }
    }

    // Add an entry for inactive varyings.
    const gl::ShaderMap<std::vector<std::string>> &inactiveVaryingMappedNames =
        varyingPacking.getInactiveVaryingMappedNames();
    for (const std::string &varyingName : inactiveVaryingMappedNames[shaderType])
    {
        ASSERT(!gl::IsBuiltInName(varyingName));

        // If name is already in the map, it will automatically have marked all other stages
        // inactive.
        if (variableInfoMapOut->contains(shaderType, varyingName))
        {
            continue;
        }

        // Otherwise, add an entry for it with all locations inactive.
        ShaderInterfaceVariableInfo &info = variableInfoMapOut->addOrGet(shaderType, varyingName);
        ASSERT(info.location == ShaderInterfaceVariableInfo::kInvalid);
    }

    // Add an entry for active builtins varyings.  This will allow inactive builtins, such as
    // gl_PointSize, gl_ClipDistance etc to be removed.
    const gl::ShaderMap<std::vector<std::string>> &activeOutputBuiltIns =
        varyingPacking.getActiveOutputBuiltInNames();
    for (const std::string &builtInName : activeOutputBuiltIns[shaderType])
    {
        ASSERT(gl::IsBuiltInName(builtInName));

        ShaderInterfaceVariableInfo &info = variableInfoMapOut->addOrGet(shaderType, builtInName);
        info.activeStages.set(shaderType);
        info.varyingIsOutput = true;
    }

    // If an output builtin is active in the previous stage, assume it's active in the input of the
    // current stage as well.
    if (frontShaderType != gl::ShaderType::InvalidEnum)
    {
        for (const std::string &builtInName : activeOutputBuiltIns[frontShaderType])
        {
            ASSERT(gl::IsBuiltInName(builtInName));

            ShaderInterfaceVariableInfo &info =
                variableInfoMapOut->addOrGet(shaderType, builtInName);
            info.activeStages.set(shaderType);
            info.varyingIsInput = true;
        }
    }

    // Add an entry for gl_PerVertex, for use with transform feedback capture of built-ins.
    ShaderInterfaceVariableInfo &info = variableInfoMapOut->addOrGet(shaderType, "gl_PerVertex");
    info.activeStages.set(shaderType);
}

// Calculates XFB layout qualifier arguments for each tranform feedback varying.  Stores calculated
// values for the SPIR-V transformation.
void AssignTransformFeedbackExtensionQualifiers(const gl::ProgramExecutable &programExecutable,
                                                const gl::VaryingPacking &varyingPacking,
                                                uint32_t locationsUsedForXfbExtension,
                                                const gl::ShaderType shaderType,
                                                ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    const std::vector<gl::TransformFeedbackVarying> &tfVaryings =
        programExecutable.getLinkedTransformFeedbackVaryings();
    const std::vector<GLsizei> &varyingStrides = programExecutable.getTransformFeedbackStrides();
    const bool isInterleaved =
        programExecutable.getTransformFeedbackBufferMode() == GL_INTERLEAVED_ATTRIBS;

    std::string xfbDecl;
    std::string xfbOut;
    uint32_t currentOffset          = 0;
    uint32_t currentStride          = 0;
    uint32_t bufferIndex            = 0;
    uint32_t currentBuiltinLocation = 0;

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

        const gl::TransformFeedbackVarying &tfVarying = tfVaryings[varyingIndex];

        if (tfVarying.isBuiltIn())
        {
            if (tfVarying.name == "gl_Position")
            {
                uint32_t xfbVaryingLocation = currentBuiltinLocation++;
                std::string xfbVaryingName  = kXfbBuiltInPrefix + tfVarying.mappedName;

                ASSERT(xfbVaryingLocation < locationsUsedForXfbExtension);

                AddLocationInfo(variableInfoMapOut, shaderType, xfbVaryingName, xfbVaryingLocation,
                                ShaderInterfaceVariableInfo::kInvalid, 0, 0);
                SetXfbInfo(variableInfoMapOut, shaderType, xfbVaryingName, -1, bufferIndex,
                           currentOffset, currentStride);
            }
            else
            {
                // gl_PerVertex is always defined as:
                //
                //    Field 0: gl_Position
                //    Field 1: gl_PointSize
                //    Field 2: gl_ClipDistance
                //    Field 3: gl_CullDistance
                //
                // All fields except gl_Position can be captured directly by decorating gl_PerVertex
                // fields.
                int fieldIndex                                                              = -1;
                constexpr int kPerVertexMemberCount                                         = 4;
                constexpr std::array<const char *, kPerVertexMemberCount> kPerVertexMembers = {
                    "gl_Position",
                    "gl_PointSize",
                    "gl_ClipDistance",
                    "gl_CullDistance",
                };
                for (int index = 1; index < kPerVertexMemberCount; ++index)
                {
                    if (tfVarying.name == kPerVertexMembers[index])
                    {
                        fieldIndex = index;
                        break;
                    }
                }
                ASSERT(fieldIndex != -1);

                SetXfbInfo(variableInfoMapOut, shaderType, "gl_PerVertex", fieldIndex, bufferIndex,
                           currentOffset, currentStride);
            }
        }
        else if (!tfVarying.isArray() || tfVarying.arrayIndex == GL_INVALID_INDEX)
        {
            // Note: capturing individual array elements using the Vulkan transform feedback
            // extension is not supported, and is unlikely to be ever supported (on the contrary, it
            // may be removed from the GLES spec).  http://anglebug.com/4140
            // ANGLE should support capturing the whole array.

            // Find the varying with this name.  If a struct is captured, we would be iterating over
            // its fields, and the name of the varying is found through parentStructMappedName.
            // This should only be done for the first field of the struct.  For I/O blocks on the
            // other hand, we need to decorate the exact member that is captured (as whole-block
            // capture is not supported).
            const gl::PackedVarying *originalVarying = nullptr;
            for (const gl::PackedVaryingRegister &varyingReg : varyingPacking.getRegisterList())
            {
                if (!IsFirstRegisterOfVarying(varyingReg, tfVarying.isShaderIOBlock))
                {
                    continue;
                }

                const gl::PackedVarying *varying = varyingReg.packedVarying;

                if (tfVarying.isShaderIOBlock)
                {
                    if (varying->frontVarying.parentStructName == tfVarying.structName)
                    {
                        size_t pos            = tfVarying.name.find_first_of(".");
                        std::string fieldName = pos == std::string::npos
                                                    ? tfVarying.name
                                                    : tfVarying.name.substr(pos + 1);

                        if (fieldName == varying->frontVarying.varying->name.c_str())
                        {
                            originalVarying = varying;
                            break;
                        }
                    }
                }
                else if (varying->frontVarying.varying->name == tfVarying.name)
                {
                    originalVarying = varying;
                    break;
                }
            }

            if (originalVarying)
            {
                const std::string &mappedName =
                    originalVarying->isStructField()
                        ? originalVarying->frontVarying.parentStructMappedName
                        : originalVarying->frontVarying.varying->mappedName;

                const int fieldIndex = tfVarying.isShaderIOBlock ? originalVarying->fieldIndex : -1;

                // Set xfb info for this varying.  AssignVaryingLocations should have already added
                // location information for these varyings.
                SetXfbInfo(variableInfoMapOut, shaderType, mappedName, fieldIndex, bufferIndex,
                           currentOffset, currentStride);
            }
        }
    }
}

void AssignUniformBindings(const GlslangSourceOptions &options,
                           const gl::ProgramExecutable &programExecutable,
                           const gl::ShaderType shaderType,
                           GlslangProgramInterfaceInfo *programInterfaceInfo,
                           ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    if (programExecutable.hasLinkedShaderStage(shaderType))
    {
        AddResourceInfo(variableInfoMapOut, shaderType, kDefaultUniformNames[shaderType],
                        programInterfaceInfo->uniformsAndXfbDescriptorSetIndex,
                        programInterfaceInfo->currentUniformBindingIndex);
        ++programInterfaceInfo->currentUniformBindingIndex;

        // Assign binding to the driver uniforms block
        AddResourceInfoToAllStages(variableInfoMapOut, shaderType, sh::vk::kDriverUniformsBlockName,
                                   programInterfaceInfo->driverUniformsDescriptorSetIndex, 0);
    }
}

// TODO: http://anglebug.com/4512: Need to combine descriptor set bindings across
// shader stages.
void AssignInterfaceBlockBindings(const GlslangSourceOptions &options,
                                  const gl::ProgramExecutable &programExecutable,
                                  const std::vector<gl::InterfaceBlock> &blocks,
                                  const gl::ShaderType shaderType,
                                  GlslangProgramInterfaceInfo *programInterfaceInfo,
                                  ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    for (const gl::InterfaceBlock &block : blocks)
    {
        if (!block.isArray || block.arrayElement == 0)
        {
            // TODO: http://anglebug.com/4523: All blocks should be active
            if (programExecutable.hasLinkedShaderStage(shaderType) && block.isActive(shaderType))
            {
                AddResourceInfo(variableInfoMapOut, shaderType, block.mappedName,
                                programInterfaceInfo->shaderResourceDescriptorSetIndex,
                                programInterfaceInfo->currentShaderResourceBindingIndex);
                ++programInterfaceInfo->currentShaderResourceBindingIndex;
            }
        }
    }
}

// TODO: http://anglebug.com/4512: Need to combine descriptor set bindings across
// shader stages.
void AssignAtomicCounterBufferBindings(const GlslangSourceOptions &options,
                                       const gl::ProgramExecutable &programExecutable,
                                       const std::vector<gl::AtomicCounterBuffer> &buffers,
                                       const gl::ShaderType shaderType,
                                       GlslangProgramInterfaceInfo *programInterfaceInfo,
                                       ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    if (buffers.size() == 0)
    {
        return;
    }

    if (programExecutable.hasLinkedShaderStage(shaderType))
    {
        AddResourceInfo(variableInfoMapOut, shaderType, sh::vk::kAtomicCountersBlockName,
                        programInterfaceInfo->shaderResourceDescriptorSetIndex,
                        programInterfaceInfo->currentShaderResourceBindingIndex);
        ++programInterfaceInfo->currentShaderResourceBindingIndex;
    }
}

// TODO: http://anglebug.com/4512: Need to combine descriptor set bindings across
// shader stages.
void AssignImageBindings(const GlslangSourceOptions &options,
                         const gl::ProgramExecutable &programExecutable,
                         const std::vector<gl::LinkedUniform> &uniforms,
                         const gl::RangeUI &imageUniformRange,
                         const gl::ShaderType shaderType,
                         GlslangProgramInterfaceInfo *programInterfaceInfo,
                         ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    for (unsigned int uniformIndex : imageUniformRange)
    {
        const gl::LinkedUniform &imageUniform = uniforms[uniformIndex];

        std::string name = imageUniform.mappedName;
        if (GetImageNameWithoutIndices(&name))
        {
            if (programExecutable.hasLinkedShaderStage(shaderType))
            {
                AddResourceInfo(variableInfoMapOut, shaderType, name,
                                programInterfaceInfo->shaderResourceDescriptorSetIndex,
                                programInterfaceInfo->currentShaderResourceBindingIndex);
                ++programInterfaceInfo->currentShaderResourceBindingIndex;
            }
        }
    }
}

void AssignNonTextureBindings(const GlslangSourceOptions &options,
                              const gl::ProgramExecutable &programExecutable,
                              const gl::ShaderType shaderType,
                              GlslangProgramInterfaceInfo *programInterfaceInfo,
                              ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    const std::vector<gl::InterfaceBlock> &uniformBlocks = programExecutable.getUniformBlocks();
    AssignInterfaceBlockBindings(options, programExecutable, uniformBlocks, shaderType,
                                 programInterfaceInfo, variableInfoMapOut);

    const std::vector<gl::InterfaceBlock> &storageBlocks =
        programExecutable.getShaderStorageBlocks();
    AssignInterfaceBlockBindings(options, programExecutable, storageBlocks, shaderType,
                                 programInterfaceInfo, variableInfoMapOut);

    const std::vector<gl::AtomicCounterBuffer> &atomicCounterBuffers =
        programExecutable.getAtomicCounterBuffers();
    AssignAtomicCounterBufferBindings(options, programExecutable, atomicCounterBuffers, shaderType,
                                      programInterfaceInfo, variableInfoMapOut);

    const std::vector<gl::LinkedUniform> &uniforms = programExecutable.getUniforms();
    const gl::RangeUI &imageUniformRange           = programExecutable.getImageUniformRange();
    AssignImageBindings(options, programExecutable, uniforms, imageUniformRange, shaderType,
                        programInterfaceInfo, variableInfoMapOut);
}

// TODO: http://anglebug.com/4512: Need to combine descriptor set bindings across
// shader stages.
void AssignTextureBindings(const GlslangSourceOptions &options,
                           const gl::ProgramExecutable &programExecutable,
                           const gl::ShaderType shaderType,
                           GlslangProgramInterfaceInfo *programInterfaceInfo,
                           ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    // Assign textures to a descriptor set and binding.
    const std::vector<gl::LinkedUniform> &uniforms = programExecutable.getUniforms();

    for (unsigned int uniformIndex : programExecutable.getSamplerUniformRange())
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

            // TODO: http://anglebug.com/4523: All uniforms should be active
            if (programExecutable.hasLinkedShaderStage(shaderType) &&
                samplerUniform.isActive(shaderType))
            {
                AddResourceInfo(variableInfoMapOut, shaderType, samplerName,
                                programInterfaceInfo->textureDescriptorSetIndex,
                                programInterfaceInfo->currentTextureBindingIndex);
                ++programInterfaceInfo->currentTextureBindingIndex;
            }
        }
    }
}

constexpr gl::ShaderMap<EShLanguage> kShLanguageMap = {
    {gl::ShaderType::Vertex, EShLangVertex},
    {gl::ShaderType::Geometry, EShLangGeometry},
    {gl::ShaderType::Fragment, EShLangFragment},
    {gl::ShaderType::Compute, EShLangCompute},
};

angle::Result CompileShader(const GlslangErrorCallback &callback,
                            const TBuiltInResource &builtInResources,
                            gl::ShaderType shaderType,
                            const std::string &shaderSource,
                            glslang::TShader *shader,
                            glslang::TProgram *program)
{
    // Enable SPIR-V and Vulkan rules when parsing GLSL
    constexpr EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    ANGLE_TRACE_EVENT0("gpu.angle", "Glslang CompileShader TShader::parse");

    const char *shaderString = shaderSource.c_str();
    int shaderLength         = static_cast<int>(shaderSource.size());

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

    program->addShader(shader);

    return angle::Result::Continue;
}

angle::Result LinkProgram(const GlslangErrorCallback &callback, glslang::TProgram *program)
{
    // Enable SPIR-V and Vulkan rules
    constexpr EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    bool linkResult = program->link(messages);
    if (!linkResult)
    {
        ERR() << "Internal error linking Vulkan shaders:\n" << program->getInfoLog() << "\n";
        ANGLE_GLSLANG_CHECK(callback, false, GlslangError::InvalidShader);
    }
    return angle::Result::Continue;
}

#if defined(ANGLE_ENABLE_ASSERTS)
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
        spirvTools.Disassemble(spirvBlob, &readableSpirv, 0);
        WARN() << "Invalid SPIR-V:\n" << readableSpirv;
    }

    return result;
}
#else   // ANGLE_ENABLE_ASSERTS
bool ValidateSpirv(const std::vector<uint32_t> &spirvBlob)
{
    // Placeholder implementation since this is only used inside an ASSERT().
    // Return false to indicate an error in case this is ever accidentally used somewhere else.
    return false;
}
#endif  // ANGLE_ENABLE_ASSERTS

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

void SetSpirvInstructionLength(uint32_t *instruction, size_t length)
{
    ASSERT(length < 0xFFFFu);

    constexpr uint32_t kLengthMask = 0xFFFF0000u;
    instruction[0] &= ~kLengthMask;
    instruction[0] |= length << 16;
}

void SetSpirvInstructionOp(uint32_t *instruction, uint32_t op)
{
    constexpr uint32_t kOpMask = 0xFFFFu;
    instruction[0] &= ~kOpMask;
    instruction[0] |= op;
}

// Base class for SPIR-V transformations.
class SpirvTransformerBase : angle::NonCopyable
{
  public:
    SpirvTransformerBase(const std::vector<uint32_t> &spirvBlobIn,
                         const ShaderInterfaceVariableInfoMap &variableInfoMap,
                         SpirvBlob *spirvBlobOut)
        : mSpirvBlobIn(spirvBlobIn), mVariableInfoMap(variableInfoMap), mSpirvBlobOut(spirvBlobOut)
    {
        gl::ShaderBitSet allStages;
        allStages.set();
        mBuiltinVariableInfo.activeStages = allStages;
    }

    std::vector<const ShaderInterfaceVariableInfo *> &getVariableInfoByIdMap()
    {
        return mVariableInfoById;
    }

  protected:
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

    // Common utilities
    void onTransformBegin();
    const uint32_t *getCurrentInstruction(uint32_t *opCodeOut, uint32_t *wordCountOut) const;
    size_t copyInstruction(const uint32_t *instruction, size_t wordCount);
    uint32_t getNewId();

    // Instruction generators:
    void writeCopyObject(uint32_t id, uint32_t typeId, uint32_t operandId);
    void writeCompositeConstruct(uint32_t id,
                                 uint32_t typeId,
                                 const angle::FixedVector<uint32_t, 4> &constituents);
    void writeCompositeExtract(uint32_t id, uint32_t typeId, uint32_t compositeId, uint32_t field);
    void writeLoad(uint32_t id, uint32_t typeId, uint32_t tempVarId);
    void writeMemberDecorate(uint32_t typeId, uint32_t member, uint32_t decoration, uint32_t value);
    void writeStore(uint32_t pointerId, uint32_t objectId);
    void writeTypePointer(uint32_t id, uint32_t storageClass, uint32_t typeId);
    void writeVariable(uint32_t id, uint32_t typeId, uint32_t storageClass);
    void writeVectorShuffle(uint32_t id,
                            uint32_t typeId,
                            uint32_t vec1Id,
                            uint32_t vec2Id,
                            const angle::FixedVector<uint32_t, 4> &fields);

    // SPIR-V to transform:
    const std::vector<uint32_t> &mSpirvBlobIn;

    // Input shader variable info map:
    const ShaderInterfaceVariableInfoMap &mVariableInfoMap;

    // Transformed SPIR-V:
    SpirvBlob *mSpirvBlobOut;

    // Traversal state:
    size_t mCurrentWord       = 0;
    bool mIsInFunctionSection = false;

    // Transformation state:

    // Shader variable info per id, if id is a shader variable.
    std::vector<const ShaderInterfaceVariableInfo *> mVariableInfoById;
    ShaderInterfaceVariableInfo mBuiltinVariableInfo;
};

void SpirvTransformerBase::onTransformBegin()
{
    // Glslang succeeded in outputting SPIR-V, so we assume it's valid.
    ASSERT(mSpirvBlobIn.size() >= kHeaderIndexInstructions);
    // Since SPIR-V comes from a local call to glslang, it necessarily has the same endianness as
    // the running architecture, so no byte-swapping is necessary.
    ASSERT(mSpirvBlobIn[kHeaderIndexMagic] == spv::MagicNumber);

    // Make sure the transformer is not reused to avoid having to reinitialize it here.
    ASSERT(mCurrentWord == 0);
    ASSERT(mIsInFunctionSection == false);

    // Make sure the SpirvBlob is not reused.
    ASSERT(mSpirvBlobOut->empty());

    // Copy the header to SpirvBlob, we need that to be defined for SpirvTransformerBase::getNewId
    // to work.
    mSpirvBlobOut->assign(mSpirvBlobIn.begin(), mSpirvBlobIn.begin() + kHeaderIndexInstructions);

    mCurrentWord = kHeaderIndexInstructions;
}

const uint32_t *SpirvTransformerBase::getCurrentInstruction(uint32_t *opCodeOut,
                                                            uint32_t *wordCountOut) const
{
    ASSERT(mCurrentWord < mSpirvBlobIn.size());
    const uint32_t *instruction = &mSpirvBlobIn[mCurrentWord];

    *wordCountOut = GetSpirvInstructionLength(instruction);
    *opCodeOut    = GetSpirvInstructionOp(instruction);

    // Since glslang succeeded in producing SPIR-V, we assume it to be valid.
    ASSERT(mCurrentWord + *wordCountOut <= mSpirvBlobIn.size());

    return instruction;
}

size_t SpirvTransformerBase::copyInstruction(const uint32_t *instruction, size_t wordCount)
{
    size_t instructionOffset = mSpirvBlobOut->size();
    mSpirvBlobOut->insert(mSpirvBlobOut->end(), instruction, instruction + wordCount);
    return instructionOffset;
}

uint32_t SpirvTransformerBase::getNewId()
{
    return (*mSpirvBlobOut)[kHeaderIndexIndexBound]++;
}

void SpirvTransformerBase::writeCopyObject(uint32_t id, uint32_t typeId, uint32_t operandId)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpCopyObject
    constexpr size_t kTypeIdIndex                 = 1;
    constexpr size_t kIdIndex                     = 2;
    constexpr size_t kOperandIdIndex              = 3;
    constexpr size_t kCopyObjectInstructionLength = 4;

    std::array<uint32_t, kCopyObjectInstructionLength> copyObject = {};

    // Fill the fields.
    SetSpirvInstructionOp(copyObject.data(), spv::OpCopyObject);
    SetSpirvInstructionLength(copyObject.data(), kCopyObjectInstructionLength);
    copyObject[kTypeIdIndex]    = typeId;
    copyObject[kIdIndex]        = id;
    copyObject[kOperandIdIndex] = operandId;

    copyInstruction(copyObject.data(), kCopyObjectInstructionLength);
}

void SpirvTransformerBase::writeCompositeConstruct(
    uint32_t id,
    uint32_t typeId,
    const angle::FixedVector<uint32_t, 4> &constituents)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpCompositeConstruct
    constexpr size_t kTypeIdIndex                             = 1;
    constexpr size_t kIdIndex                                 = 2;
    constexpr size_t kConstituentsIndexStart                  = 3;
    constexpr size_t kConstituentsMaxCount                    = 4;
    constexpr size_t kCompositeConstructInstructionBaseLength = 3;

    ASSERT(kConstituentsMaxCount == constituents.max_size());
    std::array<uint32_t, kCompositeConstructInstructionBaseLength + kConstituentsMaxCount>
        compositeConstruct = {};

    // Fill the fields.
    SetSpirvInstructionOp(compositeConstruct.data(), spv::OpCompositeConstruct);
    SetSpirvInstructionLength(compositeConstruct.data(),
                              kCompositeConstructInstructionBaseLength + constituents.size());
    compositeConstruct[kTypeIdIndex] = typeId;
    compositeConstruct[kIdIndex]     = id;

    for (size_t constituentIndex = 0; constituentIndex < constituents.size(); ++constituentIndex)
    {
        compositeConstruct[kConstituentsIndexStart + constituentIndex] =
            constituents[constituentIndex];
    }

    copyInstruction(compositeConstruct.data(),
                    kCompositeConstructInstructionBaseLength + constituents.size());
}

void SpirvTransformerBase::writeCompositeExtract(uint32_t id,
                                                 uint32_t typeId,
                                                 uint32_t compositeId,
                                                 uint32_t field)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpCompositeExtract
    constexpr size_t kTypeIdIndex                       = 1;
    constexpr size_t kIdIndex                           = 2;
    constexpr size_t kCompositeIdIndex                  = 3;
    constexpr size_t kFieldIndex                        = 4;
    constexpr size_t kCompositeExtractInstructionLength = 5;

    std::array<uint32_t, kCompositeExtractInstructionLength> compositeExtract = {};

    // Fill the fields.
    SetSpirvInstructionOp(compositeExtract.data(), spv::OpCompositeExtract);
    SetSpirvInstructionLength(compositeExtract.data(), kCompositeExtractInstructionLength);
    compositeExtract[kTypeIdIndex]      = typeId;
    compositeExtract[kIdIndex]          = id;
    compositeExtract[kCompositeIdIndex] = compositeId;
    compositeExtract[kFieldIndex]       = field;

    copyInstruction(compositeExtract.data(), kCompositeExtractInstructionLength);
}

void SpirvTransformerBase::writeLoad(uint32_t pointerId, uint32_t typeId, uint32_t resultId)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpLoad
    constexpr size_t kResultTypeIndex         = 1;
    constexpr size_t kResultIdIndex           = 2;
    constexpr size_t kPointerIdIndex          = 3;
    constexpr size_t kOpLoadInstructionLength = 4;

    std::array<uint32_t, kOpLoadInstructionLength> load = {};

    SetSpirvInstructionOp(load.data(), spv::OpLoad);
    SetSpirvInstructionLength(load.data(), kOpLoadInstructionLength);
    load[kResultTypeIndex] = typeId;
    load[kResultIdIndex]   = resultId;
    load[kPointerIdIndex]  = pointerId;
    copyInstruction(load.data(), kOpLoadInstructionLength);
}

void SpirvTransformerBase::writeMemberDecorate(uint32_t typeId,
                                               uint32_t member,
                                               uint32_t decoration,
                                               uint32_t value)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpMemberDecorate
    constexpr size_t kTypeIdIndex                       = 1;
    constexpr size_t kMemberIndex                       = 2;
    constexpr size_t kDecorationIndex                   = 3;
    constexpr size_t kValueIndex                        = 4;
    constexpr size_t kOpMemberDecorateInstructionLength = 5;

    std::array<uint32_t, kOpMemberDecorateInstructionLength> memberDecorate = {};

    SetSpirvInstructionOp(memberDecorate.data(), spv::OpMemberDecorate);
    SetSpirvInstructionLength(memberDecorate.data(), kOpMemberDecorateInstructionLength);
    memberDecorate[kTypeIdIndex]     = typeId;
    memberDecorate[kMemberIndex]     = member;
    memberDecorate[kDecorationIndex] = decoration;
    memberDecorate[kValueIndex]      = value;
    copyInstruction(memberDecorate.data(), kOpMemberDecorateInstructionLength);
}

void SpirvTransformerBase::writeStore(uint32_t pointerId, uint32_t objectId)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpStore
    constexpr size_t kPointerIdIndex         = 1;
    constexpr size_t kObjectIdIndex          = 2;
    constexpr size_t kStoreInstructionLength = 3;

    std::array<uint32_t, kStoreInstructionLength> store = {};

    // Fill the fields.
    SetSpirvInstructionOp(store.data(), spv::OpStore);
    SetSpirvInstructionLength(store.data(), kStoreInstructionLength);
    store[kPointerIdIndex] = pointerId;
    store[kObjectIdIndex]  = objectId;

    copyInstruction(store.data(), kStoreInstructionLength);
}

void SpirvTransformerBase::writeTypePointer(uint32_t id, uint32_t storageClass, uint32_t typeId)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpTypePointer
    constexpr size_t kIdIndex                      = 1;
    constexpr size_t kStorageClassIndex            = 2;
    constexpr size_t kTypeIdIndex                  = 3;
    constexpr size_t kTypePointerInstructionLength = 4;

    std::array<uint32_t, kTypePointerInstructionLength> typePointer = {};

    // Fill the fields.
    SetSpirvInstructionOp(typePointer.data(), spv::OpTypePointer);
    SetSpirvInstructionLength(typePointer.data(), kTypePointerInstructionLength);
    typePointer[kIdIndex]           = id;
    typePointer[kStorageClassIndex] = storageClass;
    typePointer[kTypeIdIndex]       = typeId;

    copyInstruction(typePointer.data(), kTypePointerInstructionLength);
}

void SpirvTransformerBase::writeVariable(uint32_t id, uint32_t typeId, uint32_t storageClass)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpVariable
    constexpr size_t kTypeIdIndex               = 1;
    constexpr size_t kIdIndex                   = 2;
    constexpr size_t kStorageClassIndex         = 3;
    constexpr size_t kVariableInstructionLength = 4;

    std::array<uint32_t, kVariableInstructionLength> variable = {};

    // Fill the fields.
    SetSpirvInstructionOp(variable.data(), spv::OpVariable);
    SetSpirvInstructionLength(variable.data(), kVariableInstructionLength);
    variable[kTypeIdIndex]       = typeId;
    variable[kIdIndex]           = id;
    variable[kStorageClassIndex] = storageClass;

    copyInstruction(variable.data(), kVariableInstructionLength);
}

void SpirvTransformerBase::writeVectorShuffle(uint32_t id,
                                              uint32_t typeId,
                                              uint32_t vec1Id,
                                              uint32_t vec2Id,
                                              const angle::FixedVector<uint32_t, 4> &fields)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpVectorShuffle
    constexpr size_t kTypeIdIndex                        = 1;
    constexpr size_t kIdIndex                            = 2;
    constexpr size_t kVec1IdIndex                        = 3;
    constexpr size_t kVec2IdIndex                        = 4;
    constexpr size_t kFieldsIndexStart                   = 5;
    constexpr size_t kFieldsMaxCount                     = 4;
    constexpr size_t kVectorShuffleInstructionBaseLength = 5;

    ASSERT(kFieldsMaxCount == fields.max_size());
    std::array<uint32_t, kVectorShuffleInstructionBaseLength + kFieldsMaxCount> vectorShuffle = {};

    // Fill the fields.
    SetSpirvInstructionOp(vectorShuffle.data(), spv::OpVectorShuffle);
    SetSpirvInstructionLength(vectorShuffle.data(),
                              kVectorShuffleInstructionBaseLength + fields.size());
    vectorShuffle[kTypeIdIndex] = typeId;
    vectorShuffle[kIdIndex]     = id;
    vectorShuffle[kVec1IdIndex] = vec1Id;
    vectorShuffle[kVec2IdIndex] = vec2Id;

    for (size_t fieldIndex = 0; fieldIndex < fields.size(); ++fieldIndex)
    {
        vectorShuffle[kFieldsIndexStart + fieldIndex] = fields[fieldIndex];
    }

    copyInstruction(vectorShuffle.data(), kVectorShuffleInstructionBaseLength + fields.size());
}

// A SPIR-V transformer.  It walks the instructions and modifies them as necessary, for example to
// assign bindings or locations.
class SpirvTransformer final : public SpirvTransformerBase
{
  public:
    SpirvTransformer(const std::vector<uint32_t> &spirvBlobIn,
                     GlslangSpirvOptions options,
                     const ShaderInterfaceVariableInfoMap &variableInfoMap,
                     SpirvBlob *spirvBlobOut)
        : SpirvTransformerBase(spirvBlobIn, variableInfoMap, spirvBlobOut),
          mOptions(options),
          mHasTransformFeedbackOutput(false),
          mOutputPerVertex{},
          mInputPerVertex{}
    {}

    bool transform();

  private:
    // A prepass to resolve interesting ids:
    void resolveVariableIds();

    // Transform instructions:
    void transformInstruction();

    // Instructions that are purely informational:
    void visitDecorate(const uint32_t *instruction);
    void visitName(const uint32_t *instruction);
    void visitMemberName(const uint32_t *instruction);
    void visitTypeHelper(const uint32_t *instruction, size_t idIndex, size_t typeIdIndex);
    void visitTypeArray(const uint32_t *instruction);
    void visitTypePointer(const uint32_t *instruction);
    void visitVariable(const uint32_t *instruction);

    // Instructions that potentially need transformation.  They return true if the instruction is
    // transformed.  If false is returned, the instruction should be copied as-is.
    bool transformAccessChain(const uint32_t *instruction, size_t wordCount);
    bool transformCapability(const uint32_t *instruction, size_t wordCount);
    bool transformDebugInfo(const uint32_t *instruction, size_t wordCount);
    bool transformEmitVertex(const uint32_t *instruction, size_t wordCount);
    bool transformEntryPoint(const uint32_t *instruction, size_t wordCount);
    bool transformDecorate(const uint32_t *instruction, size_t wordCount);
    bool transformMemberDecorate(const uint32_t *instruction, size_t wordCount);
    bool transformTypePointer(const uint32_t *instruction, size_t wordCount);
    bool transformTypeStruct(const uint32_t *instruction, size_t wordCount);
    bool transformReturn(const uint32_t *instruction, size_t wordCount);
    bool transformVariable(const uint32_t *instruction, size_t wordCount);
    bool transformExecutionMode(const uint32_t *instruction, size_t wordCount);

    // Helpers:
    void writeInputPreamble();
    void writeOutputPrologue();

    // Special flags:
    GlslangSpirvOptions mOptions;
    bool mHasTransformFeedbackOutput;

    // Traversal state:
    bool mInsertFunctionVariables = false;
    uint32_t mEntryPointId        = 0;
    uint32_t mOpFunctionId        = 0;

    // Transformation state:

    // Names associated with ids through OpName.  The same name may be assigned to multiple ids, but
    // not all names are interesting (for example function arguments).  When the variable
    // declaration is met (OpVariable), the variable info is matched with the corresponding id's
    // name based on the Storage Class.
    std::vector<const char *> mNamesById;

    // Tracks whether a given type is an I/O block.  I/O blocks are identified by their type name
    // instead of variable name, but otherwise look like varyings of struct type (which are
    // identified by their instance name).  To disambiguate them, the `OpDecorate %N Block`
    // instruction is used which decorates I/O block types.
    std::vector<bool> mIsIOBlockById;

    // Each OpTypePointer instruction that defines a type with the Output storage class is
    // duplicated with a similar instruction but which defines a type with the Private storage
    // class.  If inactive varyings are encountered, its type is changed to the Private one.  The
    // following vector maps the Output type id to the corresponding Private one.
    struct TransformedIDs
    {
        uint32_t privateID;
        uint32_t typeID;
    };
    std::vector<TransformedIDs> mTypePointerTransformedId;
    std::vector<uint32_t> mFixedVaryingId;
    std::vector<uint32_t> mFixedVaryingTypeId;

    // gl_PerVertex is unique in that it's the only builtin of struct type.  This struct is pruned
    // by removing trailing inactive members.  We therefore need to keep track of what's its type id
    // as well as which is the last active member.  Note that intermediate stages, i.e. geometry and
    // tessellation have two gl_PerVertex declarations, one for input and one for output.
    struct PerVertexData
    {
        uint32_t typeId;
        uint32_t maxActiveMember;
    };
    PerVertexData mOutputPerVertex;
    PerVertexData mInputPerVertex;
};

bool SpirvTransformer::transform()
{
    onTransformBegin();

    // First, find all necessary ids and associate them with the information required to transform
    // their decorations.
    resolveVariableIds();

    while (mCurrentWord < mSpirvBlobIn.size())
    {
        transformInstruction();
    }

    return true;
}

void SpirvTransformer::resolveVariableIds()
{
    const size_t indexBound = mSpirvBlobIn[kHeaderIndexIndexBound];

    // Allocate storage for id-to-name map.  Used to associate ShaderInterfaceVariableInfo with ids
    // based on name, but only when it's determined that the name corresponds to a shader interface
    // variable.
    mNamesById.resize(indexBound, nullptr);

    // Allocate storage for id-to-flag map.  Used to disambiguate I/O blocks instances from varyings
    // of struct type.
    mIsIOBlockById.resize(indexBound, false);

    // Allocate storage for id-to-info map.  If %i is the id of a name in mVariableInfoMap, index i
    // in this vector will hold a pointer to the ShaderInterfaceVariableInfo object associated with
    // that name in mVariableInfoMap.
    mVariableInfoById.resize(indexBound, nullptr);

    // Allocate storage for Output type pointer map.  At index i, this vector holds the identical
    // type as %i except for its storage class turned to Private.
    // Also store a FunctionID and TypeID for when we need to fix a precision mismatch
    mTypePointerTransformedId.resize(indexBound, {0, 0});
    mFixedVaryingId.resize(indexBound, {0});
    mFixedVaryingTypeId.resize(indexBound, {0});

    size_t currentWord = kHeaderIndexInstructions;

    while (currentWord < mSpirvBlobIn.size())
    {
        const uint32_t *instruction = &mSpirvBlobIn[currentWord];

        const uint32_t wordCount = GetSpirvInstructionLength(instruction);
        const uint32_t opCode    = GetSpirvInstructionOp(instruction);

        switch (opCode)
        {
            case spv::OpDecorate:
                visitDecorate(instruction);
                break;
            case spv::OpName:
                visitName(instruction);
                break;
            case spv::OpMemberName:
                visitMemberName(instruction);
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
                // SPIR-V is structured in sections (SPIR-V 1.0 Section 2.4 Logical Layout of a
                // Module). Names appear before decorations, which are followed by type+variables
                // and finally functions.  We are only interested in name and variable declarations
                // (as well as type declarations for the sake of nameless interface blocks).  Early
                // out when the function declaration section is met.
                return;
            default:
                break;
        }

        currentWord += wordCount;
    }
}

void SpirvTransformer::transformInstruction()
{
    uint32_t wordCount;
    uint32_t opCode;
    const uint32_t *instruction = getCurrentInstruction(&opCode, &wordCount);

    if (opCode == spv::OpFunction)
    {
        constexpr size_t kFunctionIdIndex = 2;
        mOpFunctionId                     = instruction[kFunctionIdIndex];

        // SPIR-V is structured in sections.  Function declarations come last.  Only Op*Access*
        // opcodes inside functions need to be inspected.
        mIsInFunctionSection = true;

        // Only write function variables for the EntryPoint function for non-compute shaders
        mInsertFunctionVariables =
            mOpFunctionId == mEntryPointId && mOptions.shaderType != gl::ShaderType::Compute;
    }

    // Only look at interesting instructions.
    bool transformed = false;

    if (mIsInFunctionSection)
    {
        // After we process an OpFunction instruction and any instructions that must come
        // immediately after OpFunction we need to check if there are any precision mismatches that
        // need to be handled. If so, output OpVariable for each variable that needed to change from
        // a StorageClassOutput to a StorageClassFunction.
        if (mInsertFunctionVariables && opCode != spv::OpFunction &&
            opCode != spv::OpFunctionParameter && opCode != spv::OpLabel &&
            opCode != spv::OpVariable)
        {
            writeInputPreamble();
            mInsertFunctionVariables = false;
        }

        // Look at in-function opcodes.
        switch (opCode)
        {
            case spv::OpAccessChain:
            case spv::OpInBoundsAccessChain:
            case spv::OpPtrAccessChain:
            case spv::OpInBoundsPtrAccessChain:
                transformed = transformAccessChain(instruction, wordCount);
                break;

            case spv::OpEmitVertex:
                transformed = transformEmitVertex(instruction, wordCount);
                break;
            case spv::OpReturn:
                transformed = transformReturn(instruction, wordCount);
                break;
            default:
                break;
        }
    }
    else
    {
        // Look at global declaration opcodes.
        switch (opCode)
        {
            case spv::OpSourceContinued:
            case spv::OpSource:
            case spv::OpSourceExtension:
            case spv::OpName:
            case spv::OpMemberName:
            case spv::OpString:
            case spv::OpLine:
            case spv::OpNoLine:
            case spv::OpModuleProcessed:
                transformed = transformDebugInfo(instruction, wordCount);
                break;
            case spv::OpCapability:
                transformed = transformCapability(instruction, wordCount);
                break;
            case spv::OpEntryPoint:
                transformed = transformEntryPoint(instruction, wordCount);
                break;
            case spv::OpDecorate:
                transformed = transformDecorate(instruction, wordCount);
                break;
            case spv::OpMemberDecorate:
                transformed = transformMemberDecorate(instruction, wordCount);
                break;
            case spv::OpTypePointer:
                transformed = transformTypePointer(instruction, wordCount);
                break;
            case spv::OpTypeStruct:
                transformed = transformTypeStruct(instruction, wordCount);
                break;
            case spv::OpVariable:
                transformed = transformVariable(instruction, wordCount);
                break;
            case spv::OpExecutionMode:
                transformed = transformExecutionMode(instruction, wordCount);
                break;
            default:
                break;
        }
    }

    // If the instruction was not transformed, copy it to output as is.
    if (!transformed)
    {
        copyInstruction(instruction, wordCount);
    }

    // Advance to next instruction.
    mCurrentWord += wordCount;
}

// Called by transformInstruction to insert necessary instructions for casting varying
void SpirvTransformer::writeInputPreamble()
{
    for (uint32_t id = 0; id < mVariableInfoById.size(); id++)
    {
        const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];
        if (info && info->useRelaxedPrecision && info->activeStages[mOptions.shaderType] &&
            info->varyingIsInput)
        {
            // This is an input varying, need to cast the mediump value that came from
            // the previous stage into a highp value that the code wants to work with.
            ASSERT(mFixedVaryingTypeId[id] != 0);

            // Build OpLoad instruction to load the mediump value into a temporary
            uint32_t tempVar     = getNewId();
            uint32_t tempVarType = mTypePointerTransformedId[mFixedVaryingTypeId[id]].typeID;
            ASSERT(tempVarType != 0);

            writeLoad(mFixedVaryingId[id], tempVarType, tempVar);

            // Build OpStore instruction to cast the mediump value to highp for use in
            // the function
            writeStore(id, tempVar);
        }
    }
}

// Called by transformInstruction to insert necessary instructions for casting varying
void SpirvTransformer::writeOutputPrologue()
{
    for (uint32_t id = 0; id < mVariableInfoById.size(); id++)
    {
        const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];
        if (info && info->useRelaxedPrecision && info->activeStages[mOptions.shaderType] &&
            info->varyingIsOutput)
        {
            ASSERT(mFixedVaryingTypeId[id] != 0);

            // Build OpLoad instruction to load the highp value into a temporary
            uint32_t tempVar     = getNewId();
            uint32_t tempVarType = mTypePointerTransformedId[mFixedVaryingTypeId[id]].typeID;
            ASSERT(tempVarType != 0);

            writeLoad(id, tempVarType, tempVar);

            // Build OpStore instruction to cast the highp value to mediump for output
            writeStore(mFixedVaryingId[id], tempVar);
        }
    }
}

void SpirvTransformer::visitDecorate(const uint32_t *instruction)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpDecorate
    constexpr size_t kIdIndex         = 1;
    constexpr size_t kDecorationIndex = 2;

    const uint32_t id   = instruction[kIdIndex];
    uint32_t decoration = instruction[kDecorationIndex];

    if (decoration == spv::DecorationBlock)
    {
        mIsIOBlockById[id] = true;

        // For I/O blocks, associate the type with the info, which is used to decorate its members
        // with transform feedback if any.
        const char *name = mNamesById[id];
        ASSERT(name != nullptr);

        const ShaderInterfaceVariableInfo &info = mVariableInfoMap.get(mOptions.shaderType, name);
        mVariableInfoById[id]                   = &info;
    }
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

    // The names and ids are unique
    ASSERT(id < mNamesById.size());
    ASSERT(mNamesById[id] == nullptr);

    mNamesById[id] = name;
}

void SpirvTransformer::visitMemberName(const uint32_t *instruction)
{
    ASSERT(IsLittleEndian());

    // SPIR-V 1.0 Section 3.32 Instructions, OpMemberName
    constexpr size_t kIdIndex     = 1;
    constexpr size_t kMemberIndex = 2;
    constexpr size_t kNameIndex   = 3;

    const uint32_t id     = instruction[kIdIndex];
    const uint32_t member = instruction[kMemberIndex];
    const char *name      = reinterpret_cast<const char *>(&instruction[kNameIndex]);

    // The names and ids are unique
    ASSERT(id < mNamesById.size());
    ASSERT(mNamesById[id] != nullptr);

    if (strcmp(mNamesById[id], "gl_PerVertex") != 0)
    {
        return;
    }

    if (!mVariableInfoMap.contains(mOptions.shaderType, name))
    {
        return;
    }

    const ShaderInterfaceVariableInfo &info = mVariableInfoMap.get(mOptions.shaderType, name);

    // Assume output gl_PerVertex is encountered first.  When the storage class of these types are
    // determined, the variables can be swapped if this assumption was incorrect.
    if (mOutputPerVertex.typeId == 0 || id == mOutputPerVertex.typeId)
    {
        mOutputPerVertex.typeId = id;

        // Keep track of the range of members that are active.
        if (info.varyingIsOutput && member > mOutputPerVertex.maxActiveMember)
        {
            mOutputPerVertex.maxActiveMember = member;
        }
    }
    else if (mInputPerVertex.typeId == 0 || id == mInputPerVertex.typeId)
    {
        mInputPerVertex.typeId = id;

        // Keep track of the range of members that are active.
        if (info.varyingIsInput && member > mInputPerVertex.maxActiveMember)
        {
            mInputPerVertex.maxActiveMember = member;
        }
    }
    else
    {
        UNREACHABLE();
    }
}

void SpirvTransformer::visitTypeHelper(const uint32_t *instruction,
                                       const size_t idIndex,
                                       const size_t typeIdIndex)
{
    const uint32_t id     = instruction[idIndex];
    const uint32_t typeId = instruction[typeIdIndex];

    // Every type id is declared only once.
    ASSERT(id < mNamesById.size());
    ASSERT(mNamesById[id] == nullptr);
    ASSERT(id < mIsIOBlockById.size());
    ASSERT(!mIsIOBlockById[id]);

    // Carry the name forward from the base type.  This is only necessary for interface blocks,
    // as the variable info is associated with the block name instead of the variable name (to
    // support nameless interface blocks).  When the variable declaration is met, either the
    // type name or the variable name is used to associate with info based on the variable's
    // storage class.
    ASSERT(typeId < mNamesById.size());
    mNamesById[id] = mNamesById[typeId];

    // Similarly, carry forward the information regarding whether this type is an I/O block.
    ASSERT(typeId < mIsIOBlockById.size());
    mIsIOBlockById[id] = mIsIOBlockById[typeId];
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
    constexpr size_t kIdIndex           = 1;
    constexpr size_t kStorageClassIndex = 2;
    constexpr size_t kTypeIdIndex       = 3;

    visitTypeHelper(instruction, kIdIndex, kTypeIdIndex);

    const uint32_t typeId       = instruction[kTypeIdIndex];
    const uint32_t storageClass = instruction[kStorageClassIndex];

    // Verify that the ids associated with input and output gl_PerVertex are correct.
    if (typeId == mOutputPerVertex.typeId || typeId == mInputPerVertex.typeId)
    {
        // If assumption about the first gl_PerVertex encountered being Output is wrong, swap the
        // two ids.
        if ((typeId == mOutputPerVertex.typeId && storageClass == spv::StorageClassInput) ||
            (typeId == mInputPerVertex.typeId && storageClass == spv::StorageClassOutput))
        {
            std::swap(mOutputPerVertex.typeId, mInputPerVertex.typeId);
        }
    }
}

void SpirvTransformer::visitVariable(const uint32_t *instruction)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpVariable
    constexpr size_t kTypeIdIndex       = 1;
    constexpr size_t kIdIndex           = 2;
    constexpr size_t kStorageClassIndex = 3;

    // All resources that take set/binding should be transformed.
    const uint32_t typeId       = instruction[kTypeIdIndex];
    const uint32_t id           = instruction[kIdIndex];
    const uint32_t storageClass = instruction[kStorageClassIndex];

    ASSERT(typeId < mNamesById.size());
    ASSERT(id < mNamesById.size());
    ASSERT(typeId < mIsIOBlockById.size());

    // If storage class indicates that this is not a shader interface variable, ignore it.
    const bool isInterfaceBlockVariable =
        storageClass == spv::StorageClassUniform || storageClass == spv::StorageClassStorageBuffer;
    const bool isOpaqueUniform = storageClass == spv::StorageClassUniformConstant;
    const bool isInOut =
        storageClass == spv::StorageClassInput || storageClass == spv::StorageClassOutput;

    if (!isInterfaceBlockVariable && !isOpaqueUniform && !isInOut)
    {
        return;
    }

    // The ids are unique.
    ASSERT(id < mVariableInfoById.size());
    ASSERT(mVariableInfoById[id] == nullptr);

    // For interface block variables, the name that's used to associate info is the block name
    // rather than the variable name.
    const bool isIOBlock = mIsIOBlockById[typeId];
    const char *name     = mNamesById[isInterfaceBlockVariable || isIOBlock ? typeId : id];

    ASSERT(name != nullptr);

    // Handle builtins, which all start with "gl_".  The variable name could be an indication of a
    // builtin variable (such as with gl_FragCoord).  gl_PerVertex is the only builtin whose "type"
    // name starts with gl_.  However, gl_PerVertex has its own entry in the info map for its
    // potential use with transform feedback.
    const bool isNameBuiltin = isInOut && !isIOBlock && gl::IsBuiltInName(name);
    if (isNameBuiltin)
    {
        // Make all builtins point to this no-op info.  Adding this entry allows us to ASSERT that
        // every shader interface variable is processed during the SPIR-V transformation.  This is
        // done when iterating the ids provided by OpEntryPoint.
        mVariableInfoById[id] = &mBuiltinVariableInfo;
        return;
    }

    // Every shader interface variable should have an associated data.
    const ShaderInterfaceVariableInfo &info = mVariableInfoMap.get(mOptions.shaderType, name);

    // Associate the id of this name with its info.
    mVariableInfoById[id] = &info;

    if (info.useRelaxedPrecision && info.activeStages[mOptions.shaderType] &&
        mFixedVaryingId[id] == 0)
    {
        mFixedVaryingId[id]     = getNewId();
        mFixedVaryingTypeId[id] = typeId;
    }

    // Note if the variable is captured by transform feedback.  In that case, the TransformFeedback
    // capability needs to be added.
    if (mOptions.isTransformFeedbackStage &&
        (info.xfb.buffer != ShaderInterfaceVariableInfo::kInvalid || !info.fieldXfb.empty()) &&
        info.activeStages[mOptions.shaderType])
    {
        mHasTransformFeedbackOutput = true;
    }
}

bool SpirvTransformer::transformDecorate(const uint32_t *instruction, size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpDecorate
    constexpr size_t kIdIndex              = 1;
    constexpr size_t kDecorationIndex      = 2;
    constexpr size_t kDecorationValueIndex = 3;

    uint32_t id         = instruction[kIdIndex];
    uint32_t decoration = instruction[kDecorationIndex];

    ASSERT(id < mVariableInfoById.size());
    const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];

    // If variable is not a shader interface variable that needs modification, there's nothing to
    // do.
    if (info == nullptr)
    {
        return false;
    }

    // If it's an inactive varying, remove the decoration altogether.
    if (!info->activeStages[mOptions.shaderType])
    {
        return true;
    }

    // If this is the Block decoration of a shader I/O block, add the transform feedback decorations
    // to its members right away.
    constexpr size_t kXfbDecorationCount                    = 3;
    constexpr uint32_t kXfbDecorations[kXfbDecorationCount] = {
        spv::DecorationXfbBuffer,
        spv::DecorationXfbStride,
        spv::DecorationOffset,
    };

    if (mOptions.isTransformFeedbackStage && decoration == spv::DecorationBlock &&
        !info->fieldXfb.empty())
    {
        for (uint32_t fieldIndex = 0; fieldIndex < info->fieldXfb.size(); ++fieldIndex)
        {
            const ShaderInterfaceVariableXfbInfo &xfb = info->fieldXfb[fieldIndex];

            if (xfb.buffer == ShaderInterfaceVariableXfbInfo::kInvalid)
            {
                continue;
            }

            ASSERT(xfb.stride != ShaderInterfaceVariableXfbInfo::kInvalid);
            ASSERT(xfb.offset != ShaderInterfaceVariableXfbInfo::kInvalid);

            const uint32_t xfbDecorationValues[kXfbDecorationCount] = {
                xfb.buffer,
                xfb.stride,
                xfb.offset,
            };

            // Generate the following three instructions:
            //
            //     OpMemberDecorate %id fieldIndex XfbBuffer xfb.buffer
            //     OpMemberDecorate %id fieldIndex XfbStride xfb.stride
            //     OpMemberDecorate %id fieldIndex Offset xfb.offset
            for (size_t i = 0; i < kXfbDecorationCount; ++i)
            {
                writeMemberDecorate(id, fieldIndex, kXfbDecorations[i], xfbDecorationValues[i]);
            }
        }

        return false;
    }

    uint32_t newDecorationValue = ShaderInterfaceVariableInfo::kInvalid;

    switch (decoration)
    {
        case spv::DecorationLocation:
            newDecorationValue = info->location;
            break;
        case spv::DecorationBinding:
            newDecorationValue = info->binding;
            break;
        case spv::DecorationDescriptorSet:
            newDecorationValue = info->descriptorSet;
            break;
        case spv::DecorationFlat:
            if (info->useRelaxedPrecision)
            {
                // Change the id to replacement variable
                ASSERT(mFixedVaryingId[id] != 0);
                const size_t instructionOffset = copyInstruction(instruction, wordCount);
                (*mSpirvBlobOut)[instructionOffset + kIdIndex] = mFixedVaryingId[id];
                return true;
            }
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
    const size_t instructionOffset = copyInstruction(instruction, wordCount);
    (*mSpirvBlobOut)[instructionOffset + kDecorationValueIndex] = newDecorationValue;

    // If there are decorations to be added, add them right after the Location decoration is
    // encountered.
    if (decoration != spv::DecorationLocation)
    {
        return true;
    }

    if (info->useRelaxedPrecision)
    {
        // Change the id of the location decoration to replacement variable
        ASSERT(mFixedVaryingId[id] != 0);
        (*mSpirvBlobOut)[instructionOffset + kIdIndex] = mFixedVaryingId[id];

        // The replacement variable is always reduced precision so add that decoration to
        // fixedVaryingId
        constexpr size_t kInstDecorateRelaxedPrecisionWordCount = 3;
        uint32_t inst[kInstDecorateRelaxedPrecisionWordCount];
        SetSpirvInstructionLength(inst, kInstDecorateRelaxedPrecisionWordCount);
        SetSpirvInstructionOp(inst, spv::OpDecorate);
        inst[kIdIndex]         = mFixedVaryingId[id];
        inst[kDecorationIndex] = spv::DecorationRelaxedPrecision;
        copyInstruction(inst, kInstDecorateRelaxedPrecisionWordCount);
    }

    // Add component decoration, if any.
    if (info->component != ShaderInterfaceVariableInfo::kInvalid)
    {
        // Copy the location decoration declaration and modify it to contain the Component
        // decoration.
        const size_t instOffset                         = copyInstruction(instruction, wordCount);
        (*mSpirvBlobOut)[instOffset + kDecorationIndex] = spv::DecorationComponent;
        (*mSpirvBlobOut)[instOffset + kDecorationValueIndex] = info->component;
    }

    // Add Xfb decorations, if any.
    if (mOptions.isTransformFeedbackStage &&
        info->xfb.buffer != ShaderInterfaceVariableXfbInfo::kInvalid)
    {
        ASSERT(info->xfb.stride != ShaderInterfaceVariableXfbInfo::kInvalid);
        ASSERT(info->xfb.offset != ShaderInterfaceVariableXfbInfo::kInvalid);

        const uint32_t xfbDecorationValues[kXfbDecorationCount] = {
            info->xfb.buffer,
            info->xfb.stride,
            info->xfb.offset,
        };

        // Copy the location decoration declaration three times, and modify them to contain the
        // XfbBuffer, XfbStride and Offset decorations.
        for (size_t i = 0; i < kXfbDecorationCount; ++i)
        {
            const size_t xfbInstructionOffset = copyInstruction(instruction, wordCount);
            if (info->useRelaxedPrecision)
            {
                // Change the id to replacement variable
                (*mSpirvBlobOut)[xfbInstructionOffset + kIdIndex] = mFixedVaryingId[id];
            }
            (*mSpirvBlobOut)[xfbInstructionOffset + kDecorationIndex]      = kXfbDecorations[i];
            (*mSpirvBlobOut)[xfbInstructionOffset + kDecorationValueIndex] = xfbDecorationValues[i];
        }
    }

    return true;
}

bool SpirvTransformer::transformMemberDecorate(const uint32_t *instruction, size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpMemberDecorate
    constexpr size_t kTypeIdIndex     = 1;
    constexpr size_t kMemberIndex     = 2;
    constexpr size_t kDecorationIndex = 3;

    uint32_t typeId     = instruction[kTypeIdIndex];
    uint32_t member     = instruction[kMemberIndex];
    uint32_t decoration = instruction[kDecorationIndex];

    // Transform only OpMemberDecorate %gl_PerVertex N BuiltIn B
    if ((typeId != mOutputPerVertex.typeId && typeId != mInputPerVertex.typeId) ||
        decoration != spv::DecorationBuiltIn)
    {
        return false;
    }

    // Drop stripped fields.
    return typeId == mOutputPerVertex.typeId ? member > mOutputPerVertex.maxActiveMember
                                             : member > mInputPerVertex.maxActiveMember;
}

bool SpirvTransformer::transformCapability(const uint32_t *instruction, size_t wordCount)
{
    if (!mHasTransformFeedbackOutput)
    {
        return false;
    }

    // SPIR-V 1.0 Section 3.32 Instructions, OpCapability
    constexpr size_t kCapabilityIndex = 1;

    uint32_t capability = instruction[kCapabilityIndex];

    // Transform feedback capability shouldn't have already been specified.
    ASSERT(capability != spv::CapabilityTransformFeedback);

    // Vulkan shaders have either Shader, Geometry or Tessellation capability.  We find this
    // capability, and add the TransformFeedback capability after it.
    if (capability != spv::CapabilityShader && capability != spv::CapabilityGeometry &&
        capability != spv::CapabilityTessellation)
    {
        return false;
    }

    // Copy the original capability declaration.
    copyInstruction(instruction, wordCount);

    // Create the TransformFeedback capability declaration.

    // SPIR-V 1.0 Section 3.32 Instructions, OpCapability
    constexpr size_t kCapabilityInstructionLength = 2;

    std::array<uint32_t, kCapabilityInstructionLength> newCapabilityDeclaration = {
        instruction[0],  // length+opcode is identical
    };
    // Fill the fields.
    newCapabilityDeclaration[kCapabilityIndex] = spv::CapabilityTransformFeedback;

    copyInstruction(newCapabilityDeclaration.data(), kCapabilityInstructionLength);

    return true;
}

bool SpirvTransformer::transformDebugInfo(const uint32_t *instruction, size_t wordCount)
{
    if (mOptions.removeDebugInfo)
    {
        // Strip debug info to reduce binary size.
        return true;
    }

    // In the case of OpMemberName, unconditionally remove stripped gl_PerVertex members.
    if (GetSpirvInstructionOp(instruction) == spv::OpMemberName)
    {
        // SPIR-V 1.0 Section 3.32 Instructions, OpMemberName
        constexpr size_t kIdIndex     = 1;
        constexpr size_t kMemberIndex = 2;

        const uint32_t id     = instruction[kIdIndex];
        const uint32_t member = instruction[kMemberIndex];

        // Remove the instruction if it's a stripped member of gl_PerVertex.
        return (id == mOutputPerVertex.typeId && member > mOutputPerVertex.maxActiveMember) ||
               (id == mInputPerVertex.typeId && member > mInputPerVertex.maxActiveMember);
    }

    return false;
}

bool SpirvTransformer::transformEmitVertex(const uint32_t *instruction, size_t wordCount)
{
    // This is only possible in geometry shaders.
    ASSERT(mOptions.shaderType == gl::ShaderType::Geometry);

    // Write the temporary variables that hold varyings data before EmitVertex().
    writeOutputPrologue();

    return false;
}

bool SpirvTransformer::transformEntryPoint(const uint32_t *instruction, size_t wordCount)
{
    // Remove inactive varyings from the shader interface declaration.

    // SPIR-V 1.0 Section 3.32 Instructions, OpEntryPoint
    constexpr size_t kEntryPointIdIndex = 2;
    constexpr size_t kNameIndex         = 3;

    // Calculate the length of entry point name in words.  Note that endianness of the string
    // doesn't matter, since we are looking for the '\0' character and rounding up to the word size.
    // This calculates (strlen(name)+1+3) / 4, which is equal to strlen(name)/4+1.
    const size_t nameLength =
        strlen(reinterpret_cast<const char *>(&instruction[kNameIndex])) / 4 + 1;
    const uint32_t instructionLength = GetSpirvInstructionLength(instruction);
    const size_t interfaceStart      = kNameIndex + nameLength;
    const size_t interfaceCount      = instructionLength - interfaceStart;

    // Should only have one EntryPoint
    ASSERT(mEntryPointId == 0);
    mEntryPointId = instruction[kEntryPointIdIndex];

    // Create a copy of the entry point for modification.
    std::vector<uint32_t> filteredEntryPoint(instruction, instruction + wordCount);

    // Filter out inactive varyings from entry point interface declaration.
    size_t writeIndex = interfaceStart;
    for (size_t index = 0; index < interfaceCount; ++index)
    {
        uint32_t id                             = instruction[interfaceStart + index];
        const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];

        ASSERT(info);

        if (!info->activeStages[mOptions.shaderType])
        {
            continue;
        }

        // If ID is one we had to replace due to varying mismatch, use the fixed ID.
        if (mFixedVaryingId[id] != 0)
        {
            id = mFixedVaryingId[id];
        }
        filteredEntryPoint[writeIndex] = id;
        ++writeIndex;
    }

    // Update the length of the instruction.
    const size_t newLength = writeIndex;
    SetSpirvInstructionLength(filteredEntryPoint.data(), newLength);

    // Copy to output.
    copyInstruction(filteredEntryPoint.data(), newLength);

    // Add an OpExecutionMode Xfb instruction if necessary.
    if (!mHasTransformFeedbackOutput)
    {
        return true;
    }

    // SPIR-V 1.0 Section 3.32 Instructions, OpExecutionMode
    constexpr size_t kExecutionModeInstructionLength  = 3;
    constexpr size_t kExecutionModeIdIndex            = 1;
    constexpr size_t kExecutionModeExecutionModeIndex = 2;

    std::array<uint32_t, kExecutionModeInstructionLength> newExecutionModeDeclaration = {};

    // Fill the fields.
    SetSpirvInstructionOp(newExecutionModeDeclaration.data(), spv::OpExecutionMode);
    SetSpirvInstructionLength(newExecutionModeDeclaration.data(), kExecutionModeInstructionLength);
    newExecutionModeDeclaration[kExecutionModeIdIndex]            = instruction[kEntryPointIdIndex];
    newExecutionModeDeclaration[kExecutionModeExecutionModeIndex] = spv::ExecutionModeXfb;

    copyInstruction(newExecutionModeDeclaration.data(), kExecutionModeInstructionLength);

    return true;
}

bool SpirvTransformer::transformTypePointer(const uint32_t *instruction, size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpTypePointer
    constexpr size_t kIdIndex           = 1;
    constexpr size_t kStorageClassIndex = 2;
    constexpr size_t kTypeIdIndex       = 3;

    const uint32_t id           = instruction[kIdIndex];
    const uint32_t storageClass = instruction[kStorageClassIndex];
    const uint32_t typeId       = instruction[kTypeIdIndex];

    // If the storage class is output, this may be used to create a variable corresponding to an
    // inactive varying, or if that varying is a struct, an Op*AccessChain retrieving a field of
    // that inactive varying.
    //
    // SPIR-V specifies the storage class both on the type and the variable declaration.  Otherwise
    // it would have been sufficient to modify the OpVariable instruction. For simplicity, copy
    // every "OpTypePointer Output" and "OpTypePointer Input" instruction except with the Private
    // storage class, in case it may be necessary later.

    // Cannot create a Private type declaration from builtins such as gl_PerVertex.
    if (mNamesById[typeId] != nullptr && gl::IsBuiltInName(mNamesById[typeId]))
    {
        return false;
    }

    // Precision fixup needs this typeID
    mTypePointerTransformedId[id].typeID = typeId;

    if (storageClass != spv::StorageClassOutput && storageClass != spv::StorageClassInput)
    {
        return false;
    }

    // Insert OpTypePointer definition for new PrivateType.
    const size_t instructionOffset = copyInstruction(instruction, wordCount);

    const uint32_t newPrivateTypeId                          = getNewId();
    (*mSpirvBlobOut)[instructionOffset + kIdIndex]           = newPrivateTypeId;
    (*mSpirvBlobOut)[instructionOffset + kStorageClassIndex] = spv::StorageClassPrivate;

    // Remember the id of the replacement.
    ASSERT(id < mTypePointerTransformedId.size());
    mTypePointerTransformedId[id].privateID = newPrivateTypeId;

    // The original instruction should still be present as well.  At this point, we don't know
    // whether we will need the Output or Private type.
    return false;
}

bool SpirvTransformer::transformTypeStruct(const uint32_t *instruction, size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpTypeStruct
    constexpr size_t kIdIndex                         = 1;
    constexpr size_t kTypeStructInstructionBaseLength = 2;

    const uint32_t id = instruction[kIdIndex];

    if (id != mOutputPerVertex.typeId && id != mInputPerVertex.typeId)
    {
        return false;
    }

    const uint32_t maxMembers = id == mOutputPerVertex.typeId ? mOutputPerVertex.maxActiveMember
                                                              : mInputPerVertex.maxActiveMember;

    // Change the definition of the gl_PerVertex struct by stripping unused fields at the end.
    const uint32_t memberCount = maxMembers + 1;
    const size_t newLength     = kTypeStructInstructionBaseLength + memberCount;
    ASSERT(wordCount >= newLength);

    const size_t instructionOffset = copyInstruction(instruction, newLength);
    SetSpirvInstructionLength(&(*mSpirvBlobOut)[instructionOffset], newLength);

    return true;
}

bool SpirvTransformer::transformReturn(const uint32_t *instruction, size_t wordCount)
{
    if (mOpFunctionId != mEntryPointId)
    {
        // We only need to process the precision info when returning from the entry point function
        return false;
    }

    // For geometry shaders, this operations is done before every EmitVertex() instead.
    // Additionally, this transformation (which affects output varyings) doesn't apply to fragment
    // shaders.
    if (mOptions.shaderType == gl::ShaderType::Geometry ||
        mOptions.shaderType == gl::ShaderType::Fragment)
    {
        return false;
    }

    writeOutputPrologue();

    return false;
}

bool SpirvTransformer::transformVariable(const uint32_t *instruction, size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpVariable
    constexpr size_t kTypeIdIndex       = 1;
    constexpr size_t kIdIndex           = 2;
    constexpr size_t kStorageClassIndex = 3;

    const uint32_t id           = instruction[kIdIndex];
    const uint32_t typeId       = instruction[kTypeIdIndex];
    const uint32_t storageClass = instruction[kStorageClassIndex];

    const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];

    // If variable is not a shader interface variable that needs modification, there's nothing to
    // do.
    if (info == nullptr)
    {
        return false;
    }

    // Furthermore, if it's not an inactive varying output, there's nothing to do.  Note that
    // inactive varying inputs are already pruned by the translator.
    // However, input or output storage class for interface block will not be pruned when a shader
    // is compiled separately.
    if (info->activeStages[mOptions.shaderType])
    {
        if (info->useRelaxedPrecision &&
            (storageClass == spv::StorageClassOutput || storageClass == spv::StorageClassInput))
        {
            // Change existing OpVariable to use fixedVaryingId
            const size_t instructionOffset = copyInstruction(instruction, wordCount);
            ASSERT(mFixedVaryingId[id] != 0);
            (*mSpirvBlobOut)[instructionOffset + kIdIndex] = mFixedVaryingId[id];

            // Make original variable a private global
            ASSERT(mTypePointerTransformedId[typeId].privateID != 0);
            writeVariable(id, mTypePointerTransformedId[typeId].privateID,
                          spv::StorageClassPrivate);

            return true;
        }
        return false;
    }

    // Copy the declaration even though the variable is inactive for the separately compiled shader.
    ASSERT(storageClass == spv::StorageClassOutput || storageClass == spv::StorageClassInput);

    // Copy the variable declaration for modification.  Change its type to the corresponding type
    // with the Private storage class, as well as changing the storage class respecified in this
    // instruction.
    const size_t instructionOffset = copyInstruction(instruction, wordCount);

    ASSERT(typeId < mTypePointerTransformedId.size());
    ASSERT(mTypePointerTransformedId[typeId].privateID != 0);

    (*mSpirvBlobOut)[instructionOffset + kTypeIdIndex] =
        mTypePointerTransformedId[typeId].privateID;
    (*mSpirvBlobOut)[instructionOffset + kStorageClassIndex] = spv::StorageClassPrivate;

    return true;
}

bool SpirvTransformer::transformAccessChain(const uint32_t *instruction, size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpAccessChain, OpInBoundsAccessChain, OpPtrAccessChain,
    // OpInBoundsPtrAccessChain
    constexpr size_t kTypeIdIndex = 1;
    constexpr size_t kBaseIdIndex = 3;

    const uint32_t typeId = instruction[kTypeIdIndex];
    const uint32_t baseId = instruction[kBaseIdIndex];

    // If not accessing an inactive output varying, nothing to do.
    const ShaderInterfaceVariableInfo *info = mVariableInfoById[baseId];
    if (info == nullptr)
    {
        return false;
    }

    if (info->activeStages[mOptions.shaderType] && !info->useRelaxedPrecision)
    {
        return false;
    }
    // Copy the instruction for modification.
    const size_t instructionOffset = copyInstruction(instruction, wordCount);

    ASSERT(typeId < mTypePointerTransformedId.size());
    ASSERT(mTypePointerTransformedId[typeId].privateID != 0);

    (*mSpirvBlobOut)[instructionOffset + kTypeIdIndex] =
        mTypePointerTransformedId[typeId].privateID;

    return true;
}

bool SpirvTransformer::transformExecutionMode(const uint32_t *instruction, size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpExecutionMode
    constexpr size_t kModeIndex  = 2;
    const uint32_t executionMode = instruction[kModeIndex];

    if (executionMode == spv::ExecutionModeEarlyFragmentTests &&
        mOptions.removeEarlyFragmentTestsOptimization)
    {
        // skip the copy
        return true;
    }
    return false;
}

struct AliasingAttributeMap
{
    // The SPIR-V id of the aliasing attribute with the most components.  This attribute will be
    // used to read from this location instead of every aliasing one.
    uint32_t attribute = 0;

    // SPIR-V ids of aliasing attributes.
    std::vector<uint32_t> aliasingAttributes;
};

void ValidateShaderInterfaceVariableIsAttribute(const ShaderInterfaceVariableInfo *info)
{
    ASSERT(info);
    ASSERT(info->activeStages[gl::ShaderType::Vertex]);
    ASSERT(info->attributeComponentCount > 0);
    ASSERT(info->attributeLocationCount > 0);
    ASSERT(info->location != ShaderInterfaceVariableInfo::kInvalid);
}

void ValidateIsAliasingAttribute(const AliasingAttributeMap *aliasingMap, uint32_t id)
{
    ASSERT(id != aliasingMap->attribute);
    ASSERT(std::find(aliasingMap->aliasingAttributes.begin(), aliasingMap->aliasingAttributes.end(),
                     id) != aliasingMap->aliasingAttributes.end());
}

// A transformation that resolves vertex attribute aliases.  Note that vertex attribute aliasing is
// only allowed in GLSL ES 100, where the attribute types can only be one of float, vec2, vec3,
// vec4, mat2, mat3, and mat4.  Matrix attributes are handled by expanding them to multiple vector
// attributes, each occupying one location.
class SpirvVertexAttributeAliasingTransformer final : public SpirvTransformerBase
{
  public:
    SpirvVertexAttributeAliasingTransformer(
        const std::vector<uint32_t> &spirvBlobIn,
        const ShaderInterfaceVariableInfoMap &variableInfoMap,
        std::vector<const ShaderInterfaceVariableInfo *> &&variableInfoById,
        SpirvBlob *spirvBlobOut)
        : SpirvTransformerBase(spirvBlobIn, variableInfoMap, spirvBlobOut)
    {
        mVariableInfoById = std::move(variableInfoById);
    }

    bool transform();

  private:
    // Preprocess aliasing attributes in preparation for their removal.
    void preprocessAliasingAttributes();

    // Transform instructions:
    void transformInstruction();

    // Helpers:
    uint32_t getAliasingAttributeReplacementId(uint32_t aliasingId, uint32_t row) const;
    bool isMatrixAttribute(uint32_t id) const;

    // Instructions that are purely informational:
    void visitTypeFloat(const uint32_t *instruction);
    void visitTypeVector(const uint32_t *instruction);
    void visitTypeMatrix(const uint32_t *instruction);
    void visitTypePointer(const uint32_t *instruction);

    // Instructions that potentially need transformation.  They return true if the instruction is
    // transformed.  If false is returned, the instruction should be copied as-is.
    bool transformEntryPoint(const uint32_t *instruction, size_t wordCount);
    bool transformName(const uint32_t *instruction, size_t wordCount);
    bool transformDecorate(const uint32_t *instruction, size_t wordCount);
    bool transformVariable(const uint32_t *instruction, size_t wordCount);
    bool transformAccessChain(const uint32_t *instruction, size_t wordCount);
    void transformLoadHelper(const uint32_t *instruction,
                             size_t wordCount,
                             uint32_t pointerId,
                             uint32_t typeId,
                             uint32_t replacementId,
                             uint32_t resultId);
    bool transformLoad(const uint32_t *instruction, size_t wordCount);

    void declareExpandedMatrixVectors();
    void writeExpandedMatrixInitialization();

    // Transformation state:

    // Map of aliasing attributes per location.
    gl::AttribArray<AliasingAttributeMap> mAliasingAttributeMap;

    // For each id, this map indicates whether it refers to an aliasing attribute that needs to be
    // removed.
    std::vector<bool> mIsAliasingAttributeById;

    // Matrix attributes are split into vectors, each occupying one location.  The SPIR-V
    // declaration would need to change from:
    //
    //     %type = OpTypeMatrix %vectorType N
    //     %matrixType = OpTypePointer Input %type
    //     %matrix = OpVariable %matrixType Input
    //
    // to:
    //
    //     %matrixType = OpTypePointer Private %type
    //     %matrix = OpVariable %matrixType Private
    //
    //     %vecType = OpTypePointer Input %vectorType
    //
    //     %vec0 = OpVariable %vecType Input
    //     ...
    //     %vecN-1 = OpVariable %vecType Input
    //
    // For each id %matrix (which corresponds to a matrix attribute), this map contains %vec0.  The
    // ids of the split vectors are consecutive, so %veci == %vec0 + i.  %veciType is taken from
    // mInputTypePointers.
    std::vector<uint32_t> mExpandedMatrixFirstVectorIdById;
    // Whether the expanded matrix OpVariables are generated.
    bool mHaveMatricesExpanded = false;
    // Whether initialization of the matrix attributes should be written at the beginning of the
    // current function.
    bool mWriteExpandedMatrixInitialization = false;
    uint32_t mEntryPointId                  = 0;

    // Id of attribute types; float and veci.  This array is one-based, and [0] is unused.
    //
    // [1]: id of OpTypeFloat 32
    // [N]: id of OpTypeVector %[1] N, N = {2, 3, 4}
    //
    // In other words, index of the array corresponds to the number of components in the type.
    std::array<uint32_t, 5> mFloatTypes = {};

    // Corresponding to mFloatTypes, [i]: id of OpMatrix %mFloatTypes[i] i.  Note that only square
    // matrices are possible as attributes in GLSL ES 1.00.  [0] and [1] are unused.
    std::array<uint32_t, 5> mMatrixTypes = {};

    // Corresponding to mFloatTypes, [i]: id of OpTypePointer Input %mFloatTypes[i].  [0] is unused.
    std::array<uint32_t, 5> mInputTypePointers = {};

    // Corresponding to mFloatTypes, [i]: id of OpTypePointer Private %mFloatTypes[i].  [0] is
    // unused.
    std::array<uint32_t, 5> mPrivateFloatTypePointers = {};

    // Corresponding to mMatrixTypes, [i]: id of OpTypePointer Private %mMatrixTypes[i].  [0] and
    // [1] are unused.
    std::array<uint32_t, 5> mPrivateMatrixTypePointers = {};
};

bool SpirvVertexAttributeAliasingTransformer::transform()
{
    onTransformBegin();

    preprocessAliasingAttributes();

    while (mCurrentWord < mSpirvBlobIn.size())
    {
        transformInstruction();
    }

    return true;
}

void SpirvVertexAttributeAliasingTransformer::preprocessAliasingAttributes()
{
    const size_t indexBound = mSpirvBlobIn[kHeaderIndexIndexBound];

    mVariableInfoById.resize(indexBound, nullptr);
    mIsAliasingAttributeById.resize(indexBound, false);
    mExpandedMatrixFirstVectorIdById.resize(indexBound, 0);

    // Go through attributes and find out which alias which.
    for (size_t id = 0; id < indexBound; ++id)
    {
        const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];

        // Ignore non attribute ids.
        if (info == nullptr || info->attributeComponentCount == 0)
        {
            continue;
        }

        ASSERT(info->activeStages[gl::ShaderType::Vertex]);
        ASSERT(info->location != ShaderInterfaceVariableInfo::kInvalid);

        const bool isMatrixAttribute = info->attributeLocationCount > 1;

        for (uint32_t offset = 0; offset < info->attributeLocationCount; ++offset)
        {
            uint32_t location = info->location + offset;
            ASSERT(location < mAliasingAttributeMap.size());

            uint32_t attributeId = id;

            // If this is a matrix attribute, expand it to vectors.
            if (isMatrixAttribute)
            {
                uint32_t matrixId = id;

                // Get a new id for this location and associate it with the matrix.
                attributeId = getNewId();
                if (offset == 0)
                {
                    mExpandedMatrixFirstVectorIdById[matrixId] = attributeId;
                }
                // The ids are consecutive.
                ASSERT(attributeId == mExpandedMatrixFirstVectorIdById[matrixId] + offset);

                mIsAliasingAttributeById.resize(attributeId + 1, false);
                mVariableInfoById.resize(attributeId + 1, nullptr);
                mVariableInfoById[attributeId] = info;
            }

            AliasingAttributeMap *aliasingMap = &mAliasingAttributeMap[location];

            // If this is the first attribute in this location, remember it.
            if (aliasingMap->attribute == 0)
            {
                aliasingMap->attribute = attributeId;
                continue;
            }

            // Otherwise, either add it to the list of aliasing attributes, or replace the main
            // attribute (and add that to the list of aliasing attributes).  The one with the
            // largest number of components is used as the main attribute.
            const ShaderInterfaceVariableInfo *curMainAttribute =
                mVariableInfoById[aliasingMap->attribute];
            ASSERT(curMainAttribute != nullptr && curMainAttribute->attributeComponentCount > 0);

            uint32_t aliasingId;
            if (info->attributeComponentCount > curMainAttribute->attributeComponentCount)
            {
                aliasingId             = aliasingMap->attribute;
                aliasingMap->attribute = attributeId;
            }
            else
            {
                aliasingId = attributeId;
            }

            aliasingMap->aliasingAttributes.push_back(aliasingId);
            ASSERT(mIsAliasingAttributeById[aliasingId] == false);
            mIsAliasingAttributeById[aliasingId] = true;
        }
    }
}

void SpirvVertexAttributeAliasingTransformer::transformInstruction()
{
    uint32_t wordCount;
    uint32_t opCode;
    const uint32_t *instruction = getCurrentInstruction(&opCode, &wordCount);

    if (opCode == spv::OpFunction)
    {
        // Declare the expanded matrix variables right before the first function declaration.
        if (!mHaveMatricesExpanded)
        {
            declareExpandedMatrixVectors();
            mHaveMatricesExpanded = true;
        }

        // SPIR-V is structured in sections.  Function declarations come last.
        mIsInFunctionSection = true;

        // The matrix attribute declarations have been changed to have Private storage class, and
        // they are initialized from the expanded (and potentially aliased) Input vectors.  This is
        // done at the beginning of the entry point.

        // SPIR-V 1.0 Section 3.32 Instructions, OpFunction
        constexpr size_t kFunctionIdIndex = 2;

        const uint32_t functionId = instruction[kFunctionIdIndex];

        mWriteExpandedMatrixInitialization = functionId == mEntryPointId;
    }

    // Only look at interesting instructions.
    bool transformed = false;

    if (mIsInFunctionSection)
    {
        // Write expanded matrix initialization right after the entry point's OpFunction and any
        // instruction that must come immediately after it.
        if (mWriteExpandedMatrixInitialization && opCode != spv::OpFunction &&
            opCode != spv::OpFunctionParameter && opCode != spv::OpLabel &&
            opCode != spv::OpVariable)
        {
            writeExpandedMatrixInitialization();
            mWriteExpandedMatrixInitialization = false;
        }

        // Look at in-function opcodes.
        switch (opCode)
        {
            case spv::OpAccessChain:
            case spv::OpInBoundsAccessChain:
                transformed = transformAccessChain(instruction, wordCount);
                break;
            case spv::OpLoad:
                transformed = transformLoad(instruction, wordCount);
                break;
            default:
                break;
        }
    }
    else
    {
        // Look at global declaration opcodes.
        switch (opCode)
        {
            // Informational instructions:
            case spv::OpTypeFloat:
                visitTypeFloat(instruction);
                break;
            case spv::OpTypeVector:
                visitTypeVector(instruction);
                break;
            case spv::OpTypeMatrix:
                visitTypeMatrix(instruction);
                break;
            case spv::OpTypePointer:
                visitTypePointer(instruction);
                break;
            // Instructions that may need transformation:
            case spv::OpEntryPoint:
                transformed = transformEntryPoint(instruction, wordCount);
                break;
            case spv::OpName:
                transformed = transformName(instruction, wordCount);
                break;
            case spv::OpDecorate:
                transformed = transformDecorate(instruction, wordCount);
                break;
            case spv::OpVariable:
                transformed = transformVariable(instruction, wordCount);
                break;
            default:
                break;
        }
    }

    // If the instruction was not transformed, copy it to output as is.
    if (!transformed)
    {
        copyInstruction(instruction, wordCount);
    }

    // Advance to next instruction.
    mCurrentWord += wordCount;
}

uint32_t SpirvVertexAttributeAliasingTransformer::getAliasingAttributeReplacementId(
    uint32_t aliasingId,
    uint32_t offset) const
{
    // Get variable info corresponding to the aliasing attribute.
    const ShaderInterfaceVariableInfo *aliasingInfo = mVariableInfoById[aliasingId];
    ValidateShaderInterfaceVariableIsAttribute(aliasingInfo);

    // Find the replacement attribute.
    const AliasingAttributeMap *aliasingMap =
        &mAliasingAttributeMap[aliasingInfo->location + offset];
    ValidateIsAliasingAttribute(aliasingMap, aliasingId);

    const uint32_t replacementId = aliasingMap->attribute;
    ASSERT(replacementId != 0 && replacementId < mIsAliasingAttributeById.size());
    ASSERT(!mIsAliasingAttributeById[replacementId]);

    return replacementId;
}

bool SpirvVertexAttributeAliasingTransformer::isMatrixAttribute(uint32_t id) const
{
    return mExpandedMatrixFirstVectorIdById[id] != 0;
}

void SpirvVertexAttributeAliasingTransformer::visitTypeFloat(const uint32_t *instruction)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpTypeFloat
    constexpr size_t kIdIndex    = 1;
    constexpr size_t kWidthIndex = 2;

    const uint32_t id    = instruction[kIdIndex];
    const uint32_t width = instruction[kWidthIndex];

    // Only interested in OpTypeFloat 32.
    if (width == 32)
    {
        ASSERT(mFloatTypes[1] == 0);
        mFloatTypes[1] = id;
    }
}

void SpirvVertexAttributeAliasingTransformer::visitTypeVector(const uint32_t *instruction)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpTypeVector
    constexpr size_t kIdIndex             = 1;
    constexpr size_t kComponentIdIndex    = 2;
    constexpr size_t kComponentCountIndex = 3;

    const uint32_t id             = instruction[kIdIndex];
    const uint32_t componentId    = instruction[kComponentIdIndex];
    const uint32_t componentCount = instruction[kComponentCountIndex];

    // Only interested in OpTypeVector %f32 N, where %f32 is the id of OpTypeFloat 32.
    if (componentId == mFloatTypes[1])
    {
        ASSERT(componentCount >= 2 && componentCount <= 4);
        ASSERT(mFloatTypes[componentCount] == 0);
        mFloatTypes[componentCount] = id;
    }
}

void SpirvVertexAttributeAliasingTransformer::visitTypeMatrix(const uint32_t *instruction)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpTypeMatrix
    constexpr size_t kIdIndex          = 1;
    constexpr size_t kColumnTypeIndex  = 2;
    constexpr size_t kColumnCountIndex = 3;

    const uint32_t id          = instruction[kIdIndex];
    const uint32_t columnType  = instruction[kColumnTypeIndex];
    const uint32_t columnCount = instruction[kColumnCountIndex];

    // Only interested in OpTypeMatrix %vecN, where %vecN is the id of OpTypeVector %f32 N.
    // This is only for square matN types (as allowed by GLSL ES 1.00), so columnCount is the same
    // as rowCount.
    if (columnType == mFloatTypes[columnCount])
    {
        ASSERT(mMatrixTypes[columnCount] == 0);
        mMatrixTypes[columnCount] = id;
    }
}

void SpirvVertexAttributeAliasingTransformer::visitTypePointer(const uint32_t *instruction)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpTypePointer
    constexpr size_t kIdIndex           = 1;
    constexpr size_t kStorageClassIndex = 2;
    constexpr size_t kTypeIdIndex       = 3;

    const uint32_t id           = instruction[kIdIndex];
    const uint32_t storageClass = instruction[kStorageClassIndex];
    const uint32_t typeId       = instruction[kTypeIdIndex];

    // Only interested in OpTypePointer Input %vecN, where %vecN is the id of OpTypeVector %f32 N,
    // as well as OpTypePointer Private %matN, where %matN is the id of OpTypeMatrix %vecN N.
    // This is only for matN types (as allowed by GLSL ES 1.00), so N >= 2.
    if (storageClass == spv::StorageClassInput)
    {
        for (size_t n = 2; n < mFloatTypes.size(); ++n)
        {
            if (typeId == mFloatTypes[n])
            {
                ASSERT(mInputTypePointers[n] == 0);
                mInputTypePointers[n] = id;
                break;
            }
        }
    }
    else if (storageClass == spv::StorageClassPrivate)
    {
        ASSERT(mFloatTypes.size() == mMatrixTypes.size());
        for (size_t n = 2; n < mMatrixTypes.size(); ++n)
        {
            // Note that Private types may not be unique, as the previous transformation can
            // generate duplicates.
            if (typeId == mFloatTypes[n])
            {
                mPrivateFloatTypePointers[n] = id;
                break;
            }
            if (typeId == mMatrixTypes[n])
            {
                mPrivateMatrixTypePointers[n] = id;
                break;
            }
        }
    }
}

bool SpirvVertexAttributeAliasingTransformer::transformEntryPoint(const uint32_t *instruction,
                                                                  size_t wordCount)
{
    // Remove aliasing attributes from the shader interface declaration.

    // SPIR-V 1.0 Section 3.32 Instructions, OpEntryPoint
    constexpr size_t kEntryPointIdIndex = 2;
    constexpr size_t kNameIndex         = 3;

    // See comment in SpirvTransformer::transformEntryPoint.
    const size_t nameLength =
        strlen(reinterpret_cast<const char *>(&instruction[kNameIndex])) / 4 + 1;
    const uint32_t instructionLength = GetSpirvInstructionLength(instruction);
    const size_t interfaceStart      = kNameIndex + nameLength;

    // Should only have one EntryPoint
    ASSERT(mEntryPointId == 0);
    mEntryPointId = instruction[kEntryPointIdIndex];

    // Create a copy of the entry point for modification.
    std::vector<uint32_t> filteredEntryPoint(instruction, instruction + wordCount);

    // As a first pass, filter out matrix attributes and append their replacement vectors.
    for (size_t index = interfaceStart; index < instructionLength; ++index)
    {
        uint32_t matrixId = instruction[index];

        if (mExpandedMatrixFirstVectorIdById[matrixId] == 0)
        {
            continue;
        }

        const ShaderInterfaceVariableInfo *info = mVariableInfoById[matrixId];
        ValidateShaderInterfaceVariableIsAttribute(info);

        // Replace the matrix id with its first vector id.
        uint32_t vec0Id           = mExpandedMatrixFirstVectorIdById[matrixId];
        filteredEntryPoint[index] = vec0Id;

        // Append the rest of the vectors to the entry point.
        for (uint32_t offset = 1; offset < info->attributeLocationCount; ++offset)
        {
            uint32_t vecId = vec0Id + offset;
            filteredEntryPoint.push_back(vecId);
        }
    }

    // Filter out aliasing attributes from entry point interface declaration.
    size_t writeIndex = interfaceStart;
    for (size_t index = interfaceStart; index < filteredEntryPoint.size(); ++index)
    {
        uint32_t id = filteredEntryPoint[index];

        // If this is an attribute that's aliasing another one in the same location, remove it.
        if (mIsAliasingAttributeById[id])
        {
            const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];
            ValidateShaderInterfaceVariableIsAttribute(info);

            // The following assertion is only valid for non-matrix attributes.
            if (info->attributeLocationCount == 1)
            {
                const AliasingAttributeMap *aliasingMap = &mAliasingAttributeMap[info->location];
                ValidateIsAliasingAttribute(aliasingMap, id);
            }

            continue;
        }

        filteredEntryPoint[writeIndex] = id;
        ++writeIndex;
    }

    // Update the length of the instruction.
    const size_t newLength = writeIndex;
    SetSpirvInstructionLength(filteredEntryPoint.data(), newLength);

    // Copy to output.
    copyInstruction(filteredEntryPoint.data(), newLength);

    return true;
}

bool SpirvVertexAttributeAliasingTransformer::transformName(const uint32_t *instruction,
                                                            size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpName
    constexpr size_t kIdIndex = 1;

    uint32_t id = instruction[kIdIndex];

    // If id is not that of an aliasing attribute, there's nothing to do.
    ASSERT(id < mIsAliasingAttributeById.size());
    if (!mIsAliasingAttributeById[id])
    {
        return false;
    }

    // Drop debug annotations for this id.
    return true;
}

bool SpirvVertexAttributeAliasingTransformer::transformDecorate(const uint32_t *instruction,
                                                                size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpDecorate
    constexpr size_t kIdIndex              = 1;
    constexpr size_t kDecorationIndex      = 2;
    constexpr size_t kDecorationValueIndex = 3;

    uint32_t id         = instruction[kIdIndex];
    uint32_t decoration = instruction[kDecorationIndex];

    if (isMatrixAttribute(id))
    {
        // If it's a matrix attribute, it's expanded to multiple vectors.  Insert the Location
        // decorations for these vectors here.

        // Keep all decorations except for Location.
        if (decoration != spv::DecorationLocation)
        {
            return false;
        }

        const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];
        ValidateShaderInterfaceVariableIsAttribute(info);

        uint32_t vec0Id = mExpandedMatrixFirstVectorIdById[id];
        ASSERT(vec0Id != 0);

        for (uint32_t offset = 0; offset < info->attributeLocationCount; ++offset)
        {
            uint32_t vecId = vec0Id + offset;
            if (mIsAliasingAttributeById[vecId])
            {
                continue;
            }

            const size_t instOffset                 = copyInstruction(instruction, wordCount);
            (*mSpirvBlobOut)[instOffset + kIdIndex] = vecId;
            (*mSpirvBlobOut)[instOffset + kDecorationValueIndex] = info->location + offset;
        }
    }
    else
    {
        // If id is not that of an active attribute, there's nothing to do.
        const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];
        if (info == nullptr || info->attributeComponentCount == 0 ||
            !info->activeStages[gl::ShaderType::Vertex])
        {
            return false;
        }

        // Always drop RelaxedPrecision from input attributes.  The temporary variable the attribute
        // is loaded into has RelaxedPrecision and will implicitly convert.
        if (decoration == spv::DecorationRelaxedPrecision)
        {
            return true;
        }

        // If id is not that of an aliasing attribute, there's nothing else to do.
        ASSERT(id < mIsAliasingAttributeById.size());
        if (!mIsAliasingAttributeById[id])
        {
            return false;
        }
    }

    // Drop every decoration for this id.
    return true;
}

bool SpirvVertexAttributeAliasingTransformer::transformVariable(const uint32_t *instruction,
                                                                size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpVariable
    constexpr size_t kIdIndex           = 2;
    constexpr size_t kStorageClassIndex = 3;

    const uint32_t id           = instruction[kIdIndex];
    const uint32_t storageClass = instruction[kStorageClassIndex];

    if (!isMatrixAttribute(id))
    {
        // If id is not that of an aliasing attribute, there's nothing to do.  Note that matrix
        // declarations are always replaced.
        ASSERT(id < mIsAliasingAttributeById.size());
        if (!mIsAliasingAttributeById[id])
        {
            return false;
        }
    }

    ASSERT(storageClass == spv::StorageClassInput);

    // Drop the declaration.
    return true;
}

bool SpirvVertexAttributeAliasingTransformer::transformAccessChain(const uint32_t *instruction,
                                                                   size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpAccessChain, OpInBoundsAccessChain
    constexpr size_t kTypeIdIndex = 1;
    constexpr size_t kBaseIdIndex = 3;

    const uint32_t typeId = instruction[kTypeIdIndex];
    const uint32_t baseId = instruction[kBaseIdIndex];

    if (isMatrixAttribute(baseId))
    {
        // Copy the OpAccessChain instruction for modification.  Only modification is that the %type
        // is replaced with the Private version of it.  If there is one %index, that would be a
        // vector type, and if there are two %index'es, it's a float type.

        const size_t instOffset = copyInstruction(instruction, wordCount);

        // SPIR-V 1.0 Section 3.32 Instructions, OpAccessChain, OpInBoundsAccessChain
        constexpr size_t kAccessChainBaseLength = 4;

        if (wordCount == kAccessChainBaseLength + 1)
        {
            // If indexed once, it uses a vector type.
            const ShaderInterfaceVariableInfo *info = mVariableInfoById[baseId];
            ValidateShaderInterfaceVariableIsAttribute(info);

            const uint32_t componentCount = info->attributeComponentCount;

            // %type must have been the Input vector type with the matrice's component size.
            ASSERT(typeId == mInputTypePointers[componentCount]);

            // Replace the type with the corresponding Private one.
            (*mSpirvBlobOut)[instOffset + kTypeIdIndex] = mPrivateFloatTypePointers[componentCount];
        }
        else
        {
            // If indexed twice, it uses the float type.
            ASSERT(wordCount == kAccessChainBaseLength + 2);

            // Replace the type with the Private pointer to float32.
            (*mSpirvBlobOut)[instOffset + kTypeIdIndex] = mPrivateFloatTypePointers[1];
        }
    }
    else
    {
        // If base id is not that of an aliasing attribute, there's nothing to do.
        ASSERT(baseId < mIsAliasingAttributeById.size());
        if (!mIsAliasingAttributeById[baseId])
        {
            return false;
        }

        // Find the replacement attribute for the aliasing one.
        const uint32_t replacementId = getAliasingAttributeReplacementId(baseId, 0);

        // Get variable info corresponding to the replacement attribute.
        const ShaderInterfaceVariableInfo *replacementInfo = mVariableInfoById[replacementId];
        ValidateShaderInterfaceVariableIsAttribute(replacementInfo);

        // Copy the OpAccessChain instruction for modification.  Currently, the instruction is:
        //
        //     %id = OpAccessChain %type %base %index
        //
        // This is modified to:
        //
        //     %id = OpAccessChain %type %replacement %index
        //
        // Note that the replacement has at least as many components as the aliasing attribute,
        // and both attributes start at component 0 (GLSL ES restriction).  So, indexing the
        // replacement attribute with the same index yields the same result and type.
        const size_t instOffset                     = copyInstruction(instruction, wordCount);
        (*mSpirvBlobOut)[instOffset + kBaseIdIndex] = replacementId;
    }

    return true;
}

void SpirvVertexAttributeAliasingTransformer::transformLoadHelper(const uint32_t *instruction,
                                                                  size_t wordCount,
                                                                  uint32_t pointerId,
                                                                  uint32_t typeId,
                                                                  uint32_t replacementId,
                                                                  uint32_t resultId)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpLoad
    constexpr size_t kTypeIdIndex    = 1;
    constexpr size_t kIdIndex        = 2;
    constexpr size_t kPointerIdIndex = 3;

    // Get variable info corresponding to the replacement attribute.
    const ShaderInterfaceVariableInfo *replacementInfo = mVariableInfoById[replacementId];
    ValidateShaderInterfaceVariableIsAttribute(replacementInfo);

    // Copy the load instruction for modification.  Currently, the instruction is:
    //
    //     %id = OpLoad %type %pointer
    //
    // This is modified to:
    //
    //     %newId = OpLoad %replacementType %replacement
    //
    const uint32_t loadResultId      = getNewId();
    const uint32_t replacementTypeId = mFloatTypes[replacementInfo->attributeComponentCount];
    ASSERT(replacementTypeId != 0);

    const size_t instOffset                        = copyInstruction(instruction, wordCount);
    (*mSpirvBlobOut)[instOffset + kIdIndex]        = loadResultId;
    (*mSpirvBlobOut)[instOffset + kTypeIdIndex]    = replacementTypeId;
    (*mSpirvBlobOut)[instOffset + kPointerIdIndex] = replacementId;

    // If swizzle is not necessary, assign %newId to %resultId.
    const ShaderInterfaceVariableInfo *aliasingInfo = mVariableInfoById[pointerId];
    if (aliasingInfo->attributeComponentCount == replacementInfo->attributeComponentCount)
    {
        writeCopyObject(resultId, typeId, loadResultId);
        return;
    }

    // Take as many components from the replacement as the aliasing attribute wanted.  This is done
    // by either of the following instructions:
    //
    // - If aliasing attribute has only one component:
    //
    //     %resultId = OpCompositeExtract %floatType %newId 0
    //
    // - If aliasing attribute has more than one component:
    //
    //     %resultId = OpVectorShuffle %vecType %newId %newId 0 1 ...
    //
    ASSERT(aliasingInfo->attributeComponentCount < replacementInfo->attributeComponentCount);
    ASSERT(mFloatTypes[aliasingInfo->attributeComponentCount] == typeId);

    if (aliasingInfo->attributeComponentCount == 1)
    {
        writeCompositeExtract(resultId, typeId, loadResultId, 0);
    }
    else
    {
        angle::FixedVector<uint32_t, 4> swizzle = {0, 1, 2, 3};
        swizzle.resize(aliasingInfo->attributeComponentCount);

        writeVectorShuffle(resultId, typeId, loadResultId, loadResultId, swizzle);
    }
}

bool SpirvVertexAttributeAliasingTransformer::transformLoad(const uint32_t *instruction,
                                                            size_t wordCount)
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpLoad
    constexpr size_t kTypeIdIndex    = 1;
    constexpr size_t kIdIndex        = 2;
    constexpr size_t kPointerIdIndex = 3;

    const uint32_t id        = instruction[kIdIndex];
    const uint32_t typeId    = instruction[kTypeIdIndex];
    const uint32_t pointerId = instruction[kPointerIdIndex];

    // Currently, the instruction is:
    //
    //     %id = OpLoad %type %pointer
    //
    // If non-matrix, this is modifed to load from the aliasing vector instead if aliasing.
    //
    // If matrix, this is modified such that %type points to the Private version of it.
    //
    if (isMatrixAttribute(pointerId))
    {
        const ShaderInterfaceVariableInfo *info = mVariableInfoById[pointerId];
        ValidateShaderInterfaceVariableIsAttribute(info);

        const uint32_t replacementTypeId = mMatrixTypes[info->attributeLocationCount];

        const size_t instOffset                     = copyInstruction(instruction, wordCount);
        (*mSpirvBlobOut)[instOffset + kTypeIdIndex] = replacementTypeId;
    }
    else
    {
        // If pointer id is not that of an aliasing attribute, there's nothing to do.
        ASSERT(pointerId < mIsAliasingAttributeById.size());
        if (!mIsAliasingAttributeById[pointerId])
        {
            return false;
        }

        // Find the replacement attribute for the aliasing one.
        const uint32_t replacementId = getAliasingAttributeReplacementId(pointerId, 0);

        // Replace the load instruction by a load from the replacement id.
        transformLoadHelper(instruction, wordCount, pointerId, typeId, replacementId, id);
    }

    return true;
}

void SpirvVertexAttributeAliasingTransformer::declareExpandedMatrixVectors()
{
    // Go through matrix attributes and expand them.
    for (size_t matrixId = 0; matrixId < mExpandedMatrixFirstVectorIdById.size(); ++matrixId)
    {
        uint32_t vec0Id = mExpandedMatrixFirstVectorIdById[matrixId];
        if (vec0Id == 0)
        {
            continue;
        }

        const ShaderInterfaceVariableInfo *info = mVariableInfoById[matrixId];
        ValidateShaderInterfaceVariableIsAttribute(info);

        // Need to generate the following:
        //
        //     %privateType = OpTypePointer Private %matrixType
        //     %id = OpVariable %privateType Private
        //     %vecType = OpTypePointer %vecType Input
        //     %vec0 = OpVariable %vecType Input
        //     ...
        //     %vecN-1 = OpVariable %vecType Input
        const uint32_t componentCount = info->attributeComponentCount;
        const uint32_t locationCount  = info->attributeLocationCount;
        ASSERT(componentCount == locationCount);
        ASSERT(mMatrixTypes[locationCount] != 0);

        // OpTypePointer Private %matrixType
        uint32_t privateType = mPrivateMatrixTypePointers[locationCount];
        if (privateType == 0)
        {
            privateType                               = getNewId();
            mPrivateMatrixTypePointers[locationCount] = privateType;
            writeTypePointer(privateType, spv::StorageClassPrivate, mMatrixTypes[locationCount]);
        }

        // OpVariable %privateType Private
        writeVariable(matrixId, privateType, spv::StorageClassPrivate);

        // If the OpTypePointer is not declared for the vector type corresponding to each location,
        // declare it now.
        //
        //     %vecType = OpTypePointer %vecType Input
        uint32_t inputType = mInputTypePointers[componentCount];
        if (inputType == 0)
        {
            inputType                          = getNewId();
            mInputTypePointers[componentCount] = inputType;
            writeTypePointer(inputType, spv::StorageClassInput, mFloatTypes[componentCount]);
        }

        // Declare a vector for each column of the matrix.
        for (uint32_t offset = 0; offset < info->attributeLocationCount; ++offset)
        {
            uint32_t vecId = vec0Id + offset;
            if (!mIsAliasingAttributeById[vecId])
            {
                writeVariable(vecId, inputType, spv::StorageClassInput);
            }
        }
    }

    // Additionally, declare OpTypePointer Private %mFloatTypes[i] in case needed (used in
    // Op*AccessChain instructions, if any).
    for (size_t n = 1; n < mFloatTypes.size(); ++n)
    {
        if (mFloatTypes[n] != 0 && mPrivateFloatTypePointers[n] == 0)
        {
            const uint32_t privateType   = getNewId();
            mPrivateFloatTypePointers[n] = privateType;
            writeTypePointer(privateType, spv::StorageClassPrivate, mFloatTypes[n]);
        }
    }
}

void SpirvVertexAttributeAliasingTransformer::writeExpandedMatrixInitialization()
{
    // SPIR-V 1.0 Section 3.32 Instructions, OpLoad
    constexpr size_t kLoadInstructionLength = 4;

    // A fake OpLoad instruction for transformLoadHelper to copy off of.
    std::array<uint32_t, kLoadInstructionLength> loadInstruction = {};
    SetSpirvInstructionOp(loadInstruction.data(), spv::OpLoad);
    SetSpirvInstructionLength(loadInstruction.data(), kLoadInstructionLength);

    // Go through matrix attributes and initialize them.  Note that their declaration is replaced
    // with a Private storage class, but otherwise has the same id.
    for (size_t matrixId = 0; matrixId < mExpandedMatrixFirstVectorIdById.size(); ++matrixId)
    {
        uint32_t vec0Id = mExpandedMatrixFirstVectorIdById[matrixId];
        if (vec0Id == 0)
        {
            continue;
        }

        // For every matrix, need to generate the following:
        //
        //     %vec0Id = OpLoad %vecType %vec0Pointer
        //     ...
        //     %vecN-1Id = OpLoad %vecType %vecN-1Pointer
        //     %mat = OpCompositeConstruct %matrixType %vec0 ... %vecN-1
        //     OpStore %matrixId %mat

        const ShaderInterfaceVariableInfo *info = mVariableInfoById[matrixId];
        ValidateShaderInterfaceVariableIsAttribute(info);

        angle::FixedVector<uint32_t, 4> vecLoadIds;
        const uint32_t locationCount = info->attributeLocationCount;
        for (uint32_t offset = 0; offset < locationCount; ++offset)
        {
            uint32_t vecId = vec0Id + offset;

            // Load into temporary, potentially through an aliasing vector.
            uint32_t replacementId = vecId;
            ASSERT(vecId < mIsAliasingAttributeById.size());
            if (mIsAliasingAttributeById[vecId])
            {
                replacementId = getAliasingAttributeReplacementId(vecId, offset);
            }

            // Write a load instruction from the replacement id.
            vecLoadIds.push_back(getNewId());
            transformLoadHelper(loadInstruction.data(), kLoadInstructionLength, matrixId,
                                mFloatTypes[info->attributeComponentCount], replacementId,
                                vecLoadIds.back());
        }

        // Aggregate the vector loads into a matrix.
        ASSERT(mMatrixTypes[locationCount] != 0);
        const uint32_t compositeId = getNewId();
        writeCompositeConstruct(compositeId, mMatrixTypes[locationCount], vecLoadIds);

        // Store it in the private variable.
        writeStore(matrixId, compositeId);
    }
}

bool HasAliasingAttributes(const ShaderInterfaceVariableInfoMap &variableInfoMap)
{
    gl::AttributesMask isLocationAssigned;

    for (const auto &infoIter : variableInfoMap.getIterator(gl::ShaderType::Vertex))
    {
        const ShaderInterfaceVariableInfo &info = infoIter.second;

        // Ignore non attribute ids.
        if (info.attributeComponentCount == 0)
        {
            continue;
        }

        ASSERT(info.activeStages[gl::ShaderType::Vertex]);
        ASSERT(info.location != ShaderInterfaceVariableInfo::kInvalid);
        ASSERT(info.attributeLocationCount > 0);

        for (uint8_t offset = 0; offset < info.attributeLocationCount; ++offset)
        {
            uint32_t location = info.location + offset;

            // If there's aliasing, return immediately.
            if (isLocationAssigned.test(location))
            {
                return true;
            }

            isLocationAssigned.set(location);
        }
    }

    return false;
}
}  // anonymous namespace

// ShaderInterfaceVariableInfo implementation.
const uint32_t ShaderInterfaceVariableInfo::kInvalid;

ShaderInterfaceVariableInfo::ShaderInterfaceVariableInfo() {}

// ShaderInterfaceVariableInfoMap implementation.
ShaderInterfaceVariableInfoMap::ShaderInterfaceVariableInfoMap() = default;

ShaderInterfaceVariableInfoMap::~ShaderInterfaceVariableInfoMap() = default;

void ShaderInterfaceVariableInfoMap::clear()
{
    for (VariableNameToInfoMap &shaderMap : mData)
    {
        shaderMap.clear();
    }
}

bool ShaderInterfaceVariableInfoMap::contains(gl::ShaderType shaderType,
                                              const std::string &variableName) const
{
    return mData[shaderType].find(variableName) != mData[shaderType].end();
}

const ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::get(
    gl::ShaderType shaderType,
    const std::string &variableName) const
{
    auto it = mData[shaderType].find(variableName);
    ASSERT(it != mData[shaderType].end());
    return it->second;
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::get(gl::ShaderType shaderType,
                                                                 const std::string &variableName)
{
    auto it = mData[shaderType].find(variableName);
    ASSERT(it != mData[shaderType].end());
    return it->second;
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::add(gl::ShaderType shaderType,
                                                                 const std::string &variableName)
{
    ASSERT(!contains(shaderType, variableName));
    return mData[shaderType][variableName];
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::addOrGet(
    gl::ShaderType shaderType,
    const std::string &variableName)
{
    return mData[shaderType][variableName];
}

ShaderInterfaceVariableInfoMap::Iterator ShaderInterfaceVariableInfoMap::getIterator(
    gl::ShaderType shaderType) const
{
    return Iterator(mData[shaderType].begin(), mData[shaderType].end());
}

void GlslangInitialize()
{
    int result = ShInitialize();
    ASSERT(result != 0);
    GlslangWarmup();
}

void GlslangRelease()
{
    int result = ShFinalize();
    ASSERT(result != 0);
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

std::string GetXfbBufferName(const uint32_t bufferIndex)
{
    return "xfbBuffer" + Str(bufferIndex);
}

void GlslangGenTransformFeedbackEmulationOutputs(const GlslangSourceOptions &options,
                                                 const gl::ProgramState &programState,
                                                 GlslangProgramInterfaceInfo *programInterfaceInfo,
                                                 std::string *vertexShader,
                                                 ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    GenerateTransformFeedbackEmulationOutputs(options, gl::ShaderType::Vertex, programState,
                                              programInterfaceInfo, vertexShader,
                                              variableInfoMapOut);
}

void GlslangAssignLocations(const GlslangSourceOptions &options,
                            const gl::ProgramExecutable &programExecutable,
                            const gl::ProgramVaryingPacking &varyingPacking,
                            const gl::ShaderType shaderType,
                            const gl::ShaderType frontShaderType,
                            GlslangProgramInterfaceInfo *programInterfaceInfo,
                            ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    // Assign outputs to the fragment shader, if any.
    if ((shaderType == gl::ShaderType::Fragment) &&
        programExecutable.hasLinkedShaderStage(gl::ShaderType::Fragment))
    {
        AssignOutputLocations(programExecutable, gl::ShaderType::Fragment, variableInfoMapOut);
    }

    // Assign attributes to the vertex shader, if any.
    if ((shaderType == gl::ShaderType::Vertex) &&
        programExecutable.hasLinkedShaderStage(gl::ShaderType::Vertex))
    {
        AssignAttributeLocations(programExecutable, gl::ShaderType::Vertex, variableInfoMapOut);
    }

    if (!programExecutable.hasLinkedShaderStage(gl::ShaderType::Compute))
    {
        const gl::VaryingPacking &inputPacking  = varyingPacking.getInputPacking(shaderType);
        const gl::VaryingPacking &outputPacking = varyingPacking.getOutputPacking(shaderType);

        // Assign varying locations.
        if (shaderType != gl::ShaderType::Vertex)
        {
            AssignVaryingLocations(options, inputPacking, shaderType, frontShaderType,
                                   programInterfaceInfo, variableInfoMapOut);
        }
        if (shaderType != gl::ShaderType::Fragment)
        {
            AssignVaryingLocations(options, outputPacking, shaderType, frontShaderType,
                                   programInterfaceInfo, variableInfoMapOut);
        }

        if (!programExecutable.getLinkedTransformFeedbackVaryings().empty() &&
            options.supportsTransformFeedbackExtension &&
            (shaderType == programExecutable.getLinkedTransformFeedbackStage()))
        {
            AssignTransformFeedbackExtensionQualifiers(
                programExecutable, outputPacking,
                programInterfaceInfo->locationsUsedForXfbExtension, shaderType, variableInfoMapOut);
        }
    }

    AssignUniformBindings(options, programExecutable, shaderType, programInterfaceInfo,
                          variableInfoMapOut);
    AssignTextureBindings(options, programExecutable, shaderType, programInterfaceInfo,
                          variableInfoMapOut);
    AssignNonTextureBindings(options, programExecutable, shaderType, programInterfaceInfo,
                             variableInfoMapOut);
}

void GlslangGetShaderSource(const GlslangSourceOptions &options,
                            const gl::ProgramState &programState,
                            const gl::ProgramLinkedResources &resources,
                            GlslangProgramInterfaceInfo *programInterfaceInfo,
                            gl::ShaderMap<std::string> *shaderSourcesOut,
                            ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    for (const gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        gl::Shader *glShader            = programState.getAttachedShader(shaderType);
        (*shaderSourcesOut)[shaderType] = glShader ? glShader->getTranslatedSource() : "";
    }

    gl::ShaderType xfbStage = programState.getAttachedTransformFeedbackStage();
    std::string *xfbSource  = &(*shaderSourcesOut)[xfbStage];

    // Write transform feedback output code.
    if (!xfbSource->empty())
    {
        if (!programState.getLinkedTransformFeedbackVaryings().empty())
        {
            if (options.supportsTransformFeedbackExtension)
            {
                GenerateTransformFeedbackExtensionOutputs(
                    programState, resources.varyingPacking.getOutputPacking(xfbStage), xfbSource,
                    &programInterfaceInfo->locationsUsedForXfbExtension);
            }
            else if (options.emulateTransformFeedback)
            {
                ASSERT(xfbStage == gl::ShaderType::Vertex);
                GenerateTransformFeedbackEmulationOutputs(options, xfbStage, programState,
                                                          programInterfaceInfo, xfbSource,
                                                          variableInfoMapOut);
            }
            else
            {
                *xfbSource = SubstituteTransformFeedbackMarkers(*xfbSource, "", "");
            }
        }
        else
        {
            *xfbSource = SubstituteTransformFeedbackMarkers(*xfbSource, "", "");
        }
    }

    std::string *tessEvalSources = &(*shaderSourcesOut)[gl::ShaderType::TessEvaluation];
    if (xfbStage > gl::ShaderType::TessEvaluation && !tessEvalSources->empty())
    {
        *tessEvalSources = SubstituteTransformFeedbackMarkers(*tessEvalSources, "", "");
    }

    std::string *vertexSource = &(*shaderSourcesOut)[gl::ShaderType::Vertex];
    if (xfbStage > gl::ShaderType::Vertex && !vertexSource->empty())
    {
        *vertexSource = SubstituteTransformFeedbackMarkers(*vertexSource, "", "");
    }

    gl::ShaderType frontShaderType = gl::ShaderType::InvalidEnum;

    for (const gl::ShaderType shaderType : programState.getExecutable().getLinkedShaderStages())
    {
        GlslangAssignLocations(options, programState.getExecutable(), resources.varyingPacking,
                               shaderType, frontShaderType, programInterfaceInfo,
                               variableInfoMapOut);

        frontShaderType = shaderType;
    }
}

angle::Result GlslangTransformSpirvCode(const GlslangErrorCallback &callback,
                                        const GlslangSpirvOptions &options,
                                        const ShaderInterfaceVariableInfoMap &variableInfoMap,
                                        const SpirvBlob &initialSpirvBlob,
                                        SpirvBlob *spirvBlobOut)
{
    if (initialSpirvBlob.empty())
    {
        return angle::Result::Continue;
    }

#if defined(ANGLE_DEBUG_SPIRV_TRANSFORMER) && ANGLE_DEBUG_SPIRV_TRANSFORMER
    spvtools::SpirvTools spirvTools(SPV_ENV_VULKAN_1_1);
    spirvTools.SetMessageConsumer(ValidateSpirvMessage);
    std::string readableSpirv;
    spirvTools.Disassemble(initialSpirvBlob, &readableSpirv, 0);
    fprintf(stderr, "%s\n", readableSpirv.c_str());
#endif  // defined(ANGLE_DEBUG_SPIRV_TRANSFORMER) && ANGLE_DEBUG_SPIRV_TRANSFORMER

    // Transform the SPIR-V code by assigning location/set/binding values.
    SpirvTransformer transformer(initialSpirvBlob, options, variableInfoMap, spirvBlobOut);
    ANGLE_GLSLANG_CHECK(callback, transformer.transform(), GlslangError::InvalidSpirv);

    // If there are aliasing vertex attributes, transform the SPIR-V again to remove them.
    if (options.shaderType == gl::ShaderType::Vertex && HasAliasingAttributes(variableInfoMap))
    {
        SpirvBlob preTransformBlob = std::move(*spirvBlobOut);
        SpirvVertexAttributeAliasingTransformer aliasingTransformer(
            preTransformBlob, variableInfoMap, std::move(transformer.getVariableInfoByIdMap()),
            spirvBlobOut);
        ANGLE_GLSLANG_CHECK(callback, aliasingTransformer.transform(), GlslangError::InvalidSpirv);
    }

    ASSERT(ValidateSpirv(*spirvBlobOut));

    return angle::Result::Continue;
}

angle::Result GlslangGetShaderSpirvCode(const GlslangErrorCallback &callback,
                                        const gl::ShaderBitSet &linkedShaderStages,
                                        const gl::Caps &glCaps,
                                        const gl::ShaderMap<std::string> &shaderSources,
                                        gl::ShaderMap<SpirvBlob> *spirvBlobsOut)
{
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

    for (const gl::ShaderType shaderType : linkedShaderStages)
    {
        if (shaderSources[shaderType].empty())
        {
            continue;
        }

        ANGLE_TRY(CompileShader(callback, builtInResources, shaderType, shaderSources[shaderType],
                                shaders[shaderType], &program));
    }

    ANGLE_TRY(LinkProgram(callback, &program));

    for (const gl::ShaderType shaderType : linkedShaderStages)
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

angle::Result GlslangCompileShaderOneOff(const GlslangErrorCallback &callback,
                                         gl::ShaderType shaderType,
                                         const std::string &shaderSource,
                                         SpirvBlob *spirvBlobOut)
{
    const TBuiltInResource builtInResources(glslang::DefaultTBuiltInResource);

    glslang::TShader shader(kShLanguageMap[shaderType]);
    glslang::TProgram program;

    ANGLE_TRY(
        CompileShader(callback, builtInResources, shaderType, shaderSource, &shader, &program));
    ANGLE_TRY(LinkProgram(callback, &program));

    glslang::TIntermediate *intermediate = program.getIntermediate(kShLanguageMap[shaderType]);
    glslang::GlslangToSpv(*intermediate, *spirvBlobOut);

    return angle::Result::Continue;
}
}  // namespace rx
