#!/usr/bin/python2
#
# Copyright 2016 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# generate_vulkan_layers_json.py:
#   Generate copies of the Vulkan layers JSON files, with no paths, forcing
#   Vulkan to use the default search path to look for layers.

import os, sys, json, glob

if len(sys.argv) != 3:
    print("Usage: " + sys.argv[0] + " <source_dir> <target_dir>")
    sys.exit(1)

source_dir = sys.argv[1]
target_dir = sys.argv[2]

data_key = 'layer'
if 'icd' in source_dir:
    data_key = 'ICD'

if not os.path.isdir(source_dir):
    print(source_dir + " is not a directory.")
    sys.exit(1)

if not os.path.exists(target_dir):
    os.makedirs(target_dir)

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
