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
  'includes': [
    'compiler.gypi',
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'libGLESv2',
          'type': 'shared_library',
          'dependencies': ['translator'],
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
                '%(AdditionalDependencies)',
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
                '%(AdditionalDependencies)',
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
