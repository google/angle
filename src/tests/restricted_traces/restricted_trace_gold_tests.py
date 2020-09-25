#! /usr/bin/env python
#
# [VPYTHON:BEGIN]
# wheel: <
#  name: "infra/python/wheels/psutil/${vpython_platform}"
#  version: "version:5.2.2"
# >
# wheel: <
#  name: "infra/python/wheels/six-py2_py3"
#  version: "version:1.10.0"
# >
# [VPYTHON:END]
#
# Copyright 2020 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# restricted_trace_gold_tests.py:
#   Uses Skia Gold (https://skia.org/dev/testing/skiagold) to run pixel tests with ANGLE traces.
#
#   Requires vpython to run standalone. Run with --help for usage instructions.

import argparse
import contextlib
import json
import os
import shutil
import sys
import tempfile
import time
import traceback

# Add //src/testing into sys.path for importing xvfb and test_env, and
# //src/testing/scripts for importing common.
d = os.path.dirname
THIS_DIR = d(os.path.abspath(__file__))
ANGLE_SRC_DIR = d(d(d(THIS_DIR)))
sys.path.insert(0, os.path.join(ANGLE_SRC_DIR, 'testing'))
sys.path.insert(0, os.path.join(ANGLE_SRC_DIR, 'testing', 'scripts'))
# Handle the Chromium-relative directory as well. As long as one directory
# is valid, Python is happy.
CHROMIUM_SRC_DIR = d(d(ANGLE_SRC_DIR))
sys.path.insert(0, os.path.join(CHROMIUM_SRC_DIR, 'testing'))
sys.path.insert(0, os.path.join(CHROMIUM_SRC_DIR, 'testing', 'scripts'))

import common
import test_env
import xvfb


def IsWindows():
    return sys.platform == 'cygwin' or sys.platform.startswith('win')


DEFAULT_TEST_SUITE = 'angle_perftests'
DEFAULT_TEST_PREFIX = '--gtest_filter=TracePerfTest.Run/vulkan_'


@contextlib.contextmanager
def temporary_dir(prefix=''):
    path = tempfile.mkdtemp(prefix=prefix)
    try:
        yield path
    finally:
        shutil.rmtree(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--isolated-script-test-output', type=str, required=True)
    parser.add_argument('--isolated-script-test-perf-output', type=str)
    parser.add_argument('--test-suite', help='Test suite to run.', default=DEFAULT_TEST_SUITE)
    parser.add_argument('--render-test-output-dir', help='Directory to store screenshots')
    parser.add_argument('--xvfb', help='Start xvfb.', action='store_true')

    args, extra_flags = parser.parse_known_args()
    env = os.environ.copy()

    if 'GTEST_TOTAL_SHARDS' in env and int(env['GTEST_TOTAL_SHARDS']) != 1:
        print('Sharding not yet implemented.')
        sys.exit(1)

    results = {
        'tests': {
            'angle_restricted_trace_gold_tests': {}
        },
        'interrupted': False,
        'seconds_since_epoch': time.time(),
        'path_delimiter': '.',
        'version': 3,
        'num_failures_by_type': {
            'PASS': 0,
            'FAIL': 0,
        }
    }

    result_tests = results['tests']['angle_restricted_trace_gold_tests']

    def run_tests(args, tests, extra_flags, env, screenshot_dir):
        for test in tests['traces']:
            with common.temporary_file() as tempfile_path:
                cmd = [
                    args.test_suite,
                    DEFAULT_TEST_PREFIX + test,
                    '--render-test-output-dir=%s' % screenshot_dir,
                    '--one-frame-only',
                ] + extra_flags

                if args.xvfb:
                    rc = xvfb.run_executable(cmd, env, stdoutfile=tempfile_path)
                else:
                    rc = test_env.run_command_with_output(cmd, env=env, stdoutfile=tempfile_path)

                pass_fail = 'PASS' if rc == 0 else 'FAIL'
                result_tests[test] = {'expected': 'PASS', 'actual': pass_fail}
                results['num_failures_by_type'][pass_fail] += 1

        return results['num_failures_by_type']['FAIL'] == 0

    rc = 0
    try:
        if IsWindows():
            args.test_suite = '.\\%s.exe' % args.test_suite
        else:
            args.test_suite = './%s' % args.test_suite

        # read test set
        json_name = os.path.join(ANGLE_SRC_DIR, 'src', 'tests', 'restricted_traces',
                                 'restricted_traces.json')
        with open(json_name) as fp:
            tests = json.load(fp)

        if args.render_test_output_dir:
            if not run_tests(args, tests, extra_flags, env, args.render_test_output_dir):
                rc = 1
        else:
            with temporary_dir('angle_trace_') as temp_dir:
                if not run_tests(args, tests, extra_flags, env, temp_dir):
                    rc = 1

    except Exception:
        traceback.print_exc()
        rc = 1

    if args.isolated_script_test_output:
        with open(args.isolated_script_test_output, 'w') as out_file:
            out_file.write(json.dumps(results))

    if args.isolated_script_test_perf_output:
        with open(args.isolated_script_test_perf_output, 'w') as out_file:
            out_file.write(json.dumps({}))

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
