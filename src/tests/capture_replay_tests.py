#! /usr/bin/env vpython3
#
# Copyright 2020 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
"""
Script testing capture_replay with angle_end2end_tests
"""

# Automation script will:
# 1. Build all tests in angle_end2end with frame capture enabled
# 2. Run each test with frame capture
# 3. Build CaptureReplayTest with cpp trace files
# 4. Run CaptureReplayTest
# 5. Output the number of test successes and failures. A test succeeds if no error occurs during
# its capture and replay, and the GL states at the end of two runs match. Any unexpected failure
# will return non-zero exit code

# Run this script with Python to test capture replay on angle_end2end tests
# python path/to/capture_replay_tests.py
# Command line arguments: run with --help for a full list.

import argparse
import difflib
import distutils.util
import fnmatch
import logging
import math
import multiprocessing
import os
import psutil
import queue
import re
import shutil
import subprocess
import sys
import time
import traceback

from sys import platform

PIPE_STDOUT = True
DEFAULT_OUT_DIR = "out/CaptureReplayTest"  # relative to angle folder
DEFAULT_FILTER = "*/ES2_Vulkan"
DEFAULT_TEST_SUITE = "angle_end2end_tests"
REPLAY_SAMPLE_FOLDER = "src/tests/capture_replay_tests"  # relative to angle folder
DEFAULT_BATCH_COUNT = 8  # number of tests batched together
TRACE_FILE_SUFFIX = "_capture_context"  # because we only deal with 1 context right now
RESULT_TAG = "*RESULT"
TIME_BETWEEN_MESSAGE = 20  # in seconds
SUBPROCESS_TIMEOUT = 600  # in seconds
DEFAULT_RESULT_FILE = "results.txt"
DEFAULT_LOG_LEVEL = "info"
DEFAULT_MAX_JOBS = 8
REPLAY_BINARY = "capture_replay_tests"
if platform == "win32":
    REPLAY_BINARY += ".exe"
TRACE_FOLDER = "traces"

EXIT_SUCCESS = 0
EXIT_FAILURE = 1

switch_case_without_return_template = """\
        case {case}:
            {namespace}::{call}({params});
            break;
"""

switch_case_with_return_template = """\
        case {case}:
            return {namespace}::{call}({params});
"""

default_case_without_return_template = """\
        default:
            break;"""
default_case_with_return_template = """\
        default:
            return {default_val};"""

test_trace_info_init_template = """\
    {{
        "{namespace}",
        {namespace}::kReplayContextClientMajorVersion,
        {namespace}::kReplayContextClientMinorVersion,
        {namespace}::kReplayPlatformType,
        {namespace}::kReplayDeviceType,
        {namespace}::kReplayFrameStart,
        {namespace}::kReplayFrameEnd,
        {namespace}::kReplayDrawSurfaceWidth,
        {namespace}::kReplayDrawSurfaceHeight,
        {namespace}::kDefaultFramebufferRedBits,
        {namespace}::kDefaultFramebufferGreenBits,
        {namespace}::kDefaultFramebufferBlueBits,
        {namespace}::kDefaultFramebufferAlphaBits,
        {namespace}::kDefaultFramebufferDepthBits,
        {namespace}::kDefaultFramebufferStencilBits,
        {namespace}::kIsBinaryDataCompressed,
        {namespace}::kAreClientArraysEnabled,
        {namespace}::kbindGeneratesResources,
        {namespace}::kWebGLCompatibility,
        {namespace}::kRobustResourceInit,
    }},
"""

composite_h_file_template = """\
#pragma once
#include <vector>
#include <string>

{trace_headers}

struct TestTraceInfo {{
    std::string testName;
    uint32_t replayContextMajorVersion;
    uint32_t replayContextMinorVersion;
    EGLint replayPlatformType;
    EGLint replayDeviceType;
    uint32_t replayFrameStart;
    uint32_t replayFrameEnd;
    EGLint replayDrawSurfaceWidth;
    EGLint replayDrawSurfaceHeight;
    EGLint defaultFramebufferRedBits;
    EGLint defaultFramebufferGreenBits;
    EGLint defaultFramebufferBlueBits;
    EGLint defaultFramebufferAlphaBits;
    EGLint defaultFramebufferDepthBits;
    EGLint defaultFramebufferStencilBits;
    bool isBinaryDataCompressed;
    bool areClientArraysEnabled;
    bool bindGeneratesResources;
    bool webGLCompatibility;
    bool robustResourceInit;
}};

extern std::vector<TestTraceInfo> testTraceInfos;
"""

composite_cpp_file_template = """\
#include "{h_filename}"

std::vector<TestTraceInfo> testTraceInfos =
{{
{test_trace_info_inits}
}};
"""


def debug(str):
    logging.debug('%s: %s' % (multiprocessing.current_process().name, str))


def info(str):
    logging.info('%s: %s' % (multiprocessing.current_process().name, str))


def winext(name, ext):
    return ("%s.%s" % (name, ext)) if platform == "win32" else name


def AutodetectGoma():
    return winext('compiler_proxy', 'exe') in (p.name() for p in psutil.process_iter())


class SubProcess():

    def __init__(self, command, env=os.environ, pipe_stdout=PIPE_STDOUT):
        # shell=False so that only 1 subprocess is spawned.
        # if shell=True, a shell process is spawned, which in turn spawns the process running
        # the command. Since we do not have a handle to the 2nd process, we cannot terminate it.
        if pipe_stdout:
            self.proc_handle = subprocess.Popen(
                command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=False)
        else:
            self.proc_handle = subprocess.Popen(command, env=env, shell=False)

    def Join(self, timeout):
        debug('Joining with subprocess %d, timeout %s' % (self.Pid(), str(timeout)))
        output = self.proc_handle.communicate(timeout=timeout)[0]
        if output:
            output = output.decode('utf-8')
        else:
            output = ''
        return self.proc_handle.returncode, output

    def Pid(self):
        return self.proc_handle.pid

    def Kill(self):
        self.proc_handle.terminate()
        self.proc_handle.wait()


# class that manages all child processes of a process. Any process thats spawns subprocesses
# should have this. This object is created inside the main process, and each worker process.
class ChildProcessesManager():

    @classmethod
    def _GetGnAndNinjaAbsolutePaths(self):
        path = os.path.join('third_party', 'depot_tools')
        return os.path.join(path, winext('gn', 'bat')), os.path.join(path, winext('ninja', 'exe'))

    def __init__(self):
        # a dictionary of Subprocess, with pid as key
        self.subprocesses = {}
        # list of Python multiprocess.Process handles
        self.workers = []

        self._gn_path, self._ninja_path = self._GetGnAndNinjaAbsolutePaths()
        self._use_goma = AutodetectGoma()

    def RunSubprocess(self, command, env=None, pipe_stdout=True, timeout=None):
        proc = SubProcess(command, env, pipe_stdout)
        debug('Created subprocess: %s with pid %d' % (' '.join(command), proc.Pid()))
        self.subprocesses[proc.Pid()] = proc
        try:
            returncode, output = self.subprocesses[proc.Pid()].Join(timeout)
            self.RemoveSubprocess(proc.Pid())
            if returncode != 0:
                return -1, output
            return returncode, output
        except KeyboardInterrupt:
            raise
        except subprocess.TimeoutExpired as e:
            self.RemoveSubprocess(proc.Pid())
            return -2, str(e)
        except Exception as e:
            self.RemoveSubprocess(proc.Pid())
            return -1, str(e)

    def RemoveSubprocess(self, subprocess_id):
        assert subprocess_id in self.subprocesses
        self.subprocesses[subprocess_id].Kill()
        del self.subprocesses[subprocess_id]

    def AddWorker(self, worker):
        self.workers.append(worker)

    def KillAll(self):
        for subprocess_id in self.subprocesses:
            self.subprocesses[subprocess_id].Kill()
        for worker in self.workers:
            worker.terminate()
            worker.join()
            worker.close()  # to release file descriptors immediately
        self.subprocesses = {}
        self.workers = []

    def JoinWorkers(self):
        for worker in self.workers:
            worker.join()
            worker.close()
        self.workers = []

    def IsAnyWorkerAlive(self):
        return any([worker.is_alive() for worker in self.workers])

    def GetRemainingWorkers(self):
        count = 0
        for worker in self.workers:
            if worker.is_alive():
                count += 1
        return count

    def RunGNGen(self, args, build_dir, pipe_stdout, extra_gn_args=[]):
        gn_args = [('angle_with_capture_by_default', 'true')] + extra_gn_args
        if self._use_goma:
            gn_args.append(('use_goma', 'true'))
            if args.goma_dir:
                gn_args.append(('goma_dir', '"%s"' % args.goma_dir))
        if not args.debug:
            gn_args.append(('is_debug', 'false'))
            gn_args.append(('symbol_level', '1'))
            gn_args.append(('angle_assert_always_on', 'true'))
        if args.asan:
            gn_args.append(('is_asan', 'true'))
        debug('Calling GN gen with %s' % str(gn_args))
        args_str = ' '.join(['%s=%s' % (k, v) for (k, v) in gn_args])
        cmd = [self._gn_path, 'gen', '--args=%s' % args_str, build_dir]
        return self.RunSubprocess(cmd, pipe_stdout=pipe_stdout)

    def RunNinja(self, args, build_dir, target, pipe_stdout):
        cmd = [self._ninja_path]

        # This code is taken from depot_tools/autoninja.py
        if self._use_goma:
            num_cores = multiprocessing.cpu_count()
            cmd.append('-j')
            core_multiplier = 40
            j_value = num_cores * core_multiplier

            if sys.platform.startswith('win'):
                # On windows, j value higher than 1000 does not improve build performance.
                j_value = min(j_value, 1000)
            elif sys.platform == 'darwin':
                # On Mac, j value higher than 500 causes 'Too many open files' error
                # (crbug.com/936864).
                j_value = min(j_value, 500)

            cmd.append('%d' % j_value)
        else:
            cmd.append('-l')
            cmd.append('%d' % os.cpu_count())

        cmd += ['-C', build_dir, target]
        return self.RunSubprocess(cmd, pipe_stdout=pipe_stdout)


def GetTestsListForFilter(args, test_path, filter):
    cmd = GetRunCommand(args, test_path) + ["--list-tests", "--gtest_filter=%s" % filter]
    info('Getting test list from "%s"' % " ".join(cmd))
    return subprocess.check_output(cmd, text=True)


class TestExpectation():
    # tests that must not be run as list
    disabled_tests = []

    # test expectations for tests that do not pass
    non_pass_results = {}

    flaky_tests = []

    non_pass_re = {}

    # yapf: disable
    # we want each pair on one line
    result_map = { "FAIL" : "Fail",
                   "TIMEOUT" : "Timeout",
                   "CRASHED" : "Crashed",
                   "COMPILE_FAILED" : "CompileFailed",
                   "SKIPPED_BY_GTEST" : "Skipped",
                   "PASS" : "Pass"}
    # yapf: enable

    def __init__(self, platform):
        expected_results_filename = "capture_replay_expectations.txt"
        expected_results_path = os.path.join(REPLAY_SAMPLE_FOLDER, expected_results_filename)
        with open(expected_results_path, "rt") as f:
            for line in f:
                l = line.strip()
                if l != "" and not l.startswith("#"):
                    self.ReadOneExpectation(l, platform)

    def ReadOneExpectation(self, line, platform):
        (testpattern, result) = line.split('=')
        (test_info_string, test_name_string) = testpattern.split(':')
        test_name = test_name_string.strip()
        test_info = test_info_string.strip().split()
        result_stripped = result.strip()

        platforms = [platform]
        if len(test_info) > 1:
            platforms = test_info[1:]

        if platform in platforms:
            test_name_regex = re.compile('^' + test_name.replace('*', '.*') + '$')
            if result_stripped == 'SKIP_FOR_CAPTURE':
                self.disabled_tests.append(test_name_regex)
            elif result_stripped == 'FLAKY':
                self.flaky_tests.append(test_name_regex)
            else:
                self.non_pass_results[test_name] = self.result_map[result_stripped]
                self.non_pass_re[test_name] = test_name_regex

    def TestIsDisabled(self, test_name):
        for p in self.disabled_tests:
            m = p.match(test_name)
            if m is not None:
                return True
        return False

    def Filter(self, test_list):
        result = {}
        for t in test_list:
            for key in self.non_pass_results.keys():
                if self.non_pass_re[key].match(t) is not None:
                    result[t] = self.non_pass_results[key]
        return result

    def IsFlaky(self, test_name):
        for flaky in self.flaky_tests:
            if flaky.match(test_name) is not None:
                return True
        return False


def ParseTestNamesFromTestList(output, test_expectation, ignore_exclude_from_run):
    output_lines = output.splitlines()
    tests = []
    seen_start_of_tests = False
    disabled = 0
    for line in output_lines:
        l = line.strip()
        if l == 'Tests list:':
            seen_start_of_tests = True
        elif l == 'End tests list.':
            break
        elif not seen_start_of_tests:
            pass
        elif not test_expectation.TestIsDisabled(l) or ignore_exclude_from_run:
            tests.append(l)
        else:
            disabled += 1

    info('Found %s tests and %d disabled tests.' % (len(tests), disabled))
    return tests


def GetRunCommand(args, command):
    if args.xvfb:
        return ['vpython', 'testing/xvfb.py', command]
    else:
        return [command]


class GroupedResult():
    Passed = "Passed"
    Failed = "Comparison Failed"
    TimedOut = "Timeout"
    Crashed = "Crashed"
    CompileFailed = "CompileFailed"
    Skipped = "Skipped"


    def __init__(self, resultcode, message, output, tests):
        self.resultcode = resultcode
        self.message = message
        self.output = output
        self.tests = []
        for test in tests:
            self.tests.append(test)


class TestBatchResult():

    display_output_lines = 20

    def __init__(self, grouped_results, verbose):
        self.passes = []
        self.fails = []
        self.timeouts = []
        self.crashes = []
        self.compile_fails = []
        self.skips = []

        for grouped_result in grouped_results:
            if grouped_result.resultcode == GroupedResult.Passed:
                for test in grouped_result.tests:
                    self.passes.append(test.full_test_name)
            elif grouped_result.resultcode == GroupedResult.Failed:
                for test in grouped_result.tests:
                    self.fails.append(test.full_test_name)
            elif grouped_result.resultcode == GroupedResult.TimedOut:
                for test in grouped_result.tests:
                    self.timeouts.append(test.full_test_name)
            elif grouped_result.resultcode == GroupedResult.Crashed:
                for test in grouped_result.tests:
                    self.crashes.append(test.full_test_name)
            elif grouped_result.resultcode == GroupedResult.CompileFailed:
                for test in grouped_result.tests:
                    self.compile_fails.append(test.full_test_name)
            elif grouped_result.resultcode == GroupedResult.Skipped:
                for test in grouped_result.tests:
                    self.skips.append(test.full_test_name)

        self.repr_str = ""
        self.GenerateRepresentationString(grouped_results, verbose)

    def __str__(self):
        return self.repr_str

    def GenerateRepresentationString(self, grouped_results, verbose):
        for grouped_result in grouped_results:
            self.repr_str += grouped_result.resultcode + ": " + grouped_result.message + "\n"
            for test in grouped_result.tests:
                self.repr_str += "\t" + test.full_test_name + "\n"
            if verbose:
                self.repr_str += grouped_result.output
            else:
                if grouped_result.resultcode == GroupedResult.CompileFailed:
                    self.repr_str += TestBatchResult.ExtractErrors(grouped_result.output)
                elif grouped_result.resultcode != GroupedResult.Passed:
                    self.repr_str += TestBatchResult.GetAbbreviatedOutput(grouped_result.output)

    def ExtractErrors(output):
        lines = output.splitlines()
        error_lines = []
        for i in range(len(lines)):
            if ": error:" in lines[i]:
                error_lines.append(lines[i] + "\n")
                if i + 1 < len(lines):
                    error_lines.append(lines[i + 1] + "\n")
        return "".join(error_lines)

    def GetAbbreviatedOutput(output):
        # Get all lines after and including the last occurance of "Run".
        lines = output.splitlines()
        line_count = 0
        for line_index in reversed(range(len(lines))):
            line_count += 1
            if "[ RUN      ]" in lines[line_index]:
                break

        return '\n' + '\n'.join(lines[-line_count:]) + '\n'


class Test():

    def __init__(self, test_name):
        self.full_test_name = test_name
        self.params = test_name.split('/')[1]
        self.context_id = 0
        self.test_index = -1  # index of test within a test batch
        self._label = self.full_test_name.replace(".", "_").replace("/", "_")

    def __str__(self):
        return self.full_test_name + " Params: " + self.params

    def GetLabel(self):
        return self._label

    def CanRunReplay(self, trace_folder_path):
        test_files = []
        label = self.GetLabel() + "_capture"
        assert (self.context_id == 0)
        for f in os.listdir(trace_folder_path):
            if os.path.isfile(os.path.join(trace_folder_path, f)) and f.startswith(label):
                test_files.append(f)
        frame_files_count = 0
        context_header_count = 0
        context_source_count = 0
        source_txt_count = 0
        context_id = 0
        for f in test_files:
            if "_frame" in f:
                frame_files_count += 1
            elif f.endswith(".txt"):
                source_txt_count += 1
            elif f.endswith(".h"):
                context_header_count += 1
                if TRACE_FILE_SUFFIX in f:
                    context = f.split(TRACE_FILE_SUFFIX)[1][:-2]
                    context_id = int(context)
            elif f.endswith(".cpp"):
                context_source_count += 1
        can_run_replay = frame_files_count >= 1 and context_header_count >= 1 \
            and context_source_count >= 1 and source_txt_count == 1
        if not can_run_replay:
            return False
        self.context_id = context_id
        return True


class TestBatch():

    CAPTURE_FRAME_END = 100

    def __init__(self, args):
        self.args = args
        self.tests = []
        self.results = []

    def SetWorkerId(self, worker_id):
        self.trace_dir = "%s%d" % (TRACE_FOLDER, worker_id)
        self.trace_folder_path = os.path.join(REPLAY_SAMPLE_FOLDER, self.trace_dir)

    def RunWithCapture(self, args, child_processes_manager):
        test_exe_path = os.path.join(args.out_dir, 'Capture', args.test_suite)

        # set the static environment variables that do not change throughout the script run
        env = os.environ.copy()
        env['ANGLE_CAPTURE_FRAME_END'] = '{}'.format(self.CAPTURE_FRAME_END)
        env['ANGLE_CAPTURE_SERIALIZE_STATE'] = '1'
        env['ANGLE_FEATURE_OVERRIDES_ENABLED'] = 'forceRobustResourceInit'
        env['ANGLE_CAPTURE_ENABLED'] = '1'

        info('Setting ANGLE_CAPTURE_OUT_DIR to %s' % self.trace_folder_path)
        env['ANGLE_CAPTURE_OUT_DIR'] = self.trace_folder_path

        if not self.args.keep_temp_files:
            ClearFolderContent(self.trace_folder_path)
        filt = ':'.join([test.full_test_name for test in self.tests])

        cmd = GetRunCommand(args, test_exe_path)
        filter_string = '--gtest_filter=%s' % filt
        cmd += [filter_string, '--angle-per-test-capture-label']

        if self.args.verbose:
            info("Run capture: '{} {}'".format(test_exe_path, filter_string))

        returncode, output = child_processes_manager.RunSubprocess(
            cmd, env, timeout=SUBPROCESS_TIMEOUT)
        if args.show_capture_stdout:
            info("Capture stdout: %s" % output)
        if returncode == -1:
            self.results.append(GroupedResult(GroupedResult.Crashed, "", output, self.tests))
            return False
        elif returncode == -2:
            self.results.append(GroupedResult(GroupedResult.TimedOut, "", "", self.tests))
            return False
        return True

    def RemoveTestsThatDoNotProduceAppropriateTraceFiles(self):
        continued_tests = []
        skipped_tests = []
        for test in self.tests:
            if not test.CanRunReplay(self.trace_folder_path):
                skipped_tests.append(test)
            else:
                continued_tests.append(test)
        if len(skipped_tests) > 0:
            self.results.append(
                GroupedResult(
                    GroupedResult.Skipped,
                    "Skipping replay since capture didn't produce necessary trace files", "",
                    skipped_tests))
        return continued_tests

    def BuildReplay(self, replay_build_dir, composite_file_id, tests, child_processes_manager):
        # write gni file that holds all the traces files in a list
        self.CreateGNIFile(composite_file_id, tests)
        # write header and cpp composite files, which glue the trace files with
        # CaptureReplayTests.cpp
        self.CreateTestsCompositeFiles(composite_file_id, tests)

        gn_args = [('angle_build_capture_replay_tests', 'true'),
                   ('angle_capture_replay_test_trace_dir', '"%s"' % self.trace_dir),
                   ('angle_capture_replay_composite_file_id', str(composite_file_id))]
        returncode, output = child_processes_manager.RunGNGen(self.args, replay_build_dir, True,
                                                              gn_args)
        if returncode != 0:
            self.results.append(
                GroupedResult(GroupedResult.CompileFailed, "Build replay failed at gn generation",
                              output, tests))
            return False
        returncode, output = child_processes_manager.RunNinja(self.args, replay_build_dir,
                                                              REPLAY_BINARY, True)
        if returncode != 0:
            self.results.append(
                GroupedResult(GroupedResult.CompileFailed, "Build replay failed at ninja", output,
                              tests))
            return False
        return True

    def RunReplay(self, replay_build_dir, replay_exe_path, child_processes_manager, tests):
        env = os.environ.copy()
        env['ANGLE_CAPTURE_ENABLED'] = '0'
        env['ANGLE_FEATURE_OVERRIDES_ENABLED'] = 'enable_capture_limits'

        if self.args.verbose:
            info("Run Replay: {}".format(replay_exe_path))

        returncode, output = child_processes_manager.RunSubprocess(
            GetRunCommand(self.args, replay_exe_path), env, timeout=SUBPROCESS_TIMEOUT)
        if returncode == -1:
            cmd = replay_exe_path
            self.results.append(
                GroupedResult(GroupedResult.Crashed, "Replay run crashed (%s)" % cmd, output,
                              tests))
            return
        elif returncode == -2:
            self.results.append(
                GroupedResult(GroupedResult.TimedOut, "Replay run timed out", output, tests))
            return

        output_lines = output.splitlines()
        passes = []
        fails = []
        count = 0
        for output_line in output_lines:
            words = output_line.split(" ")
            if len(words) == 3 and words[0] == RESULT_TAG:
                if int(words[2]) == 0:
                    passes.append(self.FindTestByLabel(words[1]))
                else:
                    fails.append(self.FindTestByLabel(words[1]))
                    logging.info("Context comparison failed: {}".format(
                        self.FindTestByLabel(words[1])))
                    self.PrintContextDiff(replay_build_dir, words[1])

                count += 1
        if len(passes) > 0:
            self.results.append(GroupedResult(GroupedResult.Passed, "", output, passes))
        if len(fails) > 0:
            self.results.append(GroupedResult(GroupedResult.Failed, "", output, fails))

    def PrintContextDiff(self, replay_build_dir, test_name):
        frame = 1
        while True:
            capture_file = "{}/{}_ContextCaptured{}.json".format(replay_build_dir, test_name,
                                                                 frame)
            replay_file = "{}/{}_ContextReplayed{}.json".format(replay_build_dir, test_name, frame)
            if os.path.exists(capture_file) and os.path.exists(replay_file):
                captured_context = open(capture_file, "r").readlines()
                replayed_context = open(replay_file, "r").readlines()
                for line in difflib.unified_diff(
                        captured_context, replayed_context, fromfile=capture_file,
                        tofile=replay_file):
                    print(line, end="")
            else:
                if frame > self.CAPTURE_FRAME_END:
                    break
            frame = frame + 1

    def FindTestByLabel(self, label):
        for test in self.tests:
            if test.GetLabel() == label:
                return test
        return None

    def AddTest(self, test):
        assert len(self.tests) <= self.args.batch_count
        test.index = len(self.tests)
        self.tests.append(test)

    # gni file, which holds all the sources for a replay application
    def CreateGNIFile(self, composite_file_id, tests):
        test_list = []
        for test in tests:
            label = test.GetLabel()
            assert (test.context_id > 0)

            fname = "%s%s%d_files.txt" % (label, TRACE_FILE_SUFFIX, test.context_id)
            fpath = os.path.join(self.trace_folder_path, fname)
            with open(fpath) as f:
                files = f.readlines()
                f.close()
            files = ['"%s/%s"' % (self.trace_dir, file.strip()) for file in files]
            angledata = "%s%s.angledata.gz" % (label, TRACE_FILE_SUFFIX)
            test_list += [
                '["%s", %s, [%s], "%s"]' % (label, test.context_id, ','.join(files), angledata)
            ]
        gni_path = os.path.join(self.trace_folder_path, "traces%d.gni" % composite_file_id)
        with open(gni_path, "w") as f:
            f.write("trace_data = [\n%s\n]\n" % ',\n'.join(test_list))
            f.close()

    # header and cpp composite files, which glue the trace files with CaptureReplayTests.cpp
    def CreateTestsCompositeFiles(self, composite_file_id, tests):
        # write CompositeTests header file
        include_header_template = '#include "{header_file_path}.h"\n'
        trace_headers = "".join([
            include_header_template.format(header_file_path=test.GetLabel() + TRACE_FILE_SUFFIX +
                                           str(test.context_id)) for test in tests
        ])

        h_filename = "CompositeTests%d.h" % composite_file_id
        with open(os.path.join(self.trace_folder_path, h_filename), "w") as h_file:
            h_file.write(composite_h_file_template.format(trace_headers=trace_headers))
            h_file.close()

        # write CompositeTests cpp file
        test_trace_info_inits = "".join([
            test_trace_info_init_template.format(namespace=tests[i].GetLabel())
            for i in range(len(tests))
        ])

        cpp_filename = "CompositeTests%d.cpp" % composite_file_id
        with open(os.path.join(self.trace_folder_path, cpp_filename), "w") as cpp_file:
            cpp_file.write(
                composite_cpp_file_template.format(
                    h_filename=h_filename, test_trace_info_inits=test_trace_info_inits))
            cpp_file.close()

    def __str__(self):
        repr_str = "TestBatch:\n"
        for test in self.tests:
            repr_str += ("\t" + str(test) + "\n")
        return repr_str

    def __getitem__(self, index):
        assert index < len(self.tests)
        return self.tests[index]

    def __iter__(self):
        return iter(self.tests)

    def GetResults(self):
        return TestBatchResult(self.results, self.args.verbose)


def ClearFolderContent(path):
    all_files = []
    for f in os.listdir(path):
        if os.path.isfile(os.path.join(path, f)):
            os.remove(os.path.join(path, f))

def SetCWDToAngleFolder():
    cwd = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    os.chdir(cwd)
    return cwd


def RunTests(args, worker_id, job_queue, result_list, message_queue):
    replay_build_dir = os.path.join(args.out_dir, 'Replay%d' % worker_id)
    replay_exec_path = os.path.join(replay_build_dir, REPLAY_BINARY)

    child_processes_manager = ChildProcessesManager()
    # used to differentiate between multiple composite files when there are multiple test batchs
    # running on the same worker and --deleted_trace is set to False
    composite_file_id = 1
    while not job_queue.empty():
        try:
            test_batch = job_queue.get()
            message_queue.put("Starting {} tests on worker {}. Unstarted jobs: {}".format(
                len(test_batch.tests), worker_id, job_queue.qsize()))

            test_batch.SetWorkerId(worker_id)

            success = test_batch.RunWithCapture(args, child_processes_manager)
            if not success:
                result_list.append(test_batch.GetResults())
                message_queue.put(str(test_batch.GetResults()))
                continue
            continued_tests = test_batch.RemoveTestsThatDoNotProduceAppropriateTraceFiles()
            if len(continued_tests) == 0:
                result_list.append(test_batch.GetResults())
                message_queue.put(str(test_batch.GetResults()))
                continue
            success = test_batch.BuildReplay(replay_build_dir, composite_file_id, continued_tests,
                                             child_processes_manager)
            if args.keep_temp_files:
                composite_file_id += 1
            if not success:
                result_list.append(test_batch.GetResults())
                message_queue.put(str(test_batch.GetResults()))
                continue
            test_batch.RunReplay(replay_build_dir, replay_exec_path, child_processes_manager,
                                 continued_tests)
            result_list.append(test_batch.GetResults())
            message_queue.put(str(test_batch.GetResults()))
        except KeyboardInterrupt:
            child_processes_manager.KillAll()
            raise
        except queue.Empty:
            child_processes_manager.KillAll()
            break
        except Exception as e:
            message_queue.put("RunTestsException: %s\n%s" % (repr(e), traceback.format_exc()))
            child_processes_manager.KillAll()
            pass
    child_processes_manager.KillAll()


def SafeDeleteFolder(folder_name):
    while os.path.isdir(folder_name):
        try:
            shutil.rmtree(folder_name)
        except KeyboardInterrupt:
            raise
        except PermissionError:
            pass


def DeleteReplayBuildFolders(folder_num, replay_build_dir, trace_folder):
    for i in range(folder_num):
        folder_name = replay_build_dir + str(i)
        if os.path.isdir(folder_name):
            SafeDeleteFolder(folder_name)


def CreateTraceFolders(folder_num):
    for i in range(folder_num):
        folder_name = TRACE_FOLDER + str(i)
        folder_path = os.path.join(REPLAY_SAMPLE_FOLDER, folder_name)
        if os.path.isdir(folder_path):
            shutil.rmtree(folder_path)
        os.makedirs(folder_path)


def DeleteTraceFolders(folder_num):
    for i in range(folder_num):
        folder_name = TRACE_FOLDER + str(i)
        folder_path = os.path.join(REPLAY_SAMPLE_FOLDER, folder_name)
        if os.path.isdir(folder_path):
            SafeDeleteFolder(folder_path)


def GetPlatformForSkip(platform):
    # yapf: disable
    # we want each pair on one line
    platform_map = { "win32" : "WIN",
                     "linux" : "LINUX" }
    # yapf: enable
    return platform_map.get(platform, "UNKNOWN")


def main(args, platform):
    child_processes_manager = ChildProcessesManager()
    try:
        start_time = time.time()
        # set the number of workers to be cpu_count - 1 (since the main process already takes up a
        # CPU core). Whenever a worker is available, it grabs the next job from the job queue and
        # runs it. The worker closes down when there is no more job.
        worker_count = min(multiprocessing.cpu_count() - 1, args.max_jobs)
        cwd = SetCWDToAngleFolder()

        CreateTraceFolders(worker_count)
        capture_build_dir = os.path.normpath(r"%s/Capture" % args.out_dir)
        returncode, output = child_processes_manager.RunGNGen(args, capture_build_dir, False)
        if returncode != 0:
            logging.error(output)
            child_processes_manager.KillAll()
            return EXIT_FAILURE
        # run ninja to build all tests
        returncode, output = child_processes_manager.RunNinja(args, capture_build_dir,
                                                              args.test_suite, False)
        if returncode != 0:
            logging.error(output)
            child_processes_manager.KillAll()
            return EXIT_FAILURE
        # get a list of tests
        test_path = os.path.join(capture_build_dir, args.test_suite)
        test_list = GetTestsListForFilter(args, test_path, args.gtest_filter)
        test_expectation = TestExpectation(platform)
        test_names = ParseTestNamesFromTestList(test_list, test_expectation,
                                                args.force_run_capture)
        test_expectation_for_list = test_expectation.Filter(test_names)
        # objects created by manager can be shared by multiple processes. We use it to create
        # collections that are shared by multiple processes such as job queue or result list.
        manager = multiprocessing.Manager()
        job_queue = manager.Queue()
        test_batch_num = int(math.ceil(len(test_names) / float(args.batch_count)))

        # put the test batchs into the job queue
        for batch_index in range(test_batch_num):
            batch = TestBatch(args)
            test_index = batch_index
            while test_index < len(test_names):
                batch.AddTest(Test(test_names[test_index]))
                test_index += test_batch_num
            job_queue.put(batch)

        passed_count = 0
        failed_count = 0
        timedout_count = 0
        crashed_count = 0
        compile_failed_count = 0
        skipped_count = 0

        failed_tests = []
        timed_out_tests = []
        crashed_tests = []
        compile_failed_tests = []
        skipped_tests = []


        # result list is created by manager and can be shared by multiple processes. Each
        # subprocess populates the result list with the results of its test runs. After all
        # subprocesses finish, the main process processes the results in the result list.
        # An item in the result list is a tuple with 3 values (testname, result, output).
        # The "result" can take 3 values "Passed", "Failed", "Skipped". The output is the
        # stdout and the stderr of the test appended together.
        result_list = manager.list()
        message_queue = manager.Queue()
        # so that we do not spawn more processes than we actually need
        worker_count = min(worker_count, test_batch_num)
        # spawning and starting up workers
        for worker_id in range(worker_count):
            proc = multiprocessing.Process(
                target=RunTests, args=(args, worker_id, job_queue, result_list, message_queue))
            child_processes_manager.AddWorker(proc)
            proc.start()

        # print out messages from the message queue populated by workers
        # if there is no message, and the elapsed time between now and when the last message is
        # print exceeds TIME_BETWEEN_MESSAGE, prints out a message to signal that tests are still
        # running
        last_message_timestamp = 0
        while child_processes_manager.IsAnyWorkerAlive():
            while not message_queue.empty():
                msg = message_queue.get()
                info(msg)
                last_message_timestamp = time.time()
            cur_time = time.time()
            if cur_time - last_message_timestamp > TIME_BETWEEN_MESSAGE:
                last_message_timestamp = cur_time
                info("Tests are still running. Remaining workers: " + \
                str(child_processes_manager.GetRemainingWorkers()) + \
                ". Unstarted jobs: " + str(job_queue.qsize()))
            time.sleep(1.0)
        child_processes_manager.JoinWorkers()
        while not message_queue.empty():
            msg = message_queue.get()
            logging.warning(msg)
        end_time = time.time()

        # print out results
        logging.info("\n\n\n")
        logging.info("Results:")

        test_results = {}
        flaky_results = []

        for test_batch_result in result_list:
            debug(str(test_batch_result))
            passed_count += len(test_batch_result.passes)
            failed_count += len(test_batch_result.fails)
            timedout_count += len(test_batch_result.timeouts)
            crashed_count += len(test_batch_result.crashes)
            compile_failed_count += len(test_batch_result.compile_fails)
            skipped_count += len(test_batch_result.skips)

            for failed_test in test_batch_result.fails:
                failed_tests.append(failed_test)
                test_results[failed_test] = "Fail"

            for timeout_test in test_batch_result.timeouts:
                timed_out_tests.append(timeout_test)
                test_results[timeout_test] = "Timeout"

            for crashed_test in test_batch_result.crashes:
                crashed_tests.append(crashed_test)
                test_results[crashed_test] = "Crashed"

            for compile_failed_test in test_batch_result.compile_fails:
                compile_failed_tests.append(compile_failed_test)
                test_results[compile_failed_test] = "CompileFailed"

            for skipped_test in test_batch_result.skips:
                skipped_tests.append(skipped_test)
                test_results[skipped_test] = "Skipped"

            for passed_test in test_batch_result.passes:
                if test_expectation.IsFlaky(passed_test):
                    flaky_results.append("  {} (Pass)".format(passed_test))

        test_result = []
        for test, result in sorted(test_results.items()):
            if not test_expectation.IsFlaky(test):
                test_result.append("{} {}\n".format(test, result))
            else:
                flaky_results.append("  {} ({})".format(test, result))

        expected_result = []
        expected_result_map = sorted(test_expectation_for_list.items())
        for test, result in expected_result_map:
            if test in test_names:
                expected_result.append("{} {}\n".format(test, result))

        if len(flaky_results):
            logging.info("\n\nFlaky test(s):")
            for line in flaky_results:
                logging.info(line)

        logging.info("\n\n")
        logging.info("Elapsed time: %.2lf seconds" % (end_time - start_time))
        logging.info(
            "Passed: %d, Comparison Failed: %d, Crashed: %d, CompileFailed %d, Skipped: %d, Timeout: %d"
            % (passed_count, failed_count, crashed_count, compile_failed_count, skipped_count,
               timedout_count))

        result_diff = difflib.unified_diff(
            expected_result,
            test_result,
            fromfile="expected result",
            tofile="obtained result",
            n=0)

        diff_lines = 0
        for line in result_diff:
            if line is not None:
                logging.info(line.rstrip("\n"))
                diff_lines = diff_lines + 1

        if diff_lines == 0:
            retval = EXIT_SUCCESS
        else:
            logging.info("\nFailure: Obtained results differed from expectation")
            retval = EXIT_FAILURE

        # delete generated folders if --keep_temp_files flag is set to false
        if args.purge:
            DeleteTraceFolders(worker_count)
            if os.path.isdir(args.out_dir):
                SafeDeleteFolder(args.out_dir)

        # Try hard to ensure output is finished before ending the test.
        logging.shutdown()
        sys.stdout.flush()
        time.sleep(2.0)
        return retval

    except KeyboardInterrupt:
        child_processes_manager.KillAll()
        return EXIT_FAILURE


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--out-dir',
        default=DEFAULT_OUT_DIR,
        help='Where to build ANGLE for capture and replay. Relative to the ANGLE folder. Default is "%s".'
        % DEFAULT_OUT_DIR)
    # TODO(jmadill): Remove this argument. http://anglebug.com/6102
    parser.add_argument(
        '--use-goma',
        action='store_true',
        help='Use goma for distributed builds. Requires internal access. Off by default.')
    parser.add_argument(
        '--gtest_filter',
        default=DEFAULT_FILTER,
        help='Same as GoogleTest\'s filter argument. Default is "%s".' % DEFAULT_FILTER)
    parser.add_argument(
        '--test-suite',
        default=DEFAULT_TEST_SUITE,
        help='Test suite binary to execute. Default is "%s".' % DEFAULT_TEST_SUITE)
    parser.add_argument(
        '--batch-count',
        default=DEFAULT_BATCH_COUNT,
        type=int,
        help='Number of tests in a batch. Default is %d.' % DEFAULT_BATCH_COUNT)
    parser.add_argument(
        '--keep-temp-files',
        action='store_true',
        help='Whether to keep the temp files and folders. Off by default')
    parser.add_argument('--purge', help='Purge all build directories on exit.')
    parser.add_argument(
        "--goma-dir",
        default="",
        help='Set custom goma directory. Uses the goma in path by default.')
    parser.add_argument(
        "--output-to-file",
        action='store_true',
        help='Whether to write output to a result file. Off by default')
    parser.add_argument(
        "--result-file",
        default=DEFAULT_RESULT_FILE,
        help='Name of the result file in the capture_replay_tests folder. Default is "%s".' %
        DEFAULT_RESULT_FILE)
    parser.add_argument('-v', "--verbose", action='store_true', help='Off by default')
    parser.add_argument(
        "-l",
        "--log",
        default=DEFAULT_LOG_LEVEL,
        help='Controls the logging level. Default is "%s".' % DEFAULT_LOG_LEVEL)
    parser.add_argument(
        '-j',
        '--max-jobs',
        default=DEFAULT_MAX_JOBS,
        type=int,
        help='Maximum number of test processes. Default is %d.' % DEFAULT_MAX_JOBS)
    parser.add_argument(
        '-f',
        '--force-run-capture',
        action='store_true',
        help='Also run tests that are disabled in the expectations by SKIP_FOR_CAPTURE')

    # TODO(jmadill): Remove this argument. http://anglebug.com/6102
    parser.add_argument('--depot-tools-path', default=None, help='Path to depot tools')
    parser.add_argument('--xvfb', action='store_true', help='Run with xvfb.')
    parser.add_argument('--asan', action='store_true', help='Build with ASAN.')
    parser.add_argument(
        '--show-capture-stdout', action='store_true', help='Print test stdout during capture.')
    parser.add_argument('--debug', action='store_true', help='Debug builds (default is Release).')
    args = parser.parse_args()
    if platform == "win32":
        args.test_suite += ".exe"
    if args.output_to_file:
        logging.basicConfig(level=args.log.upper(), filename=args.result_file)
    else:
        logging.basicConfig(level=args.log.upper())

    sys.exit(main(args, GetPlatformForSkip(platform)))
