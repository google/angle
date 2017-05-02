#!/usr/bin/python
#
# Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# touch_stamp.py: similar to the Unix touch command, used to manual make
#   stamp files when building with GYP

import os
import sys

stamp_name = sys.argv[1]

with open(stamp_name, 'a'):
    os.utime(stamp_name, None)
