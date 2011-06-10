//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/TranslatorESSL.h"

#include "compiler/OutputESSL.h"

TranslatorESSL::TranslatorESSL(ShShaderType type, ShShaderSpec spec)
    : TCompiler(type, spec) {
}

void TranslatorESSL::translate(TIntermNode* root) {
    TInfoSinkBase& sink = getInfoSink().obj;

    // Write built-in extension behaviors.
    writeExtensionBehavior();

    // FIXME(zmo): no need to emit default precision if all variables emit
    // their own precision.
    // http://code.google.com/p/angleproject/issues/detail?id=168
    if (this->getShaderType() == SH_FRAGMENT_SHADER) {
        // Write default float precision.
        sink << "#if defined(GL_FRAGMENT_PRECISION_HIGH)\n"
             << "precision highp float;\n"
             << "#else\n"
             << "precision mediump float;\n"
             << "#endif\n";
    }

    // Write translated shader.
    TOutputESSL outputESSL(sink);
    root->traverse(&outputESSL);
}

void TranslatorESSL::writeExtensionBehavior() {
    TInfoSinkBase& sink = getInfoSink().obj;
    const TExtensionBehavior& extensionBehavior = getExtensionBehavior();
    for (TExtensionBehavior::const_iterator iter = extensionBehavior.begin();
         iter != extensionBehavior.end(); ++iter) {
        sink << "#extension " << iter->first << " : "
             << getBehaviorString(iter->second) << "\n";
    }
}
