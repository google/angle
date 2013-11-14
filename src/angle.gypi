# Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'variables':
    {
        'angle_code': 1,
        'angle_post_build_script%': 0,
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
                                'inputs': [ '<(angle_post_build_script)', '<!@(python <(angle_post_build_script) inputs)' ],
                                'outputs': [ '<!@(python <(angle_post_build_script) outputs)' ],
                                'action': ['python', '<(angle_post_build_script)', 'run', '<(CONFIGURATION_NAME)', '$(Platform)', '<(PRODUCT_DIR)'],
                            }
                        ] #actions
                    }
                ] # targets
            }
        ]
    ] # conditions
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
# Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
