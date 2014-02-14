# Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'variables':
    {
        'angle_code': 1,
        'angle_post_build_script%': 0,
        'angle_relative_src_path%': '',
    },
    'includes':
    [
        'compiler.gypi',
        'libGLESv2.gypi',
        'libEGL.gypi'
    ],
    'conditions':
    [
        [
            'OS=="win"',
            {
                'target_defaults':
                {
                   'msvs_cygwin_shell': 0,
                },
                'targets':
                [
                    {
                        'target_name': 'commit_id',
                        'type': 'none',
                        'actions':
                        [
                            {
                                'action_name': 'Generate Commit ID Header',
                                'message': 'Generating commit ID header...',
                                'inputs': [ 'commit_id.bat' ],
                                'outputs': ['<(SHARED_INTERMEDIATE_DIR)/commit.h'],
                                'action': ['<(angle_relative_src_path)commit_id.bat', '<(SHARED_INTERMEDIATE_DIR)'],
                            }
                        ] #actions
                    },
                    {
                        'target_name': 'copy_compiler_dll',
                        'type': 'none',
                        'sources': [ 'copy_compiler_dll.bat' ],
                        'actions':
                        [
                            {
                                'msvs_cygwin_shell': 0,
                                'action_name': 'copy_dll',
                                'message': 'Copying D3D Compiler DLL...',
                                'inputs': [ 'copy_compiler_dll.bat' ],
                                'outputs': [ '<(PRODUCT_DIR)/D3DCompiler_46.dll' ],
                                'action': ["<(angle_relative_src_path)copy_compiler_dll.bat", "$(PlatformName)", "<(windows_sdk_path)", "<(PRODUCT_DIR)" ],
                            }
                        ] #actions
                    },
                ] # targets
            },
        ],
        [
            'angle_post_build_script!=0 and OS=="win"',
            {
                'target_defaults':
                {
                   'msvs_cygwin_shell': 0,
                },
                'targets':
                [
                    {
                        'target_name': 'post_build',
                        'type': 'none',
                        'dependencies': [ 'libGLESv2', 'libEGL' ],
                        'actions':
                        [
                            {
                                'action_name': 'ANGLE Post-Build Script',
                                'message': 'Running <(angle_post_build_script)...',
                                'inputs': [ '<(angle_post_build_script)', '<!@(["python", "<(angle_post_build_script)", "inputs", "<(CONFIGURATION_NAME)", "$(PlatformName)", "<(PRODUCT_DIR)"])' ],
                                'outputs': [ '<!@(python <(angle_post_build_script) outputs "<(CONFIGURATION_NAME)" "$(PlatformName)" "<(PRODUCT_DIR)")' ],
                                'action': ['python', '<(angle_post_build_script)', 'run', '<(CONFIGURATION_NAME)', '$(PlatformName)', '<(PRODUCT_DIR)'],
                            }
                        ] #actions
                    }
                ] # targets
            }
        ],
        [
            'OS!="win"',
            {
                'targets':
                [
                    {
                        'target_name': 'commit_id',
                        'type': 'none',
                        'actions':
                        [
                            {
                                'action_name': 'Generate Commit ID Header',
                                'message': 'Generating commit ID header...',
                                'inputs': [ 'commit_id.py' ],
                                'outputs': ['<(SHARED_INTERMEDIATE_DIR)/commit.h'],
                                'action': ['python', '<(angle_relative_src_path)commit_id.py', '<(SHARED_INTERMEDIATE_DIR)/commit.h'],
                            }
                        ] #actions
                    },
                ]
            }
        ],
    ] # conditions
}
