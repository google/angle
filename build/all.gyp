# Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'targets':
    [
        {
            'target_name': 'All',
            'type': 'none',
            'dependencies':
            [
                '../src/angle.gyp:*',
                '../samples/samples.gyp:*',
                '../tests/tests.gyp:*',
            ],
        },
    ],
}
