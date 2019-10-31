# ANGLE OpenGL Frame Capture and Replay

ANGLE currently supports a limited OpenGL capture and replay framework.

Limitations:

 * GLES capture has many unimplemented functions.
 * EGL capture and replay is not yet supported.
 * Mid-execution capture is supported with the Vulkan back-end.
 * Mid-execution capture has many unimplemented features.
 * Capture and replay is currently only tested on desktop platforms.
 * Binary replay is unimplemented. CPP replay is supported.

## Capturing and replaying an application

To build ANGLE with capture and replay enabled update your GN args:

```
angle_with_capture_by_default = true
```

Once built ANGLE will capture the OpenGL ES calls to CPP replay files. By default the replay will be
stored in the current working directory. The capture files will be named according to the pattern
`angle_capture_context{id}_frame{n}.cpp`. Each GL Context currently has its own replay sources.
ANGLE will write out data binary blobs for large Texture or Buffer contents to
`angle_capture_context{id}_frame{n}.angledata`. Replay programs must be able to load data from the
corresponding `angledata` files.

## Controlling Frame Capture

Some simple environment variables control frame capture:

 * `ANGLE_CAPTURE_ENABLED`:
   * Set to `0` to disable capture entirely. Default is `1`.
 * `ANGLE_CAPTURE_OUT_DIR=<path>`:
   * Can specify an alternate replay output directory.
   * Example: `ANGLE_CAPTURE_OUT_DIR=samples/capture_replay`. Default is the CWD.
 * `ANGLE_CAPTURE_FRAME_START=<n>`:
   * Uses mid-execution capture to write "Setup" functions that starts a Context at frame `n`.
   * Example: `ANGLE_CAPTURE_FRAME_START=2`. Default is `0`.
 * `ANGLE_CAPTURE_FRAME_END=<n>`:
   * By default ANGLE will capture the first ten frames. This variable can override the default.
   * Example: `ANGLE_CAPTURE_FRAME_END=4`. Default is `10`.

A good way to test out the capture is to use environment variables in conjunction with the sample
template. For example:

```
$ ANGLE_CAPTURE_FRAME_END=4 ANGLE_CAPTURE_OUT_DIR=samples/capture_replay out/Debug/simple_texture_2d
```

## Running a CPP replay

To run a CPP replay you can use a template located in
[samples/capture_replay](../samples/capture_replay). Update
[samples/BUILD.gn](../samples/BUILD.gn) to enable `capture_replay_sample`
sample to include your replay frames:

```
angle_capture_replay_sample("capture_replay_sample") {
  sources = [
    "capture_replay/angle_capture_context1_frame000.cpp",
    "capture_replay/angle_capture_context1_frame001.cpp",
    "capture_replay/angle_capture_context1_frame002.cpp",
    "capture_replay/angle_capture_context1_frame003.cpp",
    "capture_replay/angle_capture_context1_frame004.cpp",
  ]
}
```

Then build and run your replay sample:

```
$ autoninja -C out/Debug capture_replay
$ ANGLE_CAPTURE_ENABLED=0 out/Debug/capture_replay
```

Note that we specify `ANGLE_CAPTURE_ENABLED=0` to prevent re-capturing your replay.
