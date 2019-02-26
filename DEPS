vars = {
  'android_git': 'https://android.googlesource.com',
  'chromium_git': 'https://chromium.googlesource.com',

  # This variable is set on the Chrome infra for compatiblity with gclient.
  'angle_root': '.',

  # This variable is overrided in Chromium's DEPS file.
  'build_with_chromium': False,

  # Current revision of dEQP.
  'deqp_revision': '66a49e0a43f7af654ee1de8a3b1bcaf6c0d14aa4',

  # Current revision of glslang, the Khronos SPIRV compiler.
  'glslang_revision': 'f6e7c4d2de0d59724ea07739df70c466d169a2cd',

  # Current revision fo the SPIRV-Headers Vulkan support library.
  'spirv_headers_revision': '8bea0a266ac9b718aa0818d9e3a47c0b77c2cb23',

  # Current revision of SPIRV-Tools for Vulkan.
  'spirv_tools_revision': 'fde69dcd80cc1ca548300702adf01eeb25441f3e',

  # Current revision of Khronos Vulkan-Headers.
  'vulkan_headers_revision': 'c200cb25db0f47364d3318d92c1d8e9dfff2fef1',

  # Current revision of Khronos Vulkan-Loader.
  'vulkan_loader_revision': 'e1eafa18e17d00374253bcd37d015befa89fcc43',

  # Current revision of Khronos Vulkan-Tools.
  'vulkan_tools_revision': '91b17fd866b2e9cfb875bf516b05536d059416b1',

  # Current revision of Khronos Vulkan-ValidationLayers.
  'vulkan_validation_revision': '4eee269ae976567ef78db9c9feaafc3364578c87',
}

deps = {

  '{angle_root}/build': {
    'url': '{chromium_git}/chromium/src/build.git@9a53be87ebb636c35f2ed9772e5deaeb350d790b',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/buildtools': {
    'url': '{chromium_git}/chromium/buildtools.git@6fbda1b24c1893a893b17aa219b765b9e7c801d8',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/testing': {
    'url': '{chromium_git}/chromium/src/testing@7bdda3c6577f21fd0ec986a0383ecfce28f5d761',
    'condition': 'not build_with_chromium',
  },

  # Cherry is a dEQP management GUI written in Go. We use it for viewing test results.
  '{angle_root}/third_party/cherry': {
    'url': '{android_git}/platform/external/cherry@4f8fb08d33ca5ff05a1c638f04c85bbb8d8b52cc',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/third_party/deqp/src': {
    'url': '{chromium_git}/external/deqp@{deqp_revision}',
  },

  '{angle_root}/third_party/glslang/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/glslang@{glslang_revision}',
    'condition': 'not build_with_chromium',
   },

  '{angle_root}/third_party/googletest': {
    'url': '{chromium_git}/chromium/src/third_party/googletest@660425b1c5ca04559ab7e50c7572b5b771acca1c',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/third_party/googletest/src': {
    'url': '{chromium_git}/external/github.com/google/googletest.git@7203f37f57e4fef0d77670098aabc186309eb874',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/third_party/libpng/src': {
    'url': '{android_git}/platform/external/libpng@094e181e79a3d6c23fd005679025058b7df1ad6c',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/third_party/jsoncpp': {
    'url': '{chromium_git}/chromium/src/third_party/jsoncpp@fd0ac8ce63a47e99b71a58f1489136fbb19c9137',
    'condition': 'not build_with_chromium',
   },

  '{angle_root}/third_party/jsoncpp/source': {
    'url' : '{chromium_git}/external/github.com/open-source-parsers/jsoncpp@f572e8e42e22cfcf5ab0aea26574f408943edfa4',
    'condition': 'not build_with_chromium',
   },

  '{angle_root}/third_party/qemu-linux-x64': {
      'packages': [
          {
              'package': 'fuchsia/qemu/linux-amd64',
              'version': '9cc486c5b18a0be515c39a280ca9a309c54cf994'
          },
      ],
      'condition': 'not build_with_chromium and (host_os == "linux" and checkout_fuchsia)',
      'dep_type': 'cipd',
  },

  '{angle_root}/third_party/qemu-mac-x64': {
      'packages': [
          {
              'package': 'fuchsia/qemu/mac-amd64',
              'version': '2d3358ae9a569b2d4a474f498b32b202a152134f'
          },
      ],
      'condition': 'not build_with_chromium and (host_os == "mac" and checkout_fuchsia)',
      'dep_type': 'cipd',
  },

  '{angle_root}/third_party/rapidjson/src': {
    'url': '{chromium_git}/external/github.com/Tencent/rapidjson@7484e06c589873e1ed80382d262087e4fa80fb63',
  },

  '{angle_root}/third_party/spirv-headers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Headers@{spirv_headers_revision}',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/third_party/spirv-tools/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/SPIRV-Tools@{spirv_tools_revision}',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/third_party/vulkan-headers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Headers@{vulkan_headers_revision}',
  },

  '{angle_root}/third_party/vulkan-loader/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Loader@{vulkan_loader_revision}',
  },

  '{angle_root}/third_party/vulkan-tools/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-Tools@{vulkan_tools_revision}',
  },

  '{angle_root}/third_party/vulkan-validation-layers/src': {
    'url': '{chromium_git}/external/github.com/KhronosGroup/Vulkan-ValidationLayers@{vulkan_validation_revision}',
  },

  '{angle_root}/third_party/zlib': {
    'url': '{chromium_git}/chromium/src/third_party/zlib@f95aeb0fa7f136ef4a457a6d9ba6f3c2701a444b',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/tools/clang': {
    'url': '{chromium_git}/chromium/src/tools/clang.git@3114fbc11f9644c54dd0a4cdbfa867bac50ff983',
    'condition': 'not build_with_chromium',
  },

  '{angle_root}/third_party/fuchsia-sdk': {
    'url': '{chromium_git}/chromium/src/third_party/fuchsia-sdk.git@8e8db13b538ecb251e5ce9d5c781fc142f9752fd',
    'condition': 'checkout_fuchsia and not build_with_chromium',
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

  {
    # Update LASTCHANGE.
    'name': 'lastchange',
    'pattern': '.',
    'condition': 'not build_with_chromium',
    'action': ['python', '{angle_root}/build/util/lastchange.py',
               '-o', '{angle_root}/build/util/LASTCHANGE'],
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
    'name': 'fuchsia_sdk',
    'pattern': '.',
    'condition': 'checkout_fuchsia and not build_with_chromium',
    'action': [
      'python',
      '{angle_root}/build/fuchsia/update_sdk.py',
    ],
  },
]

recursedeps = [
  # buildtools provides clang_format.
  '{angle_root}/buildtools',
]
