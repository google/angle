//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TranslatorVulkan:
//   A GLSL-based translator that outputs shaders that fit GL_KHR_vulkan_glsl.
//   The shaders are then fed into glslang to spit out SPIR-V (libANGLE-side).
//   See: https://www.khronos.org/registry/vulkan/specs/misc/GL_KHR_vulkan_glsl.txt
//

#include "compiler/translator/TranslatorVulkan.h"

#include "angle_gl.h"
#include "common/utilities.h"
#include "compiler/translator/ImmutableStringBuilder.h"
#include "compiler/translator/OutputVulkanGLSL.h"
#include "compiler/translator/StaticType.h"
#include "compiler/translator/tree_util/BuiltIn_autogen.h"
#include "compiler/translator/tree_util/FindMain.h"
#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/ReplaceVariable.h"
#include "compiler/translator/tree_util/RunAtTheEndOfShader.h"
#include "compiler/translator/util.h"

namespace sh
{

namespace
{
// This traverser is designed to strip out samplers from structs. It moves them into
// separate uniform sampler declarations. This allows the struct to be stored in the
// default uniform block. It also requires that we rewrite any functions that take the
// struct as an argument. The struct is split into two arguments.
class RewriteStructSamplers final : public TIntermTraverser
{
  public:
    RewriteStructSamplers(TSymbolTable *symbolTable)
        : TIntermTraverser(true, false, false, symbolTable), mRemovedUniformsCount(0)
    {
    }

    int removedUniformsCount() const { return mRemovedUniformsCount; }

    bool visitDeclaration(Visit visit, TIntermDeclaration *decl) override
    {
        ASSERT(visit == PreVisit);

        if (!mInGlobalScope)
        {
            return true;
        }

        const TIntermSequence &sequence = *(decl->getSequence());
        TIntermTyped *declarator        = sequence.front()->getAsTyped();
        const TType &type               = declarator->getType();

        if (type.isStructureContainingSamplers())
        {
            TIntermSequence *newSequence = new TIntermSequence;

            if (type.isStructSpecifier())
            {
                stripStructSpecifierSamplers(type.getStruct(), newSequence);
            }
            else
            {
                TIntermSymbol *asSymbol = declarator->getAsSymbolNode();
                ASSERT(asSymbol);
                const TVariable &variable = asSymbol->variable();
                ASSERT(variable.symbolType() != SymbolType::Empty);
                extractStructSamplerUniforms(decl, variable, type.getStruct(), newSequence);
            }

            mMultiReplacements.emplace_back(getParentNode()->getAsBlock(), decl, *newSequence);
        }

        return true;
    }

    bool visitBinary(Visit visit, TIntermBinary *node) override
    {
        if (node->getOp() == EOpIndexDirectStruct && node->getType().isSampler())
        {
            std::string stringBuilder;

            TIntermTyped *currentNode = node;
            while (currentNode->getAsBinaryNode())
            {
                TIntermBinary *asBinary = currentNode->getAsBinaryNode();

                switch (asBinary->getOp())
                {
                    case EOpIndexDirect:
                    {
                        const int index = asBinary->getRight()->getAsConstantUnion()->getIConst(0);
                        const std::string strInt = Str(index);
                        stringBuilder.insert(0, strInt);
                        stringBuilder.insert(0, "_");
                        break;
                    }
                    case EOpIndexDirectStruct:
                    {
                        stringBuilder.insert(0, asBinary->getIndexStructFieldName().data());
                        stringBuilder.insert(0, "_");
                        break;
                    }

                    default:
                        UNREACHABLE();
                        break;
                }

                currentNode = asBinary->getLeft();
            }

            const ImmutableString &variableName = currentNode->getAsSymbolNode()->variable().name();
            stringBuilder.insert(0, variableName.data());

            ImmutableString newName(stringBuilder);

            TVariable *samplerReplacement = mExtractedSamplers[newName];
            ASSERT(samplerReplacement);

            TIntermSymbol *replacement = new TIntermSymbol(samplerReplacement);

            queueReplacement(replacement, OriginalNode::IS_DROPPED);
            return true;
        }

        return true;
    }

  private:
    void stripStructSpecifierSamplers(const TStructure *structure, TIntermSequence *newSequence)
    {
        TFieldList *newFieldList = new TFieldList;
        ASSERT(structure->containsSamplers());

        // Removing the sampler field may produce struct indexing bugs.
        // TODO(jmadill): Fix potential bug. http://anglebug.com/2494
        for (const TField *field : structure->fields())
        {
            const TType &fieldType = *field->type();
            if (!fieldType.isSampler() && !isRemovedStructType(fieldType))
            {
                TType *newType = new TType(fieldType);
                TField *newField =
                    new TField(newType, field->name(), field->line(), field->symbolType());
                newFieldList->push_back(newField);
            }
        }

        // Prune empty structs.
        if (newFieldList->empty())
        {
            mRemovedStructs.insert(structure->name());
            return;
        }

        TStructure *newStruct =
            new TStructure(mSymbolTable, structure->name(), newFieldList, structure->symbolType());
        TType *newStructType = new TType(newStruct, true);
        TVariable *newStructVar =
            new TVariable(mSymbolTable, kEmptyImmutableString, newStructType, SymbolType::Empty);
        TIntermSymbol *newStructRef = new TIntermSymbol(newStructVar);

        TIntermDeclaration *structDecl = new TIntermDeclaration;
        structDecl->appendDeclarator(newStructRef);

        newSequence->push_back(structDecl);
    }

    bool isRemovedStructType(const TType &type) const
    {
        const TStructure *structure = type.getStruct();
        return (structure && (mRemovedStructs.count(structure->name()) > 0));
    }

    void extractStructSamplerUniforms(TIntermDeclaration *oldDeclaration,
                                      const TVariable &variable,
                                      const TStructure *structure,
                                      TIntermSequence *newSequence)
    {
        ASSERT(structure->containsSamplers());

        size_t nonSamplerCount = 0;

        for (const TField *field : structure->fields())
        {
            nonSamplerCount +=
                extractFieldSamplers(variable.name(), field, variable.getType(), newSequence);
        }

        if (nonSamplerCount > 0)
        {
            // Keep the old declaration around if it has other members.
            newSequence->push_back(oldDeclaration);
        }
        else
        {
            mRemovedUniformsCount++;
        }
    }

    size_t extractFieldSamplers(const ImmutableString &prefix,
                                const TField *field,
                                const TType &containingType,
                                TIntermSequence *newSequence)
    {
        if (containingType.isArray())
        {
            size_t nonSamplerCount = 0;

            // Name the samplers internally as varName_<index>_fieldName
            const TVector<unsigned int> &arraySizes = *containingType.getArraySizes();
            for (unsigned int arrayElement = 0; arrayElement < arraySizes[0]; ++arrayElement)
            {
                ImmutableStringBuilder stringBuilder(prefix.length() + 10);
                stringBuilder << prefix << "_";
                stringBuilder.appendHex(arrayElement);
                nonSamplerCount = extractFieldSamplersImpl(stringBuilder, field, newSequence);
            }

            return nonSamplerCount;
        }

        return extractFieldSamplersImpl(prefix, field, newSequence);
    }

    size_t extractFieldSamplersImpl(const ImmutableString &prefix,
                                    const TField *field,
                                    TIntermSequence *newSequence)
    {
        size_t nonSamplerCount = 0;

        const TType &fieldType = *field->type();
        if (fieldType.isSampler() || fieldType.isStructureContainingSamplers())
        {
            ImmutableStringBuilder stringBuilder(prefix.length() + field->name().length() + 1);
            stringBuilder << prefix << "_" << field->name();
            ImmutableString newPrefix(stringBuilder);

            if (fieldType.isSampler())
            {
                extractSampler(newPrefix, fieldType, newSequence);
            }
            else
            {
                const TStructure *structure = fieldType.getStruct();
                for (const TField *nestedField : structure->fields())
                {
                    nonSamplerCount +=
                        extractFieldSamplers(newPrefix, nestedField, fieldType, newSequence);
                }
            }
        }
        else
        {
            nonSamplerCount++;
        }

        return nonSamplerCount;
    }

    void extractSampler(const ImmutableString &newName,
                        const TType &fieldType,
                        TIntermSequence *newSequence)
    {
        TType *newType = new TType(fieldType);
        newType->setQualifier(EvqUniform);
        TVariable *newVariable =
            new TVariable(mSymbolTable, newName, newType, SymbolType::AngleInternal);
        TIntermSymbol *newRef = new TIntermSymbol(newVariable);

        TIntermDeclaration *samplerDecl = new TIntermDeclaration;
        samplerDecl->appendDeclarator(newRef);

        newSequence->push_back(samplerDecl);

        mExtractedSamplers[newName] = newVariable;
    }

    int mRemovedUniformsCount;
    std::map<ImmutableString, TVariable *> mExtractedSamplers;
    std::set<ImmutableString> mRemovedStructs;
};

// This traverser translates embedded uniform structs into a specifier and declaration.
// This makes the declarations easier to move into uniform blocks.
class NameEmbeddedUniformStructsTraverser : public TIntermTraverser
{
  public:
    explicit NameEmbeddedUniformStructsTraverser(TSymbolTable *symbolTable)
        : TIntermTraverser(true, false, false, symbolTable)
    {
    }

    bool visitDeclaration(Visit visit, TIntermDeclaration *decl) override
    {
        ASSERT(visit == PreVisit);

        if (!mInGlobalScope)
        {
            return false;
        }

        const TIntermSequence &sequence = *(decl->getSequence());
        ASSERT(sequence.size() == 1);
        TIntermTyped *declarator = sequence.front()->getAsTyped();
        const TType &type        = declarator->getType();

        if (type.isStructSpecifier() && type.getQualifier() == EvqUniform)
        {
            const TStructure *structure = type.getStruct();

            if (structure->symbolType() == SymbolType::Empty)
            {
                doReplacement(decl, declarator, structure);
            }
        }

        return false;
    }

  private:
    void doReplacement(TIntermDeclaration *decl,
                       TIntermTyped *declarator,
                       const TStructure *oldStructure)
    {
        // struct <structName> { ... };
        TStructure *structure = new TStructure(mSymbolTable, kEmptyImmutableString,
                                               &oldStructure->fields(), SymbolType::AngleInternal);
        TType *namedType      = new TType(structure, true);
        namedType->setQualifier(EvqGlobal);

        TVariable *structVariable =
            new TVariable(mSymbolTable, kEmptyImmutableString, namedType, SymbolType::Empty);
        TIntermSymbol *structDeclarator       = new TIntermSymbol(structVariable);
        TIntermDeclaration *structDeclaration = new TIntermDeclaration;
        structDeclaration->appendDeclarator(structDeclarator);

        TIntermSequence *newSequence = new TIntermSequence;
        newSequence->push_back(structDeclaration);

        // uniform <structName> <structUniformName>;
        TIntermSymbol *asSymbol = declarator->getAsSymbolNode();
        if (asSymbol && asSymbol->variable().symbolType() != SymbolType::Empty)
        {
            TIntermDeclaration *namedDecl = new TIntermDeclaration;
            TType *uniformType            = new TType(structure, false);
            uniformType->setQualifier(EvqUniform);

            TVariable *newVar        = new TVariable(mSymbolTable, asSymbol->getName(), uniformType,
                                              asSymbol->variable().symbolType());
            TIntermSymbol *newSymbol = new TIntermSymbol(newVar);
            namedDecl->appendDeclarator(newSymbol);

            newSequence->push_back(namedDecl);
        }

        mMultiReplacements.emplace_back(getParentNode()->getAsBlock(), decl, *newSequence);
    }
};

// This traverses nodes, find the struct ones and add their declarations to the sink. It also
// removes the nodes from the tree as it processes them.
class DeclareStructTypesTraverser : public TIntermTraverser
{
  public:
    explicit DeclareStructTypesTraverser(TOutputVulkanGLSL *outputVulkanGLSL)
        : TIntermTraverser(true, false, false), mOutputVulkanGLSL(outputVulkanGLSL)
    {
    }

    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override
    {
        ASSERT(visit == PreVisit);

        if (!mInGlobalScope)
        {
            return false;
        }

        const TIntermSequence &sequence = *(node->getSequence());
        TIntermTyped *declarator        = sequence.front()->getAsTyped();
        const TType &type               = declarator->getType();

        if (type.isStructSpecifier())
        {
            const TStructure *structure = type.getStruct();

            // Embedded structs should be parsed away by now.
            ASSERT(structure->symbolType() != SymbolType::Empty);
            mOutputVulkanGLSL->writeStructType(structure);

            TIntermSymbol *symbolNode = declarator->getAsSymbolNode();
            if (symbolNode && symbolNode->variable().symbolType() == SymbolType::Empty)
            {
                // Remove the struct specifier declaration from the tree so it isn't parsed again.
                TIntermSequence emptyReplacement;
                mMultiReplacements.emplace_back(getParentNode()->getAsBlock(), node,
                                                emptyReplacement);
            }
        }

        return false;
    }

  private:
    TOutputVulkanGLSL *mOutputVulkanGLSL;
};

class DeclareDefaultUniformsTraverser : public TIntermTraverser
{
  public:
    DeclareDefaultUniformsTraverser(TInfoSinkBase *sink,
                                    ShHashFunction64 hashFunction,
                                    NameMap *nameMap)
        : TIntermTraverser(true, true, true),
          mSink(sink),
          mHashFunction(hashFunction),
          mNameMap(nameMap),
          mInDefaultUniform(false)
    {
    }

    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override
    {
        const TIntermSequence &sequence = *(node->getSequence());

        // TODO(jmadill): Compound declarations.
        ASSERT(sequence.size() == 1);

        TIntermTyped *variable = sequence.front()->getAsTyped();
        const TType &type      = variable->getType();
        bool isUniform = (type.getQualifier() == EvqUniform) && !IsOpaqueType(type.getBasicType());

        if (visit == PreVisit)
        {
            if (isUniform)
            {
                (*mSink) << "    " << GetTypeName(type, mHashFunction, mNameMap) << " ";
                mInDefaultUniform = true;
            }
        }
        else if (visit == InVisit)
        {
            mInDefaultUniform = isUniform;
        }
        else if (visit == PostVisit)
        {
            if (isUniform)
            {
                (*mSink) << ";\n";

                // Remove the uniform declaration from the tree so it isn't parsed again.
                TIntermSequence emptyReplacement;
                mMultiReplacements.emplace_back(getParentNode()->getAsBlock(), node,
                                                emptyReplacement);
            }

            mInDefaultUniform = false;
        }
        return true;
    }

    void visitSymbol(TIntermSymbol *symbol) override
    {
        if (mInDefaultUniform)
        {
            const ImmutableString &name = symbol->variable().name();
            ASSERT(!name.beginsWith("gl_"));
            (*mSink) << HashName(name, mHashFunction, mNameMap) << ArrayString(symbol->getType());
        }
    }

  private:
    TInfoSinkBase *mSink;
    ShHashFunction64 mHashFunction;
    NameMap *mNameMap;
    bool mInDefaultUniform;
};

constexpr ImmutableString kFlippedPointCoordName = ImmutableString("flippedPointCoord");

// Declares a new variable to replace gl_PointCoord with a version that is flipping the Y
// coordinate.
void FlipGLPointCoord(TIntermBlock *root, TSymbolTable *symbolTable)
{
    // Create a symbol reference to "gl_PointCoord"
    const TVariable *pointCoord  = BuiltInVariable::gl_PointCoord();
    TIntermSymbol *pointCoordRef = new TIntermSymbol(pointCoord);

    // Create a swizzle to "gl_PointCoord.x"
    TVector<int> swizzleOffsetX;
    swizzleOffsetX.push_back(0);
    TIntermSwizzle *pointCoordX = new TIntermSwizzle(pointCoordRef, swizzleOffsetX);

    // Create a swizzle to "gl_PointCoord.y"
    TVector<int> swizzleOffsetY;
    swizzleOffsetY.push_back(1);
    TIntermSwizzle *pointCoordY = new TIntermSwizzle(pointCoordRef, swizzleOffsetY);

    // Create a symbol reference to our new variable that will hold the modified gl_PointCoord.
    TVariable *replacementVar =
        new TVariable(symbolTable, kFlippedPointCoordName,
                      StaticType::Helpers::GetForVecMatHelper<EbtFloat, EbpMedium, EvqGlobal, 1>(2),
                      SymbolType::UserDefined);
    DeclareGlobalVariable(root, replacementVar);
    TIntermSymbol *flippedPointCoordsRef = new TIntermSymbol(replacementVar);

    // Create a constant "-1.0"
    const TType *constantType             = StaticType::GetBasic<EbtFloat>();
    TConstantUnion *constantValueMinusOne = new TConstantUnion();
    constantValueMinusOne->setFConst(-1.0f);
    TIntermConstantUnion *minusOne = new TIntermConstantUnion(constantValueMinusOne, *constantType);

    // Create a constant "1.0"
    TConstantUnion *constantValueOne = new TConstantUnion();
    constantValueOne->setFConst(1.0f);
    TIntermConstantUnion *one = new TIntermConstantUnion(constantValueOne, *constantType);

    // Create the expression "gl_PointCoord.y * -1.0 + 1.0"
    TIntermBinary *inverseY = new TIntermBinary(EOpMul, pointCoordY, minusOne);
    TIntermBinary *plusOne  = new TIntermBinary(EOpAdd, inverseY, one);

    // Create the new vec2 using the modified Y
    TIntermSequence *sequence = new TIntermSequence();
    sequence->push_back(pointCoordX);
    sequence->push_back(plusOne);
    TIntermAggregate *aggregate =
        TIntermAggregate::CreateConstructor(BuiltInVariable::gl_PointCoord()->getType(), sequence);

    // Use this new variable instead of gl_PointCoord everywhere.
    ReplaceVariable(root, pointCoord, replacementVar);

    // Assign this new value to flippedPointCoord
    TIntermBinary *assignment = new TIntermBinary(EOpInitialize, flippedPointCoordsRef, aggregate);

    // Add this assigment at the beginning of the main function
    TIntermFunctionDefinition *main = FindMain(root);
    TIntermSequence *mainSequence   = main->getBody()->getSequence();
    mainSequence->insert(mainSequence->begin(), assignment);
}

// This operation performs the viewport depth translation needed by Vulkan. In GL the viewport
// transformation is slightly different - see the GL 2.0 spec section "2.12.1 Controlling the
// Viewport". In Vulkan the corresponding spec section is currently "23.4. Coordinate
// Transformations".
// The equations reduce to an expression:
//
//     z_vk = w_gl * (0.5 * z_gl + 0.5)
//
// where z_vk is the depth output of a Vulkan vertex shader and z_gl is the same for GL.
void AppendVertexShaderDepthCorrectionToMain(TIntermBlock *root, TSymbolTable *symbolTable)
{
    // Create a symbol reference to "gl_Position"
    const TVariable *position  = BuiltInVariable::gl_Position();
    TIntermSymbol *positionRef = new TIntermSymbol(position);

    // Create a swizzle to "gl_Position.z"
    TVector<int> swizzleOffsetZ;
    swizzleOffsetZ.push_back(2);
    TIntermSwizzle *positionZ = new TIntermSwizzle(positionRef, swizzleOffsetZ);

    // Create a constant "0.5"
    const TType *constantType     = StaticType::GetBasic<TBasicType::EbtFloat>();
    TConstantUnion *constantValue = new TConstantUnion();
    constantValue->setFConst(0.5f);
    TIntermConstantUnion *oneHalf = new TIntermConstantUnion(constantValue, *constantType);

    // Create the expression "gl_Position.z * 0.5 + 0.5"
    TIntermBinary *halfZ         = new TIntermBinary(TOperator::EOpMul, positionZ, oneHalf);
    TIntermBinary *halfZPlusHalf = new TIntermBinary(TOperator::EOpAdd, halfZ, oneHalf->deepCopy());

    // Create a swizzle to "gl_Position.w"
    TVector<int> swizzleOffsetW;
    swizzleOffsetW.push_back(3);
    TIntermSwizzle *positionW = new TIntermSwizzle(positionRef->deepCopy(), swizzleOffsetW);

    // Create the expression "gl_Position.w * (gl_Position.z * 0.5 + 0.5)"
    TIntermBinary *vulkanZ = new TIntermBinary(TOperator::EOpMul, positionW, halfZPlusHalf);

    // Create the assignment "gl_Position.z = gl_Position.w * (gl_Position.z * 0.5 + 0.5)"
    TIntermTyped *positionZLHS = positionZ->deepCopy();
    TIntermBinary *assignment  = new TIntermBinary(TOperator::EOpAssign, positionZLHS, vulkanZ);

    // Append the assignment as a statement at the end of the shader.
    RunAtTheEndOfShader(root, assignment, symbolTable);
}
}  // anonymous namespace

TranslatorVulkan::TranslatorVulkan(sh::GLenum type, ShShaderSpec spec)
    : TCompiler(type, spec, SH_GLSL_450_CORE_OUTPUT)
{
}

void TranslatorVulkan::translate(TIntermBlock *root,
                                 ShCompileOptions compileOptions,
                                 PerformanceDiagnostics * /*perfDiagnostics*/)
{
    TInfoSinkBase &sink = getInfoSink().obj;
    TOutputVulkanGLSL outputGLSL(sink, getArrayIndexClampingStrategy(), getHashFunction(),
                                 getNameMap(), &getSymbolTable(), getShaderType(),
                                 getShaderVersion(), getOutputType(), compileOptions);

    sink << "#version 450 core\n";

    // Write out default uniforms into a uniform block assigned to a specific set/binding.
    int defaultUniformCount        = 0;
    int structTypesUsedForUniforms = 0;
    for (const auto &uniform : getUniforms())
    {
        if (!uniform.isBuiltIn() && uniform.staticUse && !gl::IsOpaqueType(uniform.type))
        {
            ++defaultUniformCount;
        }

        if (uniform.isStruct())
        {
            ++structTypesUsedForUniforms;
        }
    }

    // TODO(lucferron): Refactor this function to do less tree traversals.
    // http://anglebug.com/2461
    if (structTypesUsedForUniforms > 0)
    {
        NameEmbeddedUniformStructsTraverser nameStructs(&getSymbolTable());
        root->traverse(&nameStructs);
        nameStructs.updateTree();

        RewriteStructSamplers rewriteStructSamplers(&getSymbolTable());
        root->traverse(&rewriteStructSamplers);
        rewriteStructSamplers.updateTree();

        defaultUniformCount -= rewriteStructSamplers.removedUniformsCount();

        // We must declare the struct types before using them.
        DeclareStructTypesTraverser structTypesTraverser(&outputGLSL);
        root->traverse(&structTypesTraverser);
        structTypesTraverser.updateTree();
    }

    if (defaultUniformCount > 0)
    {
        sink << "\nlayout(@@ DEFAULT-UNIFORMS-SET-BINDING @@) uniform defaultUniforms\n{\n";

        DeclareDefaultUniformsTraverser defaultTraverser(&sink, getHashFunction(), &getNameMap());
        root->traverse(&defaultTraverser);
        defaultTraverser.updateTree();

        sink << "};\n";
    }

    // Declare gl_FragColor and glFragData as webgl_FragColor and webgl_FragData
    // if it's core profile shaders and they are used.
    if (getShaderType() == GL_FRAGMENT_SHADER)
    {
        bool hasGLFragColor = false;
        bool hasGLFragData  = false;

        for (const OutputVariable &outputVar : outputVariables)
        {
            if (outputVar.name == "gl_FragColor")
            {
                ASSERT(!hasGLFragColor);
                hasGLFragColor = true;
                continue;
            }
            else if (outputVar.name == "gl_FragData")
            {
                ASSERT(!hasGLFragData);
                hasGLFragData = true;
                continue;
            }
        }
        ASSERT(!(hasGLFragColor && hasGLFragData));
        if (hasGLFragColor)
        {
            sink << "layout(location = 0) out vec4 webgl_FragColor;\n";
        }
        if (hasGLFragData)
        {
            sink << "layout(location = 0) out vec4 webgl_FragData[gl_MaxDrawBuffers];\n";
        }

        // Search for the gl_PointCoord usage, if its used, we need to flip the y coordinate.
        for (const Varying &inputVarying : inputVaryings)
        {
            if (!inputVarying.isBuiltIn())
            {
                continue;
            }

            if (inputVarying.name == "gl_PointCoord")
            {
                FlipGLPointCoord(root, &getSymbolTable());
                break;
            }
        }
    }
    else
    {
        ASSERT(getShaderType() == GL_VERTEX_SHADER);

        // Append depth range translation to main.
        AppendVertexShaderDepthCorrectionToMain(root, &getSymbolTable());
    }

    // Write translated shader.
    root->traverse(&outputGLSL);
}

bool TranslatorVulkan::shouldFlattenPragmaStdglInvariantAll()
{
    // Not necessary.
    return false;
}

}  // namespace sh
