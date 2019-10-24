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

Once built ANGLE will capture a frame's OpenGL calls to a CPP replay stored in the current working
directory. The capture files will be named `angle_capture_context{id}_frame{n}.cpp`. Each OpenGL
context has a unique Context ID to identify its proper replay files. ANGLE will write out large
binary blobs such as Texture or Buffer data to `angle_capture_context{id}_frame{n}.angledata`.

To run a CPP replay you must stitch together the source files into a GN target. The samples
framework is well-suited for implementing a CPP replay example. Alternately you could adapt an ANGLE
end-to-end test. The `angledata` files must be accessible to the replay.

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
