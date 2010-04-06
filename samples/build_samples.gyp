# Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'essl_to_glsl',
      'type': 'executable',
      'dependencies': [
        '../src/build_angle.gyp:translator_glsl',
      ],
      'include_dirs': [
        '../include',
      ],
      'sources': [
        'translator/translator.cpp',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
