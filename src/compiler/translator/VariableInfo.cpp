//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/VariableInfo.h"

#include "angle_gl.h"
#include "common/utilities.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/util.h"

namespace sh
{

namespace
{

BlockLayoutType GetBlockLayoutType(TLayoutBlockStorage blockStorage)
{
    switch (blockStorage)
    {
        case EbsPacked:
            return BLOCKLAYOUT_PACKED;
        case EbsShared:
            return BLOCKLAYOUT_SHARED;
        case EbsStd140:
            return BLOCKLAYOUT_STANDARD;
        default:
            UNREACHABLE();
            return BLOCKLAYOUT_SHARED;
    }
}

void ExpandUserDefinedVariable(const ShaderVariable &variable,
                               const std::string &name,
                               const std::string &mappedName,
                               bool markStaticUse,
                               std::vector<ShaderVariable> *expanded)
{
    ASSERT(variable.isStruct());

    const std::vector<ShaderVariable> &fields = variable.fields;

    for (size_t fieldIndex = 0; fieldIndex < fields.size(); fieldIndex++)
    {
        const ShaderVariable &field = fields[fieldIndex];
        ExpandVariable(field, name + "." + field.name, mappedName + "." + field.mappedName,
                       markStaticUse, expanded);
    }
}

template <class VarT>
VarT *FindVariable(const TString &name, std::vector<VarT> *infoList)
{
    // TODO(zmo): optimize this function.
    for (size_t ii = 0; ii < infoList->size(); ++ii)
    {
        if ((*infoList)[ii].name.c_str() == name)
            return &((*infoList)[ii]);
    }

    return nullptr;
}

// Traverses the intermediate tree to collect all attributes, uniforms, varyings, fragment outputs,
// and interface blocks.
class CollectVariablesTraverser : public TIntermTraverser
{
  public:
    CollectVariablesTraverser(std::vector<Attribute> *attribs,
                              std::vector<OutputVariable> *outputVariables,
                              std::vector<Uniform> *uniforms,
                              std::vector<Varying> *varyings,
                              std::vector<InterfaceBlock> *interfaceBlocks,
                              ShHashFunction64 hashFunction,
                              const TSymbolTable &symbolTable,
                              int shaderVersion,
                              const TExtensionBehavior &extensionBehavior);

    void visitSymbol(TIntermSymbol *symbol) override;
    bool visitDeclaration(Visit, TIntermDeclaration *node) override;
    bool visitBinary(Visit visit, TIntermBinary *binaryNode) override;

  private:
    void setCommonVariableProperties(const TType &type,
                                     const TString &name,
                                     ShaderVariable *variableOut) const;

    Attribute recordAttribute(const TIntermSymbol &variable) const;
    OutputVariable recordOutputVariable(const TIntermSymbol &variable) const;
    Varying recordVarying(const TIntermSymbol &variable) const;
    InterfaceBlock recordInterfaceBlock(const TIntermSymbol &variable) const;
    Uniform recordUniform(const TIntermSymbol &variable) const;

    void setBuiltInInfoFromSymbolTable(const char *name, ShaderVariable *info);

    void recordBuiltInVaryingUsed(const char *name, bool *addedFlag);
    void recordBuiltInFragmentOutputUsed(const char *name, bool *addedFlag);
    void recordBuiltInAttributeUsed(const char *name, bool *addedFlag);

    std::vector<Attribute> *mAttribs;
    std::vector<OutputVariable> *mOutputVariables;
    std::vector<Uniform> *mUniforms;
    std::vector<Varying> *mVaryings;
    std::vector<InterfaceBlock> *mInterfaceBlocks;

    std::map<std::string, InterfaceBlockField *> mInterfaceBlockFields;

    bool mDepthRangeAdded;
    bool mPointCoordAdded;
    bool mFrontFacingAdded;
    bool mFragCoordAdded;

    bool mInstanceIDAdded;
    bool mVertexIDAdded;
    bool mPositionAdded;
    bool mPointSizeAdded;
    bool mLastFragDataAdded;
    bool mFragColorAdded;
    bool mFragDataAdded;
    bool mFragDepthEXTAdded;
    bool mFragDepthAdded;
    bool mSecondaryFragColorEXTAdded;
    bool mSecondaryFragDataEXTAdded;

    ShHashFunction64 mHashFunction;

    const TSymbolTable &mSymbolTable;
    int mShaderVersion;
    const TExtensionBehavior &mExtensionBehavior;
};

CollectVariablesTraverser::CollectVariablesTraverser(
    std::vector<sh::Attribute> *attribs,
    std::vector<sh::OutputVariable> *outputVariables,
    std::vector<sh::Uniform> *uniforms,
    std::vector<sh::Varying> *varyings,
    std::vector<sh::InterfaceBlock> *interfaceBlocks,
    ShHashFunction64 hashFunction,
    const TSymbolTable &symbolTable,
    int shaderVersion,
    const TExtensionBehavior &extensionBehavior)
    : TIntermTraverser(true, false, false),
      mAttribs(attribs),
      mOutputVariables(outputVariables),
      mUniforms(uniforms),
      mVaryings(varyings),
      mInterfaceBlocks(interfaceBlocks),
      mDepthRangeAdded(false),
      mPointCoordAdded(false),
      mFrontFacingAdded(false),
      mFragCoordAdded(false),
      mInstanceIDAdded(false),
      mVertexIDAdded(false),
      mPositionAdded(false),
      mPointSizeAdded(false),
      mLastFragDataAdded(false),
      mFragColorAdded(false),
      mFragDataAdded(false),
      mFragDepthEXTAdded(false),
      mFragDepthAdded(false),
      mSecondaryFragColorEXTAdded(false),
      mSecondaryFragDataEXTAdded(false),
      mHashFunction(hashFunction),
      mSymbolTable(symbolTable),
      mShaderVersion(shaderVersion),
      mExtensionBehavior(extensionBehavior)
{
}

void CollectVariablesTraverser::setBuiltInInfoFromSymbolTable(const char *name,
                                                              ShaderVariable *info)
{
    TVariable *symbolTableVar =
        reinterpret_cast<TVariable *>(mSymbolTable.findBuiltIn(name, mShaderVersion));
    ASSERT(symbolTableVar);
    const TType &type = symbolTableVar->getType();

    info->name       = name;
    info->mappedName = name;
    info->type       = GLVariableType(type);
    info->arraySize  = type.isArray() ? type.getArraySize() : 0;
    info->precision  = GLVariablePrecision(type);
}

void CollectVariablesTraverser::recordBuiltInVaryingUsed(const char *name, bool *addedFlag)
{
    if (!(*addedFlag))
    {
        Varying info;
        setBuiltInInfoFromSymbolTable(name, &info);
        info.staticUse   = true;
        info.isInvariant = mSymbolTable.isVaryingInvariant(name);
        mVaryings->push_back(info);
        (*addedFlag) = true;
    }
}

void CollectVariablesTraverser::recordBuiltInFragmentOutputUsed(const char *name, bool *addedFlag)
{
    if (!(*addedFlag))
    {
        OutputVariable info;
        setBuiltInInfoFromSymbolTable(name, &info);
        info.staticUse = true;
        mOutputVariables->push_back(info);
        (*addedFlag) = true;
    }
}

void CollectVariablesTraverser::recordBuiltInAttributeUsed(const char *name, bool *addedFlag)
{
    if (!(*addedFlag))
    {
        Attribute info;
        setBuiltInInfoFromSymbolTable(name, &info);
        info.staticUse = true;
        info.location  = -1;
        mAttribs->push_back(info);
        (*addedFlag) = true;
    }
}

// We want to check whether a uniform/varying is statically used
// because we only count the used ones in packing computing.
// Also, gl_FragCoord, gl_PointCoord, and gl_FrontFacing count
// toward varying counting if they are statically used in a fragment
// shader.
void CollectVariablesTraverser::visitSymbol(TIntermSymbol *symbol)
{
    ASSERT(symbol != nullptr);
    ShaderVariable *var       = nullptr;
    const TString &symbolName = symbol->getSymbol();

    if (IsVarying(symbol->getQualifier()))
    {
        var = FindVariable(symbolName, mVaryings);
    }
    else if (symbol->getType().getBasicType() == EbtInterfaceBlock)
    {
        UNREACHABLE();
    }
    else if (symbolName == "gl_DepthRange")
    {
        ASSERT(symbol->getQualifier() == EvqUniform);

        if (!mDepthRangeAdded)
        {
            Uniform info;
            const char kName[] = "gl_DepthRange";
            info.name          = kName;
            info.mappedName    = kName;
            info.type          = GL_STRUCT_ANGLEX;
            info.arraySize     = 0;
            info.precision     = GL_NONE;
            info.staticUse     = true;

            ShaderVariable nearInfo;
            const char kNearName[] = "near";
            nearInfo.name          = kNearName;
            nearInfo.mappedName    = kNearName;
            nearInfo.type          = GL_FLOAT;
            nearInfo.arraySize     = 0;
            nearInfo.precision     = GL_HIGH_FLOAT;
            nearInfo.staticUse     = true;

            ShaderVariable farInfo;
            const char kFarName[] = "far";
            farInfo.name          = kFarName;
            farInfo.mappedName    = kFarName;
            farInfo.type          = GL_FLOAT;
            farInfo.arraySize     = 0;
            farInfo.precision     = GL_HIGH_FLOAT;
            farInfo.staticUse     = true;

            ShaderVariable diffInfo;
            const char kDiffName[] = "diff";
            diffInfo.name          = kDiffName;
            diffInfo.mappedName    = kDiffName;
            diffInfo.type          = GL_FLOAT;
            diffInfo.arraySize     = 0;
            diffInfo.precision     = GL_HIGH_FLOAT;
            diffInfo.staticUse     = true;

            info.fields.push_back(nearInfo);
            info.fields.push_back(farInfo);
            info.fields.push_back(diffInfo);

            mUniforms->push_back(info);
            mDepthRangeAdded = true;
        }
    }
    else
    {
        switch (symbol->getQualifier())
        {
            case EvqAttribute:
            case EvqVertexIn:
                var = FindVariable(symbolName, mAttribs);
                break;
            case EvqFragmentOut:
                var = FindVariable(symbolName, mOutputVariables);
                break;
            case EvqUniform:
            {
                const TInterfaceBlock *interfaceBlock = symbol->getType().getInterfaceBlock();
                if (interfaceBlock)
                {
                    InterfaceBlock *namedBlock =
                        FindVariable(interfaceBlock->name(), mInterfaceBlocks);
                    ASSERT(namedBlock);
                    var = FindVariable(symbolName, &namedBlock->fields);

                    // Set static use on the parent interface block here
                    namedBlock->staticUse = true;
                }
                else
                {
                    var = FindVariable(symbolName, mUniforms);
                }

                // It's an internal error to reference an undefined user uniform
                ASSERT(symbolName.compare(0, 3, "gl_") != 0 || var);
            }
            break;
            case EvqFragCoord:
                recordBuiltInVaryingUsed("gl_FragCoord", &mFragCoordAdded);
                return;
            case EvqFrontFacing:
                recordBuiltInVaryingUsed("gl_FrontFacing", &mFrontFacingAdded);
                return;
            case EvqPointCoord:
                recordBuiltInVaryingUsed("gl_PointCoord", &mPointCoordAdded);
                return;
            case EvqInstanceID:
                // Whenever the SH_INITIALIZE_BUILTINS_FOR_INSTANCED_MULTIVIEW option is set,
                // gl_InstanceID is added inside expressions to initialize ViewID_OVR and
                // InstanceID. gl_InstanceID is not added to the symbol table for ESSL1 shaders
                // which makes it necessary to populate the type information explicitly instead of
                // extracting it from the symbol table.
                if (!mInstanceIDAdded)
                {
                    Attribute info;
                    const char kName[] = "gl_InstanceID";
                    info.name          = kName;
                    info.mappedName    = kName;
                    info.type          = GL_INT;
                    info.arraySize     = 0;
                    info.precision     = GL_HIGH_INT;  // Defined by spec.
                    info.staticUse     = true;
                    info.location      = -1;
                    mAttribs->push_back(info);
                    mInstanceIDAdded = true;
                }
                return;
            case EvqVertexID:
                recordBuiltInAttributeUsed("gl_VertexID", &mVertexIDAdded);
                return;
            case EvqPosition:
                recordBuiltInVaryingUsed("gl_Position", &mPositionAdded);
                return;
            case EvqPointSize:
                recordBuiltInVaryingUsed("gl_PointSize", &mPointSizeAdded);
                return;
            case EvqLastFragData:
                recordBuiltInVaryingUsed("gl_LastFragData", &mLastFragDataAdded);
                return;
            case EvqFragColor:
                recordBuiltInFragmentOutputUsed("gl_FragColor", &mFragColorAdded);
                return;
            case EvqFragData:
                if (!mFragDataAdded)
                {
                    OutputVariable info;
                    setBuiltInInfoFromSymbolTable("gl_FragData", &info);
                    if (!::IsExtensionEnabled(mExtensionBehavior, "GL_EXT_draw_buffers"))
                    {
                        info.arraySize = 1;
                    }
                    info.staticUse = true;
                    mOutputVariables->push_back(info);
                    mFragDataAdded = true;
                }
                return;
            case EvqFragDepthEXT:
                recordBuiltInFragmentOutputUsed("gl_FragDepthEXT", &mFragDepthEXTAdded);
                return;
            case EvqFragDepth:
                recordBuiltInFragmentOutputUsed("gl_FragDepth", &mFragDepthAdded);
                return;
            case EvqSecondaryFragColorEXT:
                recordBuiltInFragmentOutputUsed("gl_SecondaryFragColorEXT",
                                                &mSecondaryFragColorEXTAdded);
                return;
            case EvqSecondaryFragDataEXT:
                recordBuiltInFragmentOutputUsed("gl_SecondaryFragDataEXT",
                                                &mSecondaryFragDataEXTAdded);
                return;
            default:
                break;
        }
    }
    if (var)
    {
        var->staticUse = true;
    }
}

void CollectVariablesTraverser::setCommonVariableProperties(const TType &type,
                                                            const TString &name,
                                                            ShaderVariable *variableOut) const
{
    ASSERT(variableOut);

    const TStructure *structure = type.getStruct();

    if (!structure)
    {
        variableOut->type      = GLVariableType(type);
        variableOut->precision = GLVariablePrecision(type);
    }
    else
    {
        // Note: this enum value is not exposed outside ANGLE
        variableOut->type       = GL_STRUCT_ANGLEX;
        variableOut->structName = structure->name().c_str();

        const TFieldList &fields = structure->fields();

        for (TField *field : fields)
        {
            // Regardless of the variable type (uniform, in/out etc.) its fields are always plain
            // ShaderVariable objects.
            ShaderVariable fieldVariable;
            setCommonVariableProperties(*field->type(), field->name(), &fieldVariable);
            variableOut->fields.push_back(fieldVariable);
        }
    }
    variableOut->name       = name.c_str();
    variableOut->mappedName = TIntermTraverser::hash(name, mHashFunction).c_str();
    variableOut->arraySize  = type.getArraySize();
}

Attribute CollectVariablesTraverser::recordAttribute(const TIntermSymbol &variable) const
{
    const TType &type = variable.getType();
    ASSERT(!type.getStruct());

    Attribute attribute;
    setCommonVariableProperties(type, variable.getSymbol(), &attribute);

    attribute.location = type.getLayoutQualifier().location;
    return attribute;
}

OutputVariable CollectVariablesTraverser::recordOutputVariable(const TIntermSymbol &variable) const
{
    const TType &type = variable.getType();
    ASSERT(!type.getStruct());

    OutputVariable outputVariable;
    setCommonVariableProperties(type, variable.getSymbol(), &outputVariable);

    outputVariable.location = type.getLayoutQualifier().location;
    return outputVariable;
}

Varying CollectVariablesTraverser::recordVarying(const TIntermSymbol &variable) const
{
    const TType &type = variable.getType();

    Varying varying;
    setCommonVariableProperties(type, variable.getSymbol(), &varying);

    switch (type.getQualifier())
    {
        case EvqVaryingIn:
        case EvqVaryingOut:
        case EvqVertexOut:
        case EvqSmoothOut:
        case EvqFlatOut:
        case EvqCentroidOut:
            if (mSymbolTable.isVaryingInvariant(std::string(variable.getSymbol().c_str())) ||
                type.isInvariant())
            {
                varying.isInvariant = true;
            }
            break;
        default:
            break;
    }

    varying.interpolation = GetInterpolationType(type.getQualifier());
    return varying;
}

InterfaceBlock CollectVariablesTraverser::recordInterfaceBlock(const TIntermSymbol &variable) const
{
    const TInterfaceBlock *blockType = variable.getType().getInterfaceBlock();
    ASSERT(blockType);

    InterfaceBlock interfaceBlock;
    interfaceBlock.name = blockType->name().c_str();
    interfaceBlock.mappedName =
        TIntermTraverser::hash(blockType->name().c_str(), mHashFunction).c_str();
    interfaceBlock.instanceName =
        (blockType->hasInstanceName() ? blockType->instanceName().c_str() : "");
    interfaceBlock.arraySize        = variable.getArraySize();
    interfaceBlock.isRowMajorLayout = (blockType->matrixPacking() == EmpRowMajor);
    interfaceBlock.binding          = blockType->blockBinding();
    interfaceBlock.layout           = GetBlockLayoutType(blockType->blockStorage());

    // Gather field information
    for (const TField *field : blockType->fields())
    {
        const TType &fieldType = *field->type();

        InterfaceBlockField fieldVariable;
        setCommonVariableProperties(fieldType, field->name(), &fieldVariable);
        fieldVariable.isRowMajorLayout =
            (fieldType.getLayoutQualifier().matrixPacking == EmpRowMajor);
        interfaceBlock.fields.push_back(fieldVariable);
    }
    return interfaceBlock;
}

Uniform CollectVariablesTraverser::recordUniform(const TIntermSymbol &variable) const
{
    Uniform uniform;
    setCommonVariableProperties(variable.getType(), variable.getSymbol(), &uniform);
    uniform.binding = variable.getType().getLayoutQualifier().binding;
    uniform.location = variable.getType().getLayoutQualifier().location;
    uniform.offset   = variable.getType().getLayoutQualifier().offset;
    return uniform;
}

bool CollectVariablesTraverser::visitDeclaration(Visit, TIntermDeclaration *node)
{
    const TIntermSequence &sequence = *(node->getSequence());
    ASSERT(!sequence.empty());

    const TIntermTyped &typedNode = *(sequence.front()->getAsTyped());
    TQualifier qualifier          = typedNode.getQualifier();

    bool isShaderVariable = qualifier == EvqAttribute || qualifier == EvqVertexIn ||
                            qualifier == EvqFragmentOut || qualifier == EvqUniform ||
                            IsVarying(qualifier);

    if (typedNode.getBasicType() != EbtInterfaceBlock && !isShaderVariable)
    {
        return true;
    }

    for (TIntermNode *variableNode : sequence)
    {
        // The only case in which the sequence will not contain a TIntermSymbol node is
        // initialization. It will contain a TInterBinary node in that case. Since attributes,
        // uniforms, varyings, outputs and interface blocks cannot be initialized in a shader, we
        // must have only TIntermSymbol nodes in the sequence in the cases we are interested in.
        const TIntermSymbol &variable = *variableNode->getAsSymbolNode();
        if (typedNode.getBasicType() == EbtInterfaceBlock)
        {
            mInterfaceBlocks->push_back(recordInterfaceBlock(variable));
        }
        else
        {
            switch (qualifier)
            {
                case EvqAttribute:
                case EvqVertexIn:
                    mAttribs->push_back(recordAttribute(variable));
                    break;
                case EvqFragmentOut:
                    mOutputVariables->push_back(recordOutputVariable(variable));
                    break;
                case EvqUniform:
                    mUniforms->push_back(recordUniform(variable));
                    break;
                default:
                    mVaryings->push_back(recordVarying(variable));
                    break;
            }
        }
    }

    // None of the recorded variables can have initializers, so we don't need to traverse the
    // declarators.
    return false;
}

bool CollectVariablesTraverser::visitBinary(Visit, TIntermBinary *binaryNode)
{
    if (binaryNode->getOp() == EOpIndexDirectInterfaceBlock)
    {
        // NOTE: we do not determine static use for individual blocks of an array
        TIntermTyped *blockNode = binaryNode->getLeft()->getAsTyped();
        ASSERT(blockNode);

        TIntermConstantUnion *constantUnion = binaryNode->getRight()->getAsConstantUnion();
        ASSERT(constantUnion);

        const TInterfaceBlock *interfaceBlock = blockNode->getType().getInterfaceBlock();
        InterfaceBlock *namedBlock = FindVariable(interfaceBlock->name(), mInterfaceBlocks);
        ASSERT(namedBlock);
        namedBlock->staticUse = true;

        unsigned int fieldIndex = constantUnion->getUConst(0);
        ASSERT(fieldIndex < namedBlock->fields.size());
        namedBlock->fields[fieldIndex].staticUse = true;
        return false;
    }

    return true;
}

}  // anonymous namespace

void CollectVariables(TIntermBlock *root,
                      std::vector<Attribute> *attributes,
                      std::vector<OutputVariable> *outputVariables,
                      std::vector<Uniform> *uniforms,
                      std::vector<Varying> *varyings,
                      std::vector<InterfaceBlock> *interfaceBlocks,
                      ShHashFunction64 hashFunction,
                      const TSymbolTable &symbolTable,
                      int shaderVersion,
                      const TExtensionBehavior &extensionBehavior)
{
    CollectVariablesTraverser collect(attributes, outputVariables, uniforms, varyings,
                                      interfaceBlocks, hashFunction, symbolTable, shaderVersion,
                                      extensionBehavior);
    root->traverse(&collect);
}

void ExpandVariable(const ShaderVariable &variable,
                    const std::string &name,
                    const std::string &mappedName,
                    bool markStaticUse,
                    std::vector<ShaderVariable> *expanded)
{
    if (variable.isStruct())
    {
        if (variable.isArray())
        {
            for (unsigned int elementIndex = 0; elementIndex < variable.elementCount();
                 elementIndex++)
            {
                std::string lname       = name + ::ArrayString(elementIndex);
                std::string lmappedName = mappedName + ::ArrayString(elementIndex);
                ExpandUserDefinedVariable(variable, lname, lmappedName, markStaticUse, expanded);
            }
        }
        else
        {
            ExpandUserDefinedVariable(variable, name, mappedName, markStaticUse, expanded);
        }
    }
    else
    {
        ShaderVariable expandedVar = variable;

        expandedVar.name       = name;
        expandedVar.mappedName = mappedName;

        // Mark all expanded fields as used if the parent is used
        if (markStaticUse)
        {
            expandedVar.staticUse = true;
        }

        if (expandedVar.isArray())
        {
            expandedVar.name += "[0]";
            expandedVar.mappedName += "[0]";
        }

        expanded->push_back(expandedVar);
    }
}

void ExpandUniforms(const std::vector<Uniform> &compact, std::vector<ShaderVariable> *expanded)
{
    for (const Uniform &variable : compact)
    {
        ExpandVariable(variable, variable.name, variable.mappedName, variable.staticUse, expanded);
    }
}

}  // namespace sh
