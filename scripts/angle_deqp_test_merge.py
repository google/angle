#!/usr/bin/env python
#
# Copyright 2021 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
""" Merges dEQP sharded test results in the ANGLE testing infrastucture."""

import os
import sys


def _AddToPathIfNeeded(path):
    if path not in sys.path:
        sys.path.insert(0, path)


_AddToPathIfNeeded(
    os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'src', 'tests', 'py_utils')))
import angle_path_util

angle_path_util.AddDepsDirToPath('testing/merge_scripts')
import merge_api
import standard_isolated_script_merge


def main(raw_args):

    parser = merge_api.ArgumentParser()
    args = parser.parse_args(raw_args)

    # TODO(jmadill): Merge QPA files into one. http://anglebug.com/5236

    return standard_isolated_script_merge.StandardIsolatedScriptMerge(
        args.output_json, args.summary_json, args.jsons_to_merge)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
