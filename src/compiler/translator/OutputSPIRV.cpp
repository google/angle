//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// OutputSPIRV: Generate SPIR-V from the AST.
//

#include "compiler/translator/OutputSPIRV.h"

#include "angle_gl.h"
#include "common/debug.h"
#include "common/mathutil.h"
#include "common/spirv/spirv_instruction_builder_autogen.h"
#include "compiler/translator/BuildSPIRV.h"
#include "compiler/translator/Compiler.h"
#include "compiler/translator/tree_util/IntermTraverse.h"

#include <cfloat>

// SPIR-V tools include for disassembly
#include <spirv-tools/libspirv.hpp>

// Enable this for debug logging of pre-transform SPIR-V:
#if !defined(ANGLE_DEBUG_SPIRV_TRANSFORMER)
#    define ANGLE_DEBUG_SPIRV_TRANSFORMER 0
#endif  // !defined(ANGLE_DEBUG_SPIRV_TRANSFORMER)

namespace sh
{
namespace
{
class OutputSPIRVTraverser : public TIntermTraverser
{
  public:
    OutputSPIRVTraverser(TCompiler *compiler, ShCompileOptions compileOptions);

    spirv::Blob getSpirv();

  protected:
    void visitSymbol(TIntermSymbol *node) override;
    void visitConstantUnion(TIntermConstantUnion *node) override;
    bool visitSwizzle(Visit visit, TIntermSwizzle *node) override;
    bool visitBinary(Visit visit, TIntermBinary *node) override;
    bool visitUnary(Visit visit, TIntermUnary *node) override;
    bool visitTernary(Visit visit, TIntermTernary *node) override;
    bool visitIfElse(Visit visit, TIntermIfElse *node) override;
    bool visitSwitch(Visit visit, TIntermSwitch *node) override;
    bool visitCase(Visit visit, TIntermCase *node) override;
    void visitFunctionPrototype(TIntermFunctionPrototype *node) override;
    bool visitFunctionDefinition(Visit visit, TIntermFunctionDefinition *node) override;
    bool visitAggregate(Visit visit, TIntermAggregate *node) override;
    bool visitBlock(Visit visit, TIntermBlock *node) override;
    bool visitGlobalQualifierDeclaration(Visit visit,
                                         TIntermGlobalQualifierDeclaration *node) override;
    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override;
    bool visitLoop(Visit visit, TIntermLoop *node) override;
    bool visitBranch(Visit visit, TIntermBranch *node) override;
    void visitPreprocessorDirective(TIntermPreprocessorDirective *node) override;

  private:
    ANGLE_MAYBE_UNUSED TCompiler *mCompiler;
    ANGLE_MAYBE_UNUSED ShCompileOptions mCompileOptions;

    SPIRVBuilder mBuilder;
};

spv::StorageClass GetStorageClass(const TType &type)
{
    // Opaque uniforms (samplers and images) have the UniformConstant storage class
    if (type.isSampler() || type.isImage())
    {
        return spv::StorageClassUniformConstant;
    }

    // Input varying and IO blocks have the Input storage class
    if (IsShaderIn(type.getQualifier()))
    {
        return spv::StorageClassInput;
    }

    // Output varying and IO blocks have the Input storage class
    if (IsShaderOut(type.getQualifier()))
    {
        return spv::StorageClassOutput;
    }

    // Uniform and storage buffers have the Uniform storage class
    if (type.isInterfaceBlock())
    {
        // I/O blocks must have already been classified as input or output above.
        ASSERT(!IsShaderIoBlock(type.getQualifier()));
        return spv::StorageClassUniform;
    }

    // Compute shader shared memory has the Workgroup storage class
    if (type.getQualifier() == EvqShared)
    {
        return spv::StorageClassWorkgroup;
    }

    // All other variables are either Private or Function, based on whether they are global or
    // function-local.
    if (type.getQualifier() == EvqGlobal)
    {
        return spv::StorageClassPrivate;
    }

    ASSERT(type.getQualifier() == EvqTemporary);
    return spv::StorageClassFunction;
}

OutputSPIRVTraverser::OutputSPIRVTraverser(TCompiler *compiler, ShCompileOptions compileOptions)
    : TIntermTraverser(true, true, true, &compiler->getSymbolTable()),
      mCompiler(compiler),
      mCompileOptions(compileOptions),
      mBuilder(gl::FromGLenum<gl::ShaderType>(compiler->getShaderType()),
               compiler->getHashFunction(),
               compiler->getNameMap())
{}

void OutputSPIRVTraverser::visitSymbol(TIntermSymbol *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();
}

void OutputSPIRVTraverser::visitConstantUnion(TIntermConstantUnion *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();
}

bool OutputSPIRVTraverser::visitSwizzle(Visit visit, TIntermSwizzle *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitBinary(Visit visit, TIntermBinary *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitUnary(Visit visit, TIntermUnary *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitTernary(Visit visit, TIntermTernary *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitIfElse(Visit visit, TIntermIfElse *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitSwitch(Visit visit, TIntermSwitch *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitCase(Visit visit, TIntermCase *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return false;
}

bool OutputSPIRVTraverser::visitBlock(Visit visit, TIntermBlock *node)
{
    // If global block, nothing to generate.
    if (getCurrentTraversalDepth() == 0)
    {
        return true;
    }

    if (visit == PreVisit)
    {
        const spirv::IdRef blockLabelId = mBuilder.getNewId();
        spirv::WriteLabel(mBuilder.getSpirvFunctions(), blockLabelId);
    }

    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return false;
}

bool OutputSPIRVTraverser::visitFunctionDefinition(Visit visit, TIntermFunctionDefinition *node)
{
    if (visit == PreVisit)
    {
        const TFunction *function = node->getFunction();

        // Declare the function type
        const spirv::IdRef returnTypeId =
            mBuilder.getTypeData(function->getReturnType(), EbsUnspecified).id;

        spirv::IdRefList paramTypeIds;
        for (size_t paramIndex = 0; paramIndex < function->getParamCount(); ++paramIndex)
        {
            paramTypeIds.push_back(
                mBuilder.getTypeData(function->getParam(paramIndex)->getType(), EbsUnspecified).id);
        }

        const spirv::IdRef functionTypeId = mBuilder.getFunctionTypeId(returnTypeId, paramTypeIds);

        // Declare the function itself
        const spirv::IdRef functionId = mBuilder.getNewId();
        spirv::WriteFunction(mBuilder.getSpirvFunctions(), returnTypeId, functionId,
                             spv::FunctionControlMaskNone, functionTypeId);

        for (size_t paramIndex = 0; paramIndex < function->getParamCount(); ++paramIndex)
        {
            const spirv::IdRef paramId = mBuilder.getNewId();
            spirv::WriteFunctionParameter(mBuilder.getSpirvFunctions(), paramTypeIds[paramIndex],
                                          paramId);

            // TODO: Add to TVariable to variableId map so references to this variable can discover
            // the ID.  http://anglebug.com/4889
        }

        // Remember the ID of main() for the sake of OpEntryPoint.
        if (function->isMain())
        {
            mBuilder.setEntryPointId(functionId);
        }

        return true;
    }

    if (visit == PostVisit)
    {
        // TODO: if the function returns void, the AST may not have an explicit OpReturn node, so
        // generate one at the end if not already.  For testing, unconditionally add it.
        // http://anglebug.com/4889
        if (node->getFunction()->getReturnType().getBasicType() == EbtVoid)
        {
            spirv::WriteReturn(mBuilder.getSpirvFunctions());
        }

        // End the function
        spirv::WriteFunctionEnd(mBuilder.getSpirvFunctions());
    }

    return true;
}

bool OutputSPIRVTraverser::visitGlobalQualifierDeclaration(Visit visit,
                                                           TIntermGlobalQualifierDeclaration *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

void OutputSPIRVTraverser::visitFunctionPrototype(TIntermFunctionPrototype *node)
{
    // Nothing to do.  The function type is declared together with its definition.
}

bool OutputSPIRVTraverser::visitAggregate(Visit visit, TIntermAggregate *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return false;
}

bool OutputSPIRVTraverser::visitDeclaration(Visit visit, TIntermDeclaration *node)
{
    if (visit != PreVisit)
    {
        return true;
    }

    const TIntermSequence &sequence = *node->getSequence();

    // Enforced by ValidateASTOptions::validateMultiDeclarations.
    ASSERT(sequence.size() == 1);

    TIntermTyped *declVariable = sequence.front()->getAsTyped();
    const TType &type          = declVariable->getType();
    TIntermSymbol *symbol      = declVariable->getAsSymbolNode();
    ASSERT(symbol != nullptr);

    // If this is just a struct declaration (and not a variable declaration), don't declare the
    // struct up-front and let it be lazily defined.  If the struct is only used inside an interface
    // block for example, this avoids it being doubly defined (once with the unspecified block
    // storage and once with interface block's).
    if (type.isStructSpecifier() && symbol->variable().symbolType() == SymbolType::Empty)
    {
        return false;
    }

    const spirv::IdRef typeId = mBuilder.getTypeData(type, EbsUnspecified).id;

    // TODO: handle constant declarations.  http://anglebug.com/4889

    spv::StorageClass storageClass   = GetStorageClass(type);
    const spirv::IdRef typePointerId = mBuilder.getTypePointerId(typeId, storageClass);

    spirv::Blob *spirvSection = storageClass == spv::StorageClassFunction
                                    ? mBuilder.getSpirvFunctions()
                                    : mBuilder.getSpirvVariableDecls();

    const spirv::IdRef variableId = mBuilder.getNewId();
    // TODO: handle initializers.  http://anglebug.com/4889
    spirv::WriteVariable(spirvSection, typePointerId, variableId, storageClass, nullptr);

    // TODO: create a TVariable to variableId map so references to this variable can discover the
    // ID.  http://anglebug.com/4889

    if (IsShaderIn(type.getQualifier()) || IsShaderOut(type.getQualifier()))
    {
        // Add in and out variables to the list of interface variables.
        mBuilder.addEntryPointInterfaceVariableId(variableId);

        if (IsShaderIoBlock(type.getQualifier()) && type.isInterfaceBlock())
        {
            // For gl_PerVertex in particular, write the necessary BuiltIn decorations
            if (type.getQualifier() == EvqPerVertexIn || type.getQualifier() == EvqPerVertexOut)
            {
                mBuilder.writePerVertexBuiltIns(type, typeId);
            }

            // I/O blocks are decorated with Block
            spirv::WriteDecorate(mBuilder.getSpirvDecorations(), typeId, spv::DecorationBlock, {});
        }
    }
    else if (type.getBasicType() == EbtInterfaceBlock)
    {
        // For uniform and buffer variables, add Block and BufferBlock decorations respectively.
        const spv::Decoration decoration =
            type.getQualifier() == EvqUniform ? spv::DecorationBlock : spv::DecorationBufferBlock;
        spirv::WriteDecorate(mBuilder.getSpirvDecorations(), typeId, decoration, {});
    }

    // Write DescriptorSet, Binding, Location etc decorations if necessary.
    mBuilder.writeInterfaceVariableDecorations(type, variableId);

    // Output debug information.
    spirv::WriteName(mBuilder.getSpirvDebug(), variableId,
                     mBuilder.hashName(&symbol->variable()).data());

    return false;
}

bool OutputSPIRVTraverser::visitLoop(Visit visit, TIntermLoop *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitBranch(Visit visit, TIntermBranch *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

void OutputSPIRVTraverser::visitPreprocessorDirective(TIntermPreprocessorDirective *node)
{
    // No preprocessor directives expected at this point.
    UNREACHABLE();
}

spirv::Blob OutputSPIRVTraverser::getSpirv()
{
    spirv::Blob result = mBuilder.getSpirv();

    // Validate that correct SPIR-V was generated
    ASSERT(spirv::Validate(result));

#if ANGLE_DEBUG_SPIRV_TRANSFORMER
    // Disassemble and log the generated SPIR-V for debugging.
    spvtools::SpirvTools spirvTools(SPV_ENV_VULKAN_1_1);
    std::string readableSpirv;
    spirvTools.Disassemble(result, &readableSpirv, 0);
    fprintf(stderr, "%s\n", readableSpirv.c_str());
#endif  // ANGLE_DEBUG_SPIRV_TRANSFORMER

    return result;
}
}  // anonymous namespace

bool OutputSPIRV(TCompiler *compiler, TIntermBlock *root, ShCompileOptions compileOptions)
{
    // Traverse the tree and generate SPIR-V instructions
    OutputSPIRVTraverser traverser(compiler, compileOptions);
    root->traverse(&traverser);

    // Generate the final SPIR-V and store in the sink
    spirv::Blob spirvBlob = traverser.getSpirv();
    compiler->getInfoSink().obj.setBinary(std::move(spirvBlob));

    return true;
}
}  // namespace sh
