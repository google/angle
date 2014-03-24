# Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'conditions':
    [
        ['OS=="win"',
        {
            'targets':
            [
                {
                    'target_name': 'libEGL',
                    'type': 'shared_library',
                    'dependencies': [ 'libGLESv2', 'commit_id' ],
                    'include_dirs':
                    [
                        '.',
                        '../include',
                        'libGLESv2',
                        '<(SHARED_INTERMEDIATE_DIR)',
                    ],
                    'sources':
                    [
                        '<!@(python <(angle_path)/enumerate_files.py \
                             -dirs common libEGL ../include \
                             -types *.cpp *.h *.def *.rc)',
                    ],
                    # TODO(jschuh): http://crbug.com/167187 size_t -> int
                    'msvs_disabled_warnings': [ 4267 ],
                    'msvs_settings':
                    {
                        'VCLinkerTool':
                        {
                            'AdditionalDependencies':
                            [
                                'd3d9.lib',
                            ],
                        },
                    },
                    'configurations':
                    {
                        'Debug':
                        {
                            'defines':
                            [
                                'ANGLE_ENABLE_PERF',
                            ],
                        },
                    },
                },
            ],
        },
        ],
    ],
}
