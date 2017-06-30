//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MemoryProgramCache: Stores compiled and linked programs in memory so they don't
//   always have to be re-compiled. Can be used in conjunction with the platform
//   layer to warm up the cache from disk.

#include "libANGLE/MemoryProgramCache.h"

#include <GLSLANG/ShaderVars.h>
#include <anglebase/sha1.h>

#include "common/version.h"
#include "libANGLE/BinaryStream.h"
#include "libANGLE/Context.h"
#include "libANGLE/Uniform.h"
#include "libANGLE/renderer/ProgramImpl.h"
#include "platform/Platform.h"

namespace gl
{

namespace
{
constexpr unsigned int kWarningLimit = 3;

void WriteShaderVar(BinaryOutputStream *stream, const sh::ShaderVariable &var)
{
    stream->writeInt(var.type);
    stream->writeInt(var.precision);
    stream->writeString(var.name);
    stream->writeString(var.mappedName);
    stream->writeInt(var.arraySize);
    stream->writeInt(var.staticUse);
    stream->writeString(var.structName);
    ASSERT(var.fields.empty());
}

void LoadShaderVar(BinaryInputStream *stream, sh::ShaderVariable *var)
{
    var->type       = stream->readInt<GLenum>();
    var->precision  = stream->readInt<GLenum>();
    var->name       = stream->readString();
    var->mappedName = stream->readString();
    var->arraySize  = stream->readInt<unsigned int>();
    var->staticUse  = stream->readBool();
    var->structName = stream->readString();
}

class HashStream final : angle::NonCopyable
{
  public:
    std::string str() { return mStringStream.str(); }

    template <typename T>
    HashStream &operator<<(T value)
    {
        mStringStream << value << kSeparator;
        return *this;
    }

  private:
    static constexpr char kSeparator = ':';
    std::ostringstream mStringStream;
};

HashStream &operator<<(HashStream &stream, const Shader *shader)
{
    if (shader)
    {
        stream << shader->getSourceString().c_str() << shader->getSourceString().length()
               << shader->getCompilerResourcesString().c_str();
    }
    return stream;
}

HashStream &operator<<(HashStream &stream, const Program::Bindings &bindings)
{
    for (const auto &binding : bindings)
    {
        stream << binding.first << binding.second;
    }
    return stream;
}

HashStream &operator<<(HashStream &stream, const std::vector<std::string> &strings)
{
    for (const auto &str : strings)
    {
        stream << str;
    }
    return stream;
}

}  // anonymous namespace

MemoryProgramCache::MemoryProgramCache(size_t maxCacheSizeBytes)
    : mProgramBinaryCache(maxCacheSizeBytes), mIssuedWarnings(0)
{
}

MemoryProgramCache::~MemoryProgramCache()
{
}

// static
LinkResult MemoryProgramCache::Deserialize(const Context *context,
                                           const Program *program,
                                           ProgramState *state,
                                           const uint8_t *binary,
                                           size_t length,
                                           InfoLog &infoLog)
{
    BinaryInputStream stream(binary, length);

    unsigned char commitString[ANGLE_COMMIT_HASH_SIZE];
    stream.readBytes(commitString, ANGLE_COMMIT_HASH_SIZE);
    if (memcmp(commitString, ANGLE_COMMIT_HASH, sizeof(unsigned char) * ANGLE_COMMIT_HASH_SIZE) !=
        0)
    {
        infoLog << "Invalid program binary version.";
        return false;
    }

    int majorVersion = stream.readInt<int>();
    int minorVersion = stream.readInt<int>();
    if (majorVersion != context->getClientMajorVersion() ||
        minorVersion != context->getClientMinorVersion())
    {
        infoLog << "Cannot load program binaries across different ES context versions.";
        return false;
    }

    state->mComputeShaderLocalSize[0] = stream.readInt<int>();
    state->mComputeShaderLocalSize[1] = stream.readInt<int>();
    state->mComputeShaderLocalSize[2] = stream.readInt<int>();

    static_assert(MAX_VERTEX_ATTRIBS <= sizeof(unsigned long) * 8,
                  "Too many vertex attribs for mask");
    state->mActiveAttribLocationsMask = stream.readInt<unsigned long>();

    unsigned int attribCount = stream.readInt<unsigned int>();
    ASSERT(state->mAttributes.empty());
    for (unsigned int attribIndex = 0; attribIndex < attribCount; ++attribIndex)
    {
        sh::Attribute attrib;
        LoadShaderVar(&stream, &attrib);
        attrib.location = stream.readInt<int>();
        state->mAttributes.push_back(attrib);
    }

    unsigned int uniformCount = stream.readInt<unsigned int>();
    ASSERT(state->mUniforms.empty());
    for (unsigned int uniformIndex = 0; uniformIndex < uniformCount; ++uniformIndex)
    {
        LinkedUniform uniform;
        LoadShaderVar(&stream, &uniform);

        uniform.blockIndex                 = stream.readInt<int>();
        uniform.blockInfo.offset           = stream.readInt<int>();
        uniform.blockInfo.arrayStride      = stream.readInt<int>();
        uniform.blockInfo.matrixStride     = stream.readInt<int>();
        uniform.blockInfo.isRowMajorMatrix = stream.readBool();

        state->mUniforms.push_back(uniform);
    }

    const unsigned int uniformIndexCount = stream.readInt<unsigned int>();
    ASSERT(state->mUniformLocations.empty());
    for (unsigned int uniformIndexIndex = 0; uniformIndexIndex < uniformIndexCount;
         uniformIndexIndex++)
    {
        VariableLocation variable;
        stream.readString(&variable.name);
        stream.readInt(&variable.element);
        stream.readInt(&variable.index);
        stream.readBool(&variable.used);
        stream.readBool(&variable.ignored);

        state->mUniformLocations.push_back(variable);
    }

    unsigned int uniformBlockCount = stream.readInt<unsigned int>();
    ASSERT(state->mUniformBlocks.empty());
    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < uniformBlockCount;
         ++uniformBlockIndex)
    {
        UniformBlock uniformBlock;
        stream.readString(&uniformBlock.name);
        stream.readBool(&uniformBlock.isArray);
        stream.readInt(&uniformBlock.arrayElement);
        stream.readInt(&uniformBlock.binding);
        stream.readInt(&uniformBlock.dataSize);
        stream.readBool(&uniformBlock.vertexStaticUse);
        stream.readBool(&uniformBlock.fragmentStaticUse);

        unsigned int numMembers = stream.readInt<unsigned int>();
        for (unsigned int blockMemberIndex = 0; blockMemberIndex < numMembers; blockMemberIndex++)
        {
            uniformBlock.memberUniformIndexes.push_back(stream.readInt<unsigned int>());
        }

        state->mUniformBlocks.push_back(uniformBlock);

        state->mActiveUniformBlockBindings.set(uniformBlockIndex, uniformBlock.binding != 0);
    }

    unsigned int transformFeedbackVaryingCount = stream.readInt<unsigned int>();

    // Reject programs that use transform feedback varyings if the hardware cannot support them.
    if (transformFeedbackVaryingCount > 0 &&
        context->getWorkarounds().disableProgramCachingForTransformFeedback)
    {
        infoLog << "Current driver does not support transform feedback in binary programs.";
        return false;
    }

    ASSERT(state->mLinkedTransformFeedbackVaryings.empty());
    for (unsigned int transformFeedbackVaryingIndex = 0;
         transformFeedbackVaryingIndex < transformFeedbackVaryingCount;
         ++transformFeedbackVaryingIndex)
    {
        sh::Varying varying;
        stream.readInt(&varying.arraySize);
        stream.readInt(&varying.type);
        stream.readString(&varying.name);

        GLuint arrayIndex = stream.readInt<GLuint>();

        state->mLinkedTransformFeedbackVaryings.emplace_back(varying, arrayIndex);
    }

    stream.readInt(&state->mTransformFeedbackBufferMode);

    unsigned int outputCount = stream.readInt<unsigned int>();
    ASSERT(state->mOutputVariables.empty());
    for (unsigned int outputIndex = 0; outputIndex < outputCount; ++outputIndex)
    {
        sh::OutputVariable output;
        LoadShaderVar(&stream, &output);
        output.location = stream.readInt<int>();
        state->mOutputVariables.push_back(output);
    }

    unsigned int outputVarCount = stream.readInt<unsigned int>();
    for (unsigned int outputIndex = 0; outputIndex < outputVarCount; ++outputIndex)
    {
        int locationIndex = stream.readInt<int>();
        VariableLocation locationData;
        stream.readInt(&locationData.element);
        stream.readInt(&locationData.index);
        stream.readString(&locationData.name);
        state->mOutputLocations[locationIndex] = locationData;
    }

    unsigned int outputTypeCount = stream.readInt<unsigned int>();
    for (unsigned int outputIndex = 0; outputIndex < outputTypeCount; ++outputIndex)
    {
        state->mOutputVariableTypes.push_back(stream.readInt<GLenum>());
    }
    static_assert(IMPLEMENTATION_MAX_DRAW_BUFFERS < 8 * sizeof(uint32_t),
                  "All bits of DrawBufferMask can be contained in an uint32_t");
    state->mActiveOutputVariables = stream.readInt<uint32_t>();

    unsigned int samplerRangeLow  = stream.readInt<unsigned int>();
    unsigned int samplerRangeHigh = stream.readInt<unsigned int>();
    state->mSamplerUniformRange   = RangeUI(samplerRangeLow, samplerRangeHigh);
    unsigned int samplerCount = stream.readInt<unsigned int>();
    for (unsigned int samplerIndex = 0; samplerIndex < samplerCount; ++samplerIndex)
    {
        GLenum textureType  = stream.readInt<GLenum>();
        size_t bindingCount = stream.readInt<size_t>();
        state->mSamplerBindings.emplace_back(SamplerBinding(textureType, bindingCount));
    }

    unsigned int imageRangeLow  = stream.readInt<unsigned int>();
    unsigned int imageRangeHigh = stream.readInt<unsigned int>();
    state->mImageUniformRange   = RangeUI(imageRangeLow, imageRangeHigh);
    unsigned int imageCount     = stream.readInt<unsigned int>();
    for (unsigned int imageIndex = 0; imageIndex < imageCount; ++imageIndex)
    {
        GLuint boundImageUnit = stream.readInt<unsigned int>();
        size_t elementCount   = stream.readInt<size_t>();
        state->mImageBindings.emplace_back(ImageBinding(boundImageUnit, elementCount));
    }

    return program->getImplementation()->load(context, infoLog, &stream);
}

// static
void MemoryProgramCache::Serialize(const Context *context,
                                   const gl::Program *program,
                                   angle::MemoryBuffer *binaryOut)
{
    BinaryOutputStream stream;

    stream.writeBytes(reinterpret_cast<const unsigned char *>(ANGLE_COMMIT_HASH),
                      ANGLE_COMMIT_HASH_SIZE);

    // nullptr context is supported when computing binary length.
    if (context)
    {
        stream.writeInt(context->getClientVersion().major);
        stream.writeInt(context->getClientVersion().minor);
    }
    else
    {
        stream.writeInt(2);
        stream.writeInt(0);
    }

    const auto &state = program->getState();

    const auto &computeLocalSize = state.getComputeShaderLocalSize();

    stream.writeInt(computeLocalSize[0]);
    stream.writeInt(computeLocalSize[1]);
    stream.writeInt(computeLocalSize[2]);

    stream.writeInt(state.getActiveAttribLocationsMask().to_ulong());

    stream.writeInt(state.getAttributes().size());
    for (const sh::Attribute &attrib : state.getAttributes())
    {
        WriteShaderVar(&stream, attrib);
        stream.writeInt(attrib.location);
    }

    stream.writeInt(state.getUniforms().size());
    for (const LinkedUniform &uniform : state.getUniforms())
    {
        WriteShaderVar(&stream, uniform);

        // FIXME: referenced

        stream.writeInt(uniform.blockIndex);
        stream.writeInt(uniform.blockInfo.offset);
        stream.writeInt(uniform.blockInfo.arrayStride);
        stream.writeInt(uniform.blockInfo.matrixStride);
        stream.writeInt(uniform.blockInfo.isRowMajorMatrix);
    }

    stream.writeInt(state.getUniformLocations().size());
    for (const auto &variable : state.getUniformLocations())
    {
        stream.writeString(variable.name);
        stream.writeInt(variable.element);
        stream.writeInt(variable.index);
        stream.writeInt(variable.used);
        stream.writeInt(variable.ignored);
    }

    stream.writeInt(state.getUniformBlocks().size());
    for (const UniformBlock &uniformBlock : state.getUniformBlocks())
    {
        stream.writeString(uniformBlock.name);
        stream.writeInt(uniformBlock.isArray);
        stream.writeInt(uniformBlock.arrayElement);
        stream.writeInt(uniformBlock.binding);
        stream.writeInt(uniformBlock.dataSize);

        stream.writeInt(uniformBlock.vertexStaticUse);
        stream.writeInt(uniformBlock.fragmentStaticUse);

        stream.writeInt(uniformBlock.memberUniformIndexes.size());
        for (unsigned int memberUniformIndex : uniformBlock.memberUniformIndexes)
        {
            stream.writeInt(memberUniformIndex);
        }
    }

    // Warn the app layer if saving a binary with unsupported transform feedback.
    if (!state.getLinkedTransformFeedbackVaryings().empty() &&
        context->getWorkarounds().disableProgramCachingForTransformFeedback)
    {
        WARN() << "Saving program binary with transform feedback, which is not supported on this "
                  "driver.";
    }

    stream.writeInt(state.getLinkedTransformFeedbackVaryings().size());
    for (const auto &var : state.getLinkedTransformFeedbackVaryings())
    {
        stream.writeInt(var.arraySize);
        stream.writeInt(var.type);
        stream.writeString(var.name);

        stream.writeIntOrNegOne(var.arrayIndex);
    }

    stream.writeInt(state.getTransformFeedbackBufferMode());

    stream.writeInt(state.getOutputVariables().size());
    for (const sh::OutputVariable &output : state.getOutputVariables())
    {
        WriteShaderVar(&stream, output);
        stream.writeInt(output.location);
    }

    stream.writeInt(state.getOutputLocations().size());
    for (const auto &outputPair : state.getOutputLocations())
    {
        stream.writeInt(outputPair.first);
        stream.writeIntOrNegOne(outputPair.second.element);
        stream.writeInt(outputPair.second.index);
        stream.writeString(outputPair.second.name);
    }

    stream.writeInt(state.mOutputVariableTypes.size());
    for (const auto &outputVariableType : state.mOutputVariableTypes)
    {
        stream.writeInt(outputVariableType);
    }

    static_assert(IMPLEMENTATION_MAX_DRAW_BUFFERS < 8 * sizeof(uint32_t),
                  "All bits of DrawBufferMask can be contained in an uint32_t");
    stream.writeInt(static_cast<uint32_t>(state.mActiveOutputVariables.to_ulong()));

    stream.writeInt(state.getSamplerUniformRange().low());
    stream.writeInt(state.getSamplerUniformRange().high());

    stream.writeInt(state.getSamplerBindings().size());
    for (const auto &samplerBinding : state.getSamplerBindings())
    {
        stream.writeInt(samplerBinding.textureType);
        stream.writeInt(samplerBinding.boundTextureUnits.size());
    }

    stream.writeInt(state.getImageUniformRange().low());
    stream.writeInt(state.getImageUniformRange().high());

    stream.writeInt(state.getImageBindings().size());
    for (const auto &imageBinding : state.getImageBindings())
    {
        stream.writeInt(imageBinding.boundImageUnit);
        stream.writeInt(imageBinding.elementCount);
    }

    program->getImplementation()->save(context, &stream);

    ASSERT(binaryOut);
    binaryOut->resize(stream.length());
    memcpy(binaryOut->data(), stream.data(), stream.length());
}

// static
void MemoryProgramCache::ComputeHash(const Context *context,
                                     const Program *program,
                                     ProgramHash *hashOut)
{
    auto vertexShader   = program->getAttachedVertexShader();
    auto fragmentShader = program->getAttachedFragmentShader();
    auto computeShader  = program->getAttachedComputeShader();

    // Compute the program hash. Start with the shader hashes and resource strings.
    HashStream hashStream;
    hashStream << vertexShader << fragmentShader << computeShader;

    // Add some ANGLE metadata and Context properties, such as version and back-end.
    hashStream << ANGLE_COMMIT_HASH << context->getClientMajorVersion()
               << context->getClientMinorVersion() << context->getString(GL_RENDERER);

    // Hash pre-link program properties.
    hashStream << program->getAttributeBindings() << program->getUniformLocationBindings()
               << program->getFragmentInputBindings()
               << program->getState().getTransformFeedbackVaryingNames()
               << program->getState().getTransformFeedbackBufferMode();

    // Call the secure SHA hashing function.
    const std::string &programKey = hashStream.str();
    angle::base::SHA1HashBytes(reinterpret_cast<const unsigned char *>(programKey.c_str()),
                               programKey.length(), hashOut->data());
}

LinkResult MemoryProgramCache::getProgram(const Context *context,
                                          const Program *program,
                                          ProgramState *state,
                                          ProgramHash *hashOut)
{
    ComputeHash(context, program, hashOut);
    const angle::MemoryBuffer *binaryProgram = nullptr;
    LinkResult result(false);
    if (get(*hashOut, &binaryProgram))
    {
        InfoLog infoLog;
        ANGLE_TRY_RESULT(Deserialize(context, program, state, binaryProgram->data(),
                                     binaryProgram->size(), infoLog),
                         result);
        if (!result.getResult())
        {
            // Cache load failed, evict.
            if (mIssuedWarnings++ < kWarningLimit)
            {
                WARN() << "Failed to load binary from cache: " << infoLog.str();

                if (mIssuedWarnings == kWarningLimit)
                {
                    WARN() << "Reaching warning limit for cache load failures, silencing "
                              "subsequent warnings.";
                }
            }
            remove(*hashOut);
        }
    }
    return result;
}

bool MemoryProgramCache::get(const ProgramHash &programHash, const angle::MemoryBuffer **programOut)
{
    return mProgramBinaryCache.get(programHash, programOut);
}

void MemoryProgramCache::remove(const ProgramHash &programHash)
{
    bool result = mProgramBinaryCache.eraseByKey(programHash);
    ASSERT(result);
}

void MemoryProgramCache::put(const ProgramHash &program,
                             const Context *context,
                             angle::MemoryBuffer &&binaryProgram)
{
    const angle::MemoryBuffer *result =
        mProgramBinaryCache.put(program, std::move(binaryProgram), binaryProgram.size());
    if (!result)
    {
        ERR() << "Failed to store binary program in memory cache, program is too large.";
    }
    else
    {
        auto *platform = ANGLEPlatformCurrent();
        platform->cacheProgram(platform, program, result->size(), result->data());
    }
}

void MemoryProgramCache::putProgram(const ProgramHash &programHash,
                                    const Context *context,
                                    const Program *program)
{
    angle::MemoryBuffer binaryProgram;
    Serialize(context, program, &binaryProgram);
    put(programHash, context, std::move(binaryProgram));
}

void MemoryProgramCache::putBinary(const Context *context,
                                   const Program *program,
                                   const uint8_t *binary,
                                   size_t length)
{
    // Copy the binary.
    angle::MemoryBuffer binaryProgram;
    binaryProgram.resize(length);
    memcpy(binaryProgram.data(), binary, length);

    // Compute the hash.
    ProgramHash programHash;
    ComputeHash(context, program, &programHash);

    // Store the binary.
    put(programHash, context, std::move(binaryProgram));
}

void MemoryProgramCache::clear()
{
    mProgramBinaryCache.clear();
    mIssuedWarnings = 0;
}

}  // namespace gl
