//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VaryingPacking:
//   Class which describes a mapping from varyings to registers in D3D
//   for linking between shader stages.
//

#ifndef LIBANGLE_RENDERER_D3D_VARYINGPACKING_H_
#define LIBANGLE_RENDERER_D3D_VARYINGPACKING_H_

#include "libANGLE/renderer/d3d/RendererD3D.h"

namespace rx
{
class ProgramD3DMetadata;

struct PackedVarying
{
    PackedVarying(const sh::Varying &varyingIn)
        : varying(&varyingIn), registerIndex(GL_INVALID_INDEX), columnIndex(0), vertexOnly(false)
    {
    }

    bool registerAssigned() const { return registerIndex != GL_INVALID_INDEX; }

    void resetRegisterAssignment() { registerIndex = GL_INVALID_INDEX; }

    const sh::Varying *varying;

    // Assigned during link
    unsigned int registerIndex;

    // Assigned during link, Defaults to 0
    unsigned int columnIndex;

    // Transform feedback varyings can be only referenced in the VS.
    bool vertexOnly;
};

struct PackedVaryingRegister final
{
    PackedVaryingRegister() : varyingIndex(0), elementIndex(0), rowIndex(0) {}

    PackedVaryingRegister(const PackedVaryingRegister &) = default;
    PackedVaryingRegister &operator=(const PackedVaryingRegister &) = default;

    unsigned int registerIndex(const gl::Caps &caps,
                               const std::vector<PackedVarying> &packedVaryings) const;

    size_t varyingIndex;
    unsigned int elementIndex;
    unsigned int rowIndex;
};

class PackedVaryingIterator final : public angle::NonCopyable
{
  public:
    PackedVaryingIterator(const std::vector<PackedVarying> &packedVaryings);

    class Iterator final
    {
      public:
        Iterator(const PackedVaryingIterator &parent);

        Iterator(const Iterator &) = default;
        Iterator &operator=(const Iterator &) = delete;

        Iterator &operator++();
        bool operator==(const Iterator &other) const;
        bool operator!=(const Iterator &other) const;

        const PackedVaryingRegister &operator*() const { return mRegister; }
        void setEnd() { mRegister.varyingIndex = mParent.mPackedVaryings.size(); }

      private:
        const PackedVaryingIterator &mParent;
        PackedVaryingRegister mRegister;
    };

    Iterator begin() const;
    const Iterator &end() const;

  private:
    const std::vector<PackedVarying> &mPackedVaryings;
    Iterator mEnd;
};

typedef const PackedVarying *VaryingPacking[gl::IMPLEMENTATION_MAX_VARYING_VECTORS][4];

bool PackVaryings(const gl::Caps &caps,
                  gl::InfoLog &infoLog,
                  std::vector<PackedVarying> *packedVaryings,
                  const std::vector<std::string> &transformFeedbackVaryings,
                  unsigned int *registerCountOut);

struct SemanticInfo final
{
    struct BuiltinInfo final
    {
        BuiltinInfo();

        std::string str() const;
        void enableSystem(const std::string &systemValueSemantic);
        void enable(const std::string &semanticVal, unsigned int indexVal);

        bool enabled;
        std::string semantic;
        unsigned int index;
        bool systemValue;
    };

    BuiltinInfo dxPosition;
    BuiltinInfo glPosition;
    BuiltinInfo glFragCoord;
    BuiltinInfo glPointCoord;
    BuiltinInfo glPointSize;
};

SemanticInfo GetSemanticInfo(ShaderType shaderType,
                             const ProgramD3DMetadata &programMetadata,
                             unsigned int startRegisters);

}  // namespace rx

#endif  // LIBANGLE_RENDERER_D3D_VARYINGPACKING_H_
