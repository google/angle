//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ContextState: Container class for all GL context state, caps and objects.

#ifndef LIBANGLE_CONTEXTSTATE_H_
#define LIBANGLE_CONTEXTSTATE_H_

#include "common/angleutils.h"
#include "libANGLE/State.h"

namespace gl
{

struct ContextState final : public angle::NonCopyable
{
  public:
    ContextState(uintptr_t context,
                 GLint clientVersion,
                 const State &state,
                 const Caps &caps,
                 const TextureCapsMap &textureCaps,
                 const Extensions &extensions,
                 const ResourceManager *resourceManager,
                 const Limitations &limitations);
    ~ContextState();

    uintptr_t context;
    GLint clientVersion;
    const State *state;
    const Caps *caps;
    const TextureCapsMap *textureCaps;
    const Extensions *extensions;
    const ResourceManager *resourceManager;
    const Limitations *limitations;
};

class ValidationContext : angle::NonCopyable
{
  public:
    ValidationContext(GLint clientVersion,
                      const State &state,
                      const Caps &caps,
                      const TextureCapsMap &textureCaps,
                      const Extensions &extensions,
                      const ResourceManager *resourceManager,
                      const Limitations &limitations,
                      bool skipValidation);
    virtual ~ValidationContext() {}

    virtual void handleError(const Error &error) = 0;

    const ContextState &getData() const { return mData; }
    int getClientVersion() const { return mData.clientVersion; }
    const State &getState() const { return *mData.state; }
    const Caps &getCaps() const { return *mData.caps; }
    const TextureCapsMap &getTextureCaps() const { return *mData.textureCaps; }
    const Extensions &getExtensions() const { return *mData.extensions; }
    const Limitations &getLimitations() const { return *mData.limitations; }
    bool skipValidation() const { return mSkipValidation; }

  protected:
    ContextState mData;
    bool mSkipValidation;
};
}

#endif  // LIBANGLE_CONTEXTSTATE_H_
