# This file is used to manage the dependencies of the ANGLE git repo. It is
# used by gclient to determine what version of each dependency to check out, and
# where.

# Avoids the need for a custom root variable.
use_relative_paths = True

gclient_gn_args_file = 'build/config/gclient_args.gni'
gclient_gn_args = [
  'mac_xcode_version',
]

vars = {
  # This can be overridden, e.g. with custom_vars, to download a nonstandard
  # Xcode version in build/mac_toolchain.py instead of downloading the
  # prebuilt pinned revision.
  'mac_xcode_version': 'default',

  'android_git': 'https://android.googlesource.com',
  'chromium_git': 'https://chromium.googlesource.com',
  'chrome_internal_git': 'https://chrome-internal.googlesource.com',
  'swiftshader_git': 'https://swiftshader.googlesource.com',

  # This variable is overrided in Chromium's DEPS file.
  'build_with_chromium': False,

  # Only check out public sources by default. This can be overridden with custom_vars.
  'checkout_angle_internal': False,

  # Version of Chromium our Chromium-based DEPS are mirrored from.
  'chromium_revision': '09df13c25e1b30200b3d08531cc741be1df1ea49',
  # We never want to checkout chromium,
  # but need a dummy DEPS entry for the autoroller
  'dummy_checkout_chromium': False,

  # Current revision of VK-GL-CTS (a.k.a dEQP).
  'vk_gl_cts_revision': '3b3f93c206dc516cb372674ecec3209b7f462abb',

  # Current revision of glslang, the Khronos SPIRV compiler.
  'glslang_revision': '517f39eee46f27c83527117d831c4d7e2f7c9fe3',

  # Current revision of googletest.
  # Note: this dep cannot be auto-rolled b/c of nesting.
  'googletest_revision': '4fe018038f87675c083d0cfb6a6b57c274fb1753',

  # Current revision of Chrome's third_party googletest directory. This
  # repository is mirrored as a separate repository, with separate git hashes
  # that don't match the external googletest repository or Chrome. Mirrored
  # patches will have a different git hash associated with them.
  # To roll, first get the new hash for chromium_googletest_revision from the
  # mirror of third_party/googletest located here:
  # https://chromium.googlesource.com/chromium/src/third_party/googletest/
  # Then get the new hash for googletest_revision from the root Chrome DEPS
  # file: https://source.chromium.org/chromium/chromium/src/+/master:DEPS
  'chromium_googletest_revision': 'c20c5a3085ab4d90fdb403e3ac98e7991317dd27',

  # Current revision of jsoncpp.
  # Note: this dep cannot be auto-rolled b/c of nesting.
  'jsoncpp_revision': '645250b6690785be60ab6780ce4b58698d884d11',

  # Current revision of Chrome's third_party jsoncpp directory. This repository
  # is mirrored as a separate repository, with separate git hashes that
  # don't match the external JsonCpp repository or Chrome. Mirrored patches
  # will have a different git hash associated with them.
  # To roll, first get the new hash for chromium_jsoncpp_revision from the
  # mirror of third_party/jsoncpp located here:
  # https://chromium.googlesource.com/chromium/src/third_party/jsoncpp/
  # Then get the new hash for jsoncpp_revision from the root Chrome DEPS file:
  # https://source.chromium.org/chromium/chromium/src/+/master:DEPS
  'chromium_jsoncpp_revision': '30a6ac108e24dabac7c2e0df4d33d55032af4ee7',

  # Current revision of patched-yasm.
  # Note: this dep cannot be auto-rolled b/c of nesting.
  'patched_yasm_revision': '720b70524a4424b15fc57e82263568c8ba0496ad',

  # Current revision of spirv-cross, the Khronos SPIRV cross compiler.
  'spirv_cross_revision': 'f38cbeb814c73510b85697adbe5e894f9eac978f',

  # Current revision fo the SPIRV-Headers Vulkan support library.
  'spirv_headers_revision': '3fdabd0da2932c276b25b9b4a988ba134eba1aa6',

  # Current revision of SPIRV-Tools for Vulkan.
  'spirv_tools_revision': '2c60d16a646b7b4d1541e278dda7133cf6feaa9b',

  # Current revision of Khronos Vulkan-Headers.
  'vulkan_headers_revision': 'ced848d699b5673a7df900bcfe0b1a6ba9e989ce',

  # Current revision of Khronos Vulkan-Loader.
  'vulkan_loader_revision': 'd846ea5a2427eb0119cf98288aa337eced10edbd',

  # Current revision of Khronos Vulkan-Tools.
  'vulkan_tools_revision': '25d277579d1e1844c02e6ce3f64950ef6b5b0d80',

  # Current revision of Khronos Vulkan-ValidationLayers.
  'vulkan_validation_revision': 'b0337a548273b5c2aa25d11f5b133f5d67eeaa2f',

  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling catapult
  # and whatever else without interference from each other.
  'catapult_revision': '1b3fb455bf1849f1e6187e1eaeaef32b9f30d3c5',
}

deps = {

  'build': {
    'url': '{chromium_git}/chromium/src/build.git@c12a68d90b44722f3dcb433cbf837fd3f85e2c11',
    'condition': 'not build_with_chromium',
  },

  'buildtools': {
    'url': '{chromium_git}/chromium/src/buildtools.git@ff93f3ea1a7f033d3caf8f60ec1937cc71351419',
    'condition': 'not build_with_chromium',
  },

  'testing': {
    'url': '{chromium_git}/chromium/src/testing@c1a7e4af8e92265e54d2da1781330abc62726f92',
    'condition': 'not build_with_chromium',
  },

  'third_party/abseil-cpp': {
    'url': '{chromium_git}/chromium/src/third_party/abseil-cpp@18c972d96731013383fc55d19c6ff7c816b71390',
    'condition': 'not build_with_chromium',
  },

  'third_party/android_ndk': {
    'url': '{chromium_git}/android_ndk.git@27c0a8d090c666a50e40fceb4ee5b40b1a2d3f87',
    'condition': 'checkout_android and not build_with_chromium',
  },

  'third_party/catapult': {
    'url': '{chromium_git}/catapult.git@{catapult_revision}',
    'condition': 'checkout_android and not build_with_chromium',
  },

  # Cherry is a dEQP/VK-GL-CTS management GUI written in Go. We use it for viewing test results.
  'third_party/cherry': {
    'url': '{android_git}/platform/external/cherry@4f8fb08d33ca5ff05a1c638f04c85bbb8d8b52cc',
    'condition': 'not build_with_chromium',
  },

  # We never want to checkout chromium,
  # but need a dummy DEPS entry for the autoroller
  'third_party/dummy_chromium': {
    'url': '{chromium_git}/chromium/src.git@{chromium_revision}',
    'condition': 'dummy_checkout_chromium',
  },

  'third_party/fuchsia-sdk': {
    'url': '{chromium_git}/chromium/src/third_party/fuchsia-sdk.git@1785f0ac8e1fe81cb25e260acbe7de8f62fa3e44',
    'condition': 'checkout_fuchsia and not build_with_chromium',
  },

  # Closed-source OpenGL ES 1.1 Conformance tests.
  'third_party/gles1_conform': {
    'url': '{chrome_internal_git}/angle/es-cts.git@dc9f502f709c9cd88d7f8d3974f1c77aa246958e',
    'condition': 'checkout_angle_internal',
  },

  # glmark2 is a GPL3-licensed OpenGL ES 2.0 benchmark. We use it for testing.
  'third_party/glmark2/src': {
    'url': '{chromium_git}/external/github.com/glmark2/glmark2@9e01aef1a786b28aca73135a5b00f85c357e8f5e',
  },

  'third_party/glslang/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/glslang@{glslang_revision}',
    'condition': 'not build_with_chromium',
  },

  'third_party/googletest': {
    'url': '{chromium_git}/chromium/src/third_party/googletest@{chromium_googletest_revision}',
    'condition': 'not build_with_chromium',
  },

  # libjpeg_turbo is used by glmark2.
  'third_party/libjpeg_turbo': {
    'url': '{chromium_git}/chromium/deps/libjpeg_turbo.git@9d4f8005bc6c888e66b00fd00188531ee9bd3344',
    'condition': 'not build_with_chromium',
  },

  'third_party/libpng/src': {
    'url': '{android_git}/platform/external/libpng@094e181e79a3d6c23fd005679025058b7df1ad6c',
    'condition': 'not build_with_chromium',
  },

  'third_party/jsoncpp': {
    'url': '{chromium_git}/chromium/src/third_party/jsoncpp@{chromium_jsoncpp_revision}',
    'condition': 'not build_with_chromium',
   },

  'third_party/nasm': {
    'url': '{chromium_git}/chromium/deps/nasm.git@19f3fad68da99277b2882939d3b2fa4c4b8d51d9',
    'condition': 'not build_with_chromium',
  },

  'third_party/Python-Markdown': {
    'url': '{chromium_git}/chromium/src/third_party/Python-Markdown@ad4fc19d612de0a3f6ea19441af703ff5a5223f3',
    'condition': 'not build_with_chromium',
  },

  'third_party/qemu-linux-x64': {
      'packages': [
          {
              'package': 'fuchsia/qemu/linux-amd64',
              'version': '9cc486c5b18a0be515c39a280ca9a309c54cf994'
          },
      ],
      'condition': 'not build_with_chromium and (host_os == "linux" and checkout_fuchsia)',
      'dep_type': 'cipd',
  },

  'third_party/qemu-mac-x64': {
      'packages': [
          {
              'package': 'fuchsia/qemu/mac-amd64',
              'version': '2d3358ae9a569b2d4a474f498b32b202a152134f'
          },
      ],
      'condition': 'not build_with_chromium and (host_os == "mac" and checkout_fuchsia)',
      'dep_type': 'cipd',
  },

  'third_party/rapidjson/src': {
    'url': '{chromium_git}/external/github.com/Tencent/rapidjson@7484e06c589873e1ed80382d262087e4fa80fb63',
  },

  'third_party/spirv-cross/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Cross@{spirv_cross_revision}',
    'condition': 'not build_with_chromium',
  },

  'third_party/spirv-headers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Headers@{spirv_headers_revision}',
    'condition': 'not build_with_chromium',
  },

  'third_party/spirv-tools/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Tools@{spirv_tools_revision}',
    'condition': 'not build_with_chromium',
  },

  'third_party/SwiftShader': {
    'url': '{swiftshader_git}/SwiftShader@e1051cbaad46dc98967bee2e00a697d7f82b6658',
    'condition': 'not build_with_chromium',
  },

  'third_party/VK-GL-CTS/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/VK-GL-CTS@{vk_gl_cts_revision}',
  },

  'third_party/vulkan-headers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Headers@{vulkan_headers_revision}',
  },

  'third_party/vulkan-loader/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Loader@{vulkan_loader_revision}',
  },

  'third_party/vulkan_memory_allocator': {
    'url': '{chromium_git}/external/github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator@065e739079d9d58bef28ccd793cbf512261f09ed',
    'condition': 'not build_with_chromium',
  },

  'third_party/vulkan-tools/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Tools@{vulkan_tools_revision}',
  },

  'third_party/vulkan-validation-layers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-ValidationLayers@{vulkan_validation_revision}',
  },

  'third_party/zlib': {
    'url': '{chromium_git}/chromium/src/third_party/zlib@59187e10e96157e9135da61e95cec25a9baf9cd3',
    'condition': 'not build_with_chromium',
  },

  'tools/clang': {
    'url': '{chromium_git}/chromium/src/tools/clang.git@03bacc323567b1f522fdf52f03a315b587da1826',
    'condition': 'not build_with_chromium',
  },

  'tools/clang/dsymutil': {
    'packages': [
      {
        'package': 'chromium/llvm-build-tools/dsymutil',
        'version': 'M56jPzDv1620Rnm__jTMYS62Zi8rxHVq7yw0qeBFEgkC',
      }
    ],
    'condition': 'checkout_mac and not build_with_chromium',
    'dep_type': 'cipd',
  },

  'tools/md_browser': {
    'url': '{chromium_git}/chromium/src/tools/md_browser@60141af3603925d99bf3fb22fdfca138416339b1',
    'condition': 'not build_with_chromium',
  },

  'tools/memory': {
    'url': '{chromium_git}/chromium/src/tools/memory@ee1e2448132431e7519c2a72ff612e079f13a9d6',
    'condition': 'not build_with_chromium',
  },
}

hooks = [
  # Pull clang-format binaries using checked-in hashes.
  {
    'name': 'clang_format_win',
    'pattern': '.',
    'condition': 'host_os == "win" and not build_with_chromium',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=win32',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/win/clang-format.exe.sha1',
    ],
  },
  {
    'name': 'clang_format_mac',
    'pattern': '.',
    'condition': 'host_os == "mac" and not build_with_chromium',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/mac/clang-format.sha1',
    ],
  },
  {
    'name': 'clang_format_linux',
    'pattern': '.',
    'condition': 'host_os == "linux" and not build_with_chromium',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/linux64/clang-format.sha1',
    ],
  },
  {
    'name': 'sysroot_x86',
    'pattern': '.',
    'condition': 'checkout_linux and ((checkout_x86 or checkout_x64) and not build_with_chromium)',
    'action': ['python', 'build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=x86'],
  },
  {
    'name': 'sysroot_x64',
    'pattern': '.',
    'condition': 'checkout_linux and (checkout_x64 and not build_with_chromium)',
    'action': ['python', 'build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=x64'],
  },
  {
    # Update the Windows toolchain if necessary.  Must run before 'clang' below.
    'name': 'win_toolchain',
    'pattern': '.',
    'condition': 'checkout_win and not build_with_chromium',
    'action': ['python', 'build/vs_toolchain.py', 'update', '--force'],
  },
  {
    # Update the Mac toolchain if necessary.
    'name': 'mac_toolchain',
    'pattern': '.',
    'condition': 'checkout_mac and not build_with_chromium',
    'action': ['python', 'build/mac_toolchain.py'],
  },

  {
    # Note: On Win, this should run after win_toolchain, as it may use it.
    'name': 'clang',
    'pattern': '.',
    'action': ['python', 'tools/clang/scripts/update.py'],
    'condition': 'not build_with_chromium',
  },

  {
    # Update LASTCHANGE.
    'name': 'lastchange',
    'pattern': '.',
    'condition': 'not build_with_chromium',
    'action': ['python', 'build/util/lastchange.py',
               '-o', 'build/util/LASTCHANGE'],
  },

  # Pull rc binaries using checked-in hashes.
  {
    'name': 'rc_win',
    'pattern': '.',
    'condition': 'checkout_win and (host_os == "win" and not build_with_chromium)',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-browser-clang/rc',
                '-s', 'build/toolchain/win/rc/win/rc.exe.sha1',
    ],
  },

  {
    'name': 'fuchsia_sdk',
    'pattern': '.',
    'condition': 'checkout_fuchsia and not build_with_chromium',
    'action': [
      'python',
      'build/fuchsia/update_sdk.py',
    ],
  },

  # Download glslang validator binary for Linux.
  {
    'name': 'linux_glslang_validator',
    'pattern': '.',
    'condition': 'checkout_linux and not build_with_chromium',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'angle-glslang-validator',
                '-s', 'tools/glslang/glslang_validator.sha1',
    ],
  },

  # Download glslang validator binary for Windows.
  {
    'name': 'win_glslang_validator',
    'pattern': '.',
    'condition': 'checkout_win and not build_with_chromium',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=win32*',
                '--no_auth',
                '--bucket', 'angle-glslang-validator',
                '-s', 'tools/glslang/glslang_validator.exe.sha1',
    ],
  },

  # Download flex/bison binaries for Linux.
  {
    'name': 'linux_flex_bison',
    'pattern': '.',
    'condition': 'checkout_linux and not build_with_chromium',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'angle-flex-bison',
                '-d', 'tools/flex-bison/linux/',
    ],
  },

  # Download flex/bison binaries for Windows.
  {
    'name': 'win_flex_bison',
    'pattern': '.',
    'condition': 'checkout_win and not build_with_chromium',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=win32*',
                '--no_auth',
                '--bucket', 'angle-flex-bison',
                '-d', 'tools/flex-bison/windows/',
    ],
  },

  # Download internal captures for perf tests
  {
    'name': 'restricted_traces',
    'pattern': '\\.sha1',
    'condition': 'checkout_angle_internal',
    'action': [ 'python',
                'src/tests/perf_tests/restricted_traces/download_restricted_traces.py',
                'src/tests/perf_tests/restricted_traces',
    ]
  }
]

recursedeps = [
  # buildtools provides clang_format.
  'buildtools',
  'third_party/googletest',
  'third_party/jsoncpp',
]
