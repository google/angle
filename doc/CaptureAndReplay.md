# ANGLE OpenGL Frame Capture and Replay

ANGLE currently supports a limited OpenGL capture and replay framework.

Limitations:

 * Many OpenGL ES functions are not yet implemented.
 * EGL capture and replay is not yet supported.
 * Mid-execution is not yet implemented.
 * Capture only tested on desktop platforms currently.
 * Replays currently are implemented as CPP files.

## Capturing and replaying an application

To build ANGLE with capture and replay enabled update your GN args:

```
angle_with_capture_by_default = true
```

Once built ANGLE will capture the OpenGL ES calls to a CPP replay. By default the replay will be
stored in the current working directory. The capture files will be named according to the pattern
`angle_capture_context{id}_frame{n}.cpp`. ANGLE will additionally write out data binary blobs for
Texture or Buffer contexts to `angle_capture_context{id}_frame{n}.angledata`.

## Controlling Frame Capture

Some simple environment variables control frame capture:

 * `ANGLE_CAPTURE_ENABLED`:
   Can be set to "0" to disable capture entirely.
 * `ANGLE_CAPTURE_OUT_DIR=<path>`:
   Can specify an alternate replay output directory than the CWD.
   Example: `ANGLE_CAPTURE_OUT_DIR=samples/capture_replay`
 * `ANGLE_CAPTURE_FRAME_END=<n>`:
   By default ANGLE will capture the first ten frames. This variable can override the default.
   Example: `ANGLE_CAPTURE_FRAME_END=4`

A good way to test out the capture is to use environment variables in conjunction with the sample
template. For example:

```
$ ANGLE_CAPTURE_FRAME_END=4 ANGLE_CAPTURE_OUT_DIR=samples/capture_replay out/Debug/simple_texture_2d
```

## Running a CPP replay

To run a CPP replay you can use a template located in
[samples/capture_replay](../samples/capture_replay). Update
[samples/BUILD.gn](../samples/BUILD.gn) to enable the `capture_replay` sample to include your replay:

```
capture_replay("my_sample") {
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
$ autoninja -C out/Debug my_sample
$ ANGLE_CAPTURE_ENABLED=0 out/Debug/my_sample
```
