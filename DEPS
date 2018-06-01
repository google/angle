vars = {
  'android_git': 'https://android.googlesource.com',
  'chromium_git': 'https://chromium.googlesource.com',

  # This variable is set on the Chrome infra for compatiblity with gclient.
  'angle_root': '.',

  # This variable is overrided in Chromium's DEPS file.
  'build_with_chromium': False,

  # Current revision of dEQP.
  'deqp_revision': '5bc346ba2d5465a2e6094e254f12b1586fd0097f',

  # Current revision of glslang, the Khronos SPIRV compiler.
  'glslang_revision': '2edde6665d9a56ead5ea0e55b4e64d9a803e6164',

  # Current revision fo the SPIRV-Headers Vulkan support library.
  'spirv_headers_revision': '98b01515724c428d0f0a5d01deffcce0f5f5e61c',

  # Current revision of SPIRV-Tools for Vulkan.
  'spirv_tools_revision': '9996173f363729b3a97309685dbd4d78547a63a7',

  # Current revision of the Vulkan Validation Layers SDK.
  'vulkan_revision': '25d5884746a2de7b51a8ef3ec88e1cd8066460e8',
}

deps = {

  '{angle_root}/build': {
    'url': '{chromium_git}/chromium/src/build.git@b944b99e72923c5a6699235ed858e725db21f81f',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/buildtools': {
    'url': '{chromium_git}/chromium/buildtools.git@94288c26d2ffe3aec9848c147839afee597acefd',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/testing': {
    'url': '{chromium_git}/chromium/src/testing@4d706fd80be9e8989aec5235540e7b46d0672826',
    'condition': 'not build_with_chromium',
  },

  # Cherry is a dEQP management GUI written in Go. We use it for viewing test results.
  '{angle_root}/third_party/cherry': {
    'url': '{android_git}/platform/external/cherry@4f8fb08d33ca5ff05a1c638f04c85bbb8d8b52cc',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/third_party/deqp/src': {
    'url': '{android_git}/platform/external/deqp@{deqp_revision}',
  },

  '{angle_root}/third_party/glslang/src': {
    'url': '{android_git}/platform/external/shaderc/glslang@{glslang_revision}',
  },

  '{angle_root}/third_party/googletest/src': {
    'url': '{chromium_git}/external/github.com/google/googletest.git@145d05750b15324899473340c8dd5af50d125d33',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/third_party/libpng/src': {
    'url': '{android_git}/platform/external/libpng@094e181e79a3d6c23fd005679025058b7df1ad6c',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/third_party/spirv-headers/src': {
    'url': '{android_git}/platform/external/shaderc/spirv-headers@{spirv_headers_revision}',
  },

  '{angle_root}/third_party/spirv-tools/src': {
    'url': '{android_git}/platform/external/shaderc/spirv-tools@{spirv_tools_revision}',
  },

  '{angle_root}/third_party/vulkan-validation-layers/src': {
    'url': '{android_git}/platform/external/vulkan-validation-layers@{vulkan_revision}',
  },

  '{angle_root}/third_party/zlib': {
    'url': '{chromium_git}/chromium/src/third_party/zlib@da0819d6c816a61be6fcb2fcf9b74246f0f8b808',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/tools/clang': {
    'url': '{chromium_git}/chromium/src/tools/clang.git@c893c7eec4706f8c7fc244ee254b1dadd8f8d158',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/tools/gyp': {
    'url': '{chromium_git}/external/gyp@4d467626b0b9f59a85fb81ca4d7ea9eca99b9d8f',
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
                '-s', '{angle_root}/buildtools/win/clang-format.exe.sha1',
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
                '-s', '{angle_root}/buildtools/mac/clang-format.sha1',
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
                '-s', '{angle_root}/buildtools/linux64/clang-format.sha1',
    ],
  },
  # Pull GN binaries using checked-in hashes.
  {
    'name': 'gn_win',
    'pattern': '.',
    'condition': 'host_os == "win" and not build_with_chromium',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=win32',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', '{angle_root}/buildtools/win/gn.exe.sha1',
    ],
  },
  {
    'name': 'gn_mac',
    'pattern': '.',
    'condition': 'host_os == "mac" and not build_with_chromium',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', '{angle_root}/buildtools/mac/gn.sha1',
    ],
  },
  {
    'name': 'gn_linux64',
    'pattern': '.',
    'condition': 'host_os == "linux" and not build_with_chromium',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', '{angle_root}/buildtools/linux64/gn.sha1',
    ],
  },
  {
    'name': 'sysroot_x86',
    'pattern': '.',
    'condition': 'checkout_linux and ((checkout_x86 or checkout_x64) and not build_with_chromium)',
    'action': ['python', '{angle_root}/build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=x86'],
  },
  {
    'name': 'sysroot_x64',
    'pattern': '.',
    'condition': 'checkout_linux and (checkout_x64 and not build_with_chromium)',
    'action': ['python', '{angle_root}/build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=x64'],
  },
  {
    # Update the Windows toolchain if necessary.  Must run before 'clang' below.
    'name': 'win_toolchain',
    'pattern': '.',
    'condition': 'checkout_win and not build_with_chromium',
    'action': ['python', '{angle_root}/build/vs_toolchain.py', 'update', '--force'],
  },

  {
    # Note: On Win, this should run after win_toolchain, as it may use it.
    'name': 'clang',
    'pattern': '.',
    'action': ['python', '{angle_root}/tools/clang/scripts/update.py'],
    'condition': 'not build_with_chromium',
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
                '-s', '{angle_root}/build/toolchain/win/rc/win/rc.exe.sha1',
    ],
  },

  {
    # A change to a .gyp, .gypi, or to GYP itself should run the generator.
    'pattern': '.',
    'action': ['python', '{angle_root}/gyp/gyp_angle'],
    'condition': 'not build_with_chromium',
  },
]

recursedeps = [
  # buildtools provides clang_format.
  '{angle_root}/buildtools',
]
