# Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'targets':
    [
        {
            'target_name': 'essl_to_glsl',
            'type': 'executable',
            'dependencies': [ '../src/angle.gyp:translator_static', ],
            'include_dirs': [ '../include', ],
            'sources': [ '<!@(python enumerate_files.py translator -types *.cpp *.h)' ],
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
                    'dependencies': [ '../src/angle.gyp:translator_static', ],
                    'include_dirs':
                    [
                        '../include',
                        '../src',
                    ],
                    'sources':
                    [
                        '<!@(python enumerate_files.py translator -types *.cpp *.h)',
                    ],
                },

                {
                    'target_name': 'es_util',
                    'type': 'static_library',
                    'dependencies':
                    [
                        '../src/angle.gyp:libEGL',
                        '../src/angle.gyp:libGLESv2',
                    ],
                    'include_dirs':
                    [
                        'gles2_book/Common',
                        '../include',
                    ],
                    'sources':
                    [
                        '<!@(python enumerate_files.py gles2_book/Common -types *.c *.h)'
                    ],
                    'direct_dependent_settings':
                    {
                        'include_dirs':
                        [
                            'gles2_book/Common',
                            '../include',
                        ],
                    },
                },

                {
                    'target_name': 'hello_triangle',
                    'type': 'executable',
                    'dependencies': [ 'es_util' ],
                    'sources': [ '<!@(python enumerate_files.py gles2_book/Hello_Triangle -types *.c *.h)' ],
                },

                {
                    'target_name': 'mip_map_2d',
                    'type': 'executable',
                    'dependencies': [ 'es_util' ],
                    'sources': [ '<!@(python enumerate_files.py gles2_book/MipMap2D -types *.c *.h)' ],
                },

                {
                    'target_name': 'multi_texture',
                    'type': 'executable',
                    'dependencies': [ 'es_util' ],
                    'sources': [ '<!@(python enumerate_files.py gles2_book/MultiTexture -types *.c *.h)' ],
                    'copies':
                    [
                        {
                            'destination': '<(PRODUCT_DIR)',
                            'files': [ '<!@(python enumerate_files.py gles2_book/MultiTexture -types *.tga)' ],
                        }
                    ]
                },

                {
                    'target_name': 'particle_system',
                    'type': 'executable',
                    'dependencies': [ 'es_util' ],
                    'sources': [ '<!@(python enumerate_files.py gles2_book/ParticleSystem -types *.c *.h)' ],
                    'copies':
                    [
                        {
                            'destination': '<(PRODUCT_DIR)',
                            'files': [ '<!@(python enumerate_files.py gles2_book/ParticleSystem -types *.tga)' ],
                        }
                    ]
                },

                {
                    'target_name': 'simple_texture_2d',
                    'type': 'executable',
                    'dependencies': [ 'es_util' ],
                    'sources': [ '<!@(python enumerate_files.py gles2_book/Simple_Texture2D -types *.c *.h)' ],
                },

                {
                    'target_name': 'simple_texture_cubemap',
                    'type': 'executable',
                    'dependencies': [ 'es_util' ],
                    'sources': [ '<!@(python enumerate_files.py gles2_book/Simple_TextureCubemap -types *.c *.h)' ],
                },

                {
                    'target_name': 'simple_vertex_shader',
                    'type': 'executable',
                    'dependencies': [ 'es_util' ],
                    'sources': [ '<!@(python enumerate_files.py gles2_book/Simple_VertexShader -types *.c *.h)' ],
                },

                {
                    'target_name': 'stencil_test',
                    'type': 'executable',
                    'dependencies': [ 'es_util' ],
                    'sources': [ '<!@(python enumerate_files.py gles2_book/Stencil_Test -types *.c *.h)' ],
                },

                {
                    'target_name': 'texture_wrap',
                    'type': 'executable',
                    'dependencies': [ 'es_util' ],
                    'sources': [ '<!@(python enumerate_files.py gles2_book/TextureWrap -types *.c *.h)' ],
                },

                {
                    'target_name': 'post_sub_buffer',
                    'type': 'executable',
                    'dependencies': [ 'es_util' ],
                    'sources': [ '<!@(python enumerate_files.py gles2_book/PostSubBuffer -types *.c *.h)' ],
                },
            ],
        }
        ],
    ],
}
