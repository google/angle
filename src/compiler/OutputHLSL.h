//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_OUTPUTHLSL_H_
#define COMPILER_OUTPUTHLSL_H_

#include "compiler/intermediate.h"
#include "compiler/ParseHelper.h"

namespace sh
{
class UnfoldSelect;

class OutputHLSL : public TIntermTraverser
{
  public:
    explicit OutputHLSL(TParseContext &context);
    ~OutputHLSL();

    void output();

    TInfoSinkBase &getBodyStream();

    static TString qualifierString(TQualifier qualifier);
    static TString typeString(const TType &type);
    static TString arrayString(const TType &type);
    static TString initializer(const TType &type);
    static TString decorate(const TString &string);   // Prepend an underscore to avoid naming clashes

  protected:
    void header();
    void footer();

    // Visit AST nodes and output their code to the body stream
    void visitSymbol(TIntermSymbol*);
    void visitConstantUnion(TIntermConstantUnion*);
    bool visitBinary(Visit visit, TIntermBinary*);
    bool visitUnary(Visit visit, TIntermUnary*);
    bool visitSelection(Visit visit, TIntermSelection*);
    bool visitAggregate(Visit visit, TIntermAggregate*);
    bool visitLoop(Visit visit, TIntermLoop*);
    bool visitBranch(Visit visit, TIntermBranch*);

    bool isSingleStatement(TIntermNode *node);
    bool handleExcessiveLoop(TIntermLoop *node);
    void outputTriplet(Visit visit, const TString &preString, const TString &inString, const TString &postString);
    TString argumentString(const TIntermSymbol *symbol);
    int vectorSize(const TType &type) const;

    void addConstructor(const TType &type, const TString &name, const TIntermSequence *parameters);
    const ConstantUnion *writeConstantUnion(const TType &type, const ConstantUnion *constUnion);

    TParseContext &mContext;
    UnfoldSelect *mUnfoldSelect;
    bool mInsideFunction;

    // Output streams
    TInfoSinkBase mHeader;
    TInfoSinkBase mBody;
    TInfoSinkBase mFooter;

    std::set<std::string> mReferencedUniforms;
    std::set<std::string> mReferencedAttributes;
    std::set<std::string> mReferencedVaryings;

    // Parameters determining what goes in the header output
    bool mUsesTexture2D;
    bool mUsesTexture2D_bias;
    bool mUsesTexture2DProj;
    bool mUsesTexture2DProj_bias;
    bool mUsesTextureCube;
    bool mUsesTextureCube_bias;
    bool mUsesFaceforward1;
    bool mUsesFaceforward2;
    bool mUsesFaceforward3;
    bool mUsesFaceforward4;
    bool mUsesEqualMat2;
    bool mUsesEqualMat3;
    bool mUsesEqualMat4;
    bool mUsesEqualVec2;
    bool mUsesEqualVec3;
    bool mUsesEqualVec4;
    bool mUsesEqualIVec2;
    bool mUsesEqualIVec3;
    bool mUsesEqualIVec4;
    bool mUsesEqualBVec2;
    bool mUsesEqualBVec3;
    bool mUsesEqualBVec4;
    bool mUsesAtan2;

    struct Constructor   // Describes a constructor signature
    {
        TType type;
        TString name;

        typedef std::vector<TType> ParameterArray;
        ParameterArray parameters;
    };

    struct CompareConstructor
    {
        bool operator()(const Constructor &x, const Constructor &y) const;
    };

    typedef std::set<Constructor, CompareConstructor> ConstructorSet;
    ConstructorSet mConstructors;

    typedef std::list<TType> StructureArray;
    StructureArray mStructures;

    int mArgumentIndex;   // For creating unique argument names
};
}

#endif   // COMPILER_OUTPUTHLSL_H_
