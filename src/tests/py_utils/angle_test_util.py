# Copyright 2022 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import importlib
import io
import logging
import subprocess
import sys
import time

import android_helper
import angle_path_util

angle_path_util.AddDepsDirToPath('testing/scripts')
import common
import test_env
import xvfb


def Initialize(suite_name):
    android_helper.Initialize(suite_name)


# Requires .Initialize() to be called first
def IsAndroid():
    return android_helper.IsAndroid()


class LogFormatter(logging.Formatter):

    def __init__(self):
        logging.Formatter.__init__(self, fmt='%(levelname).1s%(asctime)s %(message)s')

    def formatTime(self, record, datefmt=None):
        # Drop date as these scripts are short lived
        return datetime.datetime.fromtimestamp(record.created).strftime('%H:%M:%S.%fZ')


def SetupLogging(level):
    # Reload to reset if it was already setup by a library
    importlib.reload(logging)

    logger = logging.getLogger()
    logger.setLevel(level)

    handler = logging.StreamHandler()
    handler.setFormatter(LogFormatter())
    logger.addHandler(handler)


def IsWindows():
    return sys.platform == 'cygwin' or sys.platform.startswith('win')


def ExecutablePathInCurrentDir(binary):
    if IsWindows():
        return '.\\%s.exe' % binary
    else:
        return './%s' % binary


def HasGtestShardsAndIndex(env):
    if 'GTEST_TOTAL_SHARDS' in env and int(env['GTEST_TOTAL_SHARDS']) != 1:
        if 'GTEST_SHARD_INDEX' not in env:
            logging.error('Sharding params must be specified together.')
            sys.exit(1)
        return True

    return False


def PopGtestShardsAndIndex(env):
    return int(env.pop('GTEST_TOTAL_SHARDS')), int(env.pop('GTEST_SHARD_INDEX'))


# From testing/test_env.py, see run_command_with_output below
def _popen(*args, **kwargs):
    assert 'creationflags' not in kwargs
    if sys.platform == 'win32':
        # Necessary for signal handling. See crbug.com/733612#c6.
        kwargs['creationflags'] = subprocess.CREATE_NEW_PROCESS_GROUP
    return subprocess.Popen(*args, **kwargs)


# Forked from testing/test_env.py to add ability to suppress logging with log=False
def run_command_with_output(argv, stdoutfile, env=None, cwd=None, log=True):
    assert stdoutfile
    with io.open(stdoutfile, 'wb') as writer, \
          io.open(stdoutfile, 'rb') as reader:
        process = _popen(argv, env=env, cwd=cwd, stdout=writer, stderr=subprocess.STDOUT)
        test_env.forward_signals([process])
        while process.poll() is None:
            if log:
                sys.stdout.write(reader.read().decode('utf-8'))
            # This sleep is needed for signal propagation. See the
            # wait_with_signals() docstring.
            time.sleep(0.1)
        if log:
            sys.stdout.write(reader.read().decode('utf-8'))
        return process.returncode


def RunTestSuite(test_suite,
                 cmd_args,
                 env,
                 runner_args=None,
                 show_test_stdout=True,
                 use_xvfb=False):
    if android_helper.IsAndroid():
        result, output = android_helper.RunTests(test_suite, cmd_args, log_output=show_test_stdout)
        return result, output.decode()

    runner_cmd = [ExecutablePathInCurrentDir(test_suite)] + cmd_args + (runner_args or [])

    logging.debug(' '.join(runner_cmd))
    with common.temporary_file() as tempfile_path:
        if use_xvfb:
            exit_code = xvfb.run_executable(runner_cmd, env, stdoutfile=tempfile_path)
        else:
            exit_code = run_command_with_output(
                runner_cmd, env=env, stdoutfile=tempfile_path, log=show_test_stdout)
        with open(tempfile_path) as f:
            output = f.read()
    return exit_code, output
