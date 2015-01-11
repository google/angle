# Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'targets':
    [
        {
            'target_name': 'essl_to_glsl',
            'type': 'executable',
            'includes': [ '../build/common_defines.gypi', ],
            'dependencies': [ '../src/angle.gyp:translator_static', ],
            'include_dirs': [ '../include', ],
            'sources': [ '<!@(python <(angle_path)/enumerate_files.py translator -types *.cpp *.h)' ],
        },
    ],
    'conditions':
    [
        ['OS=="win"',
        {
            'targets':
            [
                {
                    'target_name': 'essl_to_hlsl',
                    'type': 'executable',
                    'includes': [ '../build/common_defines.gypi', ],
                    'dependencies': [ '../src/angle.gyp:translator_static', ],
                    'include_dirs':
                    [
                        '../include',
                        '../src',
                    ],
                    'sources':
                    [
                        '<!@(python <(angle_path)/enumerate_files.py translator -types *.cpp *.h)',
                    ],
                },

                {
                    'target_name': 'dds_to_header',
                    'type': 'executable',
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources':
                    [
                        '<!@(python <(angle_path)/enumerate_files.py dds_to_header -types *.cpp)',
                    ],
                },

                {
                    'target_name': 'sample_util',
                    'type': 'static_library',
                    'includes': [ '../build/common_defines.gypi', ],
                    'dependencies':
                    [
                        '../src/angle.gyp:libEGL',
                        '../src/angle.gyp:libGLESv2',
                        '../util/util.gyp:angle_util',
                    ],
                    'include_dirs':
                    [
                        '../include',
                        'angle/sample_util',
                    ],
                    'sources':
                    [
                        '<!@(python <(angle_path)/enumerate_files.py angle/sample_util -types *.cpp *.h)'
                    ],
                    'msvs_disabled_warnings': [ 4201 ],
                    'direct_dependent_settings':
                    {
                        'msvs_disabled_warnings': [ 4201 ],
                        'include_dirs':
                        [
                            '../include',
                            'angle/sample_util',
                            '../util',
                        ],
                    },
                },

                {
                    'target_name': 'hello_triangle',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/hello_triangle -types *.cpp *.h)' ],
                },

                {
                    'target_name': 'mip_map_2d',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/mip_map_2d -types *.cpp *.h)' ],
                },

                {
                    'target_name': 'multi_texture',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/multi_texture -types *.cpp *.h)' ],
                    'copies':
                    [
                        {
                            'destination': '<(PRODUCT_DIR)',
                            'files': [ '<!@(python <(angle_path)/enumerate_files.py angle/multi_texture -types *.tga)' ],
                        },
                    ]
                },

                {
                    'target_name': 'particle_system',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/particle_system -types *.cpp *.h)' ],
                    'copies':
                    [
                        {
                            'destination': '<(PRODUCT_DIR)',
                            'files': [ '<!@(python <(angle_path)/enumerate_files.py angle/particle_system -types *.tga)' ],
                        }
                    ]
                },

                {
                    'target_name': 'simple_instancing',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/simple_instancing -types *.cpp *.h)' ],
                },

                {
                    'target_name': 'tri_fan_microbench',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/tri_fan_microbench -types *.cpp *.h *.glsl)' ],
                },

                {
                    'target_name': 'tex_redef_microbench',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/tex_redef_microbench -types *.cpp *.h *.glsl)' ],
                },

                {
                    'target_name': 'multiple_draw_buffers',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/multiple_draw_buffers -types *.cpp *.h *.glsl)' ],
                    'copies':
                    [
                        {
                            'destination': '<(PRODUCT_DIR)',
                            'files': [ '<!@(python <(angle_path)/enumerate_files.py angle/multiple_draw_buffers -types *.glsl)' ],
                        }
                    ]
                },

                {
                    'target_name': 'simple_texture_2d',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/simple_texture_2d -types *.cpp *.h)' ],
                },

                {
                    'target_name': 'simple_texture_cubemap',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/simple_texture_cubemap -types *.cpp *.h)' ],
                },

                {
                    'target_name': 'simple_vertex_shader',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/simple_vertex_shader -types *.cpp *.h)' ],
                },

                {
                    'target_name': 'stencil_operations',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/stencil_operations -types *.cpp *.h)' ],
                },

                {
                    'target_name': 'texture_wrap',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/texture_wrap -types *.cpp *.h)' ],
                },

                {
                    'target_name': 'post_sub_buffer',
                    'type': 'executable',
                    'dependencies': [ 'sample_util' ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'sources': [ '<!@(python <(angle_path)/enumerate_files.py angle/post_sub_buffer -types *.cpp *.h)' ],
                },
            ],
        }
        ],
    ],
}
