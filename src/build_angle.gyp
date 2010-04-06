# Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    # Source files common between translator_glsl and translator_hlsl
    'translator_common_sources': [
      'common/angleutils.h',
      'common/debug.cpp',
      'common/debug.h',
      'compiler/BaseTypes.h',
      'compiler/Common.h',
      'compiler/ConstantUnion.h',
      'compiler/InfoSink.cpp',
      'compiler/InfoSink.h',
      'compiler/Initialize.cpp',
      'compiler/Initialize.h',
      'compiler/InitializeDll.cpp',
      'compiler/InitializeDll.h',
      'compiler/InitializeGlobals.h',
      'compiler/InitializeParseContext.h',
      'compiler/Intermediate.cpp',
      'compiler/intermediate.h',
      'compiler/intermOut.cpp',
      'compiler/IntermTraverse.cpp',
      'compiler/Link.cpp',
      'compiler/localintermediate.h',
      'compiler/MMap.h',
      'compiler/osinclude.h',
      'compiler/ossource.cpp',
      'compiler/parseConst.cpp',
      'compiler/ParseHelper.cpp',
      'compiler/ParseHelper.h',
      'compiler/PoolAlloc.cpp',
      'compiler/PoolAlloc.h',
      'compiler/QualifierAlive.cpp',
      'compiler/QualifierAlive.h',
      'compiler/RemoveTree.cpp',
      'compiler/RemoveTree.h',
      'compiler/ShaderLang.cpp',
      'compiler/ShHandle.h',
      'compiler/SymbolTable.cpp',
      'compiler/SymbolTable.h',
      'compiler/Types.h',
      'compiler/unistd.h',
      'compiler/preprocessor/atom.c',
      'compiler/preprocessor/atom.h',
      'compiler/preprocessor/compile.h',
      'compiler/preprocessor/cpp.c',
      'compiler/preprocessor/cpp.h',
      'compiler/preprocessor/cppstruct.c',
      'compiler/preprocessor/memory.c',
      'compiler/preprocessor/memory.h',
      'compiler/preprocessor/parser.h',
      'compiler/preprocessor/preprocess.h',
      'compiler/preprocessor/scanner.c',
      'compiler/preprocessor/scanner.h',
      'compiler/preprocessor/slglobals.h',
      'compiler/preprocessor/symbols.c',
      'compiler/preprocessor/symbols.h',
      'compiler/preprocessor/tokens.c',
      'compiler/preprocessor/tokens.h',
      # Generated files
      'compiler/Gen_glslang.cpp',
      'compiler/Gen_glslang_tab.cpp',
      'compiler/glslang_tab.h',
    ],
  },
  'targets': [
    {
      'target_name': 'translator_glsl',
      'type': '<(library)',
      'include_dirs': [
        'compiler',
        '.',
        '../include',
      ],
      'sources': [
        '<@(translator_common_sources)',
        'compiler/CodeGenGLSL.cpp',
        'compiler/OutputGLSL.cpp',
        'compiler/OutputGLSL.h',
        'compiler/TranslatorGLSL.cpp',
        'compiler/TranslatorGLSL.h',
      ],
      'actions': [
        {
          'action_name': 'flex_glslang',
          'inputs': ['compiler/glslang.l'],
          'outputs': ['compiler/Gen_glslang.cpp'],
          'action': [
            'flex',
            '--noline',
            '--outfile=<(_outputs)',
            '<(_inputs)',
          ],
        },
        {
          'action_name': 'bison_glslang',
          'inputs': ['compiler/glslang.y'],
          'outputs': ['compiler/Gen_glslang_tab.cpp'],
          'action': [
            'bison',
            '--no-lines',
            '--defines=compiler/glslang_tab.h',
            '--skeleton=yacc.c',
            '--output=<(_outputs)',
            '<(_inputs)',
          ],
        },
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
