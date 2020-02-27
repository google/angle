#!/bin/bash

#  Copyright The ANGLE Project Authors. All rights reserved.
#  Use of this source code is governed by a BSD-style license that can be
#  found in the LICENSE file.
#
# Generates a roll CL within the ANGLE repository of AOSP.

git checkout -b tmp aosp/upstream-mirror
git merge -s ours master --allow-unrelated-histories -m "Merge remote-tracking branch 'aosp/upstream-mirror' into aosp-master"
git checkout master
git merge tmp
git branch -D tmp

deps=(
    "third_party/spirv-tools/src"
    "third_party/glslang/src"
    "third_party/spirv-headers/src"
    "third_party/jsoncpp"
    "third_party/jsoncpp/source"
)

# Delete dep directories so that gclient can check them out
for dep in ${deps[@]}; do
    rm -rf --preserve-root $dep
done

# Sync all of ANGLE's deps so that 'gn gen' works
python scripts/bootstrap.py
gclient sync -D

# generate gn build files and convert them to blueprints
gn gen out/Android --args="target_os = \"android\" is_component_build = false is_debug = false android32_ndk_api_level = 26 android64_ndk_api_level = 26"
gn desc out/Android --format=json "*" > out/Android/desc.json
python scripts/generate_android_bp.py out/Android/desc.json > Android.bp
rm -r out
git add Android.bp

# Delete the .git files in each dep so that it can be added to this repo. Some deps like jsoncpp
# have multiple layers of deps so delete everything before adding them.
for dep in ${deps[@]}; do
    rm -rf --preserve-root $dep/.git
done

for dep in ${deps[@]}; do
    git add -f $dep
done

git commit --amend --no-edit
