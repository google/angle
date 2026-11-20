//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Stream_unittest.cpp : Unit tests of the egl::Stream and egl::StreamSet classes.

#include "gtest/gtest.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "libANGLE/AttributeMap.h"
#include "libANGLE/Display.h"
#include "libANGLE/Stream.h"
#include "libANGLE/renderer/DisplayImpl.h"

namespace angle
{
namespace
{

class FakeThreadSafeDisplayImpl : public rx::ThreadSafeDisplayImpl
{
  public:
    bool testDeviceLost() override { return false; }
    egl::Error restoreLostDevice(const egl::ThreadSafeDisplay *display) override
    {
        return egl::NoError();
    }
    egl::Error makeCurrent(egl::Surface *drawSurface,
                           egl::Surface *readSurface,
                           gl::Context *context) override
    {
        return egl::NoError();
    }
};

class TestThreadSafeDisplay : public egl::ThreadSafeDisplay
{
  public:
    TestThreadSafeDisplay() : egl::ThreadSafeDisplay(EGL_DEFAULT_DISPLAY)
    {
        mThreadSafeImpl = &mImpl;
    }

    ~TestThreadSafeDisplay() override
    {
        mStreamSet.invalidateAll();
        mStreamSet.destroyAllInvalidStreams(this);
    }

    void setLabel(EGLLabelKHR label) override { mLabel = label; }
    EGLLabelKHR getLabel() const override { return mLabel; }

    egl::Stream *createStream(const egl::AttributeMap &attribs)
    {
        egl::Stream *stream = new egl::Stream(this, attribs);
        mStreamSet.insert(stream);
        return stream;
    }

    void invalidateAllStreams() { mStreamSet.invalidateAll(); }
    void destroyAllInvalidStreams() { mStreamSet.destroyAllInvalidStreams(this); }

  private:
    FakeThreadSafeDisplayImpl mImpl;
    EGLLabelKHR mLabel = nullptr;
};

class ThreadBarrier final : angle::NonCopyable
{
  public:
    explicit ThreadBarrier(size_t count) : mTarget(count) {}

    void wait()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        const size_t generation = mGeneration;
        if (++mArrived == mTarget)
        {
            mArrived = 0;
            ++mGeneration;
            mCondVar.notify_all();
        }
        else
        {
            mCondVar.wait(lock, [&]() { return mGeneration != generation; });
        }
    }

  private:
    std::mutex mMutex;
    std::condition_variable mCondVar;
    size_t mTarget;
    size_t mArrived    = 0;
    size_t mGeneration = 0;
};

void EnsureDisplayTLSInitialized()
{
    static thread_local bool sInitialized = false;
    if (!sInitialized)
    {
        ANGLE_SCOPED_DISABLE_LSAN();
        egl::Display::InitTLS();
        sInitialized = true;
    }
}

// Verify StreamSet lookup, removal, refcounting, and invalidation lifecycle.
TEST(StreamTest, StreamSetLifecycleAndInvalidation)
{
    EnsureDisplayTLSInitialized();

    TestThreadSafeDisplay display;
    egl::ScopedThreadSafeDisplayRef displayRef(display);

    egl::Stream *streamA = display.createStream(egl::AttributeMap());
    egl::Stream *streamB = display.createStream(egl::AttributeMap());

    ASSERT_NE(streamA, nullptr);
    ASSERT_NE(streamB, nullptr);
    EXPECT_EQ(1u, streamA->getRefCount());
    EXPECT_EQ(1u, streamB->getRefCount());

    // Borrowing a stream keeps it alive even if another caller removes it from StreamSet.
    {
        egl::ScopedStreamRef borrowedA = display.getStreamRef(streamA);
        ASSERT_EQ(borrowedA.get(), streamA);
        EXPECT_EQ(2u, streamA->getRefCount());

        {
            egl::ScopedStreamDestroy destroyA(&display, display.removeStream(streamA));
            EXPECT_EQ(destroyA.get(), streamA);
            // Once removed, subsequent lookups and removals return nullptr.
            EXPECT_EQ(display.getStreamRef(streamA).get(), nullptr);
            EXPECT_EQ(display.removeStream(streamA), nullptr);
            EXPECT_EQ(2u, streamA->getRefCount());
        }

        // ScopedStreamDestroy released StreamSet's ref, while borrowedA still keeps streamA alive.
        EXPECT_EQ(1u, streamA->getRefCount());
        {
            egl::ScopedStreamLock lock(borrowedA.get());
            EXPECT_EQ(static_cast<EGLenum>(EGL_STREAM_STATE_CREATED_KHR),
                      borrowedA.get()->getState());
        }
    }

    // Moving ScopedStreamRef transfers ownership cleanly without leaking or double-releasing.
    {
        egl::ScopedStreamRef borrowedB = display.getStreamRef(streamB);
        ASSERT_EQ(borrowedB.get(), streamB);
        EXPECT_EQ(2u, streamB->getRefCount());

        egl::ScopedStreamRef movedB(std::move(borrowedB));
        EXPECT_EQ(borrowedB.get(), nullptr);
        EXPECT_EQ(movedB.get(), streamB);
        EXPECT_EQ(2u, streamB->getRefCount());

        // Invalidate all remaining streams while movedB still references streamB.
        display.invalidateAllStreams();
        EXPECT_EQ(display.getStreamRef(streamB).get(), nullptr);
        EXPECT_EQ(display.removeStream(streamB), nullptr);

        // destroyAllInvalidStreams drops StreamSet's ref; movedB keeps streamB alive until exit.
        display.destroyAllInvalidStreams();
        EXPECT_EQ(1u, streamB->getRefCount());
    }
}

// Verify concurrent attribute queries/updates, lock-free labeling, and racing destroys on the same
// streams across multiple threads without holding the display lock.
TEST(StreamTest, ConcurrentAccessLabelAndDoubleDestroy)
{
    constexpr size_t kStreamCount = 64;
    constexpr size_t kThreadCount = 8;

    TestThreadSafeDisplay display;
    std::vector<egl::Stream *> streams;
    streams.reserve(kStreamCount);
    for (size_t i = 0; i < kStreamCount; ++i)
    {
        streams.push_back(display.createStream(egl::AttributeMap()));
    }

    ThreadBarrier barrier(kThreadCount);
    std::vector<std::atomic<size_t>> destroyCounts(kStreamCount);
    for (auto &count : destroyCounts)
    {
        count.store(0, std::memory_order_relaxed);
    }

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (size_t t = 0; t < kThreadCount; ++t)
    {
        threads.emplace_back([&, t]() {
            EnsureDisplayTLSInitialized();
            egl::ScopedThreadSafeDisplayRef displayRef(display);
            barrier.wait();

            for (size_t i = 0; i < kStreamCount; ++i)
            {
                const size_t idx    = (i + t * 7) % kStreamCount;
                egl::Stream *stream = streams[idx];

                // Simulate concurrent query/attrib/label calls.
                for (size_t iter = 0; iter < 16; ++iter)
                {
                    egl::ScopedStreamRef ref = display.getStreamRef(stream);
                    egl::Stream *obj         = ref.get();
                    if (obj == nullptr)
                    {
                        break;
                    }

                    // Label access does not require ScopedStreamLock.
                    EGLLabelKHR label = reinterpret_cast<EGLLabelKHR>((t << 16) | iter);
                    obj->setLabel(label);
                    (void)obj->getLabel();

                    // State and attribute access is serialized by ScopedStreamLock.
                    egl::ScopedStreamLock lock(obj);
                    EXPECT_EQ(static_cast<EGLenum>(EGL_STREAM_STATE_CREATED_KHR), obj->getState());
                    EXPECT_EQ(0u, obj->getProducerFrame());
                    EXPECT_EQ(0u, obj->getConsumerFrame());
                    EXPECT_EQ(egl::Stream::ConsumerType::NoConsumer, obj->getConsumerType());
                    EXPECT_EQ(egl::Stream::ProducerType::NoProducer, obj->getProducerType());
                    EXPECT_EQ(0, obj->getPlaneCount());
                    obj->setConsumerLatency(static_cast<EGLint>(t + iter));
                    (void)obj->getConsumerLatency();
                    obj->setConsumerAcquireTimeout(static_cast<EGLint>(iter));
                    (void)obj->getConsumerAcquireTimeout();
                }

                // All threads race to destroy the stream; exactly one must succeed.
                egl::ScopedStreamDestroy destroy(&display, display.removeStream(stream));
                if (destroy.get() != nullptr)
                {
                    destroyCounts[idx].fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (std::thread &thread : threads)
    {
        thread.join();
    }

    for (size_t i = 0; i < kStreamCount; ++i)
    {
        EXPECT_EQ(1u, destroyCounts[i].load(std::memory_order_relaxed));
        EXPECT_EQ(display.getStreamRef(streams[i]).get(), nullptr);
    }
}

// Verify StreamSet invalidation and destruction of invalid streams while concurrent readers hold
// references and access stream state.
TEST(StreamTest, ConcurrentInvalidateAndLookup)
{
    constexpr size_t kStreamCount = 64;
    constexpr size_t kReaderCount = 4;

    TestThreadSafeDisplay display;
    std::vector<egl::Stream *> streams;
    streams.reserve(kStreamCount);
    for (size_t i = 0; i < kStreamCount; ++i)
    {
        streams.push_back(display.createStream(egl::AttributeMap()));
    }

    ThreadBarrier barrier(kReaderCount + 1);
    std::vector<std::thread> readers;
    readers.reserve(kReaderCount);
    for (size_t r = 0; r < kReaderCount; ++r)
    {
        readers.emplace_back([&, r]() {
            EnsureDisplayTLSInitialized();
            egl::ScopedThreadSafeDisplayRef displayRef(display);
            barrier.wait();

            for (size_t round = 0; round < 32; ++round)
            {
                for (size_t i = 0; i < kStreamCount; ++i)
                {
                    egl::ScopedStreamRef ref = display.getStreamRef(streams[i]);
                    egl::Stream *obj         = ref.get();
                    if (obj == nullptr)
                    {
                        continue;
                    }
                    egl::ScopedStreamLock lock(obj);
                    obj->setConsumerLatency(static_cast<EGLint>(r + round));
                    EXPECT_EQ(static_cast<EGLenum>(EGL_STREAM_STATE_CREATED_KHR), obj->getState());
                }
            }
        });
    }

    std::thread invalidator([&]() {
        EnsureDisplayTLSInitialized();
        egl::ScopedThreadSafeDisplayRef displayRef(display);
        barrier.wait();
        display.invalidateAllStreams();
        display.destroyAllInvalidStreams();
    });

    invalidator.join();
    for (std::thread &reader : readers)
    {
        reader.join();
    }

    for (size_t i = 0; i < kStreamCount; ++i)
    {
        EXPECT_EQ(display.getStreamRef(streams[i]).get(), nullptr);
    }
}

// Verify concurrent creation, lookup, and destruction on StreamSet.
TEST(StreamTest, ConcurrentCreateLookupDestroy)
{
    constexpr size_t kStreamCount = 128;
    constexpr size_t kWorkerCount = 4;

    TestThreadSafeDisplay display;
    std::vector<std::atomic<egl::Stream *>> publishedStreams(kStreamCount);
    for (auto &slot : publishedStreams)
    {
        slot.store(nullptr, std::memory_order_relaxed);
    }
    std::vector<std::atomic<size_t>> destroyCounts(kStreamCount);
    for (auto &count : destroyCounts)
    {
        count.store(0, std::memory_order_relaxed);
    }

    std::atomic<bool> creatorDone(false);
    ThreadBarrier barrier(kWorkerCount + 1);

    std::thread creatorThread([&]() {
        barrier.wait();
        for (size_t i = 0; i < kStreamCount; ++i)
        {
            egl::Stream *stream = display.createStream(egl::AttributeMap());
            publishedStreams[i].store(stream, std::memory_order_release);
        }
        creatorDone.store(true, std::memory_order_release);
    });

    std::vector<std::thread> workers;
    workers.reserve(kWorkerCount);
    for (size_t w = 0; w < kWorkerCount; ++w)
    {
        workers.emplace_back([&, w]() {
            EnsureDisplayTLSInitialized();
            egl::ScopedThreadSafeDisplayRef displayRef(display);
            barrier.wait();

            for (;;)
            {
                bool anyPending = false;
                for (size_t i = 0; i < kStreamCount; ++i)
                {
                    egl::Stream *stream = publishedStreams[i].load(std::memory_order_acquire);
                    if (stream == nullptr)
                    {
                        if (destroyCounts[i].load(std::memory_order_acquire) == 0)
                        {
                            anyPending = true;
                        }
                        continue;
                    }

                    {
                        egl::ScopedStreamRef ref = display.getStreamRef(stream);
                        if (ref.get() != nullptr)
                        {
                            egl::ScopedStreamLock lock(ref.get());
                            ref.get()->setConsumerLatency(static_cast<EGLint>(w + i));
                            (void)ref.get()->getState();
                        }
                    }

                    if ((i + w) % 2 == 0 || creatorDone.load(std::memory_order_acquire))
                    {
                        // Claim the slot atomically so a freed pointer is never passed to
                        // removeStream() again after creatorThread may have reused that address.
                        egl::Stream *claimed =
                            publishedStreams[i].exchange(nullptr, std::memory_order_acq_rel);
                        if (claimed != nullptr)
                        {
                            egl::ScopedStreamDestroy destroy(&display,
                                                             display.removeStream(claimed));
                            EXPECT_EQ(destroy.get(), claimed);
                            destroyCounts[i].fetch_add(1, std::memory_order_release);
                        }
                    }
                    else
                    {
                        anyPending = true;
                    }
                }

                if (!anyPending && creatorDone.load(std::memory_order_acquire))
                {
                    break;
                }
            }
        });
    }

    creatorThread.join();
    for (std::thread &worker : workers)
    {
        worker.join();
    }

    for (size_t i = 0; i < kStreamCount; ++i)
    {
        EXPECT_EQ(1u, destroyCounts[i].load(std::memory_order_relaxed));
    }
}

}  // namespace
}  // namespace angle
