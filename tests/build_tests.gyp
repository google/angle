# Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'gtest',
      'type': 'static_library',
      'include_dirs': [
        '../third_party/googletest',
        '../third_party/googletest/include',
      ],
      'sources': [
        '../third_party/googletest/src/gtest-all.cc',
      ],
    },
    {
      'target_name': 'gmock',
      'type': 'static_library',
      'include_dirs': [
        '../third_party/googlemock',
        '../third_party/googlemock/include',
        '../third_party/googletest/include',
      ],
      'sources': [
        '../third_party/googlemock/src/gmock-all.cc',
      ],
    },
    {
      'target_name': 'preprocessor_tests',
      'type': 'executable',
      'dependencies': [
        '../src/build_angle.gyp:preprocessor',
        'gtest',
        'gmock',
      ],
      'variables': {
        'ANGLE_DIR': '..',
      },
      'includes': [
        'preprocessor_tests/preprocessor_tests.gypi',
      ],
      'include_dirs': [
        '../src/compiler/preprocessor',
        '../third_party/googletest/include',
        '../third_party/googlemock/include',
      ],
      'sources': [
        '../third_party/googlemock/src/gmock_main.cc',
      ],
    },
    {
      'target_name': 'compiler_tests',
      'type': 'executable',
      'dependencies': [
        '../src/build_angle.gyp:translator_static',
        'gtest',
        'gmock',
      ],
      'variables': {
        'ANGLE_DIR': '..',
      },
      'includes': [
        'compiler_tests/compiler_tests.gypi',
      ],
      'include_dirs': [
        '../include',
        '../src',
        '../third_party/googletest/include',
        '../third_party/googlemock/include',
      ],
      'sources': [
        'compiler_tests/compiler_test_main.cpp',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
