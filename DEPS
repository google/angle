# This file is used to manage the dependencies of the ANGLE git repo. It is
# used by gclient to determine what version of each dependency to check out, and
# where.

# Avoids the need for a custom root variable.
use_relative_paths = True

gclient_gn_args_file = 'build/config/gclient_args.gni'

vars = {
  'android_git': 'https://android.googlesource.com',
  'chromium_git': 'https://chromium.googlesource.com',
  'chrome_internal_git': 'https://chrome-internal.googlesource.com',
  'swiftshader_git': 'https://swiftshader.googlesource.com',

  # This variable is overrided in Chromium's DEPS file.
  'build_with_chromium': False,

  # Only check out public sources by default. This can be overridden with custom_vars.
  'checkout_angle_internal': False,

  # Version of Chromium our Chromium-based DEPS are mirrored from.
  'chromium_revision': '60fea25f23e69ff315fe56da1128fcb78a7316a8',
  # We never want to checkout chromium,
  # but need a dummy DEPS entry for the autoroller
  'dummy_checkout_chromium': False,

  # Current revision of VK-GL-CTS (a.k.a dEQP).
  'vk_gl_cts_revision': 'c4e6dbe68e043d6697f60768baf5b2876c6596c6',

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

  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling catapult
  # and whatever else without interference from each other.
  'catapult_revision': '629e117d9b566e0f1eebde8053cdc94614fd5176',

  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling luci-go
  # and whatever else without interference from each other.
  'luci_go': 'git_revision:19175e196dd495f6e092845f7d777cb5bf157b3e',
}

deps = {

  'build': {
    'url': '{chromium_git}/chromium/src/build.git@37b76d6fba1cf8e5e407cd0b0cad5acc61a73a15',
    'condition': 'not build_with_chromium',
  },

  'buildtools': {
    'url': '{chromium_git}/chromium/src/buildtools.git@69cc9b8a3ae010e0721c4bea12de7a352d9a93f9',
    'condition': 'not build_with_chromium',
  },

  'buildtools/clang_format/script': {
    'url': '{chromium_git}/external/github.com/llvm/llvm-project/clang/tools/clang-format.git@99803d74e35962f63a775f29477882afd4d57d94',
    'condition': 'not build_with_chromium',
  },

  'buildtools/linux64': {
    'packages': [
      {
        'package': 'gn/gn/linux-amd64',
        'version': 'git_revision:64b3b9401c1c3ed5f3c43c1cac00b91f83597ab8',
      }
    ],
    'dep_type': 'cipd',
    'condition': 'not build_with_chromium and host_os == "linux"',
  },

  'buildtools/mac': {
    'packages': [
      {
        'package': 'gn/gn/mac-${{arch}}',
        'version': 'git_revision:64b3b9401c1c3ed5f3c43c1cac00b91f83597ab8',
      }
    ],
    'dep_type': 'cipd',
    'condition': 'not build_with_chromium and host_os == "mac"',
  },

  'buildtools/third_party/libc++/trunk': {
    'url': '{chromium_git}/external/github.com/llvm/llvm-project/libcxx.git@8fa87946779682841e21e2da977eccfb6cb3bded',
    'condition': 'not build_with_chromium',
  },

  'buildtools/third_party/libc++abi/trunk': {
    'url': '{chromium_git}/external/github.com/llvm/llvm-project/libcxxabi.git@c93be42a4007e6fc16ea085f5b47ea50aa9134a8',
    'condition': 'not build_with_chromium',
  },

  'buildtools/win': {
    'packages': [
      {
        'package': 'gn/gn/windows-amd64',
        'version': 'git_revision:64b3b9401c1c3ed5f3c43c1cac00b91f83597ab8',
      }
    ],
    'dep_type': 'cipd',
    'condition': 'not build_with_chromium and host_os == "win"',
  },

  'testing': {
    'url': '{chromium_git}/chromium/src/testing@2408fca90b4240f67d0001b8c8075845582cbe0f',
    'condition': 'not build_with_chromium',
  },

  'third_party/abseil-cpp': {
    'url': '{chromium_git}/chromium/src/third_party/abseil-cpp@8e820e32d0d6bff14551631a5cd38ff7d3453c63',
    'condition': 'not build_with_chromium',
  },

  'third_party/android_ndk': {
    'url': '{chromium_git}/android_ndk.git@27c0a8d090c666a50e40fceb4ee5b40b1a2d3f87',
    'condition': 'checkout_android and not build_with_chromium',
  },

  'third_party/catapult': {
    'url': '{chromium_git}/catapult.git@{catapult_revision}',
    'condition': 'not build_with_chromium',
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

  'third_party/googletest': {
    'url': '{chromium_git}/chromium/src/third_party/googletest@{chromium_googletest_revision}',
    'condition': 'not build_with_chromium',
  },

  # libjpeg_turbo is used by glmark2.
  'third_party/libjpeg_turbo': {
    'url': '{chromium_git}/chromium/deps/libjpeg_turbo.git@7b4981b6500ccba10733c352b9ed2dad14ce3c73',
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

  'third_party/protobuf': {
    'url': '{chromium_git}/chromium/src/third_party/protobuf@52433541779c2d98b3ef98def5b06924e9ae85ba',
    'condition': 'not build_with_chromium',
  },

  'third_party/Python-Markdown': {
    'url': '{chromium_git}/chromium/src/third_party/Python-Markdown@2bb7b23b6398f9e79bc2fa8c6bc64a3cf1613ebf',
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

  'third_party/SwiftShader': {
    'url': '{swiftshader_git}/SwiftShader@a98fb3f8380a3a5c9ae3e0175634ba88c85a434f',
    'condition': 'not build_with_chromium',
  },

  'third_party/VK-GL-CTS/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/VK-GL-CTS@{vk_gl_cts_revision}',
  },

  'third_party/vulkan-deps': {
    'url': '{chromium_git}/vulkan-deps@105af117f0532953577198cc9bd8ee6f76c29009',
    'condition': 'not build_with_chromium',
  },

  'third_party/vulkan_memory_allocator': {
    'url': '{chromium_git}/external/github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator@732a76d9d3c70d6aa487216495eeb28518349c3a',
    'condition': 'not build_with_chromium',
  },

  'third_party/zlib': {
    'url': '{chromium_git}/chromium/src/third_party/zlib@09490503d0f201b81e03f5ca0ab8ba8ee76d4a8e',
    'condition': 'not build_with_chromium',
  },

  'tools/clang': {
    'url': '{chromium_git}/chromium/src/tools/clang.git@752611e5f254fb65788e165d2070e5aa8279cb04',
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

  'tools/luci-go': {
    'packages': [
      {
        'package': 'infra/tools/luci/isolate/${{platform}}',
        'version': Var('luci_go'),
      },
      {
        'package': 'infra/tools/luci/isolated/${{platform}}',
        'version': Var('luci_go'),
      },
      {
        'package': 'infra/tools/luci/swarming/${{platform}}',
        'version': Var('luci_go'),
      },
    ],
    'condition': 'not build_with_chromium',
    'dep_type': 'cipd',
  },

  'tools/mb': {
    'url': '{chromium_git}/chromium/src/tools/mb@67f01522b1e9b8862f3f07836ac97bfb5474adec',
    'condition': 'not build_with_chromium',
  },

  'tools/md_browser': {
    'url': '{chromium_git}/chromium/src/tools/md_browser@60141af3603925d99bf3fb22fdfca138416339b1',
    'condition': 'not build_with_chromium',
  },

  'tools/memory': {
    'url': '{chromium_git}/chromium/src/tools/memory@71214b910decfe2e7cfc8b0ffc072a1b97da2d36',
    'condition': 'not build_with_chromium',
  },

  'tools/protoc_wrapper': {
    'url': '{chromium_git}/chromium/src/tools/protoc_wrapper@4d4e1824fc1a93147a2f640a25812d1db9d3c2d2',
    'condition': 'not build_with_chromium',
  },

  'tools/skia_goldctl/linux': {
      'packages': [
        {
          'package': 'skia/tools/goldctl/linux-amd64',
          'version': '3T-SUsozYAVsudk5nd48kjHvfpos6egORLB8s7ElE8oC',
        },
      ],
      'dep_type': 'cipd',
      'condition': 'checkout_linux and not build_with_chromium',
  },

  'tools/skia_goldctl/win': {
      'packages': [
        {
          'package': 'skia/tools/goldctl/windows-amd64',
          'version': 'wr5p_MyXcHVxA-eHznijCeFaqR2HUo02Hw70e0CWCIoC',
        },
      ],
      'dep_type': 'cipd',
      'condition': 'checkout_win and not build_with_chromium',
  },

  'tools/skia_goldctl/mac': {
      'packages': [
        {
          'package': 'skia/tools/goldctl/mac-amd64',
          'version': 'TPtcrf_XH3mB9pdK4W3N0QPZunsqJ08T7U7fWPvMnRAC',
        },
      ],
      'dep_type': 'cipd',
      'condition': 'checkout_mac and not build_with_chromium',
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
    'action': [ 'vpython3',
                'src/tests/restricted_traces/download_restricted_traces.py',
                'src/tests/restricted_traces',
    ]
  }
]

recursedeps = [
  'third_party/googletest',
  'third_party/jsoncpp',
  'third_party/vulkan-deps',
]
