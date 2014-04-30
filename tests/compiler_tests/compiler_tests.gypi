# Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'sources':
    [
        '<!@(python <(angle_path)/enumerate_files.py \
          -dirs <(angle_path)/tests/compiler_tests \
          -types *.cpp *.h \
          -excludes <(angle_path)/tests/compiler_tests/compiler_test_main.cpp)'
    ],
}
