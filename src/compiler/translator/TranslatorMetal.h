//
// Copyright (c) 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TranslatorMetal:
//   Translator for Metal backend.
//

#ifndef LIBANGLE_RENDERER_METAL_TRANSLATORMETAL_H_
#define LIBANGLE_RENDERER_METAL_TRANSLATORMETAL_H_

#include "compiler/translator/Compiler.h"

namespace sh
{

class TranslatorMetal : public TCompiler
{
  public:
    TranslatorMetal(sh::GLenum type, ShShaderSpec spec);

  protected:
    ANGLE_NO_DISCARD bool translate(TIntermBlock *root,
                                    ShCompileOptions compileOptions,
                                    PerformanceDiagnostics *perfDiagnostics) override;

    bool shouldFlattenPragmaStdglInvariantAll() override;
};

}  // namespace sh

#endif /* LIBANGLE_RENDERER_METAL_TRANSLATORMETAL_H_ */
