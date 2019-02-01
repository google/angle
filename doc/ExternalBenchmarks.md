# Using External Benchmarks with ANGLE

This document contains instructions on how to run external benchmarks on ANGLE as the GLES renderer.
There is a section for each benchmark with subsections for each platform.  The general theme is to
make the benchmark application pick ANGLE's `libGLESv2.so` and `libEGL.so` files instead of the
system ones.

On Linux, this is generally achieved with setting `LD_LIBRARY_PATH`.  On Windows, ANGLE dlls may
need to be copied to the benchmark's executable directory.

## glmark2

This benchmark can be found on [github](https://github.com/glmark2/glmark2).  It's written against
GLES 2.0 and supports Linux and Android.  It performs tens of tests and reports the framerate for
each test.

### glmark2 on Linux

To build glmark2 on Linux:

```
$ git clone https://github.com/glmark2/glmark2.git
$ cd glmark2
$ ./waf configure --with-flavors=x11-glesv2 --data-path=$PWD/data/
$ ./waf
```

To run glmark2 using the native implementation of GLES:

```
$ cd build/src
$ ./glmark2-es2
```

To run glmark2 using ANGLE, we need to first create a few links in the build directory of ANGLE:

```
$ cd /path/to/angle/out/release
$ ln -s libEGL.so libEGL.so.1
$ ln -s libGLESv2.so libGLESv2.so.2
```

Back in glmark2, we need to make sure these shared objects are picked up:

```
$ cd /path/to/glmark2/build/src
$ LD_LIBRARY_PATH=/path/to/angle/out/release/ ldd ./glmark2-es2
```

With `ldd`, you can verify that `libEGL.so.1` and `libGLESv2.so.2` are correctly picked up from
ANGLE's build directory.

To run glmark2 on the default back-end of ANGLE:

```
$ LD_LIBRARY_PATH=/path/to/angle/out/release/ ./glmark2-es2
```

To run glmark2 on a specific back-end of ANGLE:

```
$ ANGLE_DEFAULT_PLATFORM=vulkan LD_LIBRARY_PATH=/path/to/angle/out/release/ ./glmark2-es2
```
