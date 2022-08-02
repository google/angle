# Copyright 2022 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import importlib
import logging
import sys


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
