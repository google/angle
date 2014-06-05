//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderbufferProxySet.h: Defines the gl::RenderbufferProxySet, a class for
// maintaining a Texture's weak references to the Renderbuffers that represent it.

#ifndef LIBGLESV2_RENDERBUFFERPROXYSET_H_
#define LIBGLESV2_RENDERBUFFERPROXYSET_H_

#include <map>

namespace gl
{
class FramebufferAttachment;

class RenderbufferProxySet
{
  public:
    void addRef(const FramebufferAttachment *proxy);
    void release(const FramebufferAttachment *proxy);

    void add(unsigned int mipLevel, unsigned int layer, FramebufferAttachment *renderBuffer);
    FramebufferAttachment *get(unsigned int mipLevel, unsigned int layer) const;

  private:
    struct RenderbufferKey
    {
        unsigned int mipLevel;
        unsigned int layer;

        bool operator<(const RenderbufferKey &other) const;
    };

    typedef std::map<RenderbufferKey, FramebufferAttachment*> BufferMap;
    BufferMap mBufferMap;

    typedef std::map<const FramebufferAttachment*, unsigned int> RefCountMap;
    RefCountMap mRefCountMap;
};

}

#endif // LIBGLESV2_RENDERBUFFERPROXYSET_H_
