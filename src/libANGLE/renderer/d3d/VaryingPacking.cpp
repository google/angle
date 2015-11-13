//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VaryingPacking:
//   Class which describes a mapping from varyings to registers in D3D
//   for linking between shader stages.
//

#include "libANGLE/renderer/d3d/VaryingPacking.h"

#include "common/utilities.h"
#include "compiler/translator/blocklayoutHLSL.h"
#include "libANGLE/renderer/d3d/DynamicHLSL.h"

namespace rx
{

SemanticInfo::BuiltinInfo::BuiltinInfo() : enabled(false), index(0), systemValue(false)
{
}

std::string SemanticInfo::BuiltinInfo::str() const
{
    return (systemValue ? semantic : (semantic + Str(index)));
}

void SemanticInfo::BuiltinInfo::enableSystem(const std::string &systemValueSemantic)
{
    enabled     = true;
    semantic    = systemValueSemantic;
    systemValue = true;
}

void SemanticInfo::BuiltinInfo::enable(const std::string &semanticVal, unsigned int indexVal)
{
    enabled  = true;
    semantic = semanticVal;
    index    = indexVal;
}

bool PackVarying(PackedVarying *packedVarying, const int maxVaryingVectors, VaryingPacking &packing)
{
    // Make sure we use transposed matrix types to count registers correctly.
    int registers = 0;
    int elements  = 0;

    const sh::Varying &varying = *packedVarying->varying;

    if (varying.isStruct())
    {
        registers = HLSLVariableRegisterCount(varying, true) * varying.elementCount();
        elements  = 4;
    }
    else
    {
        GLenum transposedType = gl::TransposeMatrixType(varying.type);
        registers             = gl::VariableRowCount(transposedType) * varying.elementCount();
        elements              = gl::VariableColumnCount(transposedType);
    }

    if (elements >= 2 && elements <= 4)
    {
        for (int r = 0; r <= maxVaryingVectors - registers; r++)
        {
            bool available = true;

            for (int y = 0; y < registers && available; y++)
            {
                for (int x = 0; x < elements && available; x++)
                {
                    if (packing[r + y][x])
                    {
                        available = false;
                    }
                }
            }

            if (available)
            {
                packedVarying->registerIndex = r;
                packedVarying->columnIndex   = 0;

                for (int y = 0; y < registers; y++)
                {
                    for (int x = 0; x < elements; x++)
                    {
                        packing[r + y][x] = packedVarying;
                    }
                }

                return true;
            }
        }

        if (elements == 2)
        {
            for (int r = maxVaryingVectors - registers; r >= 0; r--)
            {
                bool available = true;

                for (int y = 0; y < registers && available; y++)
                {
                    for (int x = 2; x < 4 && available; x++)
                    {
                        if (packing[r + y][x])
                        {
                            available = false;
                        }
                    }
                }

                if (available)
                {
                    packedVarying->registerIndex = r;
                    packedVarying->columnIndex   = 2;

                    for (int y = 0; y < registers; y++)
                    {
                        for (int x = 2; x < 4; x++)
                        {
                            packing[r + y][x] = packedVarying;
                        }
                    }

                    return true;
                }
            }
        }
    }
    else if (elements == 1)
    {
        int space[4] = {0};

        for (int y = 0; y < maxVaryingVectors; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                space[x] += packing[y][x] ? 0 : 1;
            }
        }

        int column = 0;

        for (int x = 0; x < 4; x++)
        {
            if (space[x] >= registers && (space[column] < registers || space[x] < space[column]))
            {
                column = x;
            }
        }

        if (space[column] >= registers)
        {
            for (int r = 0; r < maxVaryingVectors; r++)
            {
                if (!packing[r][column])
                {
                    packedVarying->registerIndex = r;
                    packedVarying->columnIndex   = column;

                    for (int y = r; y < r + registers; y++)
                    {
                        packing[y][column] = packedVarying;
                    }

                    break;
                }
            }

            return true;
        }
    }
    else
        UNREACHABLE();

    return false;
}

unsigned int PackedVaryingRegister::registerIndex(
    const gl::Caps &caps,
    const std::vector<PackedVarying> &packedVaryings) const
{
    const PackedVarying &packedVarying = packedVaryings[varyingIndex];
    const sh::Varying &varying         = *packedVarying.varying;

    GLenum transposedType = gl::TransposeMatrixType(varying.type);
    unsigned int variableRows =
        static_cast<unsigned int>(varying.isStruct() ? 1 : gl::VariableRowCount(transposedType));

    return (elementIndex * variableRows + (packedVarying.columnIndex * caps.maxVaryingVectors) +
            (packedVarying.registerIndex + rowIndex));
}

PackedVaryingIterator::PackedVaryingIterator(const std::vector<PackedVarying> &packedVaryings)
    : mPackedVaryings(packedVaryings), mEnd(*this)
{
    mEnd.setEnd();
}

PackedVaryingIterator::Iterator PackedVaryingIterator::begin() const
{
    return Iterator(*this);
}

const PackedVaryingIterator::Iterator &PackedVaryingIterator::end() const
{
    return mEnd;
}

PackedVaryingIterator::Iterator::Iterator(const PackedVaryingIterator &parent) : mParent(parent)
{
    while (mRegister.varyingIndex < mParent.mPackedVaryings.size() &&
           !mParent.mPackedVaryings[mRegister.varyingIndex].registerAssigned())
    {
        ++mRegister.varyingIndex;
    }
}

PackedVaryingIterator::Iterator &PackedVaryingIterator::Iterator::operator++()
{
    const sh::Varying *varying = mParent.mPackedVaryings[mRegister.varyingIndex].varying;
    GLenum transposedType = gl::TransposeMatrixType(varying->type);
    unsigned int variableRows =
        static_cast<unsigned int>(varying->isStruct() ? 1 : gl::VariableRowCount(transposedType));

    // Innermost iteration: row count
    if (mRegister.rowIndex + 1 < variableRows)
    {
        ++mRegister.rowIndex;
        return *this;
    }

    mRegister.rowIndex = 0;

    // Middle iteration: element count
    if (mRegister.elementIndex + 1 < varying->elementCount())
    {
        ++mRegister.elementIndex;
        return *this;
    }

    mRegister.elementIndex = 0;

    // Outer iteration: the varying itself. Once we pass the last varying, this Iterator will
    // equal the end Iterator.
    do
    {
        ++mRegister.varyingIndex;
    } while (mRegister.varyingIndex < mParent.mPackedVaryings.size() &&
             !mParent.mPackedVaryings[mRegister.varyingIndex].registerAssigned());
    return *this;
}

bool PackedVaryingIterator::Iterator::operator==(const Iterator &other) const
{
    return mRegister.elementIndex == other.mRegister.elementIndex &&
           mRegister.rowIndex == other.mRegister.rowIndex &&
           mRegister.varyingIndex == other.mRegister.varyingIndex;
}

bool PackedVaryingIterator::Iterator::operator!=(const Iterator &other) const
{
    return !(*this == other);
}

// Packs varyings into generic varying registers, using the algorithm from [OpenGL ES Shading
// Language 1.00 rev. 17] appendix A section 7 page 111
// Returns the number of used varying registers, or -1 if unsuccesful
bool PackVaryings(const gl::Caps &caps,
                  gl::InfoLog &infoLog,
                  std::vector<PackedVarying> *packedVaryings,
                  const std::vector<std::string> &transformFeedbackVaryings,
                  unsigned int *registerCountOut)
{
    VaryingPacking packing = {};
    *registerCountOut      = 0;

    std::set<std::string> uniqueVaryingNames;

    for (PackedVarying &packedVarying : *packedVaryings)
    {
        const sh::Varying &varying = *packedVarying.varying;

        // Do not assign registers to built-in or unreferenced varyings
        if (varying.isBuiltIn() || !varying.staticUse)
        {
            continue;
        }

        ASSERT(uniqueVaryingNames.count(varying.name) == 0);

        if (PackVarying(&packedVarying, caps.maxVaryingVectors, packing))
        {
            uniqueVaryingNames.insert(varying.name);
        }
        else
        {
            infoLog << "Could not pack varying " << varying.name;
            return false;
        }
    }

    for (const std::string &transformFeedbackVaryingName : transformFeedbackVaryings)
    {
        if (transformFeedbackVaryingName.compare(0, 3, "gl_") == 0)
        {
            // do not pack builtin XFB varyings
            continue;
        }

        for (PackedVarying &packedVarying : *packedVaryings)
        {
            const sh::Varying &varying = *packedVarying.varying;

            // Make sure transform feedback varyings aren't optimized out.
            if (uniqueVaryingNames.count(transformFeedbackVaryingName) == 0)
            {
                bool found = false;
                if (transformFeedbackVaryingName == varying.name)
                {
                    if (!PackVarying(&packedVarying, caps.maxVaryingVectors, packing))
                    {
                        infoLog << "Could not pack varying " << varying.name;
                        return false;
                    }

                    found = true;
                    break;
                }
                if (!found)
                {
                    infoLog << "Transform feedback varying " << transformFeedbackVaryingName
                            << " does not exist in the vertex shader.";
                    return false;
                }
            }

            // Add duplicate transform feedback varyings for 'flat' shaded attributes. This is
            // necessary because we write out modified vertex data to correct for the provoking
            // vertex in D3D11. This extra transform feedback varying is the unmodified stream.
            if (varying.interpolation == sh::INTERPOLATION_FLAT)
            {
                sh::Varying duplicateVarying(varying);
                duplicateVarying.name = "StreamOut_" + duplicateVarying.name;
            }
        }
    }

    // Return the number of used registers
    for (GLuint r = 0; r < caps.maxVaryingVectors; r++)
    {
        if (packing[r][0] || packing[r][1] || packing[r][2] || packing[r][3])
        {
            (*registerCountOut)++;
        }
    }

    return true;
}

SemanticInfo GetSemanticInfo(ShaderType shaderType,
                             int majorShaderModel,
                             unsigned int startRegisters,
                             bool position,
                             bool fragCoord,
                             bool pointCoord,
                             bool pointSize)
{
    SemanticInfo info;
    bool hlsl4                         = (majorShaderModel >= 4);
    const std::string &varyingSemantic = GetVaryingSemantic(majorShaderModel, pointSize);

    unsigned int reservedRegisterIndex = startRegisters;

    if (hlsl4)
    {
        info.dxPosition.enableSystem("SV_Position");
    }
    else if (shaderType == SHADER_PIXEL)
    {
        info.dxPosition.enableSystem("VPOS");
    }
    else
    {
        info.dxPosition.enableSystem("POSITION");
    }

    if (position)
    {
        info.glPosition.enable(varyingSemantic, reservedRegisterIndex++);
    }

    if (fragCoord)
    {
        info.glFragCoord.enable(varyingSemantic, reservedRegisterIndex++);
    }

    if (pointCoord)
    {
        // SM3 reserves the TEXCOORD semantic for point sprite texcoords (gl_PointCoord)
        // In D3D11 we manually compute gl_PointCoord in the GS.
        if (hlsl4)
        {
            info.glPointCoord.enable(varyingSemantic, reservedRegisterIndex++);
        }
        else
        {
            info.glPointCoord.enable("TEXCOORD", 0);
        }
    }

    // Special case: do not include PSIZE semantic in HLSL 3 pixel shaders
    if (pointSize && (shaderType != SHADER_PIXEL || hlsl4))
    {
        info.glPointSize.enableSystem("PSIZE");
    }

    return info;
}

}  // namespace rx
