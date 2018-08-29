#!/usr/bin/python2
#
# Copyright 2016 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# generate_vulkan_layers_json.py:
#   Generate copies of the Vulkan layers JSON files, with no paths, forcing
#   Vulkan to use the default search path to look for layers.

import os, sys, json, glob, platform

if len(sys.argv) != 3:
    print("Usage: " + sys.argv[0] + " <source_dir> <target_dir>")
    sys.exit(1)

source_dir = sys.argv[1]
target_dir = sys.argv[2]
angle_root_dir = os.path.dirname(sys.path[0])

data_key = 'layer'
if 'icd' in source_dir:
    data_key = 'ICD'

if not os.path.isdir(source_dir):
    print(source_dir + " is not a directory.")
    sys.exit(1)

if not os.path.exists(target_dir):
    os.makedirs(target_dir)

# Copy the *.json files from source dir to target dir
for json_fname in glob.glob(os.path.join(source_dir, "*.json")):
    with open(json_fname) as infile:
        data = json.load(infile)

        # update the path
        if not data_key in data:
            raise Exception("Could not find '" + data_key + "' key in " + json_fname)

        # The standard validation layer has no library path.
        if 'library_path' in data[data_key]:
            prev_name = os.path.basename(data[data_key]['library_path'])
            data[data_key]['library_path'] = prev_name

        target_fname = os.path.join(target_dir, os.path.basename(json_fname))
        with open(target_fname, "w") as outfile:
            json.dump(data, outfile)

# Get the Vulkan version from the vulkan_core.h file
vk_header_filename = os.path.join(angle_root_dir, "third_party", "vulkan-headers", "src", "include", "vulkan", "vulkan_core.h")
vk_version = '83'
with open(vk_header_filename) as vk_header_file:
    for line in vk_header_file:
        if line.startswith("#define VK_HEADER_VERSION"):
            vk_version = line.split()[-1]
            break

# Set json file prefix and suffix for generating files below, default to Linux
relative_path_prefix = "../lib"
file_type_suffix = ".so"
if 'Windows' == platform.system():
    relative_path_prefix = "..\\\\"
    file_type_suffix = ".dll"

# For each *.json.in template files in source dir generate actual json file in target dir
for json_in_name in glob.glob(os.path.join(source_dir, "*.json.in")):
    json_in_fname = os.path.basename(json_in_name)
    layer_name = json_in_fname[:-8] # Kill ".json.in" from end of filename
    layer_lib_name = '%s%s' % (layer_name, file_type_suffix)
    json_out_fname = os.path.join(target_dir, json_in_fname[:-3])
    with open(json_out_fname,'w') as json_out_file:
        with open(json_in_name) as infile:
            for line in infile:
                line = line.replace('@RELATIVE_LAYER_BINARY@', ('%s%s' % (relative_path_prefix, layer_lib_name)))
                line = line.replace('@VK_VERSION@', ('1.1.%s' % (vk_version)))
                json_out_file.write(line)
