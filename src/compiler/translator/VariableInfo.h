//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_VARIABLE_INFO_H_
#define COMPILER_VARIABLE_INFO_H_

#include "compiler/translator/intermediate.h"
#include "common/shadervars.h"

// Traverses intermediate tree to collect all attributes, uniforms, varyings.
class CollectVariables : public TIntermTraverser {
public:
    CollectVariables(std::vector<sh::Attribute> *attribs,
                     std::vector<sh::Uniform> *uniforms,
                     std::vector<sh::Varying> *varyings,
                     ShHashFunction64 hashFunction);

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitAggregate(Visit, TIntermAggregate*);

private:
    std::vector<sh::Attribute> *mAttribs;
    std::vector<sh::Uniform> *mUniforms;
    std::vector<sh::Varying> *mVaryings;

    bool mPointCoordAdded;
    bool mFrontFacingAdded;
    bool mFragCoordAdded;

    ShHashFunction64 mHashFunction;

    template <typename VarT>
    void visitInfoList(const TIntermSequence& sequence, std::vector<VarT> *infoList) const;
};

#endif  // COMPILER_VARIABLE_INFO_H_
