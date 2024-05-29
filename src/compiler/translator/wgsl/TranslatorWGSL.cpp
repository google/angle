//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/wgsl/TranslatorWGSL.h"

#include "GLSLANG/ShaderLang.h"
#include "common/log_utils.h"
#include "compiler/translator/BaseTypes.h"
#include "compiler/translator/Diagnostics.h"
#include "compiler/translator/ImmutableString.h"
#include "compiler/translator/InfoSink.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/tree_util/IntermTraverse.h"

namespace sh
{
namespace
{

// A traverser that generates WGSL as it walks the AST.
class OutputWGSLTraverser : public TIntermTraverser
{
  public:
    OutputWGSLTraverser(TCompiler *compiler);
    ~OutputWGSLTraverser() override;

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
    void groupedTraverse(TIntermNode &node);
    void emitFunctionSignature(const TFunction &func);
    void emitFunctionReturn(const TFunction &func);
    void emitFunctionParameter(const TFunction &func, const TVariable &param);

    TInfoSinkBase &mSink;
};

OutputWGSLTraverser::OutputWGSLTraverser(TCompiler *compiler)
    : TIntermTraverser(true, false, false), mSink(compiler->getInfoSink().obj)
{}

OutputWGSLTraverser::~OutputWGSLTraverser() = default;

void OutputWGSLTraverser::visitSymbol(TIntermSymbol *symbolNode)
{
    // TODO(anglebug.com/8662): support emitting symbols.
    UNREACHABLE();
}

void OutputWGSLTraverser::visitConstantUnion(TIntermConstantUnion *constValueNode)
{
    // TODO(anglebug.com/8662): support emitting constants..
    UNREACHABLE();
}

bool OutputWGSLTraverser::visitSwizzle(Visit, TIntermSwizzle *swizzleNode)
{
    // TODO(anglebug.com/8662): support swizzle statements.
    UNREACHABLE();
    return false;
}

bool OutputWGSLTraverser::visitBinary(Visit, TIntermBinary *binaryNode)
{
    // TODO(anglebug.com/8662): support binary statements.
    UNREACHABLE();
    return false;
}

bool OutputWGSLTraverser::visitUnary(Visit, TIntermUnary *unaryNode)
{
    // TODO(anglebug.com/8662): support unary statements.
    UNREACHABLE();
    return false;
}

bool OutputWGSLTraverser::visitTernary(Visit, TIntermTernary *conditionalNode)
{
    // TODO(anglebug.com/8662): support ternaries.
    UNREACHABLE();
    return false;
}

bool OutputWGSLTraverser::visitIfElse(Visit, TIntermIfElse *ifThenElseNode)
{
    // TODO(anglebug.com/8662): support basic control flow.
    UNREACHABLE();
    return false;
}

bool OutputWGSLTraverser::visitSwitch(Visit, TIntermSwitch *switchNode)
{
    // TODO(anglebug.com/8662): support switch statements.
    UNREACHABLE();
    return false;
}

bool OutputWGSLTraverser::visitCase(Visit, TIntermCase *caseNode)
{
    // TODO(anglebug.com/8662): support switch statements.
    UNREACHABLE();
    return false;
}

void OutputWGSLTraverser::visitFunctionPrototype(TIntermFunctionPrototype *funcProtoNode)
{
    // TODO(anglebug.com/8662): support function prototypes.
    UNREACHABLE();
}

bool OutputWGSLTraverser::visitFunctionDefinition(Visit, TIntermFunctionDefinition *funcDefNode)
{
    // TODO(anglebug.com/8662): support function definitions.
    UNREACHABLE();
    return false;
}

bool OutputWGSLTraverser::visitAggregate(Visit, TIntermAggregate *aggregateNode)
{
    // TODO(anglebug.com/8662): support aggregate statements.
    UNREACHABLE();
    return false;
}

bool OutputWGSLTraverser::visitBlock(Visit, TIntermBlock *blockNode)
{
    // TODO(anglebug.com/8662): support emitting blocks.
    return false;
}

bool OutputWGSLTraverser::visitGlobalQualifierDeclaration(Visit,
                                                          TIntermGlobalQualifierDeclaration *)
{
    return false;
}

bool OutputWGSLTraverser::visitDeclaration(Visit, TIntermDeclaration *declNode)
{
    // TODO(anglebug.com/8662): support variable declarations.
    UNREACHABLE();
    return false;
}

bool OutputWGSLTraverser::visitLoop(Visit, TIntermLoop *loopNode)
{
    // TODO(anglebug.com/8662): emit loops.
    UNREACHABLE();
    return false;
}

bool OutputWGSLTraverser::visitBranch(Visit, TIntermBranch *branchNode)
{
    // TODO(anglebug.com/8662): emit branch instructions.
    UNREACHABLE();
    return false;
}

void OutputWGSLTraverser::visitPreprocessorDirective(TIntermPreprocessorDirective *node)
{
    // No preprocessor directives expected at this point.
    UNREACHABLE();
}

}  // namespace

TranslatorWGSL::TranslatorWGSL(sh::GLenum type, ShShaderSpec spec, ShShaderOutput output)
    : TCompiler(type, spec, output)
{}

bool TranslatorWGSL::translate(TIntermBlock *root,
                               const ShCompileOptions &compileOptions,
                               PerformanceDiagnostics *perfDiagnostics)
{
    // TODO(anglebug.com/8662): until the translator is ready to translate most basic shaders, emit
    // the code commented out.
    TInfoSinkBase &sink = getInfoSink().obj;
    sink << "/*\n";
    OutputWGSLTraverser traverser(this);
    root->traverse(&traverser);
    sink << "*/\n";

    std::cout << getInfoSink().obj.str();

    // TODO(anglebug.com/8662): delete this.
    if (getShaderType() == GL_VERTEX_SHADER)
    {
        constexpr const char *kVertexShader = R"(@vertex
fn main(@builtin(vertex_index) vertex_index : u32) -> @builtin(position) vec4f
{
    const pos = array(
        vec2( 0.0,  0.5),
        vec2(-0.5, -0.5),
        vec2( 0.5, -0.5)
    );

    return vec4f(pos[vertex_index % 3], 0, 1);
})";
        sink << kVertexShader;
    }
    else if (getShaderType() == GL_FRAGMENT_SHADER)
    {
        constexpr const char *kFragmentShader = R"(@fragment
fn main() -> @location(0) vec4f
{
    return vec4(1, 0, 0, 1);
})";
        sink << kFragmentShader;
    }
    else
    {
        UNREACHABLE();
        return false;
    }

    return true;
}

bool TranslatorWGSL::shouldFlattenPragmaStdglInvariantAll()
{
    // Not neccesary for WGSL transformation.
    return false;
}
}  // namespace sh
