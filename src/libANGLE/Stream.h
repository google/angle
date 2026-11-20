//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Stream.h: Defines the egl::Stream class, representing the stream
// where frames are streamed in. Implements EGLStreanKHR.

#ifndef LIBANGLE_STREAM_H_
#define LIBANGLE_STREAM_H_

#include <array>
#include <atomic>
#include <mutex>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "common/SimpleMutex.h"
#include "common/angleutils.h"
#include "common/hash_containers.h"
#include "libANGLE/AttributeMap.h"
#include "libANGLE/Debug.h"
#include "libANGLE/RefCountObject.h"

namespace rx
{
class StreamProducerImpl;
}

namespace gl
{
class Context;
class Texture;
}  // namespace gl

namespace egl
{
class ContextMutex;
class Error;
class ScopedStreamLock;
class Thread;
class ThreadSafeDisplay;

class Stream final : public ThreadSafeRefCountObject, public LabeledObject
{
  public:
    Stream(ThreadSafeDisplay *display, const AttributeMap &attribs);
    ~Stream() override;

    void onDestroy(const ThreadSafeDisplay *display) override;

    void setLabel(EGLLabelKHR label) override;
    EGLLabelKHR getLabel() const override;

    enum class ConsumerType
    {
        NoConsumer,
        GLTextureRGB,
        GLTextureYUV,
    };

    enum class ProducerType
    {
        NoProducer,
        D3D11Texture,
    };

    // A GL texture interpretation of a part of a producer frame. For use with GL texture consumers
    struct GLTextureDescription
    {
        unsigned int width;
        unsigned int height;
        unsigned int internalFormat;
        unsigned int mipLevels;
    };

    EGLenum getState() const;

    void setConsumerLatency(EGLint latency);
    EGLint getConsumerLatency() const;

    EGLuint64KHR getProducerFrame() const;
    EGLuint64KHR getConsumerFrame() const;

    void setConsumerAcquireTimeout(EGLint timeout);
    EGLint getConsumerAcquireTimeout() const;

    ConsumerType getConsumerType() const;
    ProducerType getProducerType() const;

    EGLint getPlaneCount() const;

    rx::StreamProducerImpl *getImplementation();

    // Consumer creation methods
    Error createConsumerGLTextureExternal(const AttributeMap &attributes, gl::Context *context);

    // Producer creation methods
    Error createProducerD3D11Texture(const AttributeMap &attributes);

    // Consumer methods
    Error consumerAcquire(const gl::Context *context);
    Error consumerRelease(const gl::Context *context);

    // Some consumers are bound to GL contexts. This validates that a given context is bound to the
    // stream's consumer
    bool isConsumerBoundToContext(const gl::Context *context) const;

    // Producer methods
    Error validateD3D11Texture(const void *texture, const AttributeMap &attributes) const;
    Error postD3D11Texture(void *texture, const AttributeMap &attributes);

    ContextMutex *getContextMutex() const { return mContextMutex; }

  private:
    std::atomic<EGLLabelKHR> mLabel;

    // Associated display
    ThreadSafeDisplay *mDisplay;

    // Producer Implementation
    rx::StreamProducerImpl *mProducerImplementation;

    // Associated GL context. Note that this is a weak pointer used for validation purposes only,
    // and should never be arbitrarily dereferenced without knowing the context still exists as it
    // can become dangling at any time.
    gl::Context *mContext;

    // Reference-counted ContextMutex of mContext's share group, set when a GL texture consumer is
    // attached. Locked during Stream destruction to synchronize detaching mPlanes with concurrent
    // Texture::onDestroy() calls.
    ContextMutex *mContextMutex;

    // Serializes access to Stream state and mProducerImplementation across threads.
    // Lock order: ContextMutex -> Stream::mMutex.
    mutable angle::SimpleMutex mMutex;

    // EGL defined attributes
    EGLint mState;
    EGLuint64KHR mProducerFrame;
    EGLuint64KHR mConsumerFrame;
    EGLint mConsumerLatency;

    // EGL gltexture consumer attributes
    EGLint mConsumerAcquireTimeout;

    // EGL gltexture yuv consumer attributes
    EGLint mPlaneCount;
    struct PlaneTexture
    {
        EGLint textureUnit;
        gl::Texture *texture;
    };
    // Texture units and textures for all the planes
    std::array<PlaneTexture, 3> mPlanes;

    // Consumer and producer types
    ConsumerType mConsumerType;
    ProducerType mProducerType;

    // ANGLE-only method, used internally
    friend class gl::Texture;
    friend class ScopedStreamLock;
    void releaseTextures();
};

class [[nodiscard]] ScopedStreamRef final
{
  public:
    ScopedStreamRef() = default;
    ScopedStreamRef(const ThreadSafeDisplay *display, Stream *stream)
        : mDisplay(display), mStream(stream)
    {
        if (mStream)
        {
            ASSERT(mDisplay != nullptr);
            mStream->addRef();
        }
    }
    ~ScopedStreamRef();
    ScopedStreamRef(const ScopedStreamRef &other)            = delete;
    ScopedStreamRef &operator=(const ScopedStreamRef &other) = delete;
    // Needed by MSVC to initialize ScopedLabeledObjectRef::mStreamRef from a ?: expression.
    ScopedStreamRef(ScopedStreamRef &&other) noexcept
        : mDisplay(other.mDisplay), mStream(other.mStream)
    {
        other.mDisplay = nullptr;
        other.mStream  = nullptr;
    }

    Stream *get() const { return mStream; }

  private:
    const ThreadSafeDisplay *mDisplay = nullptr;
    Stream *mStream                   = nullptr;
};

class [[nodiscard]] ScopedStreamLock final : angle::NonCopyable
{
  public:
    explicit ScopedStreamLock(Stream *stream)
    {
        if (stream != nullptr)
        {
            mLock = std::unique_lock<angle::SimpleMutex>(stream->mMutex);
        }
    }

  private:
    std::unique_lock<angle::SimpleMutex> mLock;
};

// RAII guard used by eglDestroyStreamKHR() via RemoveStreamIfValid().
// Unlike ScopedStreamRef (which borrows a stream by incrementing its refcount), this class takes
// ownership of the reference previously held by StreamSet upon removal from the set, without
// adding a new reference. It defers calling releaseStream() until entry-point scope exit so the
// stream remains alive across validation, stub execution, and ANGLE_CAPTURE_EGL.
class [[nodiscard]] ScopedStreamDestroy final : angle::NonCopyable
{
  public:
    ScopedStreamDestroy() = default;
    ScopedStreamDestroy(ThreadSafeDisplay *display, Stream *stream)
        : mDisplay(display), mStream(stream)
    {}
    ~ScopedStreamDestroy();

    Stream *get() const { return mStream; }

  private:
    ThreadSafeDisplay *mDisplay = nullptr;
    Stream *mStream             = nullptr;
};

class StreamSet final : angle::NonCopyable
{
  public:
    StreamSet()  = default;
    ~StreamSet() = default;

    void insert(Stream *stream)
    {
        stream->addRef();

        std::lock_guard<angle::SimpleMutex> lock(mMutex);
        const bool inserted = mStreams.insert(stream).second;
        ASSERT(inserted);
    }

    Stream *removeStream(Stream *stream)
    {
        std::lock_guard<angle::SimpleMutex> lock(mMutex);
        auto iter = mStreams.find(stream);
        if (iter == mStreams.end())
        {
            return nullptr;
        }

        Stream *found = *iter;
        mStreams.erase(iter);
        return found;
    }

    void invalidateAll()
    {
        std::lock_guard<angle::SimpleMutex> lock(mMutex);
        mInvalidStreams.reserve(mInvalidStreams.size() + mStreams.size());
        for (Stream *stream : mStreams)
        {
            mInvalidStreams.push_back(stream);
        }
        mStreams.clear();
    }

    void destroyAllInvalidStreams(ThreadSafeDisplay *display);

    ScopedStreamRef getStreamRef(const ThreadSafeDisplay *display, Stream *stream) const
    {
        std::lock_guard<angle::SimpleMutex> lock(mMutex);
        auto iter = mStreams.find(stream);
        return ScopedStreamRef(display, iter != mStreams.end() ? *iter : nullptr);
    }

  private:
    mutable angle::SimpleMutex mMutex;
    angle::HashSet<Stream *> mStreams;
    std::vector<Stream *> mInvalidStreams;
};
}  // namespace egl

#endif  // LIBANGLE_STREAM_H_
