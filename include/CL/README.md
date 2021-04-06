# ANGLE OpenCL Headers

The OpenCL headers ANGLE uses are the original headers from Khronos, but they are modified to allow
the macro `CL_API_ENTRY` to be overridden externally.

The modifications have been [submitted](https://github.com/KhronosGroup/OpenCL-Headers/pull/162) to
Khronos, and this document should be updated after they are merged.

### Regenerating headers

1. Clone [https://github.com/KhronosGroup/OpenCL-Headers.git](https://github.com/KhronosGroup/OpenCL-Headers.git).
1. Copy all headers from `OpenCL-Headers/CL/` over to this folder.
1. Edit the headers:

   * Change all occurences of `typedef CL_API_ENTRY` to `typedef`.
   * In `cl_platform.h` change both `#define CL_API_ENTRY` to

        ```
            #if !defined(CL_API_ENTRY)
                #define CL_API_ENTRY
            #endif
        ```
