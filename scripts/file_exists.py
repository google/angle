#!/usr/bin/python3
# Copyright 2019 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Simple helper for use in 'gn' files to check if a file exists.

from __future__ import print_function

import os, shutil, sys


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <path>")
        sys.exit(1)

    if os.path.exists(sys.argv[1]):
        print("true")
    else:
        print("false")
    sys.exit(0)


if __name__ == '__main__':
    main()
