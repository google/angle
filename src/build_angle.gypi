# Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'angle_code': 1,
  },
  'target_defaults': {
    'defines': [
      'ANGLE_DISABLE_TRACE',
      'ANGLE_COMPILE_OPTIMIZATION_LEVEL=D3DCOMPILE_OPTIMIZATION_LEVEL1',
      'ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES={ TEXT("d3dcompiler_46.dll"), TEXT("d3dcompiler_43.dll") }',
    ],
  },
  'targets': [
    {
      'target_name': 'preprocessor',
      'type': 'static_library',
      'include_dirs': [
      ],
      'sources': [
        'compiler/preprocessor/DiagnosticsBase.cpp',
        'compiler/preprocessor/DiagnosticsBase.h',
        'compiler/preprocessor/DirectiveHandlerBase.cpp',
        'compiler/preprocessor/DirectiveHandlerBase.h',
        'compiler/preprocessor/DirectiveParser.cpp',
        'compiler/preprocessor/DirectiveParser.h',
        'compiler/preprocessor/ExpressionParser.cpp',
        'compiler/preprocessor/ExpressionParser.h',
        'compiler/preprocessor/Input.cpp',
        'compiler/preprocessor/Input.h',
        'compiler/preprocessor/length_limits.h',
        'compiler/preprocessor/Lexer.cpp',
        'compiler/preprocessor/Lexer.h',
        'compiler/preprocessor/Macro.cpp',
        'compiler/preprocessor/Macro.h',
        'compiler/preprocessor/MacroExpander.cpp',
        'compiler/preprocessor/MacroExpander.h',
        'compiler/preprocessor/numeric_lex.h',
        'compiler/preprocessor/pp_utils.h',
        'compiler/preprocessor/Preprocessor.cpp',
        'compiler/preprocessor/Preprocessor.h',
        'compiler/preprocessor/SourceLocation.h',
        'compiler/preprocessor/Token.cpp',
        'compiler/preprocessor/Token.h',
        'compiler/preprocessor/Tokenizer.cpp',
        'compiler/preprocessor/Tokenizer.h',
      ],
      # TODO(jschuh): http://crbug.com/167187
      'msvs_disabled_warnings': [
        4267,
      ],      
    },
    {
      'target_name': 'translator_common',
      'type': 'static_library',
      'dependencies': ['preprocessor'],
      'include_dirs': [
        '.',
        '../include',
      ],
      'defines': [
        'COMPILER_IMPLEMENTATION',
      ],
      'sources': [
        'compiler/BaseTypes.h',
        'compiler/BlockLayoutEncoder.cpp',
        'compiler/BlockLayoutEncoder.h',
        'compiler/BuiltInFunctionEmulator.cpp',
        'compiler/BuiltInFunctionEmulator.h',
        'compiler/Common.h',
        'compiler/Compiler.cpp',
        'compiler/ConstantUnion.h',
        'compiler/debug.cpp',
        'compiler/debug.h',
        'compiler/DetectCallDepth.cpp',
        'compiler/DetectCallDepth.h',
        'compiler/Diagnostics.h',
        'compiler/Diagnostics.cpp',
        'compiler/DirectiveHandler.h',
        'compiler/DirectiveHandler.cpp',
        'compiler/ExtensionBehavior.h',
        'compiler/ForLoopUnroll.cpp',
        'compiler/ForLoopUnroll.h',
        'compiler/glslang.h',
        'compiler/glslang_lex.cpp',
        'compiler/glslang_tab.cpp',
        'compiler/glslang_tab.h',
        'compiler/HashNames.h',
        'compiler/InfoSink.cpp',
        'compiler/InfoSink.h',
        'compiler/Initialize.cpp',
        'compiler/Initialize.h',
        'compiler/InitializeDll.cpp',
        'compiler/InitializeDll.h',
        'compiler/InitializeGlobals.h',
        'compiler/InitializeGLPosition.cpp',
        'compiler/InitializeGLPosition.h',
        'compiler/InitializeParseContext.cpp',
        'compiler/InitializeParseContext.h',
        'compiler/Intermediate.cpp',
        'compiler/intermediate.h',
        'compiler/intermOut.cpp',
        'compiler/IntermTraverse.cpp',
        'compiler/localintermediate.h',
        'compiler/MapLongVariableNames.cpp',
        'compiler/MapLongVariableNames.h',
        'compiler/MMap.h',
        'compiler/osinclude.h',
        'compiler/parseConst.cpp',
        'compiler/ParseHelper.cpp',
        'compiler/ParseHelper.h',
        'compiler/PoolAlloc.cpp',
        'compiler/PoolAlloc.h',
        'compiler/QualifierAlive.cpp',
        'compiler/QualifierAlive.h',
        'compiler/RemoveTree.cpp',
        'compiler/RemoveTree.h',
        'compiler/RenameFunction.h',
        'compiler/ShaderVariable.cpp',
        'compiler/ShaderVariable.h',
        'compiler/ShHandle.h',
        'compiler/SymbolTable.cpp',
        'compiler/SymbolTable.h',
        'compiler/Types.h',
        'compiler/util.cpp',
        'compiler/util.h',
        'compiler/ValidateLimitations.cpp',
        'compiler/ValidateLimitations.h',
        'compiler/ValidateOutput.cpp',
        'compiler/ValidateOutput.h',
        'compiler/VariableInfo.cpp',
        'compiler/VariableInfo.h',
        'compiler/VariablePacker.cpp',
        'compiler/VariablePacker.h',
        # Dependency graph
        'compiler/depgraph/DependencyGraph.cpp',
        'compiler/depgraph/DependencyGraph.h',
        'compiler/depgraph/DependencyGraphBuilder.cpp',
        'compiler/depgraph/DependencyGraphBuilder.h',
        'compiler/depgraph/DependencyGraphOutput.cpp',
        'compiler/depgraph/DependencyGraphOutput.h',
        'compiler/depgraph/DependencyGraphTraverse.cpp',
        # Timing restrictions
        'compiler/timing/RestrictFragmentShaderTiming.cpp',
        'compiler/timing/RestrictFragmentShaderTiming.h',
        'compiler/timing/RestrictVertexShaderTiming.cpp',
        'compiler/timing/RestrictVertexShaderTiming.h',
        'third_party/compiler/ArrayBoundsClamper.cpp',
        'third_party/compiler/ArrayBoundsClamper.h',
      ],
      'conditions': [
        ['OS=="win"', {
          # TODO(jschuh): http://crbug.com/167187 size_t -> int
          'msvs_disabled_warnings': [ 4267 ],
          'sources': ['compiler/ossource_win.cpp'],
        }, { # else: posix
          'sources': ['compiler/ossource_posix.cpp'],
        }],
      ],
    },
    {
      'target_name': 'translator_glsl',
      'type': '<(component)',
      'dependencies': ['translator_common'],
      'include_dirs': [
        '.',
        '../include',
      ],
      'defines': [
        'COMPILER_IMPLEMENTATION',
      ],
      'sources': [
        'compiler/CodeGenGLSL.cpp',
        'compiler/OutputESSL.cpp',
        'compiler/OutputESSL.h',        
        'compiler/OutputGLSLBase.cpp',
        'compiler/OutputGLSLBase.h',
        'compiler/OutputGLSL.cpp',
        'compiler/OutputGLSL.h',
        'compiler/ShaderLang.cpp',
        'compiler/TranslatorESSL.cpp',
        'compiler/TranslatorESSL.h',
        'compiler/TranslatorGLSL.cpp',
        'compiler/TranslatorGLSL.h',
        'compiler/VersionGLSL.cpp',
        'compiler/VersionGLSL.h',
      ],
      # TODO(jschuh): http://crbug.com/167187 size_t -> int
      'msvs_disabled_warnings': [ 4267 ],
    },
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'translator_hlsl',
          'type': '<(component)',
          'dependencies': ['translator_common'],
          'include_dirs': [
            '.',
            '../include',
          ],
          'defines': [
            'COMPILER_IMPLEMENTATION',
          ],
          'sources': [
            'compiler/ShaderLang.cpp',
            'compiler/DetectDiscontinuity.cpp',
            'compiler/DetectDiscontinuity.h',
            'compiler/CodeGenHLSL.cpp',
            'compiler/FlagStd140Structs.cpp',
            'compiler/FlagStd140Structs.h',
            'compiler/HLSLLayoutEncoder.cpp',
            'compiler/HLSLLayoutEncoder.h',
            'compiler/OutputHLSL.cpp',
            'compiler/OutputHLSL.h',
            'compiler/TranslatorHLSL.cpp',
            'compiler/TranslatorHLSL.h',
            'compiler/UnfoldShortCircuit.cpp',
            'compiler/UnfoldShortCircuit.h',
            'compiler/SearchSymbol.cpp',
            'compiler/SearchSymbol.h',
          ],
          # TODO(jschuh): http://crbug.com/167187 size_t -> int
          'msvs_disabled_warnings': [ 4267 ],
        },
        {
          'target_name': 'libGLESv2',
          'type': 'shared_library',
          'dependencies': ['translator_hlsl'],
          'include_dirs': [
            '.',
            '../include',
            'libGLESv2',
          ],
          'sources': [
            '<!@(python enumerate_files.py common libGLESv2 third_party/murmurhash -types *.cpp *.h *.hlsl *.vs *.ps *.bat *.def)',
          ],
          # TODO(jschuh): http://crbug.com/167187 size_t -> int
          'msvs_disabled_warnings': [ 4267, 4996 ],
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalDependencies': [
                'd3d9.lib',
                'dxguid.lib',
              ],
            }
          },
        },
        {
          'target_name': 'libEGL',
          'type': 'shared_library',
          'dependencies': ['libGLESv2'],
          'include_dirs': [
            '.',
            '../include',
            'libGLESv2',
          ],
          'sources': [
            '<!@(python enumerate_files.py common libEGL -types *.cpp *.h *.def)',
          ],
          # TODO(jschuh): http://crbug.com/167187 size_t -> int
          'msvs_disabled_warnings': [ 4267 ],
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalDependencies': [
                'd3d9.lib',
              ],
            }
          },
        },
      ],
    }],
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
# Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
