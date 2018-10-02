//
// Copyright 2016-2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// QueryVk.cpp:
//    Implements the class methods for QueryVk.
//

#include "libANGLE/renderer/vulkan/QueryVk.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"

#include "common/debug.h"

namespace rx
{

QueryVk::QueryVk(gl::QueryType type) : QueryImpl(type), mCachedResult(0), mCachedResultValid(false)
{
}

QueryVk::~QueryVk() = default;

gl::Error QueryVk::onDestroy(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);
    contextVk->getQueryPool(getType())->freeQuery(contextVk, &mQueryHelper);

    return gl::NoError();
}

gl::Error QueryVk::begin(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);

    ANGLE_TRY(contextVk->getQueryPool(getType())->allocateQuery(contextVk, &mQueryHelper));

    mCachedResultValid = false;

    mQueryHelper.beginQuery(contextVk, mQueryHelper.getQueryPool(), mQueryHelper.getQuery());

    return gl::NoError();
}

gl::Error QueryVk::end(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);

    mQueryHelper.endQuery(contextVk, mQueryHelper.getQueryPool(), mQueryHelper.getQuery());

    return gl::NoError();
}

gl::Error QueryVk::queryCounter(const gl::Context *context)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

angle::Result QueryVk::getResult(const gl::Context *context, bool wait)
{
    if (mCachedResultValid)
    {
        return angle::Result::Continue();
    }

    ContextVk *contextVk = vk::GetImpl(context);

    VkQueryResultFlags flags = (wait ? VK_QUERY_RESULT_WAIT_BIT : 0) | VK_QUERY_RESULT_64_BIT;

    angle::Result result = mQueryHelper.getQueryPool()->getResults(
        contextVk, mQueryHelper.getQuery(), 1, sizeof(mCachedResult), &mCachedResult,
        sizeof(mCachedResult), flags);
    ANGLE_TRY(result);

    if (result == angle::Result::Continue())
    {
        mCachedResultValid = true;

        switch (getType())
        {
            case gl::QueryType::AnySamples:
            case gl::QueryType::AnySamplesConservative:
                // OpenGL query result in these cases is binary
                mCachedResult = !!mCachedResult;
                break;
            default:
                UNREACHABLE();
                break;
        }
    }

    return angle::Result::Continue();
}

gl::Error QueryVk::getResult(const gl::Context *context, GLint *params)
{
    ANGLE_TRY(getResult(context, true));
    *params = static_cast<GLint>(mCachedResult);
    return gl::NoError();
}

gl::Error QueryVk::getResult(const gl::Context *context, GLuint *params)
{
    ANGLE_TRY(getResult(context, true));
    *params = static_cast<GLuint>(mCachedResult);
    return gl::NoError();
}

gl::Error QueryVk::getResult(const gl::Context *context, GLint64 *params)
{
    ANGLE_TRY(getResult(context, true));
    *params = static_cast<GLint64>(mCachedResult);
    return gl::NoError();
}

gl::Error QueryVk::getResult(const gl::Context *context, GLuint64 *params)
{
    ANGLE_TRY(getResult(context, true));
    *params = mCachedResult;
    return gl::NoError();
}

gl::Error QueryVk::isResultAvailable(const gl::Context *context, bool *available)
{
    ContextVk *contextVk = vk::GetImpl(context);

    // Make sure the command buffer for this query is submitted.  If not, *available should always
    // be false. This is because the reset command is not yet executed (it's only put in the command
    // graph), so actually checking the results may return "true" because of a previous submission.

    if (mQueryHelper.hasPendingWork(contextVk->getRenderer()))
    {
        *available = false;
        return gl::NoError();
    }

    ANGLE_TRY(getResult(context, false));
    *available = mCachedResultValid;

    return gl::NoError();
}

}  // namespace rx
