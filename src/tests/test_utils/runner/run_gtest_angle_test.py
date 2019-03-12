#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs an isolated non-Telemetry ANGLE test .

The main contract is that the caller passes the arguments:

  --isolated-script-test-output=[FILENAME]
json is written to that file in the format produced by
common.parse_common_test_results.

Optional argument:

  --isolated-script-test-filter=[TEST_NAMES]

is a double-colon-separated ("::") list of test names, to run just that subset
of tests. This list is parsed by this harness and sent down via the
--gtest_filter argument.

This script is intended to be the base command invoked by the isolate,
followed by a subsequent non-python executable.  It is modeled after
run_gtest_perf_test.py
"""

import argparse
import json
import os
import shutil
import sys
import tempfile
import traceback

def GetSrcDir():
  dirs = [
    os.path.join(os.path.abspath(__file__), '..', '..', '..', '..', '..'),
    os.path.join(os.path.abspath(__file__), '..', '..', '..', '..', '..', '..', '..'),
  ]

  for dir in dirs:
    if os.path.isdir(os.path.join(dir, 'testing')):
      return dir
  raise Exception('failed to find testing directory')

def GetScriptsDir():
  return os.path.join(GetSrcDir(), 'testing', 'scripts')

# Add src/testing/ into sys.path for importing xvfb and test_env.
sys.path.append(GetScriptsDir())

import common


import xvfb
import test_env

# Unfortunately we need to copy these variables from ../test_env.py.
# Importing it and using its get_sandbox_env breaks test runs on Linux
# (it seems to unset DISPLAY).
CHROME_SANDBOX_ENV = 'CHROME_DEVEL_SANDBOX'
CHROME_SANDBOX_PATH = '/opt/chromium/chrome_sandbox'


def IsWindows():
  return sys.platform == 'cygwin' or sys.platform.startswith('win')


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('executable', help='Test executable.')
  parser.add_argument(
      '--isolated-script-test-output', type=str,
      required=True)
  parser.add_argument(
      '--isolated-script-test-filter', type=str, required=False)
  parser.add_argument('--xvfb', help='Start xvfb.', action='store_true')

  args, extra_flags = parser.parse_known_args()

  env = os.environ.copy()

  total_shards = None
  shard_index = None
  if 'GTEST_TOTAL_SHARDS' in env:
    extra_flags += ['--shard-count=%d' % env['GTEST_TOTAL_SHARDS']]
  if 'GTEST_SHARD_INDEX' in env:
    extra_flags += ['--shard-index=%d' % env['GTEST_SHARD_INDEX']]

  # Assume we want to set up the sandbox environment variables all the
  # time; doing so is harmless on non-Linux platforms and is needed
  # all the time on Linux.
  env[CHROME_SANDBOX_ENV] = CHROME_SANDBOX_PATH

  rc = 0
  try:
    # Consider adding stdio control flags.
    if args.isolated_script_test_output:
      extra_flags.append('--results-directory=%s' %
        os.path.dirname(args.isolated_script_test_output))

    if args.isolated_script_test_filter:
      filter_list = common.extract_filter_list(
        args.isolated_script_test_filter)
      extra_flags.append('--gtest_filter=' + ':'.join(filter_list))

    if IsWindows():
      args.executable = '.\\%s.exe' % args.executable
    else:
      args.executable = './%s' % args.executable
    with common.temporary_file() as tempfile_path:
      env['CHROME_HEADLESS'] = '1'
      cmd = [args.executable] + extra_flags

      if args.xvfb:
        rc = xvfb.run_executable(cmd, env, stdoutfile=tempfile_path)
      else:
        rc = test_env.run_command_with_output(cmd, env=env,
                                              stdoutfile=tempfile_path)

  except Exception:
    traceback.print_exc()
    rc = 1

  valid = (rc == 0)
  failures = [] if valid else ['(entire test suite)']
  output_json = {
      'valid': valid,
      'failures': failures,
    }

  return rc


# This is not really a "script test" so does not need to manually add
# any additional compile targets.
def main_compile_targets(args):
  json.dump([], args.output)


if __name__ == '__main__':
  # Conform minimally to the protocol defined by ScriptTest.
  if 'compile_targets' in sys.argv:
    funcs = {
      'run': None,
      'compile_targets': main_compile_targets,
    }
    sys.exit(common.run_script(sys.argv[1:], funcs))
  sys.exit(main())

