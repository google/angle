# Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'target_defaults':
    {
        'defines':
        [
          'ANGLE_DISABLE_TRACE',
          'ANGLE_COMPILE_OPTIMIZATION_LEVEL=D3DCOMPILE_OPTIMIZATION_LEVEL1',
          'ANGLE_PRELOADED_D3DCOMPILER_MODULE_NAMES={ TEXT("d3dcompiler_46.dll"), TEXT("d3dcompiler_43.dll") }',
        ],
    },

    'conditions':
    [
        ['OS=="win"',
        {
            'targets':
            [
                {
                    'target_name': 'libGLESv2',
                    'type': 'shared_library',
                    'dependencies': [ 'translator', 'copy_compiler_dll' ],
                    'include_dirs':
                    [
                        '.',
                        '../include',
                        'libGLESv2',
                    ],
                    'sources': [ '<!@(python enumerate_files.py common libGLESv2 third_party/murmurhash -types *.cpp *.h *.hlsl *.vs *.ps *.bat *.def libGLESv2.rc)' ],
                    'msvs_disabled_warnings': [ 4267 ],
                    'msvs_settings':
                    {
                        'VCLinkerTool':
                        {
                            'AdditionalDependencies':
                            [
                                'd3d9.lib',
                                'dxguid.lib',
                            ]
                        }
                    },
                },
            ],
        },
        ],
    ],
}
