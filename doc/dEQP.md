# ANGLE + dEQP
drawElements (dEQP) is a very robust and comprehensive set of open-source tests for GLES2, GLES3+ and EGL. They provide a huge net of coverage for almost every GL API feature. ANGLE by default builds dEQP testing targets for testing with GLES 2 with D3D9/11 and OpenGL, and GLES 3 with D3D11.

## How to build dEQP

You should have dEQP as a target if you followed the [DevSetup](DevSetup.md) instructions. There are two executables, `angle_deqp_gles2_tests` and `angle_deqp_gles3_tests`.

Currently we only support Windows platforms.

## How to use dEQP

Running the full test suite in Debug can take a very long time. We recommend first dumping the complete lists of tests to a text file, then running the tests that apply to your changes.

### Dump the case list

To dump a list of test cases, append the command line arguments `--deqp-runmode=txt-caselist`, run the GLES2 or GLES3 target, then look for the file named `src\tests\third_party\deqp\data\dEQP-GLES2-cases.txt`. (Or GLES3).

### Choose your tests

Scan the case list files for tests with names similar to the features you're working on. Then, to run particular tests, use the command line arguments `--deqp-case=dEQP-GLES2.functional.shaders.linkage.*` (for example). Replace the test name string with the test your want from the case list files you dumped above.

### Choose a Renderer

By default the tests run on ANGLE D3D11. To specify the exact platform for ANGLE + dEQP, use the arguments:

  * `--deqp-egl-native-display-type=angle-d3d11` for D3D11 (high feature level)
  * `--deqp-egl-native-display-type=angle-d3d9` for D3D9
  * `--deqp-egl-native-display-type=angle-d3d11-fl93` for D3D11 Feature level 9_3
  * `--deqp-egl-native-display-type=angle-gl` for OpenGL

### Check your results

For now, the recommended practice is to manually compare your passes/failures with the results prior to your modifications.

dEQP uses the open-source GUI [Cherry](https://android.googlesource.com/platform/external/cherry) for viewing batch test results (see the `TestResults.qpa` file). ANGLE doesn't currently build or distribute Cherry, but feel free to check it out!