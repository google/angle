# Copyright 2022 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import glob
import json
import logging
import os
import posixpath
import random
import subprocess
import tarfile
import tempfile
import time

import angle_path_util


def _ApkPath(suite_name):
    return os.path.join('%s_apk' % suite_name, '%s-debug.apk' % suite_name)


def ApkFileExists(suite_name):
    return os.path.exists(_ApkPath(suite_name))


def _Run(cmd):
    logging.debug('Executing command: %s', cmd)
    startupinfo = None
    if hasattr(subprocess, 'STARTUPINFO'):
        # Prevent console window popping up on Windows
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startupinfo.wShowWindow = subprocess.SW_HIDE
    output = subprocess.check_output(cmd, startupinfo=startupinfo)
    return output


class Adb(object):

    def __init__(self, adb_path=None):
        if not adb_path:
            adb_path = os.path.join(angle_path_util.ANGLE_ROOT_DIR,
                                    'third_party/android_sdk/public/platform-tools/adb')
        self._adb_path = adb_path

    def Run(self, args):
        return _Run([self._adb_path] + args)

    def Shell(self, cmd):
        return _Run([self._adb_path, 'shell', cmd])


def _GetAdbRoot(adb):
    adb.Run(['root'])

    for _ in range(20):
        time.sleep(0.5)
        try:
            id_out = adb.Shell(adb_path, 'id').decode('ascii')
            if 'uid=0(root)' in id_out:
                return
        except Exception:
            continue
    raise Exception("adb root failed")


def _ReadDeviceFile(adb, device_path):
    out_wc = adb.Shell('cat %s | wc -c' % device_path)
    expected_size = int(out_wc.decode('ascii').strip())
    out = adb.Run(['exec-out', 'cat %s' % device_path])
    assert len(out) == expected_size, 'exec-out mismatch'
    return out


def _RemoveDeviceFile(adb, device_path):
    adb.Shell('rm -f ' + device_path + ' || true')  # ignore errors


def _AddRestrictedTracesJson(adb):
    adb.Shell('mkdir -p /sdcard/chromium_tests_root/')

    def add(tar, fn):
        assert (fn.startswith('../../'))
        tar.add(fn, arcname=fn.replace('../../', ''))

    with _TempLocalFile() as tempfile_path:
        with tarfile.open(tempfile_path, 'w', format=tarfile.GNU_FORMAT) as tar:
            for f in glob.glob('../../src/tests/restricted_traces/*/*.json', recursive=True):
                add(tar, f)
            add(tar, '../../src/tests/restricted_traces/restricted_traces.json')
        adb.Run(['push', tempfile_path, '/sdcard/chromium_tests_root/t.tar'])

    adb.Shell('r=/sdcard/chromium_tests_root; tar -xf $r/t.tar -C $r/ && rm $r/t.tar')


def PrepareTestSuite(adb, suite_name):
    apk_path = _ApkPath(suite_name)
    logging.info('Installing apk path=%s size=%s' % (apk_path, os.path.getsize(apk_path)))

    adb.Run(['install', '-r', '-d', apk_path])

    permissions = [
        'android.permission.CAMERA', 'android.permission.CHANGE_CONFIGURATION',
        'android.permission.READ_EXTERNAL_STORAGE', 'android.permission.RECORD_AUDIO',
        'android.permission.WRITE_EXTERNAL_STORAGE'
    ]
    adb.Shell('p=com.android.angle.test;'
              'for q in %s;do pm grant "$p" "$q";done;' % ' '.join(permissions))

    if suite_name == 'angle_perftests':
        _AddRestrictedTracesJson(adb)


def PrepareRestrictedTraces(adb, traces):
    start = time.time()
    total_size = 0
    for trace in traces:
        path_from_root = 'src/tests/restricted_traces/' + trace + '/' + trace + '.angledata.gz'
        local_path = '../../' + path_from_root
        total_size += os.path.getsize(local_path)
        adb.Run(['push', local_path, '/sdcard/chromium_tests_root/' + path_from_root])

    logging.info('Pushed %d trace files (%.1fMB) in %.1fs', len(traces), total_size / 1e6,
                 time.time() - start)


def _RandomHex():
    return hex(random.randint(0, 2**64))[2:]


@contextlib.contextmanager
def _TempDeviceDir(adb):
    path = '/sdcard/Download/temp_dir-%s' % _RandomHex()
    adb.Shell('mkdir -p ' + path)
    try:
        yield path
    finally:
        adb.Shell('rm -rf ' + path)


@contextlib.contextmanager
def _TempDeviceFile(adb):
    path = '/sdcard/Download/temp_file-%s' % _RandomHex()
    try:
        yield path
    finally:
        adb.Shell('rm -f ' + path)


@contextlib.contextmanager
def _TempLocalFile():
    fd, path = tempfile.mkstemp()
    os.close(fd)
    try:
        yield path
    finally:
        os.remove(path)


def _RunInstrumentation(adb, flags):
    with _TempDeviceFile(adb) as temp_device_file:
        cmd = ' '.join([
            'p=com.android.angle.test;',
            'ntr=org.chromium.native_test.NativeTestInstrumentationTestRunner;',
            'am instrument -w',
            '-e $ntr.NativeTestActivity "$p".AngleUnitTestActivity',
            '-e $ntr.ShardNanoTimeout 2400000000000',
            '-e org.chromium.native_test.NativeTest.CommandLineFlags "%s"' % ' '.join(flags),
            '-e $ntr.StdoutFile ' + temp_device_file,
            '"$p"/org.chromium.build.gtest_apk.NativeTestInstrumentationTestRunner',
        ])

        adb.Shell(cmd)
        return _ReadDeviceFile(adb, temp_device_file)


def AngleSystemInfo(adb, args):
    PrepareTestSuite(adb, 'angle_system_info_test')

    with _TempDeviceDir(adb) as temp_dir:
        _RunInstrumentation(adb, args + ['--render-test-output-dir=' + temp_dir])
        output_file = posixpath.join(temp_dir, 'angle_system_info.json')
        return json.loads(_ReadDeviceFile(adb, output_file))


def ListTests(adb):
    out_lines = _RunInstrumentation(adb, ["--list-tests"]).decode('ascii').split('\n')

    start = out_lines.index('Tests list:')
    end = out_lines.index('End tests list.')
    return out_lines[start + 1:end]


def _PullDir(adb, device_dir, local_dir):
    files = adb.Shell('ls -1 %s' % device_dir).decode('ascii').split('\n')
    for f in files:
        f = f.strip()
        if f:
            adb.Run(['pull', posixpath.join(device_dir, f), posixpath.join(local_dir, f)])


def RunTests(adb, test_suite, args, stdoutfile, output_dir):
    with _TempDeviceDir(adb) as temp_dir:
        output = _RunInstrumentation(adb, args + ['--render-test-output-dir=' + temp_dir])
        with open(stdoutfile, 'wb') as f:
            f.write(output)
            logging.info(output.decode())
        _PullDir(adb, temp_dir, output_dir)
