# Debugging Tips

There are many ways to debug ANGLE using generic or platform-dependent tools. Here is a list of tips on how to use them.

## Running ANGLE under apitrace on Linux

[Apitrace](http://apitrace.github.io/) that captures traces of OpenGL commands for later analysis, allowing us to see how ANGLE translates OpenGL ES commands. In order to capture the trace, it inserts a driver shim using `LD_PRELOAD` that records the command and then forwards it to the OpenGL driver.

The problem with ANGLE is that it exposes the same symbols as the OpenGL driver so apitrace captures the entry point calls intended for ANGLE and reroutes them to the OpenGL driver. In order to avoid this problem, use the following:

1. Link your application against the static ANGLE libraries (libGLESv2_static and libEGL_static) so they don't get shadowed by apitrace's shim.
2. Ask apitrace to explicitly load the driver instead of using a dlsym on the current module. Otherwise apitrace will use ANGLE's symbols as the OpenGL driver entrypoint (causing infinite recursion). To do this you must point an environment variable to your GL driver. For example: `export TRACE_LIBGL=/usr/lib/libGL.so.1`. You can find your libGL with `ldconfig -p | grep libGL`.
3. Link ANGLE against libGL instead of dlsyming the symbols at runtime; otherwise ANGLE won't use the replaced driver entry points. This is done with the gn arg `angle_link_glx = true`.

If you follow these steps, apitrace will work correctly aside from a few minor bugs like not being able to figure out what the default framebuffer size is if there is no glViewport command.

For example, to trace a run of `hello_triangle`, assuming you are using the ninja gyp generator and the apitrace executables are in `$PATH`:

```
gn args out/Debug # add "angle_link_glx = true"
# edit samples/BUILD.gn and append "_static" to "angle_util", "libEGL", "libGLESv2"
ninja -C out/Debug
export TRACE_LIBGL="/usr/lib/libGL.so.1" # may require a different path
apitrace trace -o mytrace ./out/Debug/hello_triangle
qapitrace mytrace
```

## Running ANGLE under GAPID on Linux

[GAPID](https://github.com/google/gapid) can be used to capture trace of Vulkan commands on Linux.
For it to work, libvulkan has to be a shared library, instead of being statically linked into ANGLE, which is the default behavior.
This is done with the gn arg:
```
angle_shared_libvulkan = true
```

When capturing traces of gtest based tests built inside Chromium checkout, make sure to run the tests with `--single-process-tests` argument.