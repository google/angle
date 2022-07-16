//
// Copyright 2022 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/tree_ops/RewritePixelLocalStorage.h"

#include "common/angleutils.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_ops/MonomorphizeUnsupportedFunctions.h"
#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/IntermTraverse.h"
#include "compiler/translator/tree_util/RunAtTheBeginningOfShader.h"

namespace sh
{

namespace
{

constexpr static TBasicType DataTypeOfPLSType(TBasicType plsType)
{
    switch (plsType)
    {
        case EbtPixelLocalANGLE:
            return EbtFloat;
        case EbtIPixelLocalANGLE:
            return EbtInt;
        case EbtUPixelLocalANGLE:
            return EbtUInt;
        default:
            UNREACHABLE();
            return EbtVoid;
    }
}

constexpr static TBasicType Image2DTypeOfPLSType(TBasicType plsType)
{
    switch (plsType)
    {
        case EbtPixelLocalANGLE:
            return EbtImage2D;
        case EbtIPixelLocalANGLE:
            return EbtIImage2D;
        case EbtUPixelLocalANGLE:
            return EbtUImage2D;
        default:
            UNREACHABLE();
            return EbtVoid;
    }
}

// Rewrites high level PLS operations to shader image operations.
class RewriteToImagesTraverser : public TIntermTraverser
{
  public:
    RewriteToImagesTraverser(TSymbolTable &symbolTable, int shaderVersion)
        : TIntermTraverser(true, false, false, &symbolTable), mShaderVersion(shaderVersion)
    {}

    bool visitDeclaration(Visit, TIntermDeclaration *decl) override
    {
        TIntermTyped *declVariable = (decl->getSequence())->front()->getAsTyped();
        ASSERT(declVariable);

        if (!IsPixelLocal(declVariable->getBasicType()))
        {
            return true;
        }

        // PLS is not allowed in arrays.
        ASSERT(!declVariable->isArray());

        // This visitDeclaration doesn't get called for function arguments, and opaque types can
        // otherwise only be uniforms.
        ASSERT(declVariable->getQualifier() == EvqUniform);

        TIntermSymbol *plsSymbol = declVariable->getAsSymbolNode();
        ASSERT(plsSymbol);

        // Insert a global to hold the pixel coordinate as soon as we see PLS declared. This will be
        // initialized at the beginning of main().
        if (!mGlobalPixelCoord)
        {
            TType *coordType  = new TType(EbtInt, EbpHigh, EvqGlobal, 2);
            mGlobalPixelCoord = CreateTempVariable(mSymbolTable, coordType);
            insertStatementInParentBlock(CreateTempDeclarationNode(mGlobalPixelCoord));
        }

        PLSImages &pls = insertNullPLSImages(plsSymbol);

        // Insert a readonly image2D directly before the PLS declaration.
        pls.image2DForLoading = createPLSImage(plsSymbol, ImageAccess::Readonly);
        insertStatementInParentBlock(
            new TIntermDeclaration({new TIntermSymbol(pls.image2DForLoading)}));

        // Replace the PLS declaration with a writeonly image2D.
        pls.image2DForStoring = createPLSImage(plsSymbol, ImageAccess::Writeonly);
        queueReplacement(new TIntermDeclaration({new TIntermSymbol(pls.image2DForStoring)}),
                         OriginalNode::IS_DROPPED);

        return false;
    }

    bool visitAggregate(Visit, TIntermAggregate *aggregate) override
    {
        if (!BuiltInGroup::IsPixelLocal(aggregate->getOp()))
        {
            return true;
        }

        const TIntermSequence &args = *aggregate->getSequence();
        ASSERT(args.size() >= 1);
        TIntermSymbol *plsSymbol = args[0]->getAsSymbolNode();
        const PLSImages &pls     = findPLSImages(plsSymbol);
        ASSERT(mGlobalPixelCoord);

        // Rewrite pixelLocalLoadANGLE -> imageLoad.
        if (aggregate->getOp() == EOpPixelLocalLoadANGLE)
        {
            // Replace the pixelLocalLoadANGLE with imageLoad.
            TIntermSequence imageLoadArgs = {new TIntermSymbol(pls.image2DForLoading),
                                             new TIntermSymbol(mGlobalPixelCoord)};
            queueReplacement(CreateBuiltInFunctionCallNode("imageLoad", &imageLoadArgs,
                                                           *mSymbolTable, mShaderVersion),
                             OriginalNode::IS_DROPPED);

            return false;  // No need to recurse since this node is being dropped.
        }

        // Rewrite pixelLocalStoreANGLE -> imageStore.
        if (aggregate->getOp() == EOpPixelLocalStoreANGLE)
        {
            // Since ES 3.1 makes us have readonly and writeonly aliases of the same image, we have
            // to surround every pixelLocalStoreANGLE with memoryBarrier calls.
            //
            // Also hoist the 'value' expression into a temp. In the event of
            // "pixelLocalStoreANGLE(..., pixelLocalLoadANGLE(...))", this ensures the load occurs
            // _before_ the memoryBarrier.
            //
            // NOTE: It is generally unsafe to hoist function arguments due to short circuiting,
            // e.g., "if (false && function(...))", but pixelLocalStoreANGLE returns type void, so
            // it is safe in this particular case.
            TType *valueType    = new TType(DataTypeOfPLSType(plsSymbol->getBasicType()),
                                            plsSymbol->getPrecision(), EvqTemporary, 4);
            TVariable *valueVar = CreateTempVariable(mSymbolTable, valueType);
            TIntermDeclaration *valueDecl =
                CreateTempInitDeclarationNode(valueVar, args[1]->getAsTyped());
            valueDecl->traverse(this);  // Rewrite any potential pixelLocalLoadANGLEs in valueDecl.

            insertStatementsInParentBlock({valueDecl, createMemoryBarrierNode()},  // Before.
                                          {createMemoryBarrierNode()});            // After.

            // Rewrite the pixelLocalStoreANGLE with imageStore.
            TIntermSequence imageStoreArgs = {new TIntermSymbol(pls.image2DForStoring),
                                              new TIntermSymbol(mGlobalPixelCoord),
                                              new TIntermSymbol(valueVar)};
            queueReplacement(CreateBuiltInFunctionCallNode("imageStore", &imageStoreArgs,
                                                           *mSymbolTable, mShaderVersion),
                             OriginalNode::IS_DROPPED);

            return false;  // No need to recurse since this node is being dropped.
        }

        return true;
    }

    TVariable *globalPixelCoord() const { return mGlobalPixelCoord; }

  private:
    // Internal implementation of an opaque 'gpixelLocalANGLE' handle. Since ES 3.1 requires most
    // image formats to be either readonly or writeonly, we have to make two separate images that
    // alias the same binding.
    struct PLSImages
    {
        TVariable *image2DForLoading = nullptr;
        TVariable *image2DForStoring = nullptr;
    };

    // Adds a null 'PLSImages' entry to the map for the given symbol. An entry must not already
    // exist in the map for this symbol.
    PLSImages &insertNullPLSImages(TIntermSymbol *plsSymbol)
    {
        ASSERT(plsSymbol);
        ASSERT(IsPixelLocal(plsSymbol->getBasicType()));
        int binding = plsSymbol->getType().getLayoutQualifier().binding;
        ASSERT(binding >= 0);
        auto result = mPLSImages.insert({binding, PLSImages()});
        ASSERT(result.second);  // Ensure PLSImages didn't already exist for this symbol.
        return result.first->second;
    }

    // Looks up the PLSImages for the given symbol. An entry must already exist in the map for this
    // symbol.
    PLSImages &findPLSImages(TIntermSymbol *plsSymbol)
    {
        ASSERT(plsSymbol);
        ASSERT(IsPixelLocal(plsSymbol->getBasicType()));
        int binding = plsSymbol->getType().getLayoutQualifier().binding;
        ASSERT(binding >= 0);
        auto iter = mPLSImages.find(binding);
        ASSERT(iter != mPLSImages.end());  // Ensure PLSImages already exist for this symbol.
        return iter->second;
    }

    enum class ImageAccess
    {
        Readonly,
        Writeonly
    };

    // Creates a 'gimage2D' that implements a pixel local storage handle.
    TVariable *createPLSImage(const TIntermSymbol *plsSymbol, ImageAccess access)
    {
        ASSERT(plsSymbol);
        ASSERT(IsPixelLocal(plsSymbol->getBasicType()));

        TMemoryQualifier memoryQualifier;
        memoryQualifier.coherent          = true;
        memoryQualifier.restrictQualifier = false;
        memoryQualifier.volatileQualifier = true;
        memoryQualifier.readonly          = access == ImageAccess::Readonly;
        memoryQualifier.writeonly         = access == ImageAccess::Writeonly;

        TType *imageType = new TType(plsSymbol->getType());
        imageType->setBasicType(Image2DTypeOfPLSType(plsSymbol->getBasicType()));
        imageType->setMemoryQualifier(memoryQualifier);

        std::string name = "_pls";
        name.append(plsSymbol->getName().data());
        name.append(access == ImageAccess::Readonly ? "_R" : "_W");
        return new TVariable(mSymbolTable, ImmutableString(name), imageType, SymbolType::BuiltIn);
    }

    // Creates a function call to memoryBarrier().
    TIntermNode *createMemoryBarrierNode()
    {
        TIntermSequence emptyArgs;
        return CreateBuiltInFunctionCallNode("memoryBarrier", &emptyArgs, *mSymbolTable,
                                             mShaderVersion);
    }

    const int mShaderVersion;

    // Stores the shader invocation's pixel coordinate as "ivec2(floor(gl_FragCoord.xy))".
    TVariable *mGlobalPixelCoord = nullptr;

    // Maps PLS variables to their gimage2D aliases.
    angle::HashMap<int, PLSImages> mPLSImages;
};

}  // anonymous namespace

bool RewritePixelLocalStorageToImages(TCompiler *compiler,
                                      TIntermBlock *root,
                                      TSymbolTable &symbolTable,
                                      ShCompileOptions compileOptions,
                                      int shaderVersion)
{
    // If any functions take PLS arguments, monomorphize the functions by removing said parameters
    // and making the PLS calls from main() instead, using the global uniform from the call site
    // instead of the function argument. This is necessary because function arguments don't carry
    // the necessary "binding" or "format" layout qualifiers.
    if (!MonomorphizeUnsupportedFunctions(
            compiler, root, &symbolTable, compileOptions,
            UnsupportedFunctionArgsBitSet{UnsupportedFunctionArgs::PixelLocalStorage}))
    {
        return false;
    }
    RewriteToImagesTraverser traverser(symbolTable, shaderVersion);
    root->traverse(&traverser);
    if (!traverser.updateTree(compiler, root))
    {
        return false;
    }
    if (traverser.globalPixelCoord())
    {
        // Initialize the global pixel coord at the beginning of main():
        //
        //     pixelCoord = ivec2(floor(gl_FragCoord.xy));
        //
        TIntermTyped *expr;
        expr =
            ReferenceBuiltInVariable(ImmutableString("gl_FragCoord"), symbolTable, shaderVersion);
        expr = CreateSwizzle(expr, 0, 1);
        expr = CreateBuiltInUnaryFunctionCallNode("floor", expr, symbolTable, shaderVersion);
        TIntermSequence typeConversionArgs = {expr};
        expr = TIntermAggregate::CreateConstructor(TType(EbtInt, 2), &typeConversionArgs);
        if (!RunAtTheBeginningOfShader(
                compiler, root, CreateTempAssignmentNode(traverser.globalPixelCoord(), expr)))
        {
            return false;
        }
    }
    return true;
}

}  // namespace sh
