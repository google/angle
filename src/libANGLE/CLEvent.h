//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLEvent.h: Defines the cl::Event class, which can be used to track the execution status of an
// OpenCL command.

#ifndef LIBANGLE_CLEVENT_H_
#define LIBANGLE_CLEVENT_H_

#include "libANGLE/CLObject.h"
#include "libANGLE/renderer/CLEventImpl.h"

#include <array>

namespace cl
{

class Event final : public _cl_event, public Object
{
  public:
    using PtrList = std::list<EventPtr>;

    ~Event() override;

    Context &getContext();
    const Context &getContext() const;
    const CommandQueueRefPtr &getCommandQueue() const;
    cl_command_type getCommandType() const;
    bool wasStatusChanged() const;

    template <typename T>
    T &getImpl() const;

    void retain() noexcept;
    bool release();

    void callback(cl_int commandStatus);

    cl_int setUserEventStatus(cl_int executionStatus);

    cl_int getInfo(EventInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const;

    cl_int setCallback(cl_int commandExecCallbackType, EventCB pfnNotify, void *userData);

    static bool IsValid(const _cl_event *event);
    static bool IsValidAndVersionOrNewer(const _cl_event *event, cl_uint major, cl_uint minor);

  private:
    using CallbackData = std::pair<EventCB, void *>;

    Event(Context &context, cl_int &errorCode);

    const ContextRefPtr mContext;
    const CommandQueueRefPtr mCommandQueue;
    const rx::CLEventImpl::Ptr mImpl;
    const cl_command_type mCommandType;

    bool mStatusWasChanged = false;

    // Create separate storage for each possible callback type.
    static_assert(CL_COMPLETE == 0 && CL_RUNNING == 1 && CL_SUBMITTED == 2,
                  "OpenCL command execution status values are not as assumed");
    std::array<std::vector<CallbackData>, 3u> mCallbacks;

    friend class Context;
};

inline Context &Event::getContext()
{
    return *mContext;
}

inline const Context &Event::getContext() const
{
    return *mContext;
}

inline const CommandQueueRefPtr &Event::getCommandQueue() const
{
    return mCommandQueue;
}

inline cl_command_type Event::getCommandType() const
{
    return mCommandType;
}

inline bool Event::wasStatusChanged() const
{
    return mStatusWasChanged;
}

template <typename T>
inline T &Event::getImpl() const
{
    return static_cast<T &>(*mImpl);
}

inline void Event::retain() noexcept
{
    addRef();
}

}  // namespace cl

#endif  // LIBANGLE_CLEVENT_H_
