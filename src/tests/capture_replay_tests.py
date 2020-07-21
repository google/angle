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
# Command line arguments:
# --capture_build_dir: specifies capture build directory relative to angle folder.
# Default is out/CaptureDebug
# --replay_build_dir: specifies replay build directory relative to angle folder.
# Default is out/ReplayDebug
# --use_goma: uses goma for compiling and linking test. Off by default
# --gtest_filter: same as gtest_filter of Google's test framework. Default is */ES2_Vulkan
# --test_suite: test suite to execute on. Default is angle_end2end_tests
# --batch_count: number of tests in a batch. Default is 8
# --keep_temp_files: whether to keep the temp files and folders. Off by default
# --goma_dir: set goma directory. Default is the system's default
# --output_to_file: whether to write output to output.txt. Off by default

import argparse
import distutils.util
import math
import multiprocessing
import os
import queue
import shlex
import shutil
import subprocess

import sys
import time

from sys import platform

DEFAULT_CAPTURE_BUILD_DIR = "out/CaptureTest"  # relative to angle folder
DEFAULT_REPLAY_BUILD_DIR = "out/ReplayTest"  # relative to angle folder
DEFAULT_FILTER = "*/ES2_Vulkan"
DEFAULT_TEST_SUITE = "angle_end2end_tests"
REPLAY_SAMPLE_FOLDER = "src/tests/capture_replay_tests"  # relative to angle folder
DEFAULT_BATCH_COUNT = 64  # number of tests batched together
TRACE_FILE_SUFFIX = "_capture_context1"  # because we only deal with 1 context right now
RESULT_TAG = "*RESULT"
TIME_BETWEEN_MESSAGE = 20  # in seconds
SUBPROCESS_TIMEOUT = 120  # in seconds

switch_case_without_return_template = \
"""        case {case}:
            {namespace}::{call}({params});
            break;
"""

switch_case_with_return_template = \
"""        case {case}:
            return {namespace}::{call}({params});
"""

default_case_without_return_template = \
"""        default:
            break;"""
default_case_with_return_template = \
"""        default:
            return {default_val};"""

switch_statement_template = \
"""switch(test)
    {{
{switch_cases}{default_case}
    }}
"""

test_trace_info_init_template = \
"""    {{
        "{namespace}",
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
        {namespace}::kIsBinaryDataCompressed
    }},
"""

composite_h_file_template = \
"""#pragma once
#include <vector>
#include <string>

{trace_headers}

using DecompressCallback = uint8_t *(*)(const std::vector<uint8_t> &);

void SetupContext1Replay(uint32_t test);
void ReplayContext1Frame(uint32_t test, uint32_t frameIndex);
void ResetContext1Replay(uint32_t test);
std::vector<uint8_t> GetSerializedContext1StateData(uint32_t test, uint32_t frameIndex);
void SetBinaryDataDecompressCallback(uint32_t test, DecompressCallback callback);
void SetBinaryDataDir(uint32_t test, const char *dataDir);

struct TestTraceInfo {{
    std::string testName;
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
}};

extern std::vector<TestTraceInfo> testTraceInfos;
"""

composite_cpp_file_template = \
"""#include "{h_filename}"

std::vector<TestTraceInfo> testTraceInfos =
{{
{test_trace_info_inits}
}};
void SetupContext1Replay(uint32_t test)
{{
    {setup_context1_replay_switch_statement}
}}

void ReplayContext1Frame(uint32_t test, uint32_t frameIndex)
{{
    {replay_context1_replay_switch_statement}
}}

void ResetContext1Replay(uint32_t test)
{{
    {reset_context1_replay_switch_statement}
}}

std::vector<uint8_t> GetSerializedContext1StateData(uint32_t test, uint32_t frameIndex)
{{
    {get_serialized_context1_state_data_switch_statement}
}}

void SetBinaryDataDecompressCallback(uint32_t test, DecompressCallback callback)
{{
    {set_binary_data_decompress_callback_switch_statement}
}}

void SetBinaryDataDir(uint32_t test, const char *dataDir)
{{
    {set_binary_data_dir_switch_statement}
}}
"""


class Logger():

    def __init__(self, output_to_file):
        self.output_to_file_string = ""
        self.output_to_file = output_to_file

    def Log(self, string):
        if string != None:
            print(string)
            self.output_to_file_string += string
            self.output_to_file_string += "\n"

    def End(self):
        if self.output_to_file:
            f = open(os.path.join(REPLAY_SAMPLE_FOLDER, "results.txt"), "w")
            f.write(self.output_to_file_string)
            f.close()


class SubProcess():

    def __init__(self, command, to_main_stdout):
        parsed_command = shlex.split(command)
        # shell=False so that only 1 subprocess is spawned.
        # if shell=True, a shell probess is spawned, which in turn spawns the process running
        # the command. Since we do not have a handle to the 2nd process, we cannot terminate it.
        if not to_main_stdout:
            self.proc_handle = subprocess.Popen(
                parsed_command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=False)
        else:
            self.proc_handle = subprocess.Popen(parsed_command, shell=False)

    def BlockingRun(self, timeout):
        output = self.proc_handle.communicate(timeout=timeout)[0]
        if output:
            output = output.decode("utf-8")
        return self.proc_handle.returncode, output

    def Pid(self):
        return self.proc_handle.pid

    def Kill(self):
        self.proc_handle.terminate()
        self.proc_handle.wait()


# class that manages all child processes of a process. Any process thats spawns subprocesses
# should have this. This object is created inside the main process, and each worker process.
class ChildProcessesManager():

    def __init__(self):
        # a dictionary of Subprocess, with pid as key
        self.subprocesses = {}
        # list of Python multiprocess.Process handles
        self.workers = []

    def CreateSubprocess(self, command, to_main_stdout):
        subprocess = SubProcess(command, to_main_stdout)
        self.subprocesses[subprocess.Pid()] = subprocess
        return subprocess.Pid()

    def RunSubprocessBlocking(self, subprocess_id, timeout=None):
        assert subprocess_id in self.subprocesses
        try:
            returncode, output = self.subprocesses[subprocess_id].BlockingRun(timeout)
            self.RemoveSubprocess(subprocess_id)
            return returncode, output
        except KeyboardInterrupt:
            raise
        except subprocess.TimeoutExpired as e:
            self.RemoveSubprocess(subprocess_id)
            return -2, str(e)
        except Exception as e:
            self.RemoveSubprocess(subprocess_id)
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


def CreateGNGenCommand(gn_path, build_dir, arguments):
    command = '"' + gn_path + '"' + ' gen --args="'
    is_first_argument = True
    for argument in arguments:
        if is_first_argument:
            is_first_argument = False
        else:
            command += ' '
        command += argument[0]
        command += '='
        command += argument[1]
    command += '" '
    command += build_dir
    return command


def CreateAutoninjaCommand(autoninja_path, build_dir, target):
    command = '"' + autoninja_path + '" '
    command += target
    command += " -C "
    command += build_dir
    return command


def GetGnAndAutoninjaAbsolutePaths():
    # get gn/autoninja absolute path because subprocess with shell=False doesn't look
    # into the PATH environment variable on Windows
    depot_tools_name = "depot_tools"
    if platform == "win32":
        paths = os.environ["PATH"].split(";")
    else:
        paths = os.environ["PATH"].split(":")
    for path in paths:
        if path.endswith(depot_tools_name):
            if platform == "win32":
                return os.path.join(path, "gn.bat"), os.path.join(path, "autoninja.bat")
            else:
                return os.path.join(path, "gn"), os.path.join(path, "autoninja")
    return "", ""


def GetTestNamesAndParamsCommand(test_exec_path, filter="*"):
    return '"' + test_exec_path + '" --gtest_list_tests --gtest_filter=' + filter


def ProcessGetTestNamesAndParamsCommandOutput(output):
    output_lines = output.splitlines()
    tests = []
    last_testcase_name = ""
    test_name_splitter = "# GetParam() ="
    for line in output_lines:
        if test_name_splitter in line:
            # must be a test name line
            test_name_and_params = line.split(test_name_splitter)
            tests.append((last_testcase_name + test_name_and_params[0].strip(), \
                test_name_and_params[1].strip()))
        else:
            # gtest_list_tests returns the test in this format
            # test case
            #    test name1
            #    test name2
            # Need to remember the last test case name to append to the test name
            last_testcase_name = line
    return tests


def WriteGeneratedSwitchStatements(tests, call, params, returns=False, default_val=""):
    switch_cases = "".join(
        [
            switch_case_with_return_template.format(
                case=str(i), namespace=tests[i].GetLabel(), call=call, params=params) if returns
            else switch_case_without_return_template.format(
                case=str(i), namespace=tests[i].GetLabel(), call=call, params=params) \
            for i in range(len(tests))
        ]
    )
    default_case = default_case_with_return_template.format(default_val=default_val) if returns \
        else default_case_without_return_template
    return switch_statement_template.format(switch_cases=switch_cases, default_case=default_case)


def WriteAngleTraceGLHeader():
    f = open(os.path.join(REPLAY_SAMPLE_FOLDER, "angle_trace_gl.h"), "w")
    f.write("""#ifndef ANGLE_TRACE_GL_H_
#define ANGLE_TRACE_GL_H_

#include "util/util_gl.h"

#endif  // ANGLE_TRACE_GL_H_
""")
    f.close()


class Test():

    def __init__(self, full_test_name, params):
        self.full_test_name = full_test_name
        self.params = params

    def __str__(self):
        return self.full_test_name + " Params: " + self.params

    def Run(self, test_exe_path, child_processes_manager):
        os.environ["ANGLE_CAPTURE_LABEL"] = self.GetLabel()
        command = '"' + test_exe_path + '" --gtest_filter=' + self.full_test_name
        capture_proc_id = child_processes_manager.CreateSubprocess(command, False)
        return child_processes_manager.RunSubprocessBlocking(capture_proc_id, SUBPROCESS_TIMEOUT)

    def GetLabel(self):
        return self.full_test_name.replace(".", "_").replace("/", "_")


class TestBatch():

    def __init__(self, use_goma, batch_count, keep_temp_files, goma_dir):
        self.use_goma = use_goma
        self.tests = []
        self.batch_count = batch_count
        self.keep_temp_files = keep_temp_files
        self.goma_dir = goma_dir

    def Run(self, test_exe_path, trace_folder_path, child_processes_manager):
        os.environ["ANGLE_CAPTURE_ENABLED"] = "1"
        if not self.keep_temp_files:
            ClearFolderContent(trace_folder_path)
        return [test.Run(test_exe_path, child_processes_manager) for test in self.tests]

    def BuildReplay(self, gn_path, autoninja_path, build_dir, trace_dir, replay_exec,
                    trace_folder_path, composite_file_id, tests, child_processes_manager):
        # write gni file that holds all the traces files in a list
        self.CreateGNIFile(trace_folder_path, composite_file_id, tests)
        # write header and cpp composite files, which glue the trace files with
        # CaptureReplayTests.cpp
        self.CreateTestsCompositeFiles(trace_folder_path, composite_file_id, tests)
        if not os.path.isfile(os.path.join(build_dir, "args.gn")):
            gn_args = [("use_goma", self.use_goma), ("angle_build_capture_replay_tests", "true"),
                       ("angle_capture_replay_test_trace_dir", '\\"' + trace_dir + '\\"'),
                       ("angle_with_capture_by_default", "false"),
                       ("angle_capture_replay_composite_file_id", str(composite_file_id))]
            if self.goma_dir != "":
                gn_args.append(("goma_dir", self.goma_dir))
            gn_command = CreateGNGenCommand(gn_path, build_dir, gn_args)
            gn_proc_id = child_processes_manager.CreateSubprocess(gn_command, False)
            returncode, output = child_processes_manager.RunSubprocessBlocking(gn_proc_id)
            if returncode != 0:
                return returncode, output

        autoninja_command = CreateAutoninjaCommand(autoninja_path, build_dir, replay_exec)
        autoninja_proc_id = child_processes_manager.CreateSubprocess(autoninja_command, False)
        returncode, output = child_processes_manager.RunSubprocessBlocking(autoninja_proc_id)
        if returncode != 0:
            return returncode, output
        return 0, "Built replay of " + str(self)

    def RunReplay(self, replay_exe_path, child_processes_manager):
        os.environ["ANGLE_CAPTURE_ENABLED"] = "0"
        command = '"' + replay_exe_path + '"'
        replay_proc_id = child_processes_manager.CreateSubprocess(command, False)
        returncode, output = child_processes_manager.RunSubprocessBlocking(
            replay_proc_id, SUBPROCESS_TIMEOUT)
        return returncode, output

    def AddTest(self, test):
        assert len(self.tests) <= self.batch_count
        self.tests.append(test)

    # gni file, which holds all the sources for a replay application
    def CreateGNIFile(self, trace_folder_path, composite_file_id, tests):
        capture_sources = []
        for test in tests:
            label = test.GetLabel()
            trace_files = [label + TRACE_FILE_SUFFIX + ".h", label + TRACE_FILE_SUFFIX + ".cpp"]
            try:
                # reads from {label}_capture_context1_files.txt and adds the traces files recorded
                # in there to the list of trace files
                f = open(os.path.join(trace_folder_path, label + TRACE_FILE_SUFFIX + "_files.txt"))
                trace_files += f.read().splitlines()
                f.close()
            except IOError:
                continue
            capture_sources += trace_files
        f = open(os.path.join(trace_folder_path, "traces" + str(composite_file_id) + ".gni"), "w")
        f.write("generated_sources = [\n")
        # write the list of trace files to the gni file
        for filename in capture_sources:
            f.write('    "' + filename + '",\n')
        f.write("]")
        f.close()

    # header and cpp composite files, which glue the trace files with CaptureReplayTests.cpp
    def CreateTestsCompositeFiles(self, trace_folder_path, composite_file_id, tests):
        # write CompositeTests header file
        h_filename = "CompositeTests" + str(composite_file_id) + ".h"
        h_file = open(os.path.join(trace_folder_path, h_filename), "w")

        include_header_template = '#include "{header_file_path}.h"\n'
        trace_headers = "".join([
            include_header_template.format(header_file_path=test.GetLabel() + TRACE_FILE_SUFFIX)
            for test in tests
        ])
        h_file.write(composite_h_file_template.format(trace_headers=trace_headers))
        h_file.close()

        # write CompositeTests cpp file
        cpp_file = open(
            os.path.join(trace_folder_path, "CompositeTests" + str(composite_file_id) + ".cpp"),
            "w")

        test_trace_info_inits = "".join([
            test_trace_info_init_template.format(namespace=tests[i].GetLabel())
            for i in range(len(tests))
        ])
        setup_context1_replay_switch_statement = WriteGeneratedSwitchStatements(
            tests, "SetupContext1Replay", "")
        replay_context1_replay_switch_statement = WriteGeneratedSwitchStatements(
            tests, "ReplayContext1Frame", "frameIndex")
        reset_context1_replay_switch_statement = WriteGeneratedSwitchStatements(
            tests, "ResetContext1Replay", "")
        get_serialized_context1_state_data_switch_statement = WriteGeneratedSwitchStatements(
            tests, "GetSerializedContext1StateData", "frameIndex", True, "{}")
        set_binary_data_decompress_callback_switch_statement = WriteGeneratedSwitchStatements(
            tests, "SetBinaryDataDecompressCallback", "callback")
        set_binary_data_dir_switch_statement = WriteGeneratedSwitchStatements(
            tests, "SetBinaryDataDir", "dataDir")
        cpp_file.write(
            composite_cpp_file_template.format(
                h_filename=h_filename,
                test_trace_info_inits=test_trace_info_inits,
                setup_context1_replay_switch_statement=setup_context1_replay_switch_statement,
                replay_context1_replay_switch_statement=replay_context1_replay_switch_statement,
                reset_context1_replay_switch_statement=reset_context1_replay_switch_statement,
                get_serialized_context1_state_data_switch_statement=get_serialized_context1_state_data_switch_statement,
                set_binary_data_decompress_callback_switch_statement=set_binary_data_decompress_callback_switch_statement,
                set_binary_data_dir_switch_statement=set_binary_data_dir_switch_statement))
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


def ClearFolderContent(path):
    all_files = []
    for f in os.listdir(path):
        if os.path.isfile(os.path.join(path, f)):
            os.remove(os.path.join(path, f))


def CanRunReplay(label, path):
    required_trace_files = {
        label + TRACE_FILE_SUFFIX + ".h", label + TRACE_FILE_SUFFIX + ".cpp",
        label + TRACE_FILE_SUFFIX + "_files.txt"
    }
    required_trace_files_count = 0
    frame_files_count = 0
    for f in os.listdir(path):
        if not os.path.isfile(os.path.join(path, f)):
            continue
        if f in required_trace_files:
            required_trace_files_count += 1
        elif f.startswith(label + TRACE_FILE_SUFFIX + "_frame"):
            frame_files_count += 1
        elif f.startswith(label +
                          TRACE_FILE_SUFFIX[:-1]) and not f.startswith(label + TRACE_FILE_SUFFIX):
            # if trace_files of another context exists, then the test creates multiple contexts
            return False
    # angle_capture_context1.angledata.gz can be missing
    return required_trace_files_count == len(required_trace_files) and frame_files_count >= 1


def SetCWDToAngleFolder():
    angle_folder = "angle"
    cwd = os.path.dirname(os.path.abspath(__file__))
    cwd = cwd.split(angle_folder)[0] + angle_folder
    os.chdir(cwd)
    return cwd


def RunTests(job_queue, gn_path, autoninja_path, capture_build_dir, replay_build_dir, test_exec,
             replay_exec, trace_dir, result_list, message_queue):
    trace_folder_path = os.path.join(REPLAY_SAMPLE_FOLDER, trace_dir)
    test_exec_path = os.path.join(capture_build_dir, test_exec)
    replay_exec_path = os.path.join(replay_build_dir, replay_exec)

    os.environ["ANGLE_CAPTURE_OUT_DIR"] = trace_folder_path
    child_processes_manager = ChildProcessesManager()
    # used to differentiate between multiple composite files when there are multiple test batchs
    # running on the same worker and --deleted_trace is set to False
    composite_file_id = 1
    while not job_queue.empty():
        try:
            test_batch = job_queue.get()
            message_queue.put("Running " + str(test_batch))
            test_results = test_batch.Run(test_exec_path, trace_folder_path,
                                          child_processes_manager)
            continued_tests = []
            for i in range(len(test_results)):
                returncode = test_results[i][0]
                output = test_results[i][1]
                if returncode != 0 or not CanRunReplay(test_batch[i].GetLabel(),
                                                       trace_folder_path):
                    if returncode == -2:
                        result_list.append(
                            (test_batch[i].full_test_name, "Timeout",
                             "Skipping replay since capture timed out. STDOUT: " + output))
                        message_queue.put("Timeout " + test_batch[i].full_test_name)
                    elif returncode == -1:
                        result_list.append(
                            (test_batch[i].full_test_name, "Skipped",
                             "Skipping replay since capture has crashed. STDOUT: " + output))
                        message_queue.put("Skip " + test_batch[i].full_test_name)
                    else:
                        result_list.append(
                            (test_batch[i].full_test_name, "Skipped",
                             "Skipping replay since capture didn't produce appropriate files."))
                        message_queue.put("Skip " + test_batch[i].full_test_name)
                else:
                    # otherwise, adds it to the list of continued tests
                    continued_tests.append(test_batch[i])
            if len(continued_tests) == 0:
                continue

            returncode, output = test_batch.BuildReplay(gn_path, autoninja_path, replay_build_dir,
                                                        trace_dir, replay_exec, trace_folder_path,
                                                        composite_file_id, continued_tests,
                                                        child_processes_manager)
            if test_batch.keep_temp_files:
                composite_file_id += 1
            if returncode != 0:
                for test in continued_tests:
                    result_list.append(
                        (test.full_test_name, "Skipped",
                         "Skipping batch replays since failing to build batch replays. STDOUT: " +
                         output))
                    message_queue.put("Skip " + test.full_test_name)
                continue
            returncode, output = test_batch.RunReplay(replay_exec_path, child_processes_manager)
            if returncode != 0:
                if returncode == -2:
                    for test in continued_tests:
                        result_list.append(
                            (test.full_test_name, "Timeout",
                             "Timeout batch run. STDOUT: " + output + str(returncode)))
                        message_queue.put("Timeout " + test.full_test_name)
                else:
                    for test in continued_tests:
                        result_list.append(
                            (test.full_test_name, "Failed",
                             "Failing batch run. STDOUT: " + output + str(returncode)))
                        message_queue.put("Fail " + test.full_test_name)
            else:
                output_lines = output.splitlines()
                for output_line in output_lines:
                    words = output_line.split(" ")
                    if len(words) == 3 and words[0] == RESULT_TAG:
                        if int(words[2]) == 0:
                            result_list.append((words[1], "Passed", ""))
                            message_queue.put("Pass " + words[1])
                        else:
                            result_list.append((words[1], "Failed", ""))
                            message_queue.put("Fail " + words[1])
        except KeyboardInterrupt:
            child_processes_manager.KillAll()
            raise
        except queue.Empty:
            child_processes_manager.KillAll()
            break
        except Exception as e:
            message_queue.put(str(e))
            child_processes_manager.KillAll()
            pass
    child_processes_manager.KillAll()


def CreateReplayBuildFolders(folder_num, replay_build_dir):
    for i in range(folder_num):
        replay_build_dir_name = replay_build_dir + str(i)
        if os.path.isdir(replay_build_dir_name):
            shutil.rmtree(replay_build_dir_name)
        os.makedirs(replay_build_dir_name)


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


def CreateTraceFolders(folder_num, trace_folder):
    for i in range(folder_num):
        folder_name = trace_folder + str(i)
        folder_path = os.path.join(REPLAY_SAMPLE_FOLDER, folder_name)
        if os.path.isdir(folder_path):
            shutil.rmtree(folder_path)
        os.makedirs(folder_path)


def DeleteTraceFolders(folder_num, trace_folder):
    for i in range(folder_num):
        folder_name = trace_folder + str(i)
        folder_path = os.path.join(REPLAY_SAMPLE_FOLDER, folder_name)
        if os.path.isdir(folder_path):
            SafeDeleteFolder(folder_path)


def main(capture_build_dir, replay_build_dir, use_goma, gtest_filter, test_exec, batch_count,
         keep_temp_files, goma_dir, output_to_file):
    logger = Logger(output_to_file)
    child_processes_manager = ChildProcessesManager()
    try:
        start_time = time.time()
        # set the number of workers to be cpu_count - 1 (since the main process already takes up a
        # CPU core). Whenever a worker is available, it grabs the next job from the job queue and
        # runs it. The worker closes down when there is no more job.
        worker_count = multiprocessing.cpu_count() - 1
        cwd = SetCWDToAngleFolder()
        # create traces and build folders
        trace_folder = "traces"
        if not os.path.isdir(capture_build_dir):
            os.makedirs(capture_build_dir)
        CreateReplayBuildFolders(worker_count, replay_build_dir)
        CreateTraceFolders(worker_count, trace_folder)
        WriteAngleTraceGLHeader()
        replay_exec = "capture_replay_tests"
        if platform == "win32":
            test_exec += ".exe"
            replay_exec += ".exe"
        gn_path, autoninja_path = GetGnAndAutoninjaAbsolutePaths()
        if gn_path == "" or autoninja_path == "":
            logger.Log("No gn or autoninja found on system")
            logger.End()
            return
        # generate gn files
        gn_args = [("use_goma", use_goma), ("angle_with_capture_by_default", "true")]
        if goma_dir != "":
            gn_args.append(("goma_dir", goma_dir))
        gn_command = CreateGNGenCommand(gn_path, capture_build_dir, gn_args)
        gn_proc_id = child_processes_manager.CreateSubprocess(gn_command, True)
        returncode, output = child_processes_manager.RunSubprocessBlocking(gn_proc_id)
        if returncode != 0:
            logger.Log(output)
            logger.End()
            child_processes_manager.KillAll()
            return
        # run autoninja to build all tests
        autoninja_command = CreateAutoninjaCommand(autoninja_path, capture_build_dir, test_exec)
        autoninja_proc_id = child_processes_manager.CreateSubprocess(autoninja_command, True)
        returncode, output = child_processes_manager.RunSubprocessBlocking(autoninja_proc_id)
        if returncode != 0:
            logger.Log(output)
            logger.End()
            child_processes_manager.KillAll()
            return
        # get a list of tests
        get_tests_command = GetTestNamesAndParamsCommand(
            os.path.join(capture_build_dir, test_exec), gtest_filter)
        get_tests_command_proc_id = child_processes_manager.CreateSubprocess(
            get_tests_command, False)
        returncode, output = child_processes_manager.RunSubprocessBlocking(
            get_tests_command_proc_id)
        if returncode != 0:
            logger.Log(output)
            logger.End()
            child_processes_manager.KillAll()
            return
        test_names_and_params = ProcessGetTestNamesAndParamsCommandOutput(output)
        # objects created by manager can be shared by multiple processes. We use it to create
        # collections that are shared by multiple processes such as job queue or result list.
        manager = multiprocessing.Manager()
        job_queue = manager.Queue()
        test_batch_num = int(math.ceil(len(test_names_and_params) / float(batch_count)))

        # put the test batchs into the job queue
        for batch_index in range(test_batch_num):
            batch = TestBatch(use_goma, batch_count, keep_temp_files, goma_dir)
            for test_in_batch_index in range(batch.batch_count):
                test_index = batch_index * batch.batch_count + test_in_batch_index
                if test_index >= len(test_names_and_params):
                    break
                batch.AddTest(
                    Test(test_names_and_params[test_index][0],
                         test_names_and_params[test_index][1]))
            job_queue.put(batch)

        # set the static environment variables that do not change throughout the script run
        environment_vars = [("ANGLE_CAPTURE_FRAME_END", "100"),
                            ("ANGLE_CAPTURE_SERIALIZE_STATE", "1")]
        for environment_var in environment_vars:
            os.environ[environment_var[0]] = environment_var[1]

        passed_count = 0
        failed_count = 0
        skipped_count = 0
        timeout_count = 0
        failed_tests = []
        skipped_tests = []
        timeout_tests = []

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
        for i in range(worker_count):
            proc = multiprocessing.Process(
                target=RunTests,
                args=(job_queue, gn_path, autoninja_path, capture_build_dir,
                      replay_build_dir + str(i), test_exec, replay_exec, trace_folder + str(i),
                      result_list, message_queue))
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
                logger.Log(msg)
                last_message_timestamp = time.time()
            cur_time = time.time()
            if cur_time - last_message_timestamp > TIME_BETWEEN_MESSAGE:
                last_message_timestamp = cur_time
                logger.Log("Tests are still running. Remaining workers: " + \
                str(child_processes_manager.GetRemainingWorkers()) + \
                ". Unstarted jobs: " + str(job_queue.qsize()))
            time.sleep(1.0)
        child_processes_manager.JoinWorkers()
        while not message_queue.empty():
            msg = message_queue.get()
            logger.Log(msg)
        # delete the static environment variables
        for environment_var in environment_vars:
            del os.environ[environment_var[0]]
        end_time = time.time()

        # print out results
        logger.Log("\n\n\n")
        logger.Log("Results:")
        for result in result_list:
            output_string = result[1] + ": " + result[0] + ". "
            if result[1] == "Skipped":
                output_string += result[2]
                skipped_tests.append(result[0])
                skipped_count += 1
            elif result[1] == "Failed":
                output_string += result[2]
                failed_tests.append(result[0])
                failed_count += 1
            elif result[1] == "Timeout":
                output_string += result[2]
                timeout_tests.append(result[0])
                timeout_count += 1
            else:
                passed_count += 1
            logger.Log(output_string)

        logger.Log("\n\n")
        logger.Log("Elapsed time: " + str(end_time - start_time) + " seconds")
        logger.Log("Passed: "+ str(passed_count) + " Failed: " + str(failed_count) + \
        " Skipped: " + str(skipped_count) + " Timeout: " + str(timeout_count))
        logger.Log("Failed tests:")
        for failed_test in failed_tests:
            logger.Log("\t" + failed_test)
        logger.Log("Skipped tests:")
        for skipped_test in skipped_tests:
            logger.Log("\t" + skipped_test)
        logger.Log("Timeout tests:")
        for timeout_test in timeout_tests:
            logger.Log("\t" + timeout_test)

        # delete generated folders if --keep_temp_files flag is set to false
        if not keep_temp_files:
            os.remove(os.path.join(REPLAY_SAMPLE_FOLDER, "angle_trace_gl.h"))
            DeleteTraceFolders(worker_count, trace_folder)
            DeleteReplayBuildFolders(worker_count, replay_build_dir, trace_folder)
        if not keep_temp_files and os.path.isdir(capture_build_dir):
            SafeDeleteFolder(capture_build_dir)
        logger.End()
    except KeyboardInterrupt:
        child_processes_manager.KillAll()
        logger.End()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--capture_build_dir', default=DEFAULT_CAPTURE_BUILD_DIR)
    parser.add_argument('--replay_build_dir', default=DEFAULT_REPLAY_BUILD_DIR)
    parser.add_argument('--use_goma', default="false")
    parser.add_argument('--gtest_filter', default=DEFAULT_FILTER)
    parser.add_argument('--test_suite', default=DEFAULT_TEST_SUITE)
    parser.add_argument('--batch_count', default=DEFAULT_BATCH_COUNT)
    parser.add_argument('--keep_temp_files', default="false")
    parser.add_argument("--goma_dir", default="")
    parser.add_argument("--output_to_file", default="false")
    args = parser.parse_args()
    main(args.capture_build_dir, args.replay_build_dir, args.use_goma,
         args.gtest_filter, args.test_suite, int(args.batch_count),
         distutils.util.strtobool(args.keep_temp_files), args.goma_dir,
         distutils.util.strtobool(args.output_to_file))
