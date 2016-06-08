#!/usr/bin/python
#
# Copyright 2016 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# update_canary_angle.py:
#   Helper script that copies Windows ANGLE DLLs into the Canary
#   application directory. Much faster than compiling Chrome from
#   source. The script checks for the most recent DLLs in a set of
#   search paths, and copies that into the most recent Canary
#   binary folder. Only works on Windows.

import sys, os, shutil

# Set of search paths.
source_paths = [
    os.path.join('..', 'build', 'Debug_x64'),
    os.path.join('..', 'build', 'Debug_Win32'),
    os.path.join('..', 'build', 'Release_x64'),
    os.path.join('..', 'build', 'Release_Win32'),
    os.path.join('..', 'out', 'Debug'),
    os.path.join('..', 'out', 'Debug_x64'),
    os.path.join('..', 'out', 'Release'),
    os.path.join('..', 'out', 'Release_x64'),
]

script_dir = os.path.dirname(sys.argv[0])

# Default Canary installation path.
chrome_folder = os.path.join(os.environ['LOCALAPPDATA'], 'Google', 'Chrome SxS', 'Application')

# Find the most recent ANGLE DLLs
binary_name = 'libGLESv2.dll'
newest_folder = None
newest_mtime = None
for path in source_paths:
    binary_path = os.path.join(script_dir, path, binary_name)
    if os.path.exists(binary_path):
        binary_mtime = os.path.getmtime(binary_path)
        if (newest_folder is None) or (binary_mtime > newest_mtime):
            newest_folder = os.path.join(script_dir, path)
            newest_mtime = binary_mtime

if newest_folder is None:
    sys.exit("Could not find ANGLE DLLs!")

source_folder = newest_folder

# Is a folder a chrome binary directory?
def is_chrome_bin(str):
    chrome_file = os.path.join(chrome_folder, str)
    return os.path.isdir(chrome_file) and all([char.isdigit() or char == '.' for char in str])

sorted_chrome_bins = sorted([folder for folder in os.listdir(chrome_folder) if is_chrome_bin(folder)], reverse=True)

dest_folder = os.path.join(chrome_folder, sorted_chrome_bins[0])

print('Copying DLLs from ' + source_folder + ' to ' + dest_folder + '.')

# Translator.dll appears if we build in component=shared_library mode.
for dll in ['libGLESv2.dll', 'libEGL.dll', 'translator.dll']:
    src = os.path.join(source_folder, dll)
    if os.path.exists(src):
        # Make a backup of the original unmodified DLLs if they are present.
        backup = os.path.join(source_folder, dll + '.backup')
        if not os.path.exists(backup):
            shutil.copyfile(src, backup)
        shutil.copyfile(src, os.path.join(dest_folder, dll))
