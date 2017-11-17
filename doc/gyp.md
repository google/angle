# Deprecated GYP Instructions

Use GN instead of GYP, as described in the [developer instructions](DevSetup.md).
Support for GYP will be removed in the future.

Though deprecated, GYP is still run by `gclient sync`.
You may want to set certain environment variables ahead of time.

On Windows:

 * `GYP_GENERATORS` to `msvs` (other options include `ninja` and `make`)
 * `GYP_MSVS_VERSION` to `2015`

On Linux and MacOS:

 * `GYP_GENERATORS` to `ninja` (defaults to 'make' that pollutes your source directory)

To run GYP at other times use the script `gyp/gyp_angle`.


## Running ANGLE under apitrace on Linux

See the [debugging tips](DebuggingTips.md#running-angle-under-apitrace-on-linux) and replace the gn steps with:

```
gyp/gyp_angle -D angle_link_glx=1 -D angle_gl_library_type=static_library
```

# Deprecated Windows Store Instructions

[Building for Windows Store](BuildingAngleForWindowsStore.md) is deprecated because it requires GYP.
