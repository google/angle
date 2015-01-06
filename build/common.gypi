# Copyright (c) 2010 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'includes': [ 'common_defines.gypi', ],
    'variables':
    {
        'angle_build_winrt%': '0',
        'angle_build_winphone%': '0',
        # angle_code is set to 1 for the core ANGLE targets defined in src/build_angle.gyp.
        # angle_code is set to 0 for test code, sample code, and third party code.
        # When angle_code is 1, we build with additional warning flags on Mac and Linux.
        'angle_code%': 0,
        'release_symbols%': 'true',
        'gcc_or_clang_warnings':
        [
            '-Wall',
            '-Wchar-subscripts',
            '-Werror',
            '-Wextra',
            '-Wformat=2',
            '-Winit-self',
            '-Wno-sign-compare',
            '-Wno-unused-function',
            '-Wno-unused-parameter',
            '-Wno-unknown-pragmas',
            '-Wpacked',
            '-Wpointer-arith',
            '-Wundef',
            '-Wwrite-strings',
            '-Wno-reorder',
            '-Wno-format-nonliteral',
            '-Wno-deprecated-register',
        ],
    },
    'target_defaults':
    {
        'default_configuration': 'Debug',
        'variables':
        {
            'warn_as_error%': 1,
        },
        'target_conditions':
        [
            ['warn_as_error == 1',
            {
                'msvs_settings':
                {
                    'VCCLCompilerTool':
                    {
                        'WarnAsError': 'true',
                    },
                },
            }],
        ],
        'configurations':
        {
            'Common_Base':
            {
                'abstract': 1,
                'msvs_configuration_attributes':
                {
                    'OutputDirectory': '$(SolutionDir)$(ConfigurationName)_$(Platform)',
                    'IntermediateDirectory': '$(OutDir)\\obj\\$(ProjectName)',
                    'CharacterSet': '0',    # ASCII
                },
                'msvs_settings':
                {
                    'VCCLCompilerTool':
                    {
                        'AdditionalOptions': ['/MP'],
                        'BufferSecurityCheck': 'true',
                        'DebugInformationFormat': '3',
                        'ExceptionHandling': '0',
                        'EnableFunctionLevelLinking': 'true',
                        'MinimalRebuild': 'false',
                        'RuntimeTypeInfo': 'true',
                        'WarningLevel': '4',
                    },
                    'VCLinkerTool':
                    {
                        'FixedBaseAddress': '1',
                        'ImportLibrary': '$(OutDir)\\lib\\$(TargetName).lib',
                        'MapFileName': '$(OutDir)\\$(TargetName).map',
                        # Most of the executables we'll ever create are tests
                        # and utilities with console output.
                        'SubSystem': '1',    # /SUBSYSTEM:CONSOLE
                    },
                    'VCResourceCompilerTool':
                    {
                        'Culture': '1033',
                    },
                },
            },    # Common_Base

            'Debug_Base':
            {
                'abstract': 1,
                'defines':
                [
                    '_DEBUG'
                ],
                'msvs_settings':
                {
                    'VCCLCompilerTool':
                    {
                        'Optimization': '0',    # /Od
                        'BasicRuntimeChecks': '3',
                        'conditions':
                        [
                            ['angle_build_winrt==1',
                            {
                                # Use the dynamic C runtime to match
                                # Windows Application Store requirements

                                # The C runtime for Windows Store applications
                                # is a framework package that is managed by                                
                                # the Windows deployment model and can be
                                # shared by multiple packages.

                                'RuntimeLibrary': '3', # /MDd (debug dll)
                            },
                            {
                                # Use the static C runtime to 
                                # match chromium and make sure
                                # we don't depend on the dynamic
                                # runtime's shared heaps
                                'RuntimeLibrary': '1', # /MTd (debug static)
                            }],
                        ],
                    },
                    'VCLinkerTool':
                    {
                        'GenerateDebugInformation': 'true',
                        'LinkIncremental': '2',
                        'conditions':
                        [
                            ['angle_build_winrt==1',
                            {
                                'AdditionalDependencies':
                                [
                                    'dxgi.lib',
                                ]
                            }],
                        ],
                    },
                },
                'xcode_settings':
                {
                    'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
                    'COPY_PHASE_STRIP': 'NO',
                    'GCC_OPTIMIZATION_LEVEL': '0',
                },
            },    # Debug_Base

            'Release_Base':
            {
                'abstract': 1,
                'defines':
                [
                    'NDEBUG'
                ],
                'msvs_settings':
                {
                    'VCCLCompilerTool':
                    {
                        'Optimization': '2',    # /Os
                        'conditions':
                        [
                            ['angle_build_winrt==1',
                            {
                                # Use the dynamic C runtime to match
                                # Windows Application Store requirements

                                # The C runtime for Windows Store applications
                                # is a framework package that is managed by                                
                                # the Windows deployment model and can be
                                # shared by multiple packages.
                                'RuntimeLibrary': '2', # /MD (nondebug dll)
                            },
                            {
                                # Use the static C runtime to 
                                # match chromium and make sure
                                # we don't depend on the dynamic
                                'RuntimeLibrary': '0', # /MT (nondebug static)
                            }],
                        ],
                    },
                    'VCLinkerTool':
                    {
                        'GenerateDebugInformation': '<(release_symbols)',
                        'LinkIncremental': '1',
                    },
                },
            },    # Release_Base

            'x86_Base':
            {
                'abstract': 1,
                'msvs_configuration_platform': 'Win32',
                'msvs_settings':
                {
                    'VCLinkerTool':
                    {
                        'TargetMachine': '1', # x86
                    },
                    'VCLibrarianTool':
                    {
                        'TargetMachine': '1', # x86
                    },
                },
            }, # x86_Base

            'x64_Base':
            {
                'abstract': 1,
                'msvs_configuration_platform': 'x64',
                'msvs_settings':
                {
                    'VCLinkerTool':
                    {
                        'TargetMachine': '17', # x86 - 64
                    },
                    'VCLibrarianTool':
                    {
                        'TargetMachine': '17', # x86 - 64
                    },
                },
            },    # x64_Base

            # Concrete configurations
            'Debug':
            {
                'inherit_from': ['Common_Base', 'x86_Base', 'Debug_Base'],
            },
            'Release':
            {
                'inherit_from': ['Common_Base', 'x86_Base', 'Release_Base'],
            },
            'conditions':
            [
                ['angle_build_winrt==0 and OS == "win" and MSVS_VERSION != "2010e"',
                {
                    'Debug_x64':
                    {
                        'inherit_from': ['Common_Base', 'x64_Base', 'Debug_Base'],
                    },
                    'Release_x64':
                    {
                        'inherit_from': ['Common_Base', 'x64_Base', 'Release_Base'],
                    },
                }],
                ['angle_build_winrt==1',
                {
                    'arm_Base':
                    {
                        'abstract': 1,
                        'msvs_configuration_platform': 'ARM',
                        'msvs_settings':
                        {
                            'VCLinkerTool':
                            {
                                'TargetMachine': '3', # ARM
                            },
                            'VCLibrarianTool':
                            {
                                'TargetMachine': '3', # ARM
                            },
                        },
                    }, # arm_Base
                }],
                ['angle_build_winrt==1 and angle_build_winphone==0',
                {
                    'Debug_x64':
                    {
                        'inherit_from': ['Common_Base', 'x64_Base', 'Debug_Base'],
                    },
                    'Release_x64':
                    {
                        'inherit_from': ['Common_Base', 'x64_Base', 'Release_Base'],
                    },
                    'Debug_ARM':
                    {
                        'inherit_from': ['Common_Base', 'arm_Base', 'Debug_Base'],
                    },
                    'Release_ARM':
                    {
                        'inherit_from': ['Common_Base', 'arm_Base', 'Release_Base'],
                    },
                }],
                ['angle_build_winrt==1 and angle_build_winphone==1',
                {
                    'Debug_ARM':
                    {
                        'inherit_from': ['Common_Base', 'arm_Base', 'Debug_Base'],
                    },
                    'Release_ARM':
                    {
                        'inherit_from': ['Common_Base', 'arm_Base', 'Release_Base'],
                    },
                }],
            ],
        },    # configurations
    },    # target_defaults
    'conditions':
    [
        ['OS == "win"',
        {
            'target_defaults':
            {
                'msvs_cygwin_dirs': ['../third_party/cygwin'],
            },
        },
        { # OS != win
            'target_defaults':
            {
                'cflags':
                [
                    '-fPIC',
                    '-std=c++0x',
                ],
            },
        }],
        ['OS != "win" and OS != "mac"',
        {
            'target_defaults':
            {
                'cflags':
                [
                    '-pthread',
                    '-fno-exceptions',
                ],
                'ldflags':
                [
                    '-pthread',
                ],
                'configurations':
                {
                    'Debug':
                    {
                        'variables':
                        {
                            'debug_optimize%': '0',
                        },
                        'defines':
                        [
                            '_DEBUG',
                        ],
                        'cflags':
                        [
                            '-O>(debug_optimize)',
                            '-g',
                        ],
                    }
                },
            },
        }],
        ['angle_code==1',
        {
            'target_defaults':
            {
                'conditions':
                [
                    ['OS == "mac"',
                    {
                        'xcode_settings':
                        {
                            'WARNING_CFLAGS': ['<@(gcc_or_clang_warnings)']
                        },
                    }],
                    ['OS != "win" and OS != "mac"',
                    {
                        'cflags': ['<@(gcc_or_clang_warnings)']
                    }],
                ]
            }
        }],
    ],
}
