# Copyright 2019 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Top-level presubmit script for code generation.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details on the presubmit API built into depot_tools.
"""

from subprocess import call

def TestCodeGeneration(input_api, output_api):
    code_gen_path = input_api.os_path.join(input_api.PresubmitLocalPath(),
                                           'scripts/run_code_generation.py')
    cmd_name = 'run_code_generation'
    cmd = [input_api.python_executable, code_gen_path, '--verify-no-dirty']
    test_cmd = input_api.Command(
          name=cmd_name,
          cmd=cmd,
          kwargs={},
          message=output_api.PresubmitError)
    if input_api.verbose:
        print('Running ' + cmd_name)
    return input_api.RunTests([test_cmd])

def CheckChangeOnUpload(input_api, output_api):
    output = TestCodeGeneration(input_api, output_api)

    if not input_api.change.BUG:
        output += [output_api.PresubmitError(
            'Must provide a Bug: line.')]
    return output

def CheckChangeOnCommit(input_api, output_api):
    return []
