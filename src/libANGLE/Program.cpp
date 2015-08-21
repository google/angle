//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Program.cpp: Implements the gl::Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#include "libANGLE/Program.h"

#include <algorithm>

#include "common/debug.h"
#include "common/platform.h"
#include "common/utilities.h"
#include "common/version.h"
#include "compiler/translator/blocklayout.h"
#include "libANGLE/Data.h"
#include "libANGLE/ResourceManager.h"
#include "libANGLE/features.h"
#include "libANGLE/renderer/Renderer.h"
#include "libANGLE/renderer/ProgramImpl.h"

namespace gl
{
const char * const g_fakepath = "C:\\fakepath";

namespace
{

unsigned int ParseAndStripArrayIndex(std::string* name)
{
    unsigned int subscript = GL_INVALID_INDEX;

    // Strip any trailing array operator and retrieve the subscript
    size_t open = name->find_last_of('[');
    size_t close = name->find_last_of(']');
    if (open != std::string::npos && close == name->length() - 1)
    {
        subscript = atoi(name->substr(open + 1).c_str());
        name->erase(open);
    }

    return subscript;
}

}

AttributeBindings::AttributeBindings()
{
}

AttributeBindings::~AttributeBindings()
{
}

InfoLog::InfoLog()
{
}

InfoLog::~InfoLog()
{
}

size_t InfoLog::getLength() const
{
    const std::string &logString = mStream.str();
    return logString.empty() ? 0 : logString.length() + 1;
}

void InfoLog::getLog(GLsizei bufSize, GLsizei *length, char *infoLog)
{
    size_t index = 0;

    if (bufSize > 0)
    {
        const std::string str(mStream.str());

        if (!str.empty())
        {
            index = std::min(static_cast<size_t>(bufSize) - 1, str.length());
            memcpy(infoLog, str.c_str(), index);
        }

        infoLog[index] = '\0';
    }

    if (length)
    {
        *length = static_cast<GLsizei>(index);
    }
}

// append a santized message to the program info log.
// The D3D compiler includes a fake file path in some of the warning or error 
// messages, so lets remove all occurrences of this fake file path from the log.
void InfoLog::appendSanitized(const char *message)
{
    std::string msg(message);

    size_t found;
    do
    {
        found = msg.find(g_fakepath);
        if (found != std::string::npos)
        {
            msg.erase(found, strlen(g_fakepath));
        }
    }
    while (found != std::string::npos);

    mStream << message << std::endl;
}

void InfoLog::reset()
{
}

VariableLocation::VariableLocation()
    : name(),
      element(0),
      index(0)
{
}

VariableLocation::VariableLocation(const std::string &name, unsigned int element, unsigned int index)
    : name(name),
      element(element),
      index(index)
{
}

LinkedVarying::LinkedVarying()
{
}

LinkedVarying::LinkedVarying(const std::string &name, GLenum type, GLsizei size, const std::string &semanticName,
                             unsigned int semanticIndex, unsigned int semanticIndexCount)
    : name(name), type(type), size(size), semanticName(semanticName), semanticIndex(semanticIndex), semanticIndexCount(semanticIndexCount)
{
}

Program::Data::Data()
    : mAttachedFragmentShader(nullptr),
      mAttachedVertexShader(nullptr),
      mTransformFeedbackBufferMode(GL_NONE)
{
}

Program::Data::~Data()
{
    if (mAttachedVertexShader != nullptr)
    {
        mAttachedVertexShader->release();
    }

    if (mAttachedFragmentShader != nullptr)
    {
        mAttachedFragmentShader->release();
    }
}

Program::Program(rx::ImplFactory *factory, ResourceManager *manager, GLuint handle)
    : mProgram(factory->createProgram(mData)),
      mLinkedAttributes(gl::MAX_VERTEX_ATTRIBS),
      mValidated(false),
      mLinked(false),
      mDeleteStatus(false),
      mRefCount(0),
      mResourceManager(manager),
      mHandle(handle)
{
    ASSERT(mProgram);

    resetUniformBlockBindings();
    unlink();
}

Program::~Program()
{
    unlink(true);

    SafeDelete(mProgram);
}

bool Program::attachShader(Shader *shader)
{
    if (shader->getType() == GL_VERTEX_SHADER)
    {
        if (mData.mAttachedVertexShader)
        {
            return false;
        }

        mData.mAttachedVertexShader = shader;
        mData.mAttachedVertexShader->addRef();
    }
    else if (shader->getType() == GL_FRAGMENT_SHADER)
    {
        if (mData.mAttachedFragmentShader)
        {
            return false;
        }

        mData.mAttachedFragmentShader = shader;
        mData.mAttachedFragmentShader->addRef();
    }
    else UNREACHABLE();

    return true;
}

bool Program::detachShader(Shader *shader)
{
    if (shader->getType() == GL_VERTEX_SHADER)
    {
        if (mData.mAttachedVertexShader != shader)
        {
            return false;
        }

        shader->release();
        mData.mAttachedVertexShader = nullptr;
    }
    else if (shader->getType() == GL_FRAGMENT_SHADER)
    {
        if (mData.mAttachedFragmentShader != shader)
        {
            return false;
        }

        shader->release();
        mData.mAttachedFragmentShader = nullptr;
    }
    else UNREACHABLE();

    return true;
}

int Program::getAttachedShadersCount() const
{
    return (mData.mAttachedVertexShader ? 1 : 0) + (mData.mAttachedFragmentShader ? 1 : 0);
}

void AttributeBindings::bindAttributeLocation(GLuint index, const char *name)
{
    if (index < MAX_VERTEX_ATTRIBS)
    {
        for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
        {
            mAttributeBinding[i].erase(name);
        }

        mAttributeBinding[index].insert(name);
    }
}

void Program::bindAttributeLocation(GLuint index, const char *name)
{
    mAttributeBindings.bindAttributeLocation(index, name);
    mProgram->bindAttributeLocation(index, name);
}

// Links the HLSL code of the vertex and pixel shader by matching up their varyings,
// compiling them into binaries, determining the attribute mappings, and collecting
// a list of uniforms
Error Program::link(const gl::Data &data)
{
    unlink(false);

    mInfoLog.reset();
    resetUniformBlockBindings();

    if (!mData.mAttachedFragmentShader || !mData.mAttachedFragmentShader->isCompiled())
    {
        return Error(GL_NO_ERROR);
    }
    ASSERT(mData.mAttachedFragmentShader->getType() == GL_FRAGMENT_SHADER);

    if (!mData.mAttachedVertexShader || !mData.mAttachedVertexShader->isCompiled())
    {
        return Error(GL_NO_ERROR);
    }
    ASSERT(mData.mAttachedVertexShader->getType() == GL_VERTEX_SHADER);

    if (!linkAttributes(data, mInfoLog, mAttributeBindings, mData.mAttachedVertexShader))
    {
        return Error(GL_NO_ERROR);
    }

    if (!linkVaryings(mInfoLog, mData.mAttachedVertexShader, mData.mAttachedFragmentShader))
    {
        return Error(GL_NO_ERROR);
    }

    if (!linkUniforms(mInfoLog, *data.caps))
    {
        return Error(GL_NO_ERROR);
    }

    if (!linkUniformBlocks(mInfoLog, *data.caps))
    {
        return Error(GL_NO_ERROR);
    }

    const auto &mergedVaryings = getMergedVaryings();

    if (!linkValidateTransformFeedback(mInfoLog, mergedVaryings, *data.caps))
    {
        return Error(GL_NO_ERROR);
    }

    rx::LinkResult result = mProgram->link(data, mInfoLog, mData.mAttachedFragmentShader,
                                           mData.mAttachedVertexShader, &mOutputVariables);

    if (result.error.isError() || !result.linkSuccess)
    {
        return result.error;
    }

    gatherTransformFeedbackVaryings(mergedVaryings);

    mLinked = true;
    return gl::Error(GL_NO_ERROR);
}

int AttributeBindings::getAttributeBinding(const std::string &name) const
{
    for (int location = 0; location < MAX_VERTEX_ATTRIBS; location++)
    {
        if (mAttributeBinding[location].find(name) != mAttributeBinding[location].end())
        {
            return location;
        }
    }

    return -1;
}

// Returns the program object to an unlinked state, before re-linking, or at destruction
void Program::unlink(bool destroy)
{
    if (destroy)   // Object being destructed
    {
        if (mData.mAttachedFragmentShader)
        {
            mData.mAttachedFragmentShader->release();
            mData.mAttachedFragmentShader = nullptr;
        }

        if (mData.mAttachedVertexShader)
        {
            mData.mAttachedVertexShader->release();
            mData.mAttachedVertexShader = nullptr;
        }
    }

    mLinkedAttributes.assign(mLinkedAttributes.size(), sh::Attribute());
    mData.mTransformFeedbackVaryingVars.clear();

    mProgram->reset();

    mValidated = false;

    mLinked = false;
}

bool Program::isLinked()
{
    return mLinked;
}

Error Program::loadBinary(GLenum binaryFormat, const void *binary, GLsizei length)
{
    unlink(false);

#if ANGLE_PROGRAM_BINARY_LOAD != ANGLE_ENABLED
    return Error(GL_NO_ERROR);
#else
    ASSERT(binaryFormat == mProgram->getBinaryFormat());

    BinaryInputStream stream(binary, length);

    GLenum format = stream.readInt<GLenum>();
    if (format != mProgram->getBinaryFormat())
    {
        mInfoLog << "Invalid program binary format.";
        return Error(GL_NO_ERROR);
    }

    int majorVersion = stream.readInt<int>();
    int minorVersion = stream.readInt<int>();
    if (majorVersion != ANGLE_MAJOR_VERSION || minorVersion != ANGLE_MINOR_VERSION)
    {
        mInfoLog << "Invalid program binary version.";
        return Error(GL_NO_ERROR);
    }

    unsigned char commitString[ANGLE_COMMIT_HASH_SIZE];
    stream.readBytes(commitString, ANGLE_COMMIT_HASH_SIZE);
    if (memcmp(commitString, ANGLE_COMMIT_HASH, sizeof(unsigned char) * ANGLE_COMMIT_HASH_SIZE) != 0)
    {
        mInfoLog << "Invalid program binary version.";
        return Error(GL_NO_ERROR);
    }

    // TODO(jmadill): replace MAX_VERTEX_ATTRIBS
    for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
    {
        stream.readInt(&mLinkedAttributes[i].type);
        stream.readString(&mLinkedAttributes[i].name);
        stream.readInt(&mProgram->getSemanticIndexes()[i]);
    }

    unsigned int attribCount = stream.readInt<unsigned int>();
    for (unsigned int attribIndex = 0; attribIndex < attribCount; ++attribIndex)
    {
        GLenum type = stream.readInt<GLenum>();
        GLenum precision = stream.readInt<GLenum>();
        std::string name = stream.readString();
        GLint arraySize = stream.readInt<GLint>();
        int location = stream.readInt<int>();
        mProgram->setShaderAttribute(attribIndex, type, precision, name, arraySize, location);
    }

    stream.readInt(&mData.mTransformFeedbackBufferMode);

    rx::LinkResult result = mProgram->load(mInfoLog, &stream);
    if (result.error.isError() || !result.linkSuccess)
    {
        return result.error;
    }

    mLinked = true;
    return Error(GL_NO_ERROR);
#endif // #if ANGLE_PROGRAM_BINARY_LOAD == ANGLE_ENABLED
}

Error Program::saveBinary(GLenum *binaryFormat, void *binary, GLsizei bufSize, GLsizei *length) const
{
    if (binaryFormat)
    {
        *binaryFormat = mProgram->getBinaryFormat();
    }

    BinaryOutputStream stream;

    stream.writeInt(mProgram->getBinaryFormat());
    stream.writeInt(ANGLE_MAJOR_VERSION);
    stream.writeInt(ANGLE_MINOR_VERSION);
    stream.writeBytes(reinterpret_cast<const unsigned char*>(ANGLE_COMMIT_HASH), ANGLE_COMMIT_HASH_SIZE);

    // TODO(jmadill): replace MAX_VERTEX_ATTRIBS
    for (unsigned int i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
    {
        stream.writeInt(mLinkedAttributes[i].type);
        stream.writeString(mLinkedAttributes[i].name);
        stream.writeInt(mProgram->getSemanticIndexes()[i]);
    }

    const auto &shaderAttributes = mProgram->getShaderAttributes();
    stream.writeInt(shaderAttributes.size());
    for (const auto &attrib : shaderAttributes)
    {
        stream.writeInt(attrib.type);
        stream.writeInt(attrib.precision);
        stream.writeString(attrib.name);
        stream.writeInt(attrib.arraySize);
        stream.writeInt(attrib.location);
    }

    stream.writeInt(mData.mTransformFeedbackBufferMode);

    gl::Error error = mProgram->save(&stream);
    if (error.isError())
    {
        return error;
    }

    GLsizei streamLength   = static_cast<GLsizei>(stream.length());
    const void *streamData = stream.data();

    if (streamLength > bufSize)
    {
        if (length)
        {
            *length = 0;
        }

        // TODO: This should be moved to the validation layer but computing the size of the binary before saving
        // it causes the save to happen twice.  It may be possible to write the binary to a separate buffer, validate
        // sizes and then copy it.
        return Error(GL_INVALID_OPERATION);
    }

    if (binary)
    {
        char *ptr = reinterpret_cast<char*>(binary);

        memcpy(ptr, streamData, streamLength);
        ptr += streamLength;

        ASSERT(ptr - streamLength == binary);
    }

    if (length)
    {
        *length = streamLength;
    }

    return Error(GL_NO_ERROR);
}

GLint Program::getBinaryLength() const
{
    GLint length;
    Error error = saveBinary(nullptr, nullptr, std::numeric_limits<GLint>::max(), &length);
    if (error.isError())
    {
        return 0;
    }

    return length;
}

void Program::release()
{
    mRefCount--;

    if (mRefCount == 0 && mDeleteStatus)
    {
        mResourceManager->deleteProgram(mHandle);
    }
}

void Program::addRef()
{
    mRefCount++;
}

unsigned int Program::getRefCount() const
{
    return mRefCount;
}

int Program::getInfoLogLength() const
{
    return static_cast<int>(mInfoLog.getLength());
}

void Program::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog)
{
    return mInfoLog.getLog(bufSize, length, infoLog);
}

void Program::getAttachedShaders(GLsizei maxCount, GLsizei *count, GLuint *shaders)
{
    int total = 0;

    if (mData.mAttachedVertexShader)
    {
        if (total < maxCount)
        {
            shaders[total] = mData.mAttachedVertexShader->getHandle();
        }

        total++;
    }

    if (mData.mAttachedFragmentShader)
    {
        if (total < maxCount)
        {
            shaders[total] = mData.mAttachedFragmentShader->getHandle();
        }

        total++;
    }

    if (count)
    {
        *count = total;
    }
}

GLuint Program::getAttributeLocation(const std::string &name)
{
    for (size_t index = 0; index < mLinkedAttributes.size(); index++)
    {
        if (mLinkedAttributes[index].name == name)
        {
            return static_cast<GLuint>(index);
        }
    }

    return static_cast<GLuint>(-1);
}

const int *Program::getSemanticIndexes() const
{
    return mProgram->getSemanticIndexes();
}

int Program::getSemanticIndex(int attributeIndex) const
{
    ASSERT(attributeIndex >= 0 && attributeIndex < MAX_VERTEX_ATTRIBS);

    return mProgram->getSemanticIndexes()[attributeIndex];
}

void Program::getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    if (mLinked)
    {
        // Skip over inactive attributes
        unsigned int activeAttribute = 0;
        unsigned int attribute;
        for (attribute = 0; attribute < static_cast<unsigned int>(mLinkedAttributes.size());
             attribute++)
        {
            if (mLinkedAttributes[attribute].name.empty())
            {
                continue;
            }

            if (activeAttribute == index)
            {
                break;
            }

            activeAttribute++;
        }

        if (bufsize > 0)
        {
            const char *string = mLinkedAttributes[attribute].name.c_str();

            strncpy(name, string, bufsize);
            name[bufsize - 1] = '\0';

            if (length)
            {
                *length = static_cast<GLsizei>(strlen(name));
            }
        }

        *size = 1;   // Always a single 'type' instance

        *type = mLinkedAttributes[attribute].type;
    }
    else
    {
        if (bufsize > 0)
        {
            name[0] = '\0';
        }

        if (length)
        {
            *length = 0;
        }

        *type = GL_NONE;
        *size = 1;
    }
}

GLint Program::getActiveAttributeCount()
{
    int count = 0;

    if (mLinked)
    {
        for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
        {
            if (!mLinkedAttributes[attributeIndex].name.empty())
            {
                count++;
            }
        }
    }

    return count;
}

GLint Program::getActiveAttributeMaxLength()
{
    GLint maxLength = 0;

    if (mLinked)
    {
        for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
        {
            if (!mLinkedAttributes[attributeIndex].name.empty())
            {
                maxLength = std::max(
                    static_cast<GLint>(mLinkedAttributes[attributeIndex].name.length() + 1),
                    maxLength);
            }
        }
    }

    return maxLength;
}

GLint Program::getFragDataLocation(const std::string &name) const
{
    std::string baseName(name);
    unsigned int arrayIndex = ParseAndStripArrayIndex(&baseName);
    for (auto outputPair : mOutputVariables)
    {
        const VariableLocation &outputVariable = outputPair.second;
        if (outputVariable.name == baseName && (arrayIndex == GL_INVALID_INDEX || arrayIndex == outputVariable.element))
        {
            return static_cast<GLint>(outputPair.first);
        }
    }
    return -1;
}

void Program::getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    if (mLinked)
    {
        ASSERT(index < mProgram->getUniforms().size());   // index must be smaller than getActiveUniformCount()
        LinkedUniform *uniform = mProgram->getUniforms()[index];

        if (bufsize > 0)
        {
            std::string string = uniform->name;
            if (uniform->isArray())
            {
                string += "[0]";
            }

            strncpy(name, string.c_str(), bufsize);
            name[bufsize - 1] = '\0';

            if (length)
            {
                *length = static_cast<GLsizei>(strlen(name));
            }
        }

        *size = uniform->elementCount();
        *type = uniform->type;
    }
    else
    {
        if (bufsize > 0)
        {
            name[0] = '\0';
        }

        if (length)
        {
            *length = 0;
        }

        *size = 0;
        *type = GL_NONE;
    }
}

GLint Program::getActiveUniformCount()
{
    if (mLinked)
    {
        return static_cast<GLint>(mProgram->getUniforms().size());
    }
    else
    {
        return 0;
    }
}

GLint Program::getActiveUniformMaxLength()
{
    int maxLength = 0;

    if (mLinked)
    {
        unsigned int numUniforms = static_cast<unsigned int>(mProgram->getUniforms().size());
        for (unsigned int uniformIndex = 0; uniformIndex < numUniforms; uniformIndex++)
        {
            if (!mProgram->getUniforms()[uniformIndex]->name.empty())
            {
                int length = (int)(mProgram->getUniforms()[uniformIndex]->name.length() + 1);
                if (mProgram->getUniforms()[uniformIndex]->isArray())
                {
                    length += 3;  // Counting in "[0]".
                }
                maxLength = std::max(length, maxLength);
            }
        }
    }

    return maxLength;
}

GLint Program::getActiveUniformi(GLuint index, GLenum pname) const
{
    const gl::LinkedUniform& uniform = *mProgram->getUniforms()[index];
    switch (pname)
    {
      case GL_UNIFORM_TYPE:         return static_cast<GLint>(uniform.type);
      case GL_UNIFORM_SIZE:         return static_cast<GLint>(uniform.elementCount());
      case GL_UNIFORM_NAME_LENGTH:  return static_cast<GLint>(uniform.name.size() + 1 + (uniform.isArray() ? 3 : 0));
      case GL_UNIFORM_BLOCK_INDEX:  return uniform.blockIndex;
      case GL_UNIFORM_OFFSET:       return uniform.blockInfo.offset;
      case GL_UNIFORM_ARRAY_STRIDE: return uniform.blockInfo.arrayStride;
      case GL_UNIFORM_MATRIX_STRIDE: return uniform.blockInfo.matrixStride;
      case GL_UNIFORM_IS_ROW_MAJOR: return static_cast<GLint>(uniform.blockInfo.isRowMajorMatrix);
      default:
        UNREACHABLE();
        break;
    }
    return 0;
}

bool Program::isValidUniformLocation(GLint location) const
{
    const auto &uniformIndices = mProgram->getUniformIndices();
    ASSERT(rx::IsIntegerCastSafe<GLint>(uniformIndices.size()));
    return (location >= 0 && uniformIndices.find(location) != uniformIndices.end());
}

LinkedUniform *Program::getUniformByLocation(GLint location) const
{
    return mProgram->getUniformByLocation(location);
}

LinkedUniform *Program::getUniformByName(const std::string &name) const
{
    return mProgram->getUniformByName(name);
}

GLint Program::getUniformLocation(const std::string &name)
{
    return mProgram->getUniformLocation(name);
}

GLuint Program::getUniformIndex(const std::string &name)
{
    return mProgram->getUniformIndex(name);
}

void Program::setUniform1fv(GLint location, GLsizei count, const GLfloat *v)
{
    mProgram->setUniform1fv(location, count, v);
}

void Program::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    mProgram->setUniform2fv(location, count, v);
}

void Program::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    mProgram->setUniform3fv(location, count, v);
}

void Program::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    mProgram->setUniform4fv(location, count, v);
}

void Program::setUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    mProgram->setUniform1iv(location, count, v);
}

void Program::setUniform2iv(GLint location, GLsizei count, const GLint *v)
{
    mProgram->setUniform2iv(location, count, v);
}

void Program::setUniform3iv(GLint location, GLsizei count, const GLint *v)
{
    mProgram->setUniform3iv(location, count, v);
}

void Program::setUniform4iv(GLint location, GLsizei count, const GLint *v)
{
    mProgram->setUniform4iv(location, count, v);
}

void Program::setUniform1uiv(GLint location, GLsizei count, const GLuint *v)
{
    mProgram->setUniform1uiv(location, count, v);
}

void Program::setUniform2uiv(GLint location, GLsizei count, const GLuint *v)
{
    mProgram->setUniform2uiv(location, count, v);
}

void Program::setUniform3uiv(GLint location, GLsizei count, const GLuint *v)
{
    mProgram->setUniform3uiv(location, count, v);
}

void Program::setUniform4uiv(GLint location, GLsizei count, const GLuint *v)
{
    mProgram->setUniform4uiv(location, count, v);
}

void Program::setUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    mProgram->setUniformMatrix2fv(location, count, transpose, v);
}

void Program::setUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    mProgram->setUniformMatrix3fv(location, count, transpose, v);
}

void Program::setUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    mProgram->setUniformMatrix4fv(location, count, transpose, v);
}

void Program::setUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    mProgram->setUniformMatrix2x3fv(location, count, transpose, v);
}

void Program::setUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    mProgram->setUniformMatrix2x4fv(location, count, transpose, v);
}

void Program::setUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    mProgram->setUniformMatrix3x2fv(location, count, transpose, v);
}

void Program::setUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    mProgram->setUniformMatrix3x4fv(location, count, transpose, v);
}

void Program::setUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    mProgram->setUniformMatrix4x2fv(location, count, transpose, v);
}

void Program::setUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *v)
{
    mProgram->setUniformMatrix4x3fv(location, count, transpose, v);
}

void Program::getUniformfv(GLint location, GLfloat *v)
{
    mProgram->getUniformfv(location, v);
}

void Program::getUniformiv(GLint location, GLint *v)
{
    mProgram->getUniformiv(location, v);
}

void Program::getUniformuiv(GLint location, GLuint *v)
{
    mProgram->getUniformuiv(location, v);
}

// Applies all the uniforms set for this program object to the renderer
Error Program::applyUniforms()
{
    return mProgram->applyUniforms();
}

void Program::flagForDeletion()
{
    mDeleteStatus = true;
}

bool Program::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}

void Program::validate(const Caps &caps)
{
    mInfoLog.reset();
    mValidated = false;

    if (mLinked)
    {
        applyUniforms();
        mValidated = mProgram->validateSamplers(&mInfoLog, caps);
    }
    else
    {
        mInfoLog << "Program has not been successfully linked.";
    }
}

bool Program::validateSamplers(InfoLog *infoLog, const Caps &caps)
{
    return mProgram->validateSamplers(infoLog, caps);
}

bool Program::isValidated() const
{
    return mValidated;
}

GLuint Program::getActiveUniformBlockCount()
{
    return static_cast<GLuint>(mProgram->getUniformBlocks().size());
}

void Program::getActiveUniformBlockName(GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) const
{
    ASSERT(uniformBlockIndex < mProgram->getUniformBlocks().size());   // index must be smaller than getActiveUniformBlockCount()

    const UniformBlock &uniformBlock = *mProgram->getUniformBlocks()[uniformBlockIndex];

    if (bufSize > 0)
    {
        std::string string = uniformBlock.name;

        if (uniformBlock.isArrayElement())
        {
            string += ArrayString(uniformBlock.elementIndex);
        }

        strncpy(uniformBlockName, string.c_str(), bufSize);
        uniformBlockName[bufSize - 1] = '\0';

        if (length)
        {
            *length = static_cast<GLsizei>(strlen(uniformBlockName));
        }
    }
}

void Program::getActiveUniformBlockiv(GLuint uniformBlockIndex, GLenum pname, GLint *params) const
{
    ASSERT(uniformBlockIndex < mProgram->getUniformBlocks().size());   // index must be smaller than getActiveUniformBlockCount()

    const UniformBlock &uniformBlock = *mProgram->getUniformBlocks()[uniformBlockIndex];

    switch (pname)
    {
      case GL_UNIFORM_BLOCK_DATA_SIZE:
        *params = static_cast<GLint>(uniformBlock.dataSize);
        break;
      case GL_UNIFORM_BLOCK_NAME_LENGTH:
        *params = static_cast<GLint>(uniformBlock.name.size() + 1 + (uniformBlock.isArrayElement() ? 3 : 0));
        break;
      case GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS:
        *params = static_cast<GLint>(uniformBlock.memberUniformIndexes.size());
        break;
      case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES:
        {
            for (unsigned int blockMemberIndex = 0; blockMemberIndex < uniformBlock.memberUniformIndexes.size(); blockMemberIndex++)
            {
                params[blockMemberIndex] = static_cast<GLint>(uniformBlock.memberUniformIndexes[blockMemberIndex]);
            }
        }
        break;
      case GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER:
        *params = static_cast<GLint>(uniformBlock.isReferencedByVertexShader());
        break;
      case GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER:
        *params = static_cast<GLint>(uniformBlock.isReferencedByFragmentShader());
        break;
      default: UNREACHABLE();
    }
}

GLint Program::getActiveUniformBlockMaxLength()
{
    int maxLength = 0;

    if (mLinked)
    {
        unsigned int numUniformBlocks =
            static_cast<unsigned int>(mProgram->getUniformBlocks().size());
        for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < numUniformBlocks; uniformBlockIndex++)
        {
            const UniformBlock &uniformBlock = *mProgram->getUniformBlocks()[uniformBlockIndex];
            if (!uniformBlock.name.empty())
            {
                const int length = static_cast<int>(uniformBlock.name.length()) + 1;

                // Counting in "[0]".
                const int arrayLength = (uniformBlock.isArrayElement() ? 3 : 0);

                maxLength = std::max(length + arrayLength, maxLength);
            }
        }
    }

    return maxLength;
}

GLuint Program::getUniformBlockIndex(const std::string &name)
{
    return mProgram->getUniformBlockIndex(name);
}

const UniformBlock *Program::getUniformBlockByIndex(GLuint index) const
{
    return mProgram->getUniformBlockByIndex(index);
}

void Program::bindUniformBlock(GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    mData.mUniformBlockBindings[uniformBlockIndex] = uniformBlockBinding;
}

GLuint Program::getUniformBlockBinding(GLuint uniformBlockIndex) const
{
    return mData.getUniformBlockBinding(uniformBlockIndex);
}

void Program::resetUniformBlockBindings()
{
    for (unsigned int blockId = 0; blockId < IMPLEMENTATION_MAX_COMBINED_SHADER_UNIFORM_BUFFERS; blockId++)
    {
        mData.mUniformBlockBindings[blockId] = 0;
    }
}

void Program::setTransformFeedbackVaryings(GLsizei count, const GLchar *const *varyings, GLenum bufferMode)
{
    mData.mTransformFeedbackVaryingNames.resize(count);
    for (GLsizei i = 0; i < count; i++)
    {
        mData.mTransformFeedbackVaryingNames[i] = varyings[i];
    }

    mData.mTransformFeedbackBufferMode = bufferMode;
}

void Program::getTransformFeedbackVarying(GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) const
{
    if (mLinked)
    {
        ASSERT(index < mData.mTransformFeedbackVaryingVars.size());
        const sh::Varying &varying = mData.mTransformFeedbackVaryingVars[index];
        GLsizei lastNameIdx = std::min(bufSize - 1, static_cast<GLsizei>(varying.name.length()));
        if (length)
        {
            *length = lastNameIdx;
        }
        if (size)
        {
            *size = varying.elementCount();
        }
        if (type)
        {
            *type = varying.type;
        }
        if (name)
        {
            memcpy(name, varying.name.c_str(), lastNameIdx);
            name[lastNameIdx] = '\0';
        }
    }
}

GLsizei Program::getTransformFeedbackVaryingCount() const
{
    if (mLinked)
    {
        return static_cast<GLsizei>(mData.mTransformFeedbackVaryingVars.size());
    }
    else
    {
        return 0;
    }
}

GLsizei Program::getTransformFeedbackVaryingMaxLength() const
{
    if (mLinked)
    {
        GLsizei maxSize = 0;
        for (const sh::Varying &varying : mData.mTransformFeedbackVaryingVars)
        {
            maxSize = std::max(maxSize, static_cast<GLsizei>(varying.name.length() + 1));
        }

        return maxSize;
    }
    else
    {
        return 0;
    }
}

GLenum Program::getTransformFeedbackBufferMode() const
{
    return mData.mTransformFeedbackBufferMode;
}

// static
bool Program::linkVaryings(InfoLog &infoLog,
                           const Shader *vertexShader,
                           const Shader *fragmentShader)
{
    const std::vector<PackedVarying> &vertexVaryings   = vertexShader->getVaryings();
    const std::vector<PackedVarying> &fragmentVaryings = fragmentShader->getVaryings();

    for (const PackedVarying &output : fragmentVaryings)
    {
        bool matched = false;

        // Built-in varyings obey special rules
        if (output.isBuiltIn())
        {
            continue;
        }

        for (const PackedVarying &input : vertexVaryings)
        {
            if (output.name == input.name)
            {
                ASSERT(!input.isBuiltIn());
                if (!linkValidateVaryings(infoLog, output.name, input, output))
                {
                    return false;
                }

                matched = true;
                break;
            }
        }

        // We permit unmatched, unreferenced varyings
        if (!matched && output.staticUse)
        {
            infoLog << "Fragment varying " << output.name << " does not match any vertex varying";
            return false;
        }
    }

    // TODO(jmadill): verify no unmatched vertex varyings?

    return true;
}

bool Program::linkUniforms(gl::InfoLog &infoLog, const gl::Caps & /*caps*/) const
{
    const std::vector<sh::Uniform> &vertexUniforms   = mData.mAttachedVertexShader->getUniforms();
    const std::vector<sh::Uniform> &fragmentUniforms = mData.mAttachedFragmentShader->getUniforms();

    // Check that uniforms defined in the vertex and fragment shaders are identical
    std::map<std::string, const sh::Uniform *> linkedUniforms;

    for (const sh::Uniform &vertexUniform : vertexUniforms)
    {
        linkedUniforms[vertexUniform.name] = &vertexUniform;
    }

    for (const sh::Uniform &fragmentUniform : fragmentUniforms)
    {
        auto entry = linkedUniforms.find(fragmentUniform.name);
        if (entry != linkedUniforms.end())
        {
            const sh::Uniform &vertexUniform = *entry->second;
            const std::string &uniformName = "uniform '" + vertexUniform.name + "'";
            if (!linkValidateUniforms(infoLog, uniformName, vertexUniform, fragmentUniform))
            {
                return false;
            }
        }
    }

    // TODO(jmadill): check sampler uniforms with caps
    return true;
}

bool Program::linkValidateInterfaceBlockFields(InfoLog &infoLog, const std::string &uniformName, const sh::InterfaceBlockField &vertexUniform, const sh::InterfaceBlockField &fragmentUniform)
{
    if (!linkValidateVariablesBase(infoLog, uniformName, vertexUniform, fragmentUniform, true))
    {
        return false;
    }

    if (vertexUniform.isRowMajorLayout != fragmentUniform.isRowMajorLayout)
    {
        infoLog << "Matrix packings for " << uniformName << " differ between vertex and fragment shaders";
        return false;
    }

    return true;
}

// Determines the mapping between GL attributes and Direct3D 9 vertex stream usage indices
bool Program::linkAttributes(const gl::Data &data,
                             InfoLog &infoLog,
                             const AttributeBindings &attributeBindings,
                             const Shader *vertexShader)
{
    unsigned int usedLocations = 0;
    const std::vector<sh::Attribute> &shaderAttributes = vertexShader->getActiveAttributes();
    GLuint maxAttribs = data.caps->maxVertexAttributes;

    // TODO(jmadill): handle aliasing robustly
    if (shaderAttributes.size() > maxAttribs)
    {
        infoLog << "Too many vertex attributes.";
        return false;
    }

    // Link attributes that have a binding location
    for (unsigned int attributeIndex = 0; attributeIndex < shaderAttributes.size(); attributeIndex++)
    {
        const sh::Attribute &attribute = shaderAttributes[attributeIndex];

        ASSERT(attribute.staticUse);

        const int location = attribute.location == -1 ? attributeBindings.getAttributeBinding(attribute.name) : attribute.location;

        mProgram->setShaderAttribute(attributeIndex, attribute);

        if (location != -1)   // Set by glBindAttribLocation or by location layout qualifier
        {
            const int rows = VariableRegisterCount(attribute.type);

            if (static_cast<GLuint>(rows + location) > maxAttribs)
            {
                infoLog << "Active attribute (" << attribute.name << ") at location "
                        << location << " is too big to fit";

                return false;
            }

            for (int row = 0; row < rows; row++)
            {
                const int rowLocation = location + row;
                sh::ShaderVariable *linkedAttribute = &mLinkedAttributes[rowLocation];

                // In GLSL 3.00, attribute aliasing produces a link error
                // In GLSL 1.00, attribute aliasing is allowed, but ANGLE currently has a bug
                // TODO(jmadill): fix aliasing on ES2
                // if (mProgram->getShaderVersion() >= 300)
                {
                    if (!linkedAttribute->name.empty())
                    {
                        infoLog << "Attribute '" << attribute.name << "' aliases attribute '"
                                << linkedAttribute->name << "' at location " << rowLocation;
                        return false;
                    }
                }

                *linkedAttribute = attribute;
                usedLocations |= 1 << rowLocation;
            }
        }
    }

    // Link attributes that don't have a binding location
    for (unsigned int attributeIndex = 0; attributeIndex < shaderAttributes.size(); attributeIndex++)
    {
        const sh::Attribute &attribute = shaderAttributes[attributeIndex];

        ASSERT(attribute.staticUse);

        const int location = attribute.location == -1 ? attributeBindings.getAttributeBinding(attribute.name) : attribute.location;

        if (location == -1)   // Not set by glBindAttribLocation or by location layout qualifier
        {
            int rows = VariableRegisterCount(attribute.type);
            int availableIndex = AllocateFirstFreeBits(&usedLocations, rows, maxAttribs);

            if (availableIndex == -1 || static_cast<GLuint>(availableIndex + rows) > maxAttribs)
            {
                infoLog << "Too many active attributes (" << attribute.name << ")";
                return false;   // Fail to link
            }

            mLinkedAttributes[availableIndex] = attribute;
        }
    }

    for (GLuint attributeIndex = 0; attributeIndex < maxAttribs;)
    {
        int index = vertexShader->getSemanticIndex(mLinkedAttributes[attributeIndex].name);
        int rows  = VariableRegisterCount(mLinkedAttributes[attributeIndex].type);

        for (int r = 0; r < rows; r++)
        {
            mProgram->getSemanticIndexes()[attributeIndex++] = index++;
        }
    }

    return true;
}

bool Program::linkUniformBlocks(InfoLog &infoLog, const Caps &caps)
{
    const Shader &vertexShader   = *mData.mAttachedVertexShader;
    const Shader &fragmentShader = *mData.mAttachedFragmentShader;

    const std::vector<sh::InterfaceBlock> &vertexInterfaceBlocks = vertexShader.getInterfaceBlocks();
    const std::vector<sh::InterfaceBlock> &fragmentInterfaceBlocks = fragmentShader.getInterfaceBlocks();

    // Check that interface blocks defined in the vertex and fragment shaders are identical
    typedef std::map<std::string, const sh::InterfaceBlock*> UniformBlockMap;
    UniformBlockMap linkedUniformBlocks;

    GLuint vertexBlockCount = 0;
    for (const sh::InterfaceBlock &vertexInterfaceBlock : vertexInterfaceBlocks)
    {
        linkedUniformBlocks[vertexInterfaceBlock.name] = &vertexInterfaceBlock;

        // Note: shared and std140 layouts are always considered active
        if (vertexInterfaceBlock.staticUse || vertexInterfaceBlock.layout != sh::BLOCKLAYOUT_PACKED)
        {
            if (++vertexBlockCount > caps.maxVertexUniformBlocks)
            {
                infoLog << "Vertex shader uniform block count exceed GL_MAX_VERTEX_UNIFORM_BLOCKS ("
                        << caps.maxVertexUniformBlocks << ")";
                return false;
            }
        }
    }

    GLuint fragmentBlockCount = 0;
    for (const sh::InterfaceBlock &fragmentInterfaceBlock : fragmentInterfaceBlocks)
    {
        auto entry = linkedUniformBlocks.find(fragmentInterfaceBlock.name);
        if (entry != linkedUniformBlocks.end())
        {
            const sh::InterfaceBlock &vertexInterfaceBlock = *entry->second;
            if (!areMatchingInterfaceBlocks(infoLog, vertexInterfaceBlock, fragmentInterfaceBlock))
            {
                return false;
            }
        }

        // Note: shared and std140 layouts are always considered active
        if (fragmentInterfaceBlock.staticUse ||
            fragmentInterfaceBlock.layout != sh::BLOCKLAYOUT_PACKED)
        {
            if (++fragmentBlockCount > caps.maxFragmentUniformBlocks)
            {
                infoLog
                    << "Fragment shader uniform block count exceed GL_MAX_FRAGMENT_UNIFORM_BLOCKS ("
                    << caps.maxFragmentUniformBlocks << ")";
                return false;
            }
        }
    }

    return true;
}

bool Program::areMatchingInterfaceBlocks(gl::InfoLog &infoLog, const sh::InterfaceBlock &vertexInterfaceBlock,
                                         const sh::InterfaceBlock &fragmentInterfaceBlock)
{
    const char* blockName = vertexInterfaceBlock.name.c_str();
    // validate blocks for the same member types
    if (vertexInterfaceBlock.fields.size() != fragmentInterfaceBlock.fields.size())
    {
        infoLog << "Types for interface block '" << blockName
                << "' differ between vertex and fragment shaders";
        return false;
    }
    if (vertexInterfaceBlock.arraySize != fragmentInterfaceBlock.arraySize)
    {
        infoLog << "Array sizes differ for interface block '" << blockName
                << "' between vertex and fragment shaders";
        return false;
    }
    if (vertexInterfaceBlock.layout != fragmentInterfaceBlock.layout || vertexInterfaceBlock.isRowMajorLayout != fragmentInterfaceBlock.isRowMajorLayout)
    {
        infoLog << "Layout qualifiers differ for interface block '" << blockName
                << "' between vertex and fragment shaders";
        return false;
    }
    const unsigned int numBlockMembers =
        static_cast<unsigned int>(vertexInterfaceBlock.fields.size());
    for (unsigned int blockMemberIndex = 0; blockMemberIndex < numBlockMembers; blockMemberIndex++)
    {
        const sh::InterfaceBlockField &vertexMember = vertexInterfaceBlock.fields[blockMemberIndex];
        const sh::InterfaceBlockField &fragmentMember = fragmentInterfaceBlock.fields[blockMemberIndex];
        if (vertexMember.name != fragmentMember.name)
        {
            infoLog << "Name mismatch for field " << blockMemberIndex
                    << " of interface block '" << blockName
                    << "': (in vertex: '" << vertexMember.name
                    << "', in fragment: '" << fragmentMember.name << "')";
            return false;
        }
        std::string memberName = "interface block '" + vertexInterfaceBlock.name + "' member '" + vertexMember.name + "'";
        if (!linkValidateInterfaceBlockFields(infoLog, memberName, vertexMember, fragmentMember))
        {
            return false;
        }
    }
    return true;
}

bool Program::linkValidateVariablesBase(InfoLog &infoLog, const std::string &variableName, const sh::ShaderVariable &vertexVariable,
                                              const sh::ShaderVariable &fragmentVariable, bool validatePrecision)
{
    if (vertexVariable.type != fragmentVariable.type)
    {
        infoLog << "Types for " << variableName << " differ between vertex and fragment shaders";
        return false;
    }
    if (vertexVariable.arraySize != fragmentVariable.arraySize)
    {
        infoLog << "Array sizes for " << variableName << " differ between vertex and fragment shaders";
        return false;
    }
    if (validatePrecision && vertexVariable.precision != fragmentVariable.precision)
    {
        infoLog << "Precisions for " << variableName << " differ between vertex and fragment shaders";
        return false;
    }

    if (vertexVariable.fields.size() != fragmentVariable.fields.size())
    {
        infoLog << "Structure lengths for " << variableName << " differ between vertex and fragment shaders";
        return false;
    }
    const unsigned int numMembers = static_cast<unsigned int>(vertexVariable.fields.size());
    for (unsigned int memberIndex = 0; memberIndex < numMembers; memberIndex++)
    {
        const sh::ShaderVariable &vertexMember = vertexVariable.fields[memberIndex];
        const sh::ShaderVariable &fragmentMember = fragmentVariable.fields[memberIndex];

        if (vertexMember.name != fragmentMember.name)
        {
            infoLog << "Name mismatch for field '" << memberIndex
                    << "' of " << variableName
                    << ": (in vertex: '" << vertexMember.name
                    << "', in fragment: '" << fragmentMember.name << "')";
            return false;
        }

        const std::string memberName = variableName.substr(0, variableName.length() - 1) + "." +
                                       vertexMember.name + "'";

        if (!linkValidateVariablesBase(infoLog, vertexMember.name, vertexMember, fragmentMember, validatePrecision))
        {
            return false;
        }
    }

    return true;
}

bool Program::linkValidateUniforms(InfoLog &infoLog, const std::string &uniformName, const sh::Uniform &vertexUniform, const sh::Uniform &fragmentUniform)
{
#if ANGLE_PROGRAM_LINK_VALIDATE_UNIFORM_PRECISION == ANGLE_ENABLED
    const bool validatePrecision = true;
#else
    const bool validatePrecision = false;
#endif

    if (!linkValidateVariablesBase(infoLog, uniformName, vertexUniform, fragmentUniform, validatePrecision))
    {
        return false;
    }

    return true;
}

bool Program::linkValidateVaryings(InfoLog &infoLog, const std::string &varyingName, const sh::Varying &vertexVarying, const sh::Varying &fragmentVarying)
{
    if (!linkValidateVariablesBase(infoLog, varyingName, vertexVarying, fragmentVarying, false))
    {
        return false;
    }

    if (!sh::InterpolationTypesMatch(vertexVarying.interpolation, fragmentVarying.interpolation))
    {
        infoLog << "Interpolation types for " << varyingName << " differ between vertex and fragment shaders";
        return false;
    }

    return true;
}

bool Program::linkValidateTransformFeedback(InfoLog &infoLog,
                                            const std::vector<const sh::Varying *> &varyings,
                                            const Caps &caps) const
{
    size_t totalComponents = 0;

    std::set<std::string> uniqueNames;

    for (const std::string &tfVaryingName : mData.mTransformFeedbackVaryingNames)
    {
        bool found = false;
        for (const sh::Varying *varying : varyings)
        {
            if (tfVaryingName == varying->name)
            {
                if (uniqueNames.count(tfVaryingName) > 0)
                {
                    infoLog << "Two transform feedback varyings specify the same output variable ("
                            << tfVaryingName << ").";
                    return false;
                }
                uniqueNames.insert(tfVaryingName);

                // TODO(jmadill): Investigate implementation limits on D3D11
                size_t componentCount = gl::VariableComponentCount(varying->type);
                if (mData.mTransformFeedbackBufferMode == GL_SEPARATE_ATTRIBS &&
                    componentCount > caps.maxTransformFeedbackSeparateComponents)
                {
                    infoLog << "Transform feedback varying's " << varying->name << " components ("
                            << componentCount << ") exceed the maximum separate components ("
                            << caps.maxTransformFeedbackSeparateComponents << ").";
                    return false;
                }

                totalComponents += componentCount;
                found = true;
                break;
            }
        }

        // All transform feedback varyings are expected to exist since packVaryings checks for them.
        ASSERT(found);
        UNUSED_ASSERTION_VARIABLE(found);
    }

    if (mData.mTransformFeedbackBufferMode == GL_INTERLEAVED_ATTRIBS &&
        totalComponents > caps.maxTransformFeedbackInterleavedComponents)
    {
        infoLog << "Transform feedback varying total components (" << totalComponents
                << ") exceed the maximum interleaved components ("
                << caps.maxTransformFeedbackInterleavedComponents << ").";
        return false;
    }

    return true;
}

void Program::gatherTransformFeedbackVaryings(const std::vector<const sh::Varying *> &varyings)
{
    // Gather the linked varyings that are used for transform feedback, they should all exist.
    mData.mTransformFeedbackVaryingVars.clear();
    for (const std::string &tfVaryingName : mData.mTransformFeedbackVaryingNames)
    {
        for (const sh::Varying *varying : varyings)
        {
            if (tfVaryingName == varying->name)
            {
                mData.mTransformFeedbackVaryingVars.push_back(*varying);
                break;
            }
        }
    }
}

std::vector<const sh::Varying *> Program::getMergedVaryings() const
{
    std::set<std::string> uniqueNames;
    std::vector<const sh::Varying *> varyings;

    for (const sh::Varying &varying : mData.mAttachedVertexShader->getVaryings())
    {
        if (uniqueNames.count(varying.name) == 0)
        {
            uniqueNames.insert(varying.name);
            varyings.push_back(&varying);
        }
    }

    for (const sh::Varying &varying : mData.mAttachedFragmentShader->getVaryings())
    {
        if (uniqueNames.count(varying.name) == 0)
        {
            uniqueNames.insert(varying.name);
            varyings.push_back(&varying);
        }
    }

    return varyings;
}
}
