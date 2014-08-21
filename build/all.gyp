# Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'targets':
    [
        {
            'target_name': 'All',
            'type': 'none',
            'dependencies': [ '../src/angle.gyp:*', ],
            'conditions':
            [
                ['angle_build_samples==1',
                {
                    'dependencies': [ '../samples/samples.gyp:*', ],
                }],
                ['angle_build_tests==1',
                {
                    'dependencies': [ '../tests/tests.gyp:*', ],
                }],
                ['angle_build_samples==1 or angle_build_tests==1',
                {
                    'dependencies': [ '../util/util.gyp:*', ],
                }],
            ],
        },
    ],
}
