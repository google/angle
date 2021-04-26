#!/usr/bin/env python
# Copyright 2021 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to generate the test spec JSON files. Calls Chromium's generate_buildbot_json.
"""

import os
import sys
import subprocess

d = os.path.dirname
THIS_DIR = d(os.path.abspath(__file__))
TESTING_DIR = os.path.join(d(d(THIS_DIR)), 'testing', 'buildbot')
sys.path.insert(0, TESTING_DIR)

import generate_buildbot_json

if __name__ == '__main__':  # pragma: no cover
    # Append the path to the isolate map.
    override_args = sys.argv[1:] + ['--pyl-files-dir', THIS_DIR]
    parsed_args = generate_buildbot_json.BBJSONGenerator.parse_args(override_args)
    generator = generate_buildbot_json.BBJSONGenerator(parsed_args)
    sys.exit(generator.main())
