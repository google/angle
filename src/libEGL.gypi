# Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    # Everything below this is duplicated in the GN build. If you change
    # anything also change angle/BUILD.gn
    'conditions':
    [
        ['OS=="win"',
        {
            'targets':
            [
                {
                    'target_name': 'libEGL',
                    'type': 'shared_library',
                    'dependencies': [ 'libGLESv2', 'libANGLE', ],
                    'includes': [ '../build/common_defines.gypi', ],
                    'include_dirs':
                    [
                        '.',
                        '../include',
                    ],
                    'sources':
                    [
                        'common/angleutils.cpp',
                        'common/angleutils.h',
                        'common/debug.cpp',
                        'common/debug.h',
                        'common/event_tracer.cpp',
                        'common/event_tracer.h',
                        'common/tls.cpp',
                        'common/tls.h',
                        'libEGL/libEGL.cpp',
                        'libEGL/libEGL.def',
                        'libEGL/libEGL.rc',
                        'libEGL/main.cpp',
                        'libEGL/main.h',
                        'libEGL/resource.h',
                    ],
                    'defines':
                    [
                        'LIBEGL_IMPLEMENTATION',
                    ],
                    'conditions':
                    [
                        ['angle_build_winrt==1',
                        {
                            'msvs_enable_winrt' : '1',
                            'msvs_requires_importlibrary' : 'true',
                            'msvs_settings':
                            {
                                'VCLinkerTool':
                                {
                                    'EnableCOMDATFolding': '1',
                                    'OptimizeReferences': '1',
                                }
                            },
                        }],
                        ['angle_build_winphone==1',
                        {
                            'msvs_enable_winphone' : '1',
                        }],
                    ],
                },
            ],
        },
        ],
    ],
}
