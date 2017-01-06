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
    print("Usage: " + sys.argv[0] + " <json_source_path> <output_file> <product_dir>")
    sys.exit(1)

layers_source_path = sys.argv[1]
output_file = sys.argv[2]
product_dir = sys.argv[3].rstrip("\"")

def fixpath(inpath):
    return os.path.relpath(inpath, product_dir).replace('\\', '/')

with open(output_file, "w") as outfile:
    outfile.write("#define LAYERS_SOURCE_PATH \"" + fixpath(layers_source_path) + "\"\n")
    outfile.write("#define DEFAULT_VK_LAYERS_PATH \"" + fixpath(product_dir) + "\"\n")
