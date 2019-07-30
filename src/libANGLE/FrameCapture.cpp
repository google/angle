//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FrameCapture.cpp:
//   ANGLE Frame capture implementation.
//

#include "libANGLE/FrameCapture.h"

#include <string>

#include "libANGLE/Context.h"
#include "libANGLE/VertexArray.h"

namespace angle
{
#if !ANGLE_CAPTURE_ENABLED
CallCapture::~CallCapture() {}
ParamBuffer::~ParamBuffer() {}
ParamCapture::~ParamCapture() {}

FrameCapture::FrameCapture() {}
FrameCapture::~FrameCapture() {}
void FrameCapture::onEndFrame() {}
#else
namespace
{
std::string GetCaptureFileName(size_t frameIndex, const char *suffix)
{
    std::stringstream fnameStream;
    fnameStream << "angle_capture_frame" << std::setfill('0') << std::setw(3) << frameIndex
                << suffix;
    return fnameStream.str();
}

void WriteParamStaticVarName(const CallCapture &call,
                             const ParamCapture &param,
                             int counter,
                             std::ostream &out)
{
    out << call.name() << "_" << param.name << "_" << counter;
}

template <typename T, typename CastT = T>
void WriteInlineData(const std::vector<uint8_t> &vec, std::ostream &out)
{
    const T *data = reinterpret_cast<const T *>(vec.data());
    size_t count  = vec.size() / sizeof(T);

    out << static_cast<CastT>(data[0]);

    for (size_t dataIndex = 1; dataIndex < count; ++dataIndex)
    {
        out << ", " << static_cast<CastT>(data[dataIndex]);
    }
}

constexpr size_t kInlineDataThreshold = 128;
}  // anonymous namespace

ParamCapture::ParamCapture() : type(ParamType::TGLenum) {}

ParamCapture::ParamCapture(const char *nameIn, ParamType typeIn) : name(nameIn), type(typeIn) {}

ParamCapture::~ParamCapture() = default;

ParamCapture::ParamCapture(ParamCapture &&other) : type(ParamType::TGLenum)
{
    *this = std::move(other);
}

ParamCapture &ParamCapture::operator=(ParamCapture &&other)
{
    std::swap(name, other.name);
    std::swap(type, other.type);
    std::swap(value, other.value);
    std::swap(data, other.data);
    std::swap(arrayClientPointerIndex, other.arrayClientPointerIndex);
    std::swap(readBufferSizeBytes, other.readBufferSizeBytes);
    return *this;
}

ParamBuffer::ParamBuffer() {}

ParamBuffer::~ParamBuffer() = default;

ParamBuffer::ParamBuffer(ParamBuffer &&other)
{
    *this = std::move(other);
}

ParamBuffer &ParamBuffer::operator=(ParamBuffer &&other)
{
    std::swap(mParamCaptures, other.mParamCaptures);
    std::swap(mClientArrayDataParam, other.mClientArrayDataParam);
    std::swap(mReadBufferSize, other.mReadBufferSize);
    return *this;
}

ParamCapture &ParamBuffer::getParam(const char *paramName, ParamType paramType, int index)
{
    ParamCapture &capture = mParamCaptures[index];
    ASSERT(capture.name == paramName);
    ASSERT(capture.type == paramType);
    return capture;
}

void ParamBuffer::addParam(ParamCapture &&param)
{
    if (param.arrayClientPointerIndex != -1)
    {
        ASSERT(mClientArrayDataParam == -1);
        mClientArrayDataParam = static_cast<int>(mParamCaptures.size());
    }

    mReadBufferSize = std::max(param.readBufferSizeBytes, mReadBufferSize);
    mParamCaptures.emplace_back(std::move(param));
}

void ParamBuffer::addReturnValue(ParamCapture &&returnValue)
{
    mReturnValueCapture = std::move(returnValue);
}

ParamCapture &ParamBuffer::getClientArrayPointerParameter()
{
    ASSERT(hasClientArrayData());
    return mParamCaptures[mClientArrayDataParam];
}

CallCapture::CallCapture(gl::EntryPoint entryPointIn, ParamBuffer &&paramsIn)
    : entryPoint(entryPointIn), params(std::move(paramsIn))
{}

CallCapture::CallCapture(const std::string &customFunctionNameIn, ParamBuffer &&paramsIn)
    : entryPoint(gl::EntryPoint::Invalid),
      customFunctionName(customFunctionNameIn),
      params(std::move(paramsIn))
{}

CallCapture::~CallCapture() = default;

CallCapture::CallCapture(CallCapture &&other)
{
    *this = std::move(other);
}

CallCapture &CallCapture::operator=(CallCapture &&other)
{
    std::swap(entryPoint, other.entryPoint);
    std::swap(customFunctionName, other.customFunctionName);
    std::swap(params, other.params);
    return *this;
}

const char *CallCapture::name() const
{
    if (entryPoint == gl::EntryPoint::Invalid)
    {
        ASSERT(!customFunctionName.empty());
        return customFunctionName.c_str();
    }

    return gl::GetEntryPointName(entryPoint);
}

FrameCapture::FrameCapture() : mFrameIndex(0), mReadBufferSize(0)
{
    reset();
}

FrameCapture::~FrameCapture() = default;

void FrameCapture::captureCall(const gl::Context *context, CallCapture &&call)
{
    if (call.entryPoint == gl::EntryPoint::VertexAttribPointer)
    {
        // Get array location
        GLuint index = call.params.getParam("index", ParamType::TGLuint, 0).value.GLuintVal;

        if (call.params.hasClientArrayData())
        {
            mClientVertexArrayMap[index] = mCalls.size();
        }
        else
        {
            mClientVertexArrayMap[index] = -1;
        }
    }
    else if (call.entryPoint == gl::EntryPoint::DrawArrays)
    {
        if (context->getStateCache().hasAnyActiveClientAttrib())
        {
            // Get counts from paramBuffer.
            GLint firstVertex = call.params.getParam("first", ParamType::TGLint, 1).value.GLintVal;
            GLsizei drawCount =
                call.params.getParam("count", ParamType::TGLsizei, 2).value.GLsizeiVal;
            captureClientArraySnapshot(context, firstVertex + drawCount, 1);
        }
    }
    else if (call.entryPoint == gl::EntryPoint::DrawElements)
    {
        if (context->getStateCache().hasAnyActiveClientAttrib())
        {
            GLsizei count = call.params.getParam("count", ParamType::TGLsizei, 1).value.GLsizeiVal;
            gl::DrawElementsType drawElementsType =
                call.params.getParam("typePacked", ParamType::TDrawElementsType, 2)
                    .value.DrawElementsTypeVal;
            const void *indices = call.params.getParam("indices", ParamType::TvoidConstPointer, 3)
                                      .value.voidConstPointerVal;

            gl::IndexRange indexRange;

            bool restart = context->getState().isPrimitiveRestartEnabled();

            gl::Buffer *elementArrayBuffer =
                context->getState().getVertexArray()->getElementArrayBuffer();
            if (elementArrayBuffer)
            {
                size_t offset = reinterpret_cast<size_t>(indices);
                (void)elementArrayBuffer->getIndexRange(context, drawElementsType, offset, count,
                                                        restart, &indexRange);
            }
            else
            {
                indexRange = gl::ComputeIndexRange(drawElementsType, indices, count, restart);
            }

            captureClientArraySnapshot(context, indexRange.end, 1);
        }
    }

    mReadBufferSize = std::max(mReadBufferSize, call.params.getReadBufferSize());
    mCalls.emplace_back(std::move(call));
}

void FrameCapture::captureClientArraySnapshot(const gl::Context *context,
                                              size_t vertexCount,
                                              size_t instanceCount)
{
    const gl::VertexArray *vao = context->getState().getVertexArray();

    // Capture client array data.
    for (size_t attribIndex : context->getStateCache().getActiveClientAttribsMask())
    {
        const gl::VertexAttribute &attrib = vao->getVertexAttribute(attribIndex);
        const gl::VertexBinding &binding  = vao->getVertexBinding(attrib.bindingIndex);

        int callIndex = mClientVertexArrayMap[attribIndex];

        if (callIndex != -1)
        {
            size_t count = vertexCount;

            if (binding.getDivisor() > 0)
            {
                count = rx::UnsignedCeilDivide(instanceCount, binding.getDivisor());
            }

            // The last capture element doesn't take up the full stride.
            size_t bytesToCapture = (count - 1) * binding.getStride() + attrib.format->pixelBytes;

            CallCapture &call   = mCalls[callIndex];
            ParamCapture &param = call.params.getClientArrayPointerParameter();
            ASSERT(param.type == ParamType::TvoidConstPointer);

            ParamBuffer updateParamBuffer;
            updateParamBuffer.addValueParam<GLint>("arrayIndex", ParamType::TGLint, attribIndex);

            ParamCapture updateMemory("pointer", ParamType::TvoidConstPointer);
            CaptureMemory(param.value.voidConstPointerVal, bytesToCapture, &updateMemory);
            updateParamBuffer.addParam(std::move(updateMemory));

            mCalls.emplace_back("UpdateClientArrayPointer", std::move(updateParamBuffer));

            mClientArraySizes[attribIndex] =
                std::max(mClientArraySizes[attribIndex], bytesToCapture);
        }
    }
}

bool FrameCapture::anyClientArray() const
{
    for (size_t size : mClientArraySizes)
    {
        if (size > 0)
            return true;
    }

    return false;
}

void FrameCapture::onEndFrame()
{
    if (!mCalls.empty())
    {
        saveCapturedFrameAsCpp();
    }

    reset();
    mFrameIndex++;
}

void FrameCapture::saveCapturedFrameAsCpp()
{
    bool useClientArrays = anyClientArray();

    std::stringstream out;
    std::stringstream header;
    std::vector<uint8_t> binaryData;

    header << "#include \"util/gles_loader_autogen.h\"\n";
    header << "\n";
    header << "#include <cstdio>\n";
    header << "#include <vector>\n";
    header << "\n";
    header << "namespace\n";
    header << "{\n";
    if (mReadBufferSize > 0)
    {
        header << "std::vector<uint8_t> gReadBuffer;\n";
    }
    if (useClientArrays)
    {
        header << "std::vector<uint8_t> gClientArrays[" << gl::MAX_VERTEX_ATTRIBS << "];\n";
        header << "template <size_t N>\n";
        header << "void UpdateClientArrayPointer(int arrayIndex, const uint8_t (&data)[N])\n";
        header << "{\n";
        header << "    memcpy(gClientArrays[arrayIndex].data(), data, N);\n";
        header << "}\n";
    }

    out << "void ReplayFrame" << mFrameIndex << "()\n";
    out << "{\n";
    out << "    LoadBinaryData();\n";

    for (size_t arrayIndex = 0; arrayIndex < mClientArraySizes.size(); ++arrayIndex)
    {
        if (mClientArraySizes[arrayIndex] > 0)
        {
            out << "    gClientArrays[" << arrayIndex << "].resize("
                << mClientArraySizes[arrayIndex] << ");\n";
        }
    }

    if (mReadBufferSize > 0)
    {
        out << "    gReadBuffer.resize(" << mReadBufferSize << ");\n";
    }

    for (const CallCapture &call : mCalls)
    {
        out << "    ";
        writeCallReplay(call, out, header, &binaryData);
        out << ";\n";
    }

    if (!binaryData.empty())
    {
        std::string fname = GetCaptureFileName(mFrameIndex, ".angledata");

        FILE *fp = fopen(fname.c_str(), "wb");
        fwrite(binaryData.data(), 1, binaryData.size(), fp);
        fclose(fp);

        header << "std::vector<uint8_t> gBinaryData;\n";
        header << "void LoadBinaryData()\n";
        header << "{\n";
        header << "    gBinaryData.resize(" << static_cast<int>(binaryData.size()) << ");\n";
        header << "    FILE *fp = fopen(\"" << fname << "\", \"rb\");\n";
        header << "    fread(gBinaryData.data(), 1, " << static_cast<int>(binaryData.size())
               << ", fp);\n";
        header << "    fclose(fp);\n";
        header << "}\n";
    }
    else
    {
        header << "// No binary data.\n";
        header << "void LoadBinaryData() {}\n";
    }

    out << "}\n";

    header << "}  // anonymous namespace\n";

    std::string outString    = out.str();
    std::string headerString = header.str();

    std::string fname = GetCaptureFileName(mFrameIndex, ".cpp");
    FILE *fp          = fopen(fname.c_str(), "w");
    fprintf(fp, "%s\n\n%s", headerString.c_str(), outString.c_str());
    fclose(fp);

    printf("Saved '%s'.\n", fname.c_str());
}

int FrameCapture::getAndIncrementCounter(gl::EntryPoint entryPoint, const std::string &paramName)
{
    auto counterKey = std::tie(entryPoint, paramName);
    return mDataCounters[counterKey]++;
}

void FrameCapture::writeCallReplay(const CallCapture &call,
                                   std::ostream &out,
                                   std::ostream &header,
                                   std::vector<uint8_t> *binaryData)
{
    out << call.name() << "(";

    bool first = true;
    for (const ParamCapture &param : call.params.getParamCaptures())
    {
        if (!first)
        {
            out << ", ";
        }

        if (param.arrayClientPointerIndex != -1)
        {
            out << "gClientArrays[" << param.arrayClientPointerIndex << "].data()";
        }
        else if (param.readBufferSizeBytes > 0)
        {
            out << "reinterpret_cast<" << ParamTypeToString(param.type) << ">(gReadBuffer.data())";
        }
        else if (param.data.empty())
        {
            out << param;
        }
        else
        {
            if (param.type == ParamType::TGLcharConstPointer)
            {
                const std::vector<uint8_t> &data = param.data[0];
                std::string str(data.begin(), data.end());
                out << "\"" << str << "\"";
            }
            else if (param.type == ParamType::TGLcharConstPointerPointer)
            {
                int counter = getAndIncrementCounter(call.entryPoint, param.name);

                header << "const char *";
                WriteParamStaticVarName(call, param, counter, header);
                header << "[] = { \n";

                for (const std::vector<uint8_t> &data : param.data)
                {
                    std::string str(data.begin(), data.end());
                    header << "    R\"(" << str << ")\",\n";
                }

                header << " };\n";
                WriteParamStaticVarName(call, param, counter, out);
            }
            else
            {
                int counter = getAndIncrementCounter(call.entryPoint, param.name);

                ASSERT(param.data.size() == 1);
                const std::vector<uint8_t> &data = param.data[0];

                if (data.size() > kInlineDataThreshold)
                {
                    size_t offset = binaryData->size();
                    binaryData->resize(offset + data.size());
                    memcpy(binaryData->data() + offset, data.data(), data.size());
                    out << "reinterpret_cast<" << ParamTypeToString(param.type) << ">(&gBinaryData["
                        << offset << "])";
                }
                else
                {
                    ParamType overrideType = param.type;
                    if (param.type == ParamType::TGLvoidConstPointer ||
                        param.type == ParamType::TvoidConstPointer)
                    {
                        overrideType = ParamType::TGLubyteConstPointer;
                    }

                    std::string paramTypeString = ParamTypeToString(overrideType);
                    header << paramTypeString.substr(0, paramTypeString.length() - 1);
                    WriteParamStaticVarName(call, param, counter, header);

                    header << "[] = { ";

                    switch (overrideType)
                    {
                        case ParamType::TGLintConstPointer:
                            WriteInlineData<GLint>(data, header);
                            break;
                        case ParamType::TGLshortConstPointer:
                            WriteInlineData<GLshort>(data, header);
                            break;
                        case ParamType::TGLfloatConstPointer:
                            WriteInlineData<GLfloat>(data, header);
                            break;
                        case ParamType::TGLubyteConstPointer:
                            WriteInlineData<GLubyte, int>(data, header);
                            break;
                        case ParamType::TGLuintConstPointer:
                        case ParamType::TGLenumConstPointer:
                            WriteInlineData<GLuint>(data, header);
                            break;
                        default:
                            UNIMPLEMENTED();
                            break;
                    }

                    header << " };\n";

                    WriteParamStaticVarName(call, param, counter, out);
                }
            }
        }

        first = false;
    }

    out << ")";
}

bool FrameCapture::enabled() const
{
    return mFrameIndex < 100;
}

void FrameCapture::reset()
{
    mCalls.clear();
    mClientVertexArrayMap.fill(-1);
    mClientArraySizes.fill(0);
    mDataCounters.clear();
    mReadBufferSize = 0;
}

std::ostream &operator<<(std::ostream &os, const ParamCapture &capture)
{
    WriteParamTypeToStream(os, capture.type, capture.value);
    return os;
}

void CaptureMemory(const void *source, size_t size, ParamCapture *paramCapture)
{
    std::vector<uint8_t> data(size);
    memcpy(data.data(), source, size);
    paramCapture->data.emplace_back(std::move(data));
}

void CaptureString(const GLchar *str, ParamCapture *paramCapture)
{
    CaptureMemory(str, strlen(str), paramCapture);
}

template <>
void WriteParamValueToStream<ParamType::TGLboolean>(std::ostream &os, GLboolean value)
{
    switch (value)
    {
        case GL_TRUE:
            os << "GL_TRUE";
            break;
        case GL_FALSE:
            os << "GL_FALSE";
            break;
        default:
            os << "GL_INVALID_ENUM";
    }
}

template <>
void WriteParamValueToStream<ParamType::TvoidConstPointer>(std::ostream &os, const void *value)
{
    if (value == 0)
    {
        os << "nullptr";
    }
    else
    {
        os << "reinterpret_cast<const void *>("
           << static_cast<int>(reinterpret_cast<uintptr_t>(value)) << ")";
    }
}

template <>
void WriteParamValueToStream<ParamType::TGLDEBUGPROCKHR>(std::ostream &os, GLDEBUGPROCKHR value)
{}

template <>
void WriteParamValueToStream<ParamType::TGLDEBUGPROC>(std::ostream &os, GLDEBUGPROC value)
{}
#endif  // ANGLE_CAPTURE_ENABLED
}  // namespace angle
