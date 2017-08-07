# ANGLE + dEQP

drawElements (dEQP) is a very robust and comprehensive set of open-source
tests for GLES2, GLES3+ and EGL. They provide a huge net of coverage for
almost every GL API feature. ANGLE by default builds dEQP testing targets for
testing against GLES 2, GLES 3, EGL, and GLES 3.1 (on supported platforms).

## How to build dEQP

You should have dEQP as a target if you followed the [DevSetup](DevSetup.md)
instructions. Current targets:

  * `angle_deqp_gles2_tests` for GLES 2.0 tests
  * `angle_deqp_gles3_tests` for GLES 3.0 tests
  * `angle_deqp_egl_tests` for EGL 1.x tests
  * `angle_deqp_gles31_tests` for GLES 3.1 tests (currently very experimental)

## How to use dEQP

The `--deqp-case` flag allows you to run individual tests, with simple
wildcard support. For example: `--deqp-case=dEQP-
GLES2.functional.shaders.linkage.*`.

The tests lists are sourced from the Android CTS masters in
`third_party/deqp/src/android/cts/master`. See `gles2-master.txt`,
`gles3-master.txt`, `gles31-master.txt` and `egl-master.txt`.

If you're running a full test suite, it might take very long time. Running in
Debug is only useful to isolate and fix particular failures, Release will give
a better sense of total passing rate.

### Choosing a Renderer on Windows

By default Windows ANGLE tests with D3D11. To specify the exact platform for
ANGLE + dEQP, use the arguments:

  * `--deqp-egl-display-type=angle-d3d11` for D3D11 (highest available feature level)
  * `--deqp-egl-display-type=angle-d3d9` for D3D9
  * `--deqp-egl-display-type=angle-d3d11-fl93` for D3D11 Feature level 9_3
  * `--deqp-egl-display-type=angle-gl` for OpenGL Desktop (OSX, Linux and Windows)
  * `--deqp-egl-display-type=angle-gles` for OpenGL ES (Android/ChromeOS, some Windows platforms)

### Check your results

If run from Visual Studio 2015, dEQP generates a test log to
`src/tests/TestResults.qpa`. To view the test log information, you'll need to
use the open-source GUI
[Cherry](https://android.googlesource.com/platform/external/cherry). ANGLE
checks out a copy of Cherry to `angle/third_party/cherry` when you sync with
gclient. Note, if you are using ninja or another build system, the qpa file
will be located in your working directory.

See the [official Cherry README](https://android.googlesource.com/platform/ext
ernal/cherry/+/master/README) for instructions on how to run Cherry on Linux
or Windows.

### GoogleTest, ANGLE and dEQP

ANGLE also supports the same set of targets built with GoogleTest, for running
on the bots. We don't currently recommend using these for local debugging, but
we do maintain lists of test expectations in `src/tests/deqp_support`. When
you fix tests, please remove the suppression(s) from the relevant files!
