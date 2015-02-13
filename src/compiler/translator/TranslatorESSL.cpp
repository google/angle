//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/TranslatorESSL.h"

#include "compiler/translator/EmulatePrecision.h"
#include "compiler/translator/OutputESSL.h"
#include "angle_gl.h"

TranslatorESSL::TranslatorESSL(sh::GLenum type, ShShaderSpec spec)
    : TCompiler(type, spec, SH_ESSL_OUTPUT) {
}

void TranslatorESSL::translate(TIntermNode *root, int) {
    TInfoSinkBase& sink = getInfoSink().obj;

    writePragma();

    // Write built-in extension behaviors.
    writeExtensionBehavior();

    bool precisionEmulation = getResources().WEBGL_debug_shader_precision && getPragma().debugShaderPrecision;

    if (precisionEmulation)
    {
        EmulatePrecision emulatePrecision;
        root->traverse(&emulatePrecision);
        emulatePrecision.updateTree();
        emulatePrecision.writeEmulationHelpers(sink, SH_ESSL_OUTPUT);
    }

    // Write emulated built-in functions if needed.
    getBuiltInFunctionEmulator().OutputEmulatedFunctionDefinition(
        sink, getShaderType() == GL_FRAGMENT_SHADER);

    // Write array bounds clamping emulation if needed.
    getArrayBoundsClamper().OutputClampingFunctionDefinition(sink);

    // Write translated shader.
    TOutputESSL outputESSL(sink, getArrayIndexClampingStrategy(), getHashFunction(), getNameMap(), getSymbolTable(), getShaderVersion(), precisionEmulation);
    root->traverse(&outputESSL);
}

void TranslatorESSL::writeExtensionBehavior() {
    TInfoSinkBase& sink = getInfoSink().obj;
    const TExtensionBehavior& extBehavior = getExtensionBehavior();
    for (TExtensionBehavior::const_iterator iter = extBehavior.begin();
         iter != extBehavior.end(); ++iter) {
        if (iter->second != EBhUndefined) {
            if (getResources().NV_shader_framebuffer_fetch && iter->first == "GL_EXT_shader_framebuffer_fetch") {
                sink << "#extension GL_NV_shader_framebuffer_fetch : "
                     << getBehaviorString(iter->second) << "\n";
            } else if (getResources().NV_draw_buffers && iter->first == "GL_EXT_draw_buffers") {
                sink << "#extension GL_NV_draw_buffers : "
                     << getBehaviorString(iter->second) << "\n";
            } else {
                sink << "#extension " << iter->first << " : "
                     << getBehaviorString(iter->second) << "\n";
            }
        }
    }
}
