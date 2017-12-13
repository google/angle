#!/usr/bin/python2
#
# Copyright 2017 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# remove_file_if_exists.py:
#   This special action is needed to remove generated headers.
#   Otherwise ANGLE will pick up the old file(s) and the build will fail.
#

import sys
import os

if len(sys.argv) < 3:
    print("Usage: " + sys.argv[0] + " <remove_file> <stamp_file>")

remove_file = sys.argv[1]
if os.path.isfile(remove_file):
    os.remove(remove_file)

# touch a dummy file to keep a timestamp
with open(sys.argv[2], "w") as f:
    f.write("blah")
    f.close()
