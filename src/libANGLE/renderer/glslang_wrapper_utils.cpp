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

// SPIR-V tools include for AST validation.
#include <spirv-tools/libspirv.hpp>

#include <array>
#include <numeric>

#include "common/FixedVector.h"
#include "common/spirv/spirv_instruction_builder_autogen.h"
#include "common/spirv/spirv_instruction_parser_autogen.h"
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

namespace spirv = angle::spirv;

namespace rx
{
namespace
{
constexpr char kXfbOutMarker[] = "@@ XFB-OUT @@;";

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
    outBuiltInResources->maxPatchVertices                 = caps.maxPatchVertices;
    outBuiltInResources->maxLights                        = caps.maxLights;
    outBuiltInResources->maxProgramTexelOffset            = caps.maxProgramTexelOffset;
    outBuiltInResources->maxVaryingComponents             = caps.maxVaryingComponents;
    outBuiltInResources->maxVaryingVectors                = caps.maxVaryingVectors;
    outBuiltInResources->maxVertexAttribs                 = caps.maxVertexAttributes;
    outBuiltInResources->maxVertexOutputComponents        = caps.maxVertexOutputComponents;
    outBuiltInResources->maxVertexUniformVectors          = caps.maxVertexUniformVectors;
    outBuiltInResources->maxClipDistances                 = caps.maxClipDistances;
    outBuiltInResources->maxSamples                       = caps.maxSamples;
    outBuiltInResources->maxCullDistances                 = caps.maxCullDistances;
    outBuiltInResources->maxCombinedClipAndCullDistances  = caps.maxCombinedClipAndCullDistances;
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

bool IsRotationIdentity(SurfaceRotation rotation)
{
    return rotation == SurfaceRotation::Identity || rotation == SurfaceRotation::FlippedIdentity;
}

// Test if there are non-zero indices in the uniform name, returning false in that case.  This
// happens for multi-dimensional arrays, where a uniform is created for every possible index of the
// array (except for the innermost dimension).  When assigning decorations (set/binding/etc), only
// the indices corresponding to the first element of the array should be specified.  This function
// is used to skip the other indices.
bool UniformNameIsIndexZero(const std::string &name)
{
    size_t lastBracketClose = 0;

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
                                               const std::string &xfbOut)
{
    const size_t xfbOutMarkerStart = originalSource.find(kXfbOutMarker);
    const size_t xfbOutMarkerEnd   = xfbOutMarkerStart + ConstStrLen(kXfbOutMarker);

    // The shader is the following form:
    //
    // ..part1..
    // @@ XFB-OUT @@;
    // ..part2..
    //
    // Construct the string by concatenating these three pieces, replacing the marker with the given
    // value.
    std::string result;

    result.append(&originalSource[0], &originalSource[xfbOutMarkerStart]);
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
                *xfbOut << sh::vk::kXfbEmulationBufferName << bufferIndex << "."
                        << sh::vk::kXfbEmulationBufferFieldName << "[xfbOffsets[" << bufferIndex
                        << "] + " << offset << "] = " << info.glslAsFloat << "("
                        << varying.mappedName;

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

void AssignTransformFeedbackEmulationBindings(gl::ShaderType shaderType,
                                              const gl::ProgramState &programState,
                                              bool isTransformFeedbackStage,
                                              GlslangProgramInterfaceInfo *programInterfaceInfo,
                                              ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    size_t bufferCount = 0;
    if (isTransformFeedbackStage)
    {
        ASSERT(!programState.getLinkedTransformFeedbackVaryings().empty());
        const bool isInterleaved =
            programState.getTransformFeedbackBufferMode() == GL_INTERLEAVED_ATTRIBS;
        bufferCount = isInterleaved ? 1 : programState.getLinkedTransformFeedbackVaryings().size();
    }

    // Add entries for the transform feedback buffers to the info map, so they can have correct
    // set/binding.
    for (uint32_t bufferIndex = 0; bufferIndex < bufferCount; ++bufferIndex)
    {
        AddResourceInfo(variableInfoMapOut, shaderType, GetXfbBufferName(bufferIndex),
                        programInterfaceInfo->uniformsAndXfbDescriptorSetIndex,
                        programInterfaceInfo->currentUniformBindingIndex);
        ++programInterfaceInfo->currentUniformBindingIndex;
    }

    // Remove inactive transform feedback buffers.
    for (uint32_t bufferIndex = bufferCount;
         bufferIndex < gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS; ++bufferIndex)
    {
        variableInfoMapOut->add(shaderType, GetXfbBufferName(bufferIndex));
    }
}

void AssignTransformFeedbackExtensionLocations(gl::ShaderType shaderType,
                                               const gl::ProgramState &programState,
                                               bool isTransformFeedbackStage,
                                               GlslangProgramInterfaceInfo *programInterfaceInfo,
                                               ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    // The only varying that requires additional resources is gl_Position, as it's indirectly
    // captured through ANGLEXfbPosition.

    const std::vector<gl::TransformFeedbackVarying> &tfVaryings =
        programState.getLinkedTransformFeedbackVaryings();

    bool capturesPosition = false;

    if (isTransformFeedbackStage)
    {
        for (uint32_t varyingIndex = 0; varyingIndex < tfVaryings.size(); ++varyingIndex)
        {
            const gl::TransformFeedbackVarying &tfVarying = tfVaryings[varyingIndex];
            const std::string &tfVaryingName              = tfVarying.mappedName;

            if (tfVaryingName == "gl_Position")
            {
                ASSERT(tfVarying.isBuiltIn());
                capturesPosition = true;
                break;
            }
        }
    }

    if (capturesPosition)
    {
        AddLocationInfo(variableInfoMapOut, shaderType, sh::vk::kXfbExtensionPositionOutName,
                        programInterfaceInfo->locationsUsedForXfbExtension, 0, 0, 0);
        ++programInterfaceInfo->locationsUsedForXfbExtension;
    }
    else
    {
        // Make sure this varying is removed from the other stages, or if position is not captured
        // at all.
        variableInfoMapOut->add(shaderType, sh::vk::kXfbExtensionPositionOutName);
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
        GenerateTransformFeedbackVaryingOutput(varying, info, outputOffset, Str(bufferIndex),
                                               &xfbOut);

        if (isInterleaved)
        {
            outputOffset += info.columnCount * info.rowCount * varying.size();
        }
    }
    xfbOut << "}\n";

    *vertexShader = SubstituteTransformFeedbackMarkers(*vertexShader, xfbOut.str());
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
                                                const gl::ShaderType shaderType,
                                                ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    const std::vector<gl::TransformFeedbackVarying> &tfVaryings =
        programExecutable.getLinkedTransformFeedbackVaryings();
    const std::vector<GLsizei> &varyingStrides = programExecutable.getTransformFeedbackStrides();
    const bool isInterleaved =
        programExecutable.getTransformFeedbackBufferMode() == GL_INTERLEAVED_ATTRIBS;

    uint32_t currentOffset = 0;
    uint32_t currentStride = 0;
    uint32_t bufferIndex   = 0;

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
                SetXfbInfo(variableInfoMapOut, shaderType, sh::vk::kXfbExtensionPositionOutName, -1,
                           bufferIndex, currentOffset, currentStride);
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
                    if (varying->frontVarying.parentStructName == tfVarying.structOrBlockName)
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

        if (gl::SamplerNameContainsNonZeroArrayElement(samplerUniform.name))
        {
            continue;
        }

        if (UniformNameIsIndexZero(samplerUniform.name))
        {
            // Samplers in structs are extracted and renamed.
            const std::string samplerName = GlslangGetMappedSamplerName(samplerUniform.name);

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
    {gl::ShaderType::TessControl, EShLangTessControl},
    {gl::ShaderType::TessEvaluation, EShLangTessEvaluation},
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
    const uint32_t *getCurrentInstruction(spv::Op *opCodeOut, uint32_t *wordCountOut) const;
    void copyInstruction(const uint32_t *instruction, size_t wordCount);
    spirv::IdRef getNewId();

    // SPIR-V to transform:
    const SpirvBlob &mSpirvBlobIn;

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

const uint32_t *SpirvTransformerBase::getCurrentInstruction(spv::Op *opCodeOut,
                                                            uint32_t *wordCountOut) const
{
    ASSERT(mCurrentWord < mSpirvBlobIn.size());
    const uint32_t *instruction = &mSpirvBlobIn[mCurrentWord];

    spirv::GetInstructionOpAndLength(instruction, opCodeOut, wordCountOut);

    // Since glslang succeeded in producing SPIR-V, we assume it to be valid.
    ASSERT(mCurrentWord + *wordCountOut <= mSpirvBlobIn.size());

    return instruction;
}

void SpirvTransformerBase::copyInstruction(const uint32_t *instruction, size_t wordCount)
{
    mSpirvBlobOut->insert(mSpirvBlobOut->end(), instruction, instruction + wordCount);
}

spirv::IdRef SpirvTransformerBase::getNewId()
{
    return spirv::IdRef((*mSpirvBlobOut)[kHeaderIndexIndexBound]++);
}

// A SPIR-V transformer.  It walks the instructions and modifies them as necessary, for example to
// assign bindings or locations.
class SpirvTransformer final : public SpirvTransformerBase
{
  public:
    SpirvTransformer(const SpirvBlob &spirvBlobIn,
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
    void visitTypeArray(const uint32_t *instruction);
    void visitTypeFloat(const uint32_t *instruction);
    void visitTypeInt(const uint32_t *instruction);
    void visitTypePointer(const uint32_t *instruction);
    void visitTypeVector(const uint32_t *instruction);
    void visitVariable(const uint32_t *instruction);

    // Instructions that potentially need transformation.  They return true if the instruction is
    // transformed.  If false is returned, the instruction should be copied as-is.
    bool transformAccessChain(const uint32_t *instruction);
    bool transformCapability(const uint32_t *instruction, size_t wordCount);
    bool transformDebugInfo(const uint32_t *instruction, spv::Op op);
    bool transformEmitVertex(const uint32_t *instruction);
    bool transformEntryPoint(const uint32_t *instruction);
    bool transformDecorate(const uint32_t *instruction);
    bool transformMemberDecorate(const uint32_t *instruction);
    bool transformTypePointer(const uint32_t *instruction);
    bool transformTypeStruct(const uint32_t *instruction);
    bool transformReturn(const uint32_t *instruction);
    bool transformVariable(const uint32_t *instruction);
    bool transformExecutionMode(const uint32_t *instruction);

    // Helpers:
    void visitTypeHelper(spirv::IdResult id, spirv::IdRef typeId);
    void writePendingDeclarations();
    void writeInputPreamble();
    void writeOutputPrologue();
    void preRotateXY(spirv::IdRef xId,
                     spirv::IdRef yId,
                     spirv::IdRef *rotatedXIdOut,
                     spirv::IdRef *rotatedYIdOut);
    void transformZToVulkanClipSpace(spirv::IdRef zId,
                                     spirv::IdRef wId,
                                     spirv::IdRef *correctedZIdOut);
    void writeTransformFeedbackExtensionOutput(spirv::IdRef positionId);

    // Special flags:
    GlslangSpirvOptions mOptions;
    bool mHasTransformFeedbackOutput;

    // Traversal state:
    bool mInsertFunctionVariables = false;
    spirv::IdRef mEntryPointId;
    spirv::IdRef mOpFunctionId;

    // Transformation state:

    // Names associated with ids through OpName.  The same name may be assigned to multiple ids, but
    // not all names are interesting (for example function arguments).  When the variable
    // declaration is met (OpVariable), the variable info is matched with the corresponding id's
    // name based on the Storage Class.
    std::vector<spirv::LiteralString> mNamesById;

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
        spirv::IdRef privateID;
        spirv::IdRef typeID;
    };
    std::vector<TransformedIDs> mTypePointerTransformedId;
    std::vector<spirv::IdRef> mFixedVaryingId;
    std::vector<spirv::IdRef> mFixedVaryingTypeId;

    // gl_PerVertex is unique in that it's the only builtin of struct type.  This struct is pruned
    // by removing trailing inactive members.  We therefore need to keep track of what's its type id
    // as well as which is the last active member.  Note that intermediate stages, i.e. geometry and
    // tessellation have two gl_PerVertex declarations, one for input and one for output.
    struct PerVertexData
    {
        spirv::IdRef typeId;
        uint32_t maxActiveMember;
    };
    PerVertexData mOutputPerVertex;
    PerVertexData mInputPerVertex;

    // Ids needed to generate transform feedback support code.
    spirv::IdRef mTransformFeedbackExtensionPositionId;

    // A handful of ids that are used to generate gl_Position transformation code (for pre-rotation
    // or depth correction).  These IDs are used to load/store gl_Position and apply modifications
    // and swizzles.
    //
    // - mFloatId: id of OpTypeFloat 32
    // - mVec4Id: id of OpTypeVector %mFloatID 4
    // - mVec4OutTypePointerId: id of OpTypePointer Output %mVec4ID
    // - mIntId: id of OpTypeInt 32 1
    // - mInt0Id: id of OpConstant %mIntID 0
    // - mFloatHalfId: id of OpConstant %mFloatId 0.5f
    // - mOutputPerVertexTypePointerId: id of OpTypePointer Output %mOutputPerVertex.typeId
    // - mOutputPerVertexId: id of OpVariable %mOutputPerVertexTypePointerId Output
    //
    spirv::IdRef mFloatId;
    spirv::IdRef mVec4Id;
    spirv::IdRef mVec4OutTypePointerId;
    spirv::IdRef mIntId;
    spirv::IdRef mInt0Id;
    spirv::IdRef mFloatHalfId;
    spirv::IdRef mOutputPerVertexTypePointerId;
    spirv::IdRef mOutputPerVertexId;
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
    mTypePointerTransformedId.resize(indexBound);
    mFixedVaryingId.resize(indexBound);
    mFixedVaryingTypeId.resize(indexBound);

    size_t currentWord = kHeaderIndexInstructions;

    while (currentWord < mSpirvBlobIn.size())
    {
        const uint32_t *instruction = &mSpirvBlobIn[currentWord];

        uint32_t wordCount;
        spv::Op opCode;
        spirv::GetInstructionOpAndLength(instruction, &opCode, &wordCount);

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
            case spv::OpTypeFloat:
                visitTypeFloat(instruction);
                break;
            case spv::OpTypeInt:
                visitTypeInt(instruction);
                break;
            case spv::OpTypePointer:
                visitTypePointer(instruction);
                break;
            case spv::OpTypeVector:
                visitTypeVector(instruction);
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
    spv::Op opCode;
    const uint32_t *instruction = getCurrentInstruction(&opCode, &wordCount);

    if (opCode == spv::OpFunction)
    {
        spirv::IdResultType id;
        spv::FunctionControlMask functionControl;
        spirv::IdRef functionType;
        spirv::ParseFunction(instruction, &id, &mOpFunctionId, &functionControl, &functionType);

        // SPIR-V is structured in sections.  Function declarations come last.  Only a few
        // instructions such as Op*Access* or OpEmitVertex opcodes inside functions need to be
        // inspected.
        //
        // If this is the first OpFunction instruction, this is also where the declaration section
        // finishes, so we need to declare anything that we need but didn't find there already right
        // now.
        if (!mIsInFunctionSection)
        {
            writePendingDeclarations();
        }
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
                transformed = transformAccessChain(instruction);
                break;

            case spv::OpEmitVertex:
                transformed = transformEmitVertex(instruction);
                break;
            case spv::OpReturn:
                transformed = transformReturn(instruction);
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
            case spv::OpName:
            case spv::OpMemberName:
            case spv::OpString:
            case spv::OpLine:
            case spv::OpNoLine:
            case spv::OpModuleProcessed:
                transformed = transformDebugInfo(instruction, opCode);
                break;
            case spv::OpCapability:
                transformed = transformCapability(instruction, wordCount);
                break;
            case spv::OpEntryPoint:
                transformed = transformEntryPoint(instruction);
                break;
            case spv::OpDecorate:
                transformed = transformDecorate(instruction);
                break;
            case spv::OpMemberDecorate:
                transformed = transformMemberDecorate(instruction);
                break;
            case spv::OpTypePointer:
                transformed = transformTypePointer(instruction);
                break;
            case spv::OpTypeStruct:
                transformed = transformTypeStruct(instruction);
                break;
            case spv::OpVariable:
                transformed = transformVariable(instruction);
                break;
            case spv::OpExecutionMode:
                transformed = transformExecutionMode(instruction);
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

// Called at the end of the declarations section.  Any declarations that are necessary but weren't
// present in the original shader need to be done here.
void SpirvTransformer::writePendingDeclarations()
{
    // Pre-rotation and transformation of depth to Vulkan clip space require declarations that may
    // not necessarily be in the shader.
    if (IsRotationIdentity(mOptions.preRotation) && !mOptions.transformPositionToVulkanClipSpace)
    {
        return;
    }

    if (!mFloatId.valid())
    {
        mFloatId = getNewId();
        spirv::WriteTypeFloat(mSpirvBlobOut, mFloatId, spirv::LiteralInteger(32));
    }

    if (!mVec4Id.valid())
    {
        mVec4Id = getNewId();
        spirv::WriteTypeVector(mSpirvBlobOut, mVec4Id, mFloatId, spirv::LiteralInteger(4));
    }

    if (!mVec4OutTypePointerId.valid())
    {
        mVec4OutTypePointerId = getNewId();
        spirv::WriteTypePointer(mSpirvBlobOut, mVec4OutTypePointerId, spv::StorageClassOutput,
                                mVec4Id);
    }

    if (!mIntId.valid())
    {
        mIntId = getNewId();
        spirv::WriteTypeInt(mSpirvBlobOut, mIntId, spirv::LiteralInteger(32),
                            spirv::LiteralInteger(1));
    }

    ASSERT(!mInt0Id.valid());
    mInt0Id = getNewId();
    spirv::WriteConstant(mSpirvBlobOut, mIntId, mInt0Id, spirv::LiteralContextDependentNumber(0));

    constexpr uint32_t kFloatHalfAsUint = 0x3F00'0000;

    ASSERT(!mFloatHalfId.valid());
    mFloatHalfId = getNewId();
    spirv::WriteConstant(mSpirvBlobOut, mFloatId, mFloatHalfId,
                         spirv::LiteralContextDependentNumber(kFloatHalfAsUint));
}

// Called by transformInstruction to insert necessary instructions for casting varyings.
void SpirvTransformer::writeInputPreamble()
{
    if (mOptions.shaderType == gl::ShaderType::Vertex ||
        mOptions.shaderType == gl::ShaderType::Compute)
    {
        return;
    }

    // Copy from corrected varyings to temp global variables with original precision.
    for (uint32_t idIndex = 0; idIndex < mVariableInfoById.size(); idIndex++)
    {
        const spirv::IdRef id(idIndex);
        const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];
        if (info && info->useRelaxedPrecision && info->activeStages[mOptions.shaderType] &&
            info->varyingIsInput)
        {
            // This is an input varying, need to cast the mediump value that came from
            // the previous stage into a highp value that the code wants to work with.
            ASSERT(mFixedVaryingTypeId[id].valid());

            // Build OpLoad instruction to load the mediump value into a temporary
            const spirv::IdRef tempVar(getNewId());
            const spirv::IdRef tempVarType(
                mTypePointerTransformedId[mFixedVaryingTypeId[id]].typeID);
            ASSERT(tempVarType.valid());

            spirv::WriteLoad(mSpirvBlobOut, tempVarType, tempVar, mFixedVaryingId[id], nullptr);

            // Build OpStore instruction to cast the mediump value to highp for use in
            // the function
            spirv::WriteStore(mSpirvBlobOut, id, tempVar, nullptr);
        }
    }
}

// Called by transformInstruction to insert necessary instructions for casting varyings and
// modifying gl_Position.
void SpirvTransformer::writeOutputPrologue()
{
    if (mOptions.shaderType == gl::ShaderType::Fragment ||
        mOptions.shaderType == gl::ShaderType::Compute)
    {
        return;
    }

    // Copy from temp global variables with original precision to corrected varyings.
    for (uint32_t idIndex = 0; idIndex < mVariableInfoById.size(); idIndex++)
    {
        const spirv::IdRef id(idIndex);
        const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];
        if (info && info->useRelaxedPrecision && info->activeStages[mOptions.shaderType] &&
            info->varyingIsOutput)
        {
            ASSERT(mFixedVaryingTypeId[id].valid());

            // Build OpLoad instruction to load the highp value into a temporary
            const spirv::IdRef tempVar(getNewId());
            const spirv::IdRef tempVarType(
                mTypePointerTransformedId[mFixedVaryingTypeId[id]].typeID);
            ASSERT(tempVarType.valid());

            spirv::WriteLoad(mSpirvBlobOut, tempVarType, tempVar, id, nullptr);

            // Build OpStore instruction to cast the highp value to mediump for output
            spirv::WriteStore(mSpirvBlobOut, mFixedVaryingId[id], tempVar, nullptr);
        }
    }

    // Transform gl_Position to account for prerotation and Vulkan clip space if necessary.
    if (!mOutputPerVertexId.valid() ||
        (IsRotationIdentity(mOptions.preRotation) && !mOptions.transformPositionToVulkanClipSpace))
    {
        return;
    }

    // In GL the viewport transformation is slightly different - see the GL 2.0 spec section "2.12.1
    // Controlling the Viewport".  In Vulkan the corresponding spec section is currently "23.4.
    // Coordinate Transformations".  The following transformation needs to be done:
    //
    //     z_vk = 0.5 * (w_gl + z_gl)
    //
    // where z_vk is the depth output of a Vulkan geometry-stage shader and z_gl is the same for GL.

    // Generate the following SPIR-V for prerotation and depth transformation:
    //
    //     // Create an access chain to output gl_PerVertex.gl_Position, which is always at index 0.
    //     %PositionPointer = OpAccessChain %mVec4OutTypePointerId %mOutputPerVertexId %mInt0Id
    //     // Load gl_Position
    //     %Position = OpLoad %mVec4Id %PositionPointer
    //     // Create gl_Position.x and gl_Position.y for transformation, as well as gl_Position.z
    //     // and gl_Position.w for later.
    //     %x = OpCompositeExtract %mFloatId %Position 0
    //     %y = OpCompositeExtract %mFloatId %Position 1
    //     %z = OpCompositeExtract %mFloatId %Position 2
    //     %w = OpCompositeExtract %mFloatId %Position 3
    //
    //     // Transform %x and %y based on pre-rotation.  This could include swapping the two ids
    //     // (in the transformer, no need to generate SPIR-V instructions for that), and/or
    //     // negating either component.  To negate a component, the following instruction is used:
    //     (optional:) %negated = OpFNegate %mFloatId %component
    //
    //     // Transform %z if necessary, based on the above formula.
    //     %zPlusW = OpFAdd %mFloatId %z %w
    //     %correctedZ = OpFMul %mFloatId %zPlusW %mFloatHalfId
    //
    //     // Create the rotated gl_Position from the rotated x and y and corrected z components.
    //     %RotatedPosition = OpCompositeConstruct %mVec4Id %rotatedX %rotatedY %correctedZ %w
    //     // Store the results back in gl_Position
    //     OpStore %PositionPointer %RotatedPosition
    //
    const spirv::IdRef positionPointerId(getNewId());
    const spirv::IdRef positionId(getNewId());
    const spirv::IdRef xId(getNewId());
    const spirv::IdRef yId(getNewId());
    const spirv::IdRef zId(getNewId());
    const spirv::IdRef wId(getNewId());
    const spirv::IdRef rotatedPositionId(getNewId());

    spirv::WriteAccessChain(mSpirvBlobOut, mVec4OutTypePointerId, positionPointerId,
                            mOutputPerVertexId, {mInt0Id});
    spirv::WriteLoad(mSpirvBlobOut, mVec4Id, positionId, positionPointerId, nullptr);

    if (mOptions.isTransformFeedbackStage && mTransformFeedbackExtensionPositionId.valid())
    {
        writeTransformFeedbackExtensionOutput(positionId);
    }

    spirv::WriteCompositeExtract(mSpirvBlobOut, mFloatId, xId, positionId,
                                 {spirv::LiteralInteger{0}});
    spirv::WriteCompositeExtract(mSpirvBlobOut, mFloatId, yId, positionId,
                                 {spirv::LiteralInteger{1}});
    spirv::WriteCompositeExtract(mSpirvBlobOut, mFloatId, zId, positionId,
                                 {spirv::LiteralInteger{2}});
    spirv::WriteCompositeExtract(mSpirvBlobOut, mFloatId, wId, positionId,
                                 {spirv::LiteralInteger{3}});

    spirv::IdRef rotatedXId;
    spirv::IdRef rotatedYId;
    preRotateXY(xId, yId, &rotatedXId, &rotatedYId);

    spirv::IdRef correctedZId;
    transformZToVulkanClipSpace(zId, wId, &correctedZId);

    spirv::WriteCompositeConstruct(mSpirvBlobOut, mVec4Id, rotatedPositionId,
                                   {rotatedXId, rotatedYId, correctedZId, wId});
    spirv::WriteStore(mSpirvBlobOut, positionPointerId, rotatedPositionId, nullptr);
}

void SpirvTransformer::preRotateXY(spirv::IdRef xId,
                                   spirv::IdRef yId,
                                   spirv::IdRef *rotatedXIdOut,
                                   spirv::IdRef *rotatedYIdOut)
{
    switch (mOptions.preRotation)
    {
        case SurfaceRotation::Identity:
        case SurfaceRotation::FlippedIdentity:
            // [ 1  0]   [x]
            // [ 0  1] * [y]
            *rotatedXIdOut = xId;
            *rotatedYIdOut = yId;
            break;
        case SurfaceRotation::Rotated90Degrees:
        case SurfaceRotation::FlippedRotated90Degrees:
            // [ 0  1]   [x]
            // [-1  0] * [y]
            *rotatedXIdOut = yId;
            *rotatedYIdOut = getNewId();
            spirv::WriteFNegate(mSpirvBlobOut, mFloatId, *rotatedYIdOut, xId);
            break;
        case SurfaceRotation::Rotated180Degrees:
        case SurfaceRotation::FlippedRotated180Degrees:
            // [-1  0]   [x]
            // [ 0 -1] * [y]
            *rotatedXIdOut = getNewId();
            *rotatedYIdOut = getNewId();
            spirv::WriteFNegate(mSpirvBlobOut, mFloatId, *rotatedXIdOut, xId);
            spirv::WriteFNegate(mSpirvBlobOut, mFloatId, *rotatedYIdOut, yId);
            break;
        case SurfaceRotation::Rotated270Degrees:
        case SurfaceRotation::FlippedRotated270Degrees:
            // [ 0 -1]   [x]
            // [ 1  0] * [y]
            *rotatedXIdOut = getNewId();
            *rotatedYIdOut = xId;
            spirv::WriteFNegate(mSpirvBlobOut, mFloatId, *rotatedXIdOut, yId);
            break;
        default:
            UNREACHABLE();
    }
}

void SpirvTransformer::transformZToVulkanClipSpace(spirv::IdRef zId,
                                                   spirv::IdRef wId,
                                                   spirv::IdRef *correctedZIdOut)
{
    if (!mOptions.transformPositionToVulkanClipSpace)
    {
        *correctedZIdOut = zId;
        return;
    }

    const spirv::IdRef zPlusWId(getNewId());
    *correctedZIdOut = getNewId();

    // %zPlusW = OpFAdd %mFloatId %z %w
    spirv::WriteFAdd(mSpirvBlobOut, mFloatId, zPlusWId, zId, wId);

    // %correctedZ = OpFMul %mFloatId %zPlusW %mFloatHalfId
    spirv::WriteFMul(mSpirvBlobOut, mFloatId, *correctedZIdOut, zPlusWId, mFloatHalfId);
}

void SpirvTransformer::writeTransformFeedbackExtensionOutput(spirv::IdRef positionId)
{
    spirv::WriteStore(mSpirvBlobOut, mTransformFeedbackExtensionPositionId, positionId, nullptr);
}

void SpirvTransformer::visitDecorate(const uint32_t *instruction)
{
    spirv::IdRef id;
    spv::Decoration decoration;
    spirv::ParseDecorate(instruction, &id, &decoration, nullptr);

    if (decoration == spv::DecorationBlock)
    {
        mIsIOBlockById[id] = true;

        // For I/O blocks, associate the type with the info, which is used to decorate its members
        // with transform feedback if any.
        spirv::LiteralString name = mNamesById[id];
        ASSERT(name != nullptr);

        const ShaderInterfaceVariableInfo &info = mVariableInfoMap.get(mOptions.shaderType, name);
        mVariableInfoById[id]                   = &info;
    }
}

void SpirvTransformer::visitName(const uint32_t *instruction)
{
    spirv::IdRef id;
    spirv::LiteralString name;
    spirv::ParseName(instruction, &id, &name);

    // The names and ids are unique
    ASSERT(id < mNamesById.size());
    ASSERT(mNamesById[id] == nullptr);

    mNamesById[id] = name;
}

void SpirvTransformer::visitMemberName(const uint32_t *instruction)
{
    spirv::IdRef id;
    spirv::LiteralInteger member;
    spirv::LiteralString name;
    spirv::ParseMemberName(instruction, &id, &member, &name);

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
    if (!mOutputPerVertex.typeId.valid() || id == mOutputPerVertex.typeId)
    {
        mOutputPerVertex.typeId = id;

        // Keep track of the range of members that are active.
        if (info.varyingIsOutput && member > mOutputPerVertex.maxActiveMember)
        {
            mOutputPerVertex.maxActiveMember = member;
        }
    }
    else if (!mInputPerVertex.typeId.valid() || id == mInputPerVertex.typeId)
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

void SpirvTransformer::visitTypeHelper(spirv::IdResult id, spirv::IdRef typeId)
{
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
    spirv::IdResult id;
    spirv::IdRef elementType;
    spirv::IdRef length;
    spirv::ParseTypeArray(instruction, &id, &elementType, &length);

    visitTypeHelper(id, elementType);
}

void SpirvTransformer::visitTypeFloat(const uint32_t *instruction)
{
    spirv::IdResult id;
    spirv::LiteralInteger width;
    spirv::ParseTypeFloat(instruction, &id, &width);

    // Only interested in OpTypeFloat 32.
    if (width == 32)
    {
        ASSERT(!mFloatId.valid());
        mFloatId = id;
    }
}

void SpirvTransformer::visitTypeInt(const uint32_t *instruction)
{
    spirv::IdResult id;
    spirv::LiteralInteger width;
    spirv::LiteralInteger signedness;
    spirv::ParseTypeInt(instruction, &id, &width, &signedness);

    // Only interested in OpTypeInt 32 1.
    if (width == 32 && signedness == 1)
    {
        ASSERT(!mIntId.valid());
        mIntId = id;
    }
}

void SpirvTransformer::visitTypePointer(const uint32_t *instruction)
{
    spirv::IdResult id;
    spv::StorageClass storageClass;
    spirv::IdRef typeId;
    spirv::ParseTypePointer(instruction, &id, &storageClass, &typeId);

    visitTypeHelper(id, typeId);

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

        // Remember type pointer of output gl_PerVertex for gl_Position transformations.
        if (storageClass == spv::StorageClassOutput)
        {
            mOutputPerVertexTypePointerId = id;
        }
    }

    // If OpTypePointer Output %mVec4ID was encountered, remember that.  Otherwise we'll have to
    // generate one.
    if (typeId == mVec4Id && storageClass == spv::StorageClassOutput)
    {
        mVec4OutTypePointerId = id;
    }
}

void SpirvTransformer::visitTypeVector(const uint32_t *instruction)
{
    spirv::IdResult id;
    spirv::IdRef componentId;
    spirv::LiteralInteger componentCount;
    spirv::ParseTypeVector(instruction, &id, &componentId, &componentCount);

    // Only interested in OpTypeVector %mFloatId 4
    if (componentId == mFloatId && componentCount == 4)
    {
        ASSERT(!mVec4Id.valid());
        mVec4Id = id;
    }
}

void SpirvTransformer::visitVariable(const uint32_t *instruction)
{
    spirv::IdResultType typeId;
    spirv::IdResult id;
    spv::StorageClass storageClass;
    spirv::ParseVariable(instruction, &typeId, &id, &storageClass, nullptr);

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
    const bool isIOBlock      = mIsIOBlockById[typeId];
    spirv::LiteralString name = mNamesById[isInterfaceBlockVariable || isIOBlock ? typeId : id];

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

    if (typeId == mOutputPerVertexTypePointerId)
    {
        // If this is the output gl_PerVertex variable, remember its id for gl_Position
        // transformations.
        ASSERT(storageClass == spv::StorageClassOutput && isIOBlock &&
               strcmp(name, "gl_PerVertex") == 0);
        mOutputPerVertexId = id;
    }

    // Every shader interface variable should have an associated data.
    const ShaderInterfaceVariableInfo &info = mVariableInfoMap.get(mOptions.shaderType, name);

    // Associate the id of this name with its info.
    mVariableInfoById[id] = &info;

    if (info.useRelaxedPrecision && info.activeStages[mOptions.shaderType] &&
        !mFixedVaryingId[id].valid())
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

        // If this is the special ANGLEXfbPosition variable, remember its id to be used for the
        // ANGLEXfbPosition = gl_Position; assignment code generation.
        if (strcmp(name, sh::vk::kXfbExtensionPositionOutName) == 0)
        {
            mTransformFeedbackExtensionPositionId = id;
        }
    }
}

bool SpirvTransformer::transformDecorate(const uint32_t *instruction)
{
    spirv::IdRef id;
    spv::Decoration decoration;
    spirv::LiteralIntegerList decorationValues;
    spirv::ParseDecorate(instruction, &id, &decoration, &decorationValues);

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

    // If using relaxed precision, generate instructions for the replacement id instead.
    if (info->useRelaxedPrecision)
    {
        ASSERT(mFixedVaryingId[id].valid());
        id = mFixedVaryingId[id];
    }

    // If this is the Block decoration of a shader I/O block, add the transform feedback decorations
    // to its members right away.
    constexpr size_t kXfbDecorationCount                           = 3;
    constexpr spv::Decoration kXfbDecorations[kXfbDecorationCount] = {
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
                spirv::WriteMemberDecorate(mSpirvBlobOut, id, spirv::LiteralInteger(fieldIndex),
                                           kXfbDecorations[i],
                                           {spirv::LiteralInteger(xfbDecorationValues[i])});
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
                spirv::WriteDecorate(mSpirvBlobOut, id, decoration, decorationValues);
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

    // Modify the decoration value.
    ASSERT(decorationValues.size() == 1);
    spirv::WriteDecorate(mSpirvBlobOut, id, decoration,
                         {spirv::LiteralInteger(newDecorationValue)});

    // If there are decorations to be added, add them right after the Location decoration is
    // encountered.
    if (decoration != spv::DecorationLocation)
    {
        return true;
    }

    // If any, the replacement variable is always reduced precision so add that decoration to
    // fixedVaryingId.
    if (info->useRelaxedPrecision)
    {
        spirv::WriteDecorate(mSpirvBlobOut, id, spv::DecorationRelaxedPrecision, {});
    }

    // Add component decoration, if any.
    if (info->component != ShaderInterfaceVariableInfo::kInvalid)
    {
        spirv::WriteDecorate(mSpirvBlobOut, id, spv::DecorationComponent,
                             {spirv::LiteralInteger(info->component)});
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
            spirv::WriteDecorate(mSpirvBlobOut, id, kXfbDecorations[i],
                                 {spirv::LiteralInteger(xfbDecorationValues[i])});
        }
    }

    return true;
}

bool SpirvTransformer::transformMemberDecorate(const uint32_t *instruction)
{
    spirv::IdRef typeId;
    spirv::LiteralInteger member;
    spv::Decoration decoration;
    spirv::ParseMemberDecorate(instruction, &typeId, &member, &decoration, nullptr);

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

    spv::Capability capability;
    spirv::ParseCapability(instruction, &capability);

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

    // Write the TransformFeedback capability declaration.
    spirv::WriteCapability(mSpirvBlobOut, spv::CapabilityTransformFeedback);

    return true;
}

bool SpirvTransformer::transformDebugInfo(const uint32_t *instruction, spv::Op op)
{
    if (mOptions.removeDebugInfo)
    {
        // Strip debug info to reduce binary size.
        return true;
    }

    // In the case of OpMemberName, unconditionally remove stripped gl_PerVertex members.
    if (op == spv::OpMemberName)
    {
        spirv::IdRef id;
        spirv::LiteralInteger member;
        spirv::LiteralString name;
        spirv::ParseMemberName(instruction, &id, &member, &name);

        // Remove the instruction if it's a stripped member of gl_PerVertex.
        return (id == mOutputPerVertex.typeId && member > mOutputPerVertex.maxActiveMember) ||
               (id == mInputPerVertex.typeId && member > mInputPerVertex.maxActiveMember);
    }

    // In the case of ANGLEXfbN, unconditionally remove the variable names.  If transform
    // feedback is not active, the corresponding variables will be removed.
    if (op == spv::OpName)
    {
        spirv::IdRef id;
        spirv::LiteralString name;
        spirv::ParseName(instruction, &id, &name);

        // SPIR-V 1.0 Section 3.32 Instructions, OpName
        if (angle::BeginsWith(name, sh::vk::kXfbEmulationBufferName))
        {
            return true;
        }
    }

    return false;
}

bool SpirvTransformer::transformEmitVertex(const uint32_t *instruction)
{
    // This is only possible in geometry shaders.
    ASSERT(mOptions.shaderType == gl::ShaderType::Geometry);

    // Write the temporary variables that hold varyings data before EmitVertex().
    writeOutputPrologue();

    return false;
}

bool SpirvTransformer::transformEntryPoint(const uint32_t *instruction)
{
    // Should only have one EntryPoint
    ASSERT(!mEntryPointId.valid());

    // Remove inactive varyings from the shader interface declaration.
    spv::ExecutionModel executionModel;
    spirv::LiteralString name;
    spirv::IdRefList interfaceList;
    spirv::ParseEntryPoint(instruction, &executionModel, &mEntryPointId, &name, &interfaceList);

    // Filter out inactive varyings from entry point interface declaration.
    size_t writeIndex = 0;
    for (size_t index = 0; index < interfaceList.size(); ++index)
    {
        spirv::IdRef id(interfaceList[index]);
        const ShaderInterfaceVariableInfo *info = mVariableInfoById[id];

        ASSERT(info);

        if (!info->activeStages[mOptions.shaderType])
        {
            continue;
        }

        // If ID is one we had to replace due to varying mismatch, use the fixed ID.
        if (mFixedVaryingId[id].valid())
        {
            id = mFixedVaryingId[id];
        }
        interfaceList[writeIndex] = id;
        ++writeIndex;
    }

    // Update the number of interface variables.
    interfaceList.resize(writeIndex);

    // Write the entry point with the inactive interface variables removed.
    spirv::WriteEntryPoint(mSpirvBlobOut, executionModel, mEntryPointId, name, interfaceList);

    // Add an OpExecutionMode Xfb instruction if necessary.
    if (mHasTransformFeedbackOutput)
    {
        spirv::WriteExecutionMode(mSpirvBlobOut, mEntryPointId, spv::ExecutionModeXfb);
    }

    return true;
}

bool SpirvTransformer::transformTypePointer(const uint32_t *instruction)
{
    spirv::IdResult id;
    spv::StorageClass storageClass;
    spirv::IdRef typeId;
    spirv::ParseTypePointer(instruction, &id, &storageClass, &typeId);

    // If the storage class is output, this may be used to create a variable corresponding to an
    // inactive varying, or if that varying is a struct, an Op*AccessChain retrieving a field of
    // that inactive varying.
    //
    // SPIR-V specifies the storage class both on the type and the variable declaration.  Otherwise
    // it would have been sufficient to modify the OpVariable instruction. For simplicity, duplicate
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

    const spirv::IdRef newPrivateTypeId(getNewId());

    // Write OpTypePointer for the new PrivateType.
    spirv::WriteTypePointer(mSpirvBlobOut, newPrivateTypeId, spv::StorageClassPrivate, typeId);

    // Remember the id of the replacement.
    ASSERT(id < mTypePointerTransformedId.size());
    mTypePointerTransformedId[id].privateID = newPrivateTypeId;

    // The original instruction should still be present as well.  At this point, we don't know
    // whether we will need the original or Private type.
    return false;
}

bool SpirvTransformer::transformTypeStruct(const uint32_t *instruction)
{
    spirv::IdResult id;
    spirv::IdRefList memberList;
    ParseTypeStruct(instruction, &id, &memberList);

    if (id != mOutputPerVertex.typeId && id != mInputPerVertex.typeId)
    {
        return false;
    }

    const uint32_t maxMembers = id == mOutputPerVertex.typeId ? mOutputPerVertex.maxActiveMember
                                                              : mInputPerVertex.maxActiveMember;

    // Change the definition of the gl_PerVertex struct by stripping unused fields at the end.
    const uint32_t memberCount = maxMembers + 1;
    memberList.resize(memberCount);

    spirv::WriteTypeStruct(mSpirvBlobOut, id, memberList);

    return true;
}

bool SpirvTransformer::transformReturn(const uint32_t *instruction)
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

bool SpirvTransformer::transformVariable(const uint32_t *instruction)
{
    spirv::IdResultType typeId;
    spirv::IdResult id;
    spv::StorageClass storageClass;
    spirv::ParseVariable(instruction, &typeId, &id, &storageClass, nullptr);

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
            ASSERT(mFixedVaryingId[id].valid());
            spirv::WriteVariable(mSpirvBlobOut, typeId, mFixedVaryingId[id], storageClass, nullptr);

            // Make original variable a private global
            ASSERT(mTypePointerTransformedId[typeId].privateID.valid());
            spirv::WriteVariable(mSpirvBlobOut, mTypePointerTransformedId[typeId].privateID, id,
                                 spv::StorageClassPrivate, nullptr);

            return true;
        }
        return false;
    }

    if (mOptions.shaderType == gl::ShaderType::Vertex && storageClass == spv::StorageClassUniform)
    {
        // Exceptionally, the ANGLEXfbN variables are unconditionally generated and may be inactive.
        // Remove these variables in that case.
        ASSERT(info == &mVariableInfoMap.get(mOptions.shaderType, GetXfbBufferName(0)) ||
               info == &mVariableInfoMap.get(mOptions.shaderType, GetXfbBufferName(1)) ||
               info == &mVariableInfoMap.get(mOptions.shaderType, GetXfbBufferName(2)) ||
               info == &mVariableInfoMap.get(mOptions.shaderType, GetXfbBufferName(3)));

        // Drop the declaration.
        return true;
    }

    ASSERT(storageClass == spv::StorageClassOutput || storageClass == spv::StorageClassInput);

    // The variable is inactive.  Output a modified variable declaration, where the type is the
    // corresponding type with the Private storage class.
    ASSERT(typeId < mTypePointerTransformedId.size());
    ASSERT(mTypePointerTransformedId[typeId].privateID.valid());

    spirv::WriteVariable(mSpirvBlobOut, mTypePointerTransformedId[typeId].privateID, id,
                         spv::StorageClassPrivate, nullptr);

    return true;
}

bool SpirvTransformer::transformAccessChain(const uint32_t *instruction)
{
    spirv::IdResultType typeId;
    spirv::IdResult id;
    spirv::IdRef baseId;
    spirv::IdRefList indexList;
    spirv::ParseAccessChain(instruction, &typeId, &id, &baseId, &indexList);

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

    // Modifiy the instruction to use the private type.
    ASSERT(typeId < mTypePointerTransformedId.size());
    ASSERT(mTypePointerTransformedId[typeId].privateID.valid());

    spirv::WriteAccessChain(mSpirvBlobOut, mTypePointerTransformedId[typeId].privateID, id, baseId,
                            indexList);

    return true;
}

bool SpirvTransformer::transformExecutionMode(const uint32_t *instruction)
{
    spirv::IdRef entryPoint;
    spv::ExecutionMode mode;
    spirv::ParseExecutionMode(instruction, &entryPoint, &mode);

    if (mode == spv::ExecutionModeEarlyFragmentTests &&
        mOptions.removeEarlyFragmentTestsOptimization)
    {
        // Drop the instruction.
        return true;
    }
    return false;
}

struct AliasingAttributeMap
{
    // The SPIR-V id of the aliasing attribute with the most components.  This attribute will be
    // used to read from this location instead of every aliasing one.
    spirv::IdRef attribute;

    // SPIR-V ids of aliasing attributes.
    std::vector<spirv::IdRef> aliasingAttributes;
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
        const SpirvBlob &spirvBlobIn,
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
    spirv::IdRef getAliasingAttributeReplacementId(spirv::IdRef aliasingId, uint32_t offset) const;
    bool isMatrixAttribute(spirv::IdRef id) const;

    // Instructions that are purely informational:
    void visitTypeFloat(const uint32_t *instruction);
    void visitTypeVector(const uint32_t *instruction);
    void visitTypeMatrix(const uint32_t *instruction);
    void visitTypePointer(const uint32_t *instruction);

    // Instructions that potentially need transformation.  They return true if the instruction is
    // transformed.  If false is returned, the instruction should be copied as-is.
    bool transformEntryPoint(const uint32_t *instruction);
    bool transformName(const uint32_t *instruction);
    bool transformDecorate(const uint32_t *instruction);
    bool transformVariable(const uint32_t *instruction);
    bool transformAccessChain(const uint32_t *instruction);
    void transformLoadHelper(spirv::IdRef pointerId,
                             spirv::IdRef typeId,
                             spirv::IdRef replacementId,
                             spirv::IdRef resultId);
    bool transformLoad(const uint32_t *instruction);

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
    std::vector<spirv::IdRef> mExpandedMatrixFirstVectorIdById;
    // Whether the expanded matrix OpVariables are generated.
    bool mHaveMatricesExpanded = false;
    // Whether initialization of the matrix attributes should be written at the beginning of the
    // current function.
    bool mWriteExpandedMatrixInitialization = false;
    spirv::IdRef mEntryPointId;

    // Id of attribute types; float and veci.  This array is one-based, and [0] is unused.
    //
    // [1]: id of OpTypeFloat 32
    // [N]: id of OpTypeVector %[1] N, N = {2, 3, 4}
    //
    // In other words, index of the array corresponds to the number of components in the type.
    std::array<spirv::IdRef, 5> mFloatTypes;

    // Corresponding to mFloatTypes, [i]: id of OpMatrix %mFloatTypes[i] i.  Note that only square
    // matrices are possible as attributes in GLSL ES 1.00.  [0] and [1] are unused.
    std::array<spirv::IdRef, 5> mMatrixTypes;

    // Corresponding to mFloatTypes, [i]: id of OpTypePointer Input %mFloatTypes[i].  [0] is unused.
    std::array<spirv::IdRef, 5> mInputTypePointers;

    // Corresponding to mFloatTypes, [i]: id of OpTypePointer Private %mFloatTypes[i].  [0] is
    // unused.
    std::array<spirv::IdRef, 5> mPrivateFloatTypePointers;

    // Corresponding to mMatrixTypes, [i]: id of OpTypePointer Private %mMatrixTypes[i].  [0] and
    // [1] are unused.
    std::array<spirv::IdRef, 5> mPrivateMatrixTypePointers;
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
    mExpandedMatrixFirstVectorIdById.resize(indexBound);

    // Go through attributes and find out which alias which.
    for (size_t idIndex = 0; idIndex < indexBound; ++idIndex)
    {
        const spirv::IdRef id(idIndex);

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

            spirv::IdRef attributeId(id);

            // If this is a matrix attribute, expand it to vectors.
            if (isMatrixAttribute)
            {
                const spirv::IdRef matrixId(id);

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
            if (!aliasingMap->attribute.valid())
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

            spirv::IdRef aliasingId;
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
            ASSERT(!mIsAliasingAttributeById[aliasingId]);
            mIsAliasingAttributeById[aliasingId] = true;
        }
    }
}

void SpirvVertexAttributeAliasingTransformer::transformInstruction()
{
    uint32_t wordCount;
    spv::Op opCode;
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

        spirv::IdResultType id;
        spirv::IdResult functionId;
        spv::FunctionControlMask functionControl;
        spirv::IdRef functionType;
        spirv::ParseFunction(instruction, &id, &functionId, &functionControl, &functionType);

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
                transformed = transformAccessChain(instruction);
                break;
            case spv::OpLoad:
                transformed = transformLoad(instruction);
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
                transformed = transformEntryPoint(instruction);
                break;
            case spv::OpName:
                transformed = transformName(instruction);
                break;
            case spv::OpDecorate:
                transformed = transformDecorate(instruction);
                break;
            case spv::OpVariable:
                transformed = transformVariable(instruction);
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

spirv::IdRef SpirvVertexAttributeAliasingTransformer::getAliasingAttributeReplacementId(
    spirv::IdRef aliasingId,
    uint32_t offset) const
{
    // Get variable info corresponding to the aliasing attribute.
    const ShaderInterfaceVariableInfo *aliasingInfo = mVariableInfoById[aliasingId];
    ValidateShaderInterfaceVariableIsAttribute(aliasingInfo);

    // Find the replacement attribute.
    const AliasingAttributeMap *aliasingMap =
        &mAliasingAttributeMap[aliasingInfo->location + offset];
    ValidateIsAliasingAttribute(aliasingMap, aliasingId);

    const spirv::IdRef replacementId(aliasingMap->attribute);
    ASSERT(replacementId.valid() && replacementId < mIsAliasingAttributeById.size());
    ASSERT(!mIsAliasingAttributeById[replacementId]);

    return replacementId;
}

bool SpirvVertexAttributeAliasingTransformer::isMatrixAttribute(spirv::IdRef id) const
{
    return mExpandedMatrixFirstVectorIdById[id].valid();
}

void SpirvVertexAttributeAliasingTransformer::visitTypeFloat(const uint32_t *instruction)
{
    spirv::IdResult id;
    spirv::LiteralInteger width;
    spirv::ParseTypeFloat(instruction, &id, &width);

    // Only interested in OpTypeFloat 32.
    if (width == 32)
    {
        ASSERT(!mFloatTypes[1].valid());
        mFloatTypes[1] = id;
    }
}

void SpirvVertexAttributeAliasingTransformer::visitTypeVector(const uint32_t *instruction)
{
    spirv::IdResult id;
    spirv::IdRef componentId;
    spirv::LiteralInteger componentCount;
    spirv::ParseTypeVector(instruction, &id, &componentId, &componentCount);

    // Only interested in OpTypeVector %f32 N, where %f32 is the id of OpTypeFloat 32.
    if (componentId == mFloatTypes[1])
    {
        ASSERT(componentCount >= 2 && componentCount <= 4);
        ASSERT(!mFloatTypes[componentCount].valid());
        mFloatTypes[componentCount] = id;
    }
}

void SpirvVertexAttributeAliasingTransformer::visitTypeMatrix(const uint32_t *instruction)
{
    spirv::IdResult id;
    spirv::IdRef columnType;
    spirv::LiteralInteger columnCount;
    spirv::ParseTypeMatrix(instruction, &id, &columnType, &columnCount);

    // Only interested in OpTypeMatrix %vecN, where %vecN is the id of OpTypeVector %f32 N.
    // This is only for square matN types (as allowed by GLSL ES 1.00), so columnCount is the same
    // as rowCount.
    if (columnType == mFloatTypes[columnCount])
    {
        ASSERT(!mMatrixTypes[columnCount].valid());
        mMatrixTypes[columnCount] = id;
    }
}

void SpirvVertexAttributeAliasingTransformer::visitTypePointer(const uint32_t *instruction)
{
    spirv::IdResult id;
    spv::StorageClass storageClass;
    spirv::IdRef typeId;
    spirv::ParseTypePointer(instruction, &id, &storageClass, &typeId);

    // Only interested in OpTypePointer Input %vecN, where %vecN is the id of OpTypeVector %f32 N,
    // as well as OpTypePointer Private %matN, where %matN is the id of OpTypeMatrix %vecN N.
    // This is only for matN types (as allowed by GLSL ES 1.00), so N >= 2.
    if (storageClass == spv::StorageClassInput)
    {
        for (size_t n = 2; n < mFloatTypes.size(); ++n)
        {
            if (typeId == mFloatTypes[n])
            {
                ASSERT(!mInputTypePointers[n].valid());
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

bool SpirvVertexAttributeAliasingTransformer::transformEntryPoint(const uint32_t *instruction)
{
    // Should only have one EntryPoint
    ASSERT(!mEntryPointId.valid());

    // Remove aliasing attributes from the shader interface declaration.
    spv::ExecutionModel executionModel;
    spirv::LiteralString name;
    spirv::IdRefList interfaceList;
    spirv::ParseEntryPoint(instruction, &executionModel, &mEntryPointId, &name, &interfaceList);

    // As a first pass, filter out matrix attributes and append their replacement vectors.
    size_t originalInterfaceListSize = interfaceList.size();
    for (size_t index = 0; index < originalInterfaceListSize; ++index)
    {
        const spirv::IdRef matrixId(interfaceList[index]);

        if (!mExpandedMatrixFirstVectorIdById[matrixId].valid())
        {
            continue;
        }

        const ShaderInterfaceVariableInfo *info = mVariableInfoById[matrixId];
        ValidateShaderInterfaceVariableIsAttribute(info);

        // Replace the matrix id with its first vector id.
        const spirv::IdRef vec0Id(mExpandedMatrixFirstVectorIdById[matrixId]);
        interfaceList[index] = vec0Id;

        // Append the rest of the vectors to the entry point.
        for (uint32_t offset = 1; offset < info->attributeLocationCount; ++offset)
        {
            const spirv::IdRef vecId(vec0Id + offset);
            interfaceList.push_back(vecId);
        }
    }

    // Filter out aliasing attributes from entry point interface declaration.
    size_t writeIndex = 0;
    for (size_t index = 0; index < interfaceList.size(); ++index)
    {
        const spirv::IdRef id(interfaceList[index]);

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

        interfaceList[writeIndex] = id;
        ++writeIndex;
    }

    // Update the number of interface variables.
    interfaceList.resize(writeIndex);

    // Write the entry point with the aliasing attributes removed.
    spirv::WriteEntryPoint(mSpirvBlobOut, executionModel, mEntryPointId, name, interfaceList);

    return true;
}

bool SpirvVertexAttributeAliasingTransformer::transformName(const uint32_t *instruction)
{
    spirv::IdRef id;
    spirv::LiteralString name;
    spirv::ParseName(instruction, &id, &name);

    // If id is not that of an aliasing attribute, there's nothing to do.
    ASSERT(id < mIsAliasingAttributeById.size());
    if (!mIsAliasingAttributeById[id])
    {
        return false;
    }

    // Drop debug annotations for this id.
    return true;
}

bool SpirvVertexAttributeAliasingTransformer::transformDecorate(const uint32_t *instruction)
{
    spirv::IdRef id;
    spv::Decoration decoration;
    spirv::ParseDecorate(instruction, &id, &decoration, nullptr);

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

        const spirv::IdRef vec0Id(mExpandedMatrixFirstVectorIdById[id]);
        ASSERT(vec0Id.valid());

        for (uint32_t offset = 0; offset < info->attributeLocationCount; ++offset)
        {
            const spirv::IdRef vecId(vec0Id + offset);
            if (mIsAliasingAttributeById[vecId])
            {
                continue;
            }

            spirv::WriteDecorate(mSpirvBlobOut, vecId, decoration,
                                 {spirv::LiteralInteger(info->location + offset)});
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

bool SpirvVertexAttributeAliasingTransformer::transformVariable(const uint32_t *instruction)
{
    spirv::IdResultType typeId;
    spirv::IdResult id;
    spv::StorageClass storageClass;
    spirv::ParseVariable(instruction, &typeId, &id, &storageClass, nullptr);

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

bool SpirvVertexAttributeAliasingTransformer::transformAccessChain(const uint32_t *instruction)
{
    spirv::IdResultType typeId;
    spirv::IdResult id;
    spirv::IdRef baseId;
    spirv::IdRefList indexList;
    spirv::ParseAccessChain(instruction, &typeId, &id, &baseId, &indexList);

    if (isMatrixAttribute(baseId))
    {
        // Write a modified OpAccessChain instruction.  Only modification is that the %type is
        // replaced with the Private version of it.  If there is one %index, that would be a vector
        // type, and if there are two %index'es, it's a float type.
        spirv::IdRef replacementTypeId;

        if (indexList.size() == 1)
        {
            // If indexed once, it uses a vector type.
            const ShaderInterfaceVariableInfo *info = mVariableInfoById[baseId];
            ValidateShaderInterfaceVariableIsAttribute(info);

            const uint32_t componentCount = info->attributeComponentCount;

            // %type must have been the Input vector type with the matrice's component size.
            ASSERT(typeId == mInputTypePointers[componentCount]);

            // Replace the type with the corresponding Private one.
            replacementTypeId = mPrivateFloatTypePointers[componentCount];
        }
        else
        {
            // If indexed twice, it uses the float type.
            ASSERT(indexList.size() == 2);

            // Replace the type with the Private pointer to float32.
            replacementTypeId = mPrivateFloatTypePointers[1];
        }

        spirv::WriteAccessChain(mSpirvBlobOut, replacementTypeId, id, baseId, indexList);
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
        const spirv::IdRef replacementId(getAliasingAttributeReplacementId(baseId, 0));

        // Get variable info corresponding to the replacement attribute.
        const ShaderInterfaceVariableInfo *replacementInfo = mVariableInfoById[replacementId];
        ValidateShaderInterfaceVariableIsAttribute(replacementInfo);

        // Write a modified OpAccessChain instruction.  Currently, the instruction is:
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
        spirv::WriteAccessChain(mSpirvBlobOut, typeId, id, replacementId, indexList);
    }

    return true;
}

void SpirvVertexAttributeAliasingTransformer::transformLoadHelper(spirv::IdRef pointerId,
                                                                  spirv::IdRef typeId,
                                                                  spirv::IdRef replacementId,
                                                                  spirv::IdRef resultId)
{
    // Get variable info corresponding to the replacement attribute.
    const ShaderInterfaceVariableInfo *replacementInfo = mVariableInfoById[replacementId];
    ValidateShaderInterfaceVariableIsAttribute(replacementInfo);

    // Currently, the instruction is:
    //
    //     %id = OpLoad %type %pointer
    //
    // This is modified to:
    //
    //     %newId = OpLoad %replacementType %replacement
    //
    const spirv::IdRef loadResultId(getNewId());
    const spirv::IdRef replacementTypeId(mFloatTypes[replacementInfo->attributeComponentCount]);
    ASSERT(replacementTypeId.valid());

    spirv::WriteLoad(mSpirvBlobOut, replacementTypeId, loadResultId, replacementId, nullptr);

    // If swizzle is not necessary, assign %newId to %resultId.
    const ShaderInterfaceVariableInfo *aliasingInfo = mVariableInfoById[pointerId];
    if (aliasingInfo->attributeComponentCount == replacementInfo->attributeComponentCount)
    {
        spirv::WriteCopyObject(mSpirvBlobOut, typeId, resultId, loadResultId);
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
        spirv::WriteCompositeExtract(mSpirvBlobOut, typeId, resultId, loadResultId,
                                     {spirv::LiteralInteger(0)});
    }
    else
    {
        spirv::LiteralIntegerList swizzle = {spirv::LiteralInteger(0), spirv::LiteralInteger(1),
                                             spirv::LiteralInteger(2), spirv::LiteralInteger(3)};
        swizzle.resize(aliasingInfo->attributeComponentCount);

        spirv::WriteVectorShuffle(mSpirvBlobOut, typeId, resultId, loadResultId, loadResultId,
                                  swizzle);
    }
}

bool SpirvVertexAttributeAliasingTransformer::transformLoad(const uint32_t *instruction)
{
    spirv::IdResultType typeId;
    spirv::IdResult id;
    spirv::IdRef pointerId;
    ParseLoad(instruction, &typeId, &id, &pointerId, nullptr);

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

        const spirv::IdRef replacementTypeId(mMatrixTypes[info->attributeLocationCount]);

        spirv::WriteLoad(mSpirvBlobOut, replacementTypeId, id, pointerId, nullptr);
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
        const spirv::IdRef replacementId(getAliasingAttributeReplacementId(pointerId, 0));

        // Replace the load instruction by a load from the replacement id.
        transformLoadHelper(pointerId, typeId, replacementId, id);
    }

    return true;
}

void SpirvVertexAttributeAliasingTransformer::declareExpandedMatrixVectors()
{
    // Go through matrix attributes and expand them.
    for (uint32_t matrixIdIndex = 0; matrixIdIndex < mExpandedMatrixFirstVectorIdById.size();
         ++matrixIdIndex)
    {
        const spirv::IdRef matrixId(matrixIdIndex);
        const spirv::IdRef vec0Id(mExpandedMatrixFirstVectorIdById[matrixId]);
        if (!vec0Id.valid())
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
        ASSERT(mMatrixTypes[locationCount].valid());

        // OpTypePointer Private %matrixType
        spirv::IdRef privateType(mPrivateMatrixTypePointers[locationCount]);
        if (!privateType.valid())
        {
            privateType                               = getNewId();
            mPrivateMatrixTypePointers[locationCount] = privateType;
            spirv::WriteTypePointer(mSpirvBlobOut, privateType, spv::StorageClassPrivate,
                                    mMatrixTypes[locationCount]);
        }

        // OpVariable %privateType Private
        spirv::WriteVariable(mSpirvBlobOut, privateType, matrixId, spv::StorageClassPrivate,
                             nullptr);

        // If the OpTypePointer is not declared for the vector type corresponding to each location,
        // declare it now.
        //
        //     %vecType = OpTypePointer %vecType Input
        spirv::IdRef inputType(mInputTypePointers[componentCount]);
        if (!inputType.valid())
        {
            inputType                          = getNewId();
            mInputTypePointers[componentCount] = inputType;
            spirv::WriteTypePointer(mSpirvBlobOut, inputType, spv::StorageClassInput,
                                    mFloatTypes[componentCount]);
        }

        // Declare a vector for each column of the matrix.
        for (uint32_t offset = 0; offset < info->attributeLocationCount; ++offset)
        {
            const spirv::IdRef vecId(vec0Id + offset);
            if (!mIsAliasingAttributeById[vecId])
            {
                spirv::WriteVariable(mSpirvBlobOut, inputType, vecId, spv::StorageClassInput,
                                     nullptr);
            }
        }
    }

    // Additionally, declare OpTypePointer Private %mFloatTypes[i] in case needed (used in
    // Op*AccessChain instructions, if any).
    for (size_t n = 1; n < mFloatTypes.size(); ++n)
    {
        if (mFloatTypes[n].valid() && mPrivateFloatTypePointers[n] == 0)
        {
            const spirv::IdRef privateType(getNewId());
            mPrivateFloatTypePointers[n] = privateType;
            spirv::WriteTypePointer(mSpirvBlobOut, privateType, spv::StorageClassPrivate,
                                    mFloatTypes[n]);
        }
    }
}

void SpirvVertexAttributeAliasingTransformer::writeExpandedMatrixInitialization()
{
    // Go through matrix attributes and initialize them.  Note that their declaration is replaced
    // with a Private storage class, but otherwise has the same id.
    for (uint32_t matrixIdIndex = 0; matrixIdIndex < mExpandedMatrixFirstVectorIdById.size();
         ++matrixIdIndex)
    {
        const spirv::IdRef matrixId(matrixIdIndex);
        const spirv::IdRef vec0Id(mExpandedMatrixFirstVectorIdById[matrixId]);
        if (!vec0Id.valid())
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

        spirv::IdRefList vecLoadIds;
        const uint32_t locationCount = info->attributeLocationCount;
        for (uint32_t offset = 0; offset < locationCount; ++offset)
        {
            const spirv::IdRef vecId(vec0Id + offset);

            // Load into temporary, potentially through an aliasing vector.
            spirv::IdRef replacementId(vecId);
            ASSERT(vecId < mIsAliasingAttributeById.size());
            if (mIsAliasingAttributeById[vecId])
            {
                replacementId = getAliasingAttributeReplacementId(vecId, offset);
            }

            // Write a load instruction from the replacement id.
            vecLoadIds.push_back(getNewId());
            transformLoadHelper(matrixId, mFloatTypes[info->attributeComponentCount], replacementId,
                                vecLoadIds.back());
        }

        // Aggregate the vector loads into a matrix.
        ASSERT(mMatrixTypes[locationCount].valid());
        const spirv::IdRef compositeId(getNewId());
        spirv::WriteCompositeConstruct(mSpirvBlobOut, mMatrixTypes[locationCount], compositeId,
                                       vecLoadIds);

        // Store it in the private variable.
        spirv::WriteStore(mSpirvBlobOut, matrixId, compositeId, nullptr);
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

    if (!UniformNameIsIndexZero(*name))
    {
        return false;
    }

    // Strip all indices
    *name = name->substr(0, name->find('['));
    return true;
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
    return sh::vk::kXfbEmulationBufferBlockName + Str(bufferIndex);
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
                            const gl::ProgramState &programState,
                            const gl::ProgramVaryingPacking &varyingPacking,
                            const gl::ShaderType shaderType,
                            const gl::ShaderType frontShaderType,
                            bool isTransformFeedbackStage,
                            GlslangProgramInterfaceInfo *programInterfaceInfo,
                            ShaderInterfaceVariableInfoMap *variableInfoMapOut)
{
    const gl::ProgramExecutable &programExecutable = programState.getExecutable();

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

        // Assign location to varyings generated for transform feedback capture
        if (options.supportsTransformFeedbackExtension &&
            gl::ShaderTypeSupportsTransformFeedback(shaderType))
        {
            AssignTransformFeedbackExtensionLocations(shaderType, programState,
                                                      isTransformFeedbackStage,
                                                      programInterfaceInfo, variableInfoMapOut);
        }

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

        // Assign qualifiers to all varyings captured by transform feedback
        if (!programExecutable.getLinkedTransformFeedbackVaryings().empty() &&
            options.supportsTransformFeedbackExtension &&
            (shaderType == programExecutable.getLinkedTransformFeedbackStage()))
        {
            AssignTransformFeedbackExtensionQualifiers(programExecutable, outputPacking, shaderType,
                                                       variableInfoMapOut);
        }
    }

    AssignUniformBindings(options, programExecutable, shaderType, programInterfaceInfo,
                          variableInfoMapOut);
    AssignTextureBindings(options, programExecutable, shaderType, programInterfaceInfo,
                          variableInfoMapOut);
    AssignNonTextureBindings(options, programExecutable, shaderType, programInterfaceInfo,
                             variableInfoMapOut);

    if (options.supportsTransformFeedbackEmulation &&
        gl::ShaderTypeSupportsTransformFeedback(shaderType))
    {
        // If transform feedback emulation is not enabled, mark all transform feedback output
        // buffers as inactive.
        isTransformFeedbackStage =
            isTransformFeedbackStage && options.enableTransformFeedbackEmulation;

        AssignTransformFeedbackEmulationBindings(shaderType, programState, isTransformFeedbackStage,
                                                 programInterfaceInfo, variableInfoMapOut);
    }
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

    // Write transform feedback output code for emulation path
    if (xfbStage == gl::ShaderType::Vertex && !xfbSource->empty() &&
        options.supportsTransformFeedbackEmulation)
    {
        if (options.enableTransformFeedbackEmulation &&
            !programState.getLinkedTransformFeedbackVaryings().empty())
        {
            GenerateTransformFeedbackEmulationOutputs(options, xfbStage, programState,
                                                      programInterfaceInfo, xfbSource,
                                                      variableInfoMapOut);
        }
        else
        {
            *xfbSource = SubstituteTransformFeedbackMarkers(*xfbSource, "");
        }
    }

    gl::ShaderType frontShaderType = gl::ShaderType::InvalidEnum;

    for (const gl::ShaderType shaderType : programState.getExecutable().getLinkedShaderStages())
    {
        const bool isXfbStage =
            shaderType == xfbStage && !programState.getLinkedTransformFeedbackVaryings().empty();
        GlslangAssignLocations(options, programState, resources.varyingPacking, shaderType,
                               frontShaderType, isXfbStage, programInterfaceInfo,
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
    glslang::TShader tessControlShader(EShLangTessControl);
    glslang::TShader tessEvaluationShader(EShLangTessEvaluation);
    glslang::TShader computeShader(EShLangCompute);

    gl::ShaderMap<glslang::TShader *> shaders = {
        {gl::ShaderType::Vertex, &vertexShader},
        {gl::ShaderType::Fragment, &fragmentShader},
        {gl::ShaderType::TessControl, &tessControlShader},
        {gl::ShaderType::TessEvaluation, &tessEvaluationShader},
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
