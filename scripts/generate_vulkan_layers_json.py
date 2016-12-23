#!/usr/bin/python
#
# Copyright 2016 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# generate_vulkan_layers_json.py:
#   Generate copies of the Vulkan layers JSON files, with no paths, forcing
#   Vulkan to use the default search path to look for layers.

import os, sys, json, glob

if len(sys.argv) < 3:
    print("Usage: " + sys.argv[0] + " <source_dir> <target_dir>")
    sys.exit(1)

source_dir = sys.argv[1]
target_dir = sys.argv[2]

if not os.path.isdir(source_dir):
    print(source_dir + " is not a directory.")
    sys.exit(1)

if not os.path.exists(target_dir):
    os.makedirs(target_dir)

for json_fname in glob.glob(os.path.join(source_dir, "*.json")):
    with open(json_fname) as infile:
        data = json.load(infile)

        # update the path
        prev_name = os.path.basename(data['layer']['library_path'])
        data['layer']['library_path'] = prev_name

        target_fname = os.path.join(target_dir, os.path.basename(json_fname))
        with open(target_fname, "w") as outfile:
            json.dump(data, outfile)
