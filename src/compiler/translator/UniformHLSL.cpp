//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// UniformHLSL.cpp:
//   Methods for GLSL to HLSL translation for uniforms and interface blocks.
//

#include "OutputHLSL.h"
#include "common/blocklayout.h"
#include "common/utilities.h"
#include "compiler/translator/UniformHLSL.h"
#include "compiler/translator/StructureHLSL.h"
#include "compiler/translator/util.h"
#include "compiler/translator/UtilsHLSL.h"

namespace sh
{

static const char *UniformRegisterPrefix(const TType &type)
{
    if (IsSampler(type.getBasicType()))
    {
        return "s";
    }
    else
    {
        return "c";
    }
}

static TString InterfaceBlockFieldName(const TInterfaceBlock &interfaceBlock, const TField &field)
{
    if (interfaceBlock.hasInstanceName())
    {
        return interfaceBlock.name() + "." + field.name();
    }
    else
    {
        return field.name();
    }
}

static TString InterfaceBlockFieldTypeString(const TField &field, TLayoutBlockStorage blockStorage)
{
    const TType &fieldType = *field.type();
    const TLayoutMatrixPacking matrixPacking = fieldType.getLayoutQualifier().matrixPacking;
    ASSERT(matrixPacking != EmpUnspecified);
    TStructure *structure = fieldType.getStruct();

    if (fieldType.isMatrix())
    {
        // Use HLSL row-major packing for GLSL column-major matrices
        const TString &matrixPackString = (matrixPacking == EmpRowMajor ? "column_major" : "row_major");
        return matrixPackString + " " + TypeString(fieldType);
    }
    else if (structure)
    {
        // Use HLSL row-major packing for GLSL column-major matrices
        return QualifiedStructNameString(*structure, matrixPacking == EmpColumnMajor,
            blockStorage == EbsStd140);
    }
    else
    {
        return TypeString(fieldType);
    }
}

static TString InterfaceBlockStructName(const TInterfaceBlock &interfaceBlock)
{
    return DecoratePrivate(interfaceBlock.name()) + "_type";
}

UniformHLSL::UniformHLSL(StructureHLSL *structureHLSL, ShShaderOutput outputType)
    : mUniformRegister(0),
      mInterfaceBlockRegister(0),
      mSamplerRegister(0),
      mStructureHLSL(structureHLSL),
      mOutputType(outputType)
{}

void UniformHLSL::reserveUniformRegisters(unsigned int registerCount)
{
    mUniformRegister = registerCount;
}

void UniformHLSL::reserveInterfaceBlockRegisters(unsigned int registerCount)
{
    mInterfaceBlockRegister = registerCount;
}

unsigned int UniformHLSL::declareUniformAndAssignRegister(const TType &type, const TString &name)
{
    unsigned int registerIndex = (IsSampler(type.getBasicType()) ? mSamplerRegister : mUniformRegister);

    GetVariableTraverser<Uniform> traverser(&mActiveUniforms);
    traverser.traverse(type, name);

    const sh::Uniform &activeUniform = mActiveUniforms.back();
    mUniformRegisterMap[activeUniform.name] = registerIndex;

    unsigned int registerCount = HLSLVariableRegisterCount(activeUniform, mOutputType);
    if (IsSampler(type.getBasicType()))
    {
        mSamplerRegister += registerCount;
    }
    else
    {
        mUniformRegister += registerCount;
    }

    return registerIndex;
}

TString UniformHLSL::uniformsHeader(ShShaderOutput outputType, const ReferencedSymbols &referencedUniforms)
{
    TString uniforms;

    for (ReferencedSymbols::const_iterator uniformIt = referencedUniforms.begin();
         uniformIt != referencedUniforms.end(); uniformIt++)
    {
        const TIntermSymbol &uniform = *uniformIt->second;
        const TType &type = uniform.getType();
        const TString &name = uniform.getSymbol();

        unsigned int registerIndex = declareUniformAndAssignRegister(type, name);

        if (outputType == SH_HLSL11_OUTPUT && IsSampler(type.getBasicType()))   // Also declare the texture
        {
            uniforms += "uniform " + SamplerString(type) + " sampler_" + DecorateUniform(name, type) + ArrayString(type) +
                        " : register(s" + str(registerIndex) + ");\n";

            uniforms += "uniform " + TextureString(type) + " texture_" + DecorateUniform(name, type) + ArrayString(type) +
                        " : register(t" + str(registerIndex) + ");\n";
        }
        else
        {
            const TStructure *structure = type.getStruct();
            const TString &typeName = (structure ? QualifiedStructNameString(*structure, false, false) : TypeString(type));

            const TString &registerString = TString("register(") + UniformRegisterPrefix(type) + str(registerIndex) + ")";

            uniforms += "uniform " + typeName + " " + DecorateUniform(name, type) + ArrayString(type) + " : " + registerString + ";\n";
        }
    }

    return (uniforms.empty() ? "" : ("// Uniforms\n\n" + uniforms));
}

TString UniformHLSL::interfaceBlocksHeader(const ReferencedSymbols &referencedInterfaceBlocks)
{
    TString interfaceBlocks;

    for (ReferencedSymbols::const_iterator interfaceBlockIt = referencedInterfaceBlocks.begin();
         interfaceBlockIt != referencedInterfaceBlocks.end(); interfaceBlockIt++)
    {
        const TType &nodeType = interfaceBlockIt->second->getType();
        const TInterfaceBlock &interfaceBlock = *nodeType.getInterfaceBlock();
        const TFieldList &fieldList = interfaceBlock.fields();

        unsigned int arraySize = static_cast<unsigned int>(interfaceBlock.arraySize());
        unsigned int activeRegister = mInterfaceBlockRegister;

        InterfaceBlock activeBlock(interfaceBlock.name().c_str(), arraySize);
        for (unsigned int typeIndex = 0; typeIndex < fieldList.size(); typeIndex++)
        {
            const TField &field = *fieldList[typeIndex];
            const TString &fullFieldName = InterfaceBlockFieldName(interfaceBlock, field);

            bool isRowMajor = (field.type()->getLayoutQualifier().matrixPacking == EmpRowMajor);
            GetInterfaceBlockFieldTraverser traverser(&activeBlock.fields, isRowMajor);
            traverser.traverse(*field.type(), fullFieldName);
        }

        mInterfaceBlockRegisterMap[activeBlock.name] = activeRegister;
        mInterfaceBlockRegister += std::max(1u, arraySize);

        activeBlock.layout = GetBlockLayoutType(interfaceBlock.blockStorage());

        if (interfaceBlock.matrixPacking() == EmpRowMajor)
        {
            activeBlock.isRowMajorLayout = true;
        }

        mActiveInterfaceBlocks.push_back(activeBlock);

        if (interfaceBlock.hasInstanceName())
        {
            interfaceBlocks += interfaceBlockStructString(interfaceBlock);
        }

        if (arraySize > 0)
        {
            for (unsigned int arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
            {
                interfaceBlocks += interfaceBlockString(interfaceBlock, activeRegister + arrayIndex, arrayIndex);
            }
        }
        else
        {
            interfaceBlocks += interfaceBlockString(interfaceBlock, activeRegister, GL_INVALID_INDEX);
        }
    }

    return (interfaceBlocks.empty() ? "" : ("// Interface Blocks\n\n" + interfaceBlocks));
}

TString UniformHLSL::interfaceBlockString(const TInterfaceBlock &interfaceBlock, unsigned int registerIndex, unsigned int arrayIndex)
{
    const TString &arrayIndexString =  (arrayIndex != GL_INVALID_INDEX ? Decorate(str(arrayIndex)) : "");
    const TString &blockName = interfaceBlock.name() + arrayIndexString;
    TString hlsl;

    hlsl += "cbuffer " + blockName + " : register(b" + str(registerIndex) + ")\n"
            "{\n";

    if (interfaceBlock.hasInstanceName())
    {
        hlsl += "    " + InterfaceBlockStructName(interfaceBlock) + " " +
                interfaceBlockInstanceString(interfaceBlock, arrayIndex) + ";\n";
    }
    else
    {
        const TLayoutBlockStorage blockStorage = interfaceBlock.blockStorage();
        hlsl += interfaceBlockMembersString(interfaceBlock, blockStorage);
    }

    hlsl += "};\n\n";

    return hlsl;
}

TString UniformHLSL::interfaceBlockInstanceString(const TInterfaceBlock& interfaceBlock, unsigned int arrayIndex)
{
    if (!interfaceBlock.hasInstanceName())
    {
        return "";
    }
    else if (interfaceBlock.isArray())
    {
        return DecoratePrivate(interfaceBlock.instanceName()) + "_" + str(arrayIndex);
    }
    else
    {
        return Decorate(interfaceBlock.instanceName());
    }
}

TString UniformHLSL::interfaceBlockMembersString(const TInterfaceBlock &interfaceBlock, TLayoutBlockStorage blockStorage)
{
    TString hlsl;

    Std140PaddingHelper padHelper = mStructureHLSL->getPaddingHelper();

    for (unsigned int typeIndex = 0; typeIndex < interfaceBlock.fields().size(); typeIndex++)
    {
        const TField &field = *interfaceBlock.fields()[typeIndex];
        const TType &fieldType = *field.type();

        if (blockStorage == EbsStd140)
        {
            // 2 and 3 component vector types in some cases need pre-padding
            hlsl += padHelper.prePadding(fieldType);
        }

        hlsl += "    " + InterfaceBlockFieldTypeString(field, blockStorage) +
                " " + Decorate(field.name()) + ArrayString(fieldType) + ";\n";

        // must pad out after matrices and arrays, where HLSL usually allows itself room to pack stuff
        if (blockStorage == EbsStd140)
        {
            const bool useHLSLRowMajorPacking = (fieldType.getLayoutQualifier().matrixPacking == EmpColumnMajor);
            hlsl += padHelper.postPaddingString(fieldType, useHLSLRowMajorPacking);
        }
    }

    return hlsl;
}

TString UniformHLSL::interfaceBlockStructString(const TInterfaceBlock &interfaceBlock)
{
    const TLayoutBlockStorage blockStorage = interfaceBlock.blockStorage();

    return "struct " + InterfaceBlockStructName(interfaceBlock) + "\n"
           "{\n" +
           interfaceBlockMembersString(interfaceBlock, blockStorage) +
           "};\n\n";
}

}
