# Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'all',
      'type': 'none',
      'dependencies': [
        '../src/angle.gyp:*',
      ],
      'conditions': [
        ['angle_build_samples==1', {
          'dependencies': [
            '../samples/samples.gyp:*',
          ],
        }],
        ['angle_build_tests==1', {
          'dependencies': [
            '../tests/tests.gyp:*',
          ],
        }],
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
