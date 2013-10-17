@ECHO OFF
REM
REM Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
REM Use of this source code is governed by a BSD-style license that can be
REM found in the LICENSE file.
REM

REM This script regenerates the canonical visual studio project files that
REM are distributed with the ANGLE repository. It requires that depot_tools
REM is installed. The project files are generated and then git added so they
REM can be committed.

set oldGYPDefines=%GYP_DEFINES%
set oldGYPMSVS=%GYP_MSVS_VERSION%
set oldGYPGenerators=%GYP_GENERATORS%

set GYP_DEFINES=angle_build_tests=0 angle_build_samples=1
set GYP_MSVS_VERSION=2010e
set GYP_GENERATORS=msvs
call gclient sync

call git apply convert_solutions_to_express.patch
call git add build/*.sln
call git add build/*.vcxproj
call git add build/*.vcxproj.filters
call git add src/*.sln
call git add src/*.vcxproj
call git add src/*.vcxproj.filters
call git add samples/*.sln
call git add samples/*.vcxproj
call git add samples/*.vcxproj.filters

set GYP_DEFINES=%oldGYPDefines%
set GYP_MSVS_VERSION=%oldGYPMSVS%
set GYP_GENERATORS=%oldGYPGenerators%

call gclient runhooks
