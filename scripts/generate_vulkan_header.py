#!/usr/bin/python
#
# Copyright 2016 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# generate_vulkan_header.py:
#   Workaround problems with Windows and GYP and the Vulkan loader paths.

import os, sys

if len(sys.argv) < 4:
    print("Usage: " + sys.argv[0] + " <layers_source_path> <output_file> <default_vk_layers_path>")
    sys.exit(1)

layers_source_path = sys.argv[1]
output_file = sys.argv[2]
default_vk_layers_path = sys.argv[3].rstrip("\"")

def fixpath(inpath):
    return os.path.relpath(inpath).replace('\\', '/')

def all_paths(inpath):
    return fixpath(inpath) + ";" + fixpath(os.path.join("..", fixpath(inpath)))

with open(output_file, "w") as outfile:
    outfile.write("#define LAYERS_SOURCE_PATH \"" + all_paths(layers_source_path) + "\"\n")
    outfile.write("#define DEFAULT_VK_LAYERS_PATH \"" + all_paths(default_vk_layers_path) + "\"\n")
