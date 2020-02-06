//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ProgramExecutable.h: Collects the information and interfaces common to both Programs and
// ProgramPipelines in order to execute/draw with either.

#ifndef LIBANGLE_PROGRAMEXECUTABLE_H_
#define LIBANGLE_PROGRAMEXECUTABLE_H_

#include "libANGLE/Caps.h"
#include "libANGLE/InfoLog.h"
#include "libANGLE/VaryingPacking.h"
#include "libANGLE/angletypes.h"

namespace gl
{

class ProgramExecutable
{
  public:
    ProgramExecutable();
    virtual ~ProgramExecutable();

    void reset();

    int getInfoLogLength() const;
    InfoLog &getInfoLog() { return mInfoLog; }
    void getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog) const;
    std::string getInfoLogString() const;
    void resetInfoLog() { mInfoLog.reset(); }

    const ShaderBitSet &getLinkedShaderStages() const { return mLinkedShaderStages; }
    ShaderBitSet &getLinkedShaderStages() { return mLinkedShaderStages; }
    bool hasLinkedShaderStage(ShaderType shaderType) const
    {
        ASSERT(shaderType != ShaderType::InvalidEnum);
        return mLinkedShaderStages[shaderType];
    }
    size_t getLinkedShaderStageCount() const { return mLinkedShaderStages.count(); }
    bool isCompute() const { return hasLinkedShaderStage(ShaderType::Compute); }

    const AttributesMask &getActiveAttribLocationsMask() const
    {
        return mActiveAttribLocationsMask;
    }
    bool isAttribLocationActive(size_t attribLocation) const;
    const AttributesMask &getNonBuiltinAttribLocationsMask() const { return mAttributesMask; }
    unsigned int getMaxActiveAttribLocation() const { return mMaxActiveAttribLocation; }
    const ComponentTypeMask &getAttributesTypeMask() const { return mAttributesTypeMask; }
    AttributesMask getAttributesMask() const;

  private:
    // TODO(timvp): http://anglebug.com/3570: Investigate removing these friend
    // class declarations and accessing the necessary members with getters/setters.
    friend class Program;
    friend class ProgramPipeline;
    friend class ProgramState;

    InfoLog mInfoLog;

    ShaderBitSet mLinkedShaderStages;

    angle::BitSet<MAX_VERTEX_ATTRIBS> mActiveAttribLocationsMask;
    unsigned int mMaxActiveAttribLocation;
    ComponentTypeMask mAttributesTypeMask;
    // mAttributesMask is identical to mActiveAttribLocationsMask with built-in attributes removed.
    AttributesMask mAttributesMask;
};

}  // namespace gl

#endif  // LIBANGLE_PROGRAMEXECUTABLE_H_
