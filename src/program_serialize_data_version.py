#!/usr/bin/env python3
#  Copyright 2022 The ANGLE Project Authors. All rights reserved.
#  Use of this source code is governed by a BSD-style license that can be
#  found in the LICENSE file.

# Generate ANGLEShaderProgramVersion.h with hash of files affecting data
# used in serializing/deserializing shader programs.

import hashlib
import os
import argparse

script_path = os.path.dirname(__file__)
script_parent_path = os.path.join(script_path, os.pardir)


def GenerateHashOfAffectedFiles(angle_code_files):
    hash_md5 = hashlib.md5()
    for file in angle_code_files:
        assert (file != "")
        absoluteFilePath = os.path.join(script_parent_path, file)
        with open(absoluteFilePath, 'r') as f:
            for chunk in iter(lambda: f.read(4096), ""):
                hash_md5.update(chunk.encode())
    return hash_md5.hexdigest(), hash_md5.digest_size


parser = argparse.ArgumentParser(description='Generate the file ANGLEShaderProgramVersion.h')
parser.add_argument(
    'output_file',
    help='full path to output file name, stores ANGLE_PROGRAM_VERSION and ANGLE_PROGRAM_VERSION_HASH_SIZE'
)
parser.add_argument(
    'input_file',
    help='full path to input file name, the input file stores a list of program files that GenerateHashOfAffectedFiles will generate ANGLE_PROGRAM_VERSION hash value from'
)
args = parser.parse_args()
output_file = args.output_file
input_file = args.input_file
input_file_absolute_path = os.path.join(script_parent_path, input_file)
with open(input_file_absolute_path, "r") as input_files:
    angle_code_files = input_files.read().split('\n')
#remove the last empty item because of the '\n' in input_file (file $root_gen_dir/angle_code_affecting_program_serialize)
if (len(angle_code_files) > 0):
    assert (angle_code_files.pop() == "")
angle_shader_program_version_hash_result = GenerateHashOfAffectedFiles(angle_code_files)
hfile = open(output_file, 'w')
hfile.write('#define ANGLE_PROGRAM_VERSION "%s"\n' % angle_shader_program_version_hash_result[0])
hfile.write('#define ANGLE_PROGRAM_VERSION_HASH_SIZE %d\n' %
            angle_shader_program_version_hash_result[1])
hfile.close()
