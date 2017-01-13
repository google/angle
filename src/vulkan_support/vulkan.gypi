# Copyright 2016 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'variables':
    {
        'glslang_path': '../../third_party/glslang-angle/src',
        'spirv_headers_path': '../../third_party/spirv-headers/src',
        'spirv_tools_path': '../../third_party/spirv-tools-angle/src',
        'vulkan_layers_path': '../../third_party/vulkan-validation-layers/src',
        'vulkan_loader_sources':
        [
            '<(vulkan_layers_path)/loader/cJSON.c',
            '<(vulkan_layers_path)/loader/cJSON.h',
            '<(vulkan_layers_path)/loader/debug_report.c',
            '<(vulkan_layers_path)/loader/debug_report.h',
            '<(vulkan_layers_path)/loader/dev_ext_trampoline.c',
            '<(vulkan_layers_path)/loader/extensions.c',
            '<(vulkan_layers_path)/loader/extensions.h',
            '<(vulkan_layers_path)/loader/gpa_helper.h',
            '<(vulkan_layers_path)/loader/loader.c',
            '<(vulkan_layers_path)/loader/loader.h',
            '<(vulkan_layers_path)/loader/murmurhash.c',
            '<(vulkan_layers_path)/loader/murmurhash.h',
            '<(vulkan_layers_path)/loader/table_ops.h',
            '<(vulkan_layers_path)/loader/trampoline.c',
            '<(vulkan_layers_path)/loader/vk_loader_platform.h',
            '<(vulkan_layers_path)/loader/wsi.c',
            '<(vulkan_layers_path)/loader/wsi.h',
        ],
        'vulkan_loader_win_sources':
        [
            '<(vulkan_layers_path)/loader/dirent_on_windows.c',
            '<(vulkan_layers_path)/loader/dirent_on_windows.h',
        ],
        'vulkan_loader_include_dirs':
        [
            '<(vulkan_layers_path)/include',
            '<(vulkan_layers_path)/loader',
        ],
        'vulkan_loader_cflags_win':
        [
            '/wd4054', # Type cast from function pointer
            '/wd4055', # Type cast from data pointer
            '/wd4100', # Unreferenced formal parameter
            '/wd4152', # Nonstandard extension used (pointer conversion)
            '/wd4201', # Nonstandard extension used: nameless struct/union
            '/wd4214', # Nonstandard extension used: bit field types other than int
            '/wd4232', # Nonstandard extension used: address of dllimport is not static
            '/wd4706', # Assignment within conditional expression
            '/wd4996', # Unsafe stdlib function
        ],
        'spirv_tools_sources':
        [
            '<(angle_gen_path)/vulkan/core.insts-1.0.inc',
            '<(angle_gen_path)/vulkan/core.insts-1.1.inc',
            '<(angle_gen_path)/vulkan/generators.inc',
            '<(angle_gen_path)/vulkan/glsl.std.450.insts-1.0.inc',
            '<(angle_gen_path)/vulkan/opencl.std.insts-1.0.inc',
            '<(angle_gen_path)/vulkan/operand.kinds-1.0.inc',
            '<(angle_gen_path)/vulkan/operand.kinds-1.1.inc',
            '<(spirv_tools_path)/source/assembly_grammar.cpp',
            '<(spirv_tools_path)/source/assembly_grammar.h',
            '<(spirv_tools_path)/source/binary.cpp',
            '<(spirv_tools_path)/source/binary.h',
            '<(spirv_tools_path)/source/diagnostic.cpp',
            '<(spirv_tools_path)/source/diagnostic.h',
            '<(spirv_tools_path)/source/disassemble.cpp',
            '<(spirv_tools_path)/source/enum_set.h',
            '<(spirv_tools_path)/source/ext_inst.cpp',
            '<(spirv_tools_path)/source/ext_inst.h',
            '<(spirv_tools_path)/source/instruction.h',
            '<(spirv_tools_path)/source/libspirv.cpp',
            '<(spirv_tools_path)/source/macro.h',
            '<(spirv_tools_path)/source/message.cpp',
            '<(spirv_tools_path)/source/name_mapper.cpp',
            '<(spirv_tools_path)/source/name_mapper.h',
            '<(spirv_tools_path)/source/opcode.cpp',
            '<(spirv_tools_path)/source/opcode.h',
            '<(spirv_tools_path)/source/operand.cpp',
            '<(spirv_tools_path)/source/operand.h',
            '<(spirv_tools_path)/source/parsed_operand.cpp',
            '<(spirv_tools_path)/source/parsed_operand.h',
            '<(spirv_tools_path)/source/print.cpp',
            '<(spirv_tools_path)/source/print.h',
            # TODO(jmadill): Determine if this is ever needed.
            #'<(spirv_tools_path)/source/software_version.cpp',
            '<(spirv_tools_path)/source/spirv_constant.h',
            '<(spirv_tools_path)/source/spirv_definition.h',
            '<(spirv_tools_path)/source/spirv_endian.cpp',
            '<(spirv_tools_path)/source/spirv_endian.h',
            '<(spirv_tools_path)/source/spirv_target_env.cpp',
            '<(spirv_tools_path)/source/spirv_target_env.h',
            '<(spirv_tools_path)/source/table.cpp',
            '<(spirv_tools_path)/source/table.h',
            '<(spirv_tools_path)/source/text.cpp',
            '<(spirv_tools_path)/source/text.h',
            '<(spirv_tools_path)/source/text_handler.cpp',
            '<(spirv_tools_path)/source/text_handler.h',
            '<(spirv_tools_path)/source/util/bitutils.h',
            '<(spirv_tools_path)/source/util/hex_float.h',
            '<(spirv_tools_path)/source/util/parse_number.cpp',
            '<(spirv_tools_path)/source/util/parse_number.h',
            '<(spirv_tools_path)/source/val/basic_block.cpp',
            '<(spirv_tools_path)/source/val/construct.cpp',
            '<(spirv_tools_path)/source/val/function.cpp',
            '<(spirv_tools_path)/source/val/instruction.cpp',
            '<(spirv_tools_path)/source/val/validation_state.cpp',
            '<(spirv_tools_path)/source/validate.cpp',
            '<(spirv_tools_path)/source/validate.h',
            '<(spirv_tools_path)/source/validate_cfg.cpp',
            '<(spirv_tools_path)/source/validate_datarules.cpp',
            '<(spirv_tools_path)/source/validate_id.cpp',
            '<(spirv_tools_path)/source/validate_instruction.cpp',
            '<(spirv_tools_path)/source/validate_layout.cpp',
        ],
        'vulkan_layer_utils_sources':
        [
            '<(vulkan_layers_path)/layers/vk_layer_config.cpp',
            '<(vulkan_layers_path)/layers/vk_layer_config.h',
            '<(vulkan_layers_path)/layers/vk_layer_extension_utils.cpp',
            '<(vulkan_layers_path)/layers/vk_layer_extension_utils.h',
            '<(vulkan_layers_path)/layers/vk_layer_utils.cpp',
            '<(vulkan_layers_path)/layers/vk_layer_utils.h',
        ],
        'vulkan_struct_wrappers_outputs':
        [
            '<(angle_gen_path)/vulkan/vk_safe_struct.cpp',
            '<(angle_gen_path)/vulkan/vk_safe_struct.h',
            '<(angle_gen_path)/vulkan/vk_struct_size_helper.c',
            '<(angle_gen_path)/vulkan/vk_struct_size_helper.h',
            '<(angle_gen_path)/vulkan/vk_struct_string_helper.h',
            '<(angle_gen_path)/vulkan/vk_struct_string_helper_cpp.h',
        ],
        'VkLayer_core_validation_sources':
        [
            # This file is manually included in the layer
            # '<(angle_gen_path)/vulkan/vk_safe_struct.cpp',
            '<(angle_gen_path)/vulkan/vk_safe_struct.h',
            '<(vulkan_layers_path)/layers/core_validation.cpp',
            '<(vulkan_layers_path)/layers/core_validation.h',
            '<(vulkan_layers_path)/layers/descriptor_sets.cpp',
            '<(vulkan_layers_path)/layers/descriptor_sets.h',
        ],
        'VkLayer_image_sources':
        [
            '<(vulkan_layers_path)/layers/image.cpp',
            '<(vulkan_layers_path)/layers/image.h',
        ],
        'VkLayer_swapchain_sources':
        [
            '<(vulkan_layers_path)/layers/swapchain.cpp',
            '<(vulkan_layers_path)/layers/swapchain.h',
        ],
        'VkLayer_object_tracker_sources':
        [
            '<(vulkan_layers_path)/layers/object_tracker.cpp',
            '<(vulkan_layers_path)/layers/object_tracker.h',
        ],
        'VkLayer_unique_objects_sources':
        [
            '<(angle_gen_path)/vulkan/unique_objects_wrappers.h',
            # This file is manually included in the layer
            # '<(angle_gen_path)/vulkan/vk_safe_struct.cpp',
            '<(angle_gen_path)/vulkan/vk_safe_struct.h',
            '<(vulkan_layers_path)/layers/unique_objects.cpp',
            '<(vulkan_layers_path)/layers/unique_objects.h',
        ],
        'VkLayer_threading_sources':
        [
            '<(angle_gen_path)/vulkan/thread_check.h',
            '<(vulkan_layers_path)/layers/threading.cpp',
            '<(vulkan_layers_path)/layers/threading.h',
        ],
        'VkLayer_parameter_validation_sources':
        [
            '<(angle_gen_path)/vulkan/parameter_validation.h',
            '<(vulkan_layers_path)/layers/parameter_validation.cpp',
        ],
        'vulkan_gen_json_files_sources_win':
        [
            '<(vulkan_layers_path)/layers/windows/VkLayer_core_validation.json',
            '<(vulkan_layers_path)/layers/windows/VkLayer_image.json',
            '<(vulkan_layers_path)/layers/windows/VkLayer_object_tracker.json',
            '<(vulkan_layers_path)/layers/windows/VkLayer_parameter_validation.json',
            '<(vulkan_layers_path)/layers/windows/VkLayer_swapchain.json',
            '<(vulkan_layers_path)/layers/windows/VkLayer_threading.json',
            '<(vulkan_layers_path)/layers/windows/VkLayer_unique_objects.json',
        ],
        'vulkan_gen_json_files_outputs':
        [
            '<(angle_gen_path)/vulkan/json/VkLayer_core_validation.json',
            '<(angle_gen_path)/vulkan/json/VkLayer_image.json',
            '<(angle_gen_path)/vulkan/json/VkLayer_object_tracker.json',
            '<(angle_gen_path)/vulkan/json/VkLayer_parameter_validation.json',
            '<(angle_gen_path)/vulkan/json/VkLayer_swapchain.json',
            '<(angle_gen_path)/vulkan/json/VkLayer_threading.json',
            '<(angle_gen_path)/vulkan/json/VkLayer_unique_objects.json',
        ],
    },
    'conditions':
    [
        ['angle_enable_vulkan==1',
        {
            'targets':
            [
                {
                    'target_name': 'vulkan_loader',
                    'type': 'static_library',
                    'sources':
                    [
                        '<@(vulkan_loader_sources)',
                    ],
                    'include_dirs':
                    [
                        '<@(vulkan_loader_include_dirs)',
                        '<(angle_gen_path)',
                    ],
                    'defines':
                    [
                        'API_NAME="Vulkan"',
                    ],
                    'msvs_settings':
                    {
                        'VCCLCompilerTool':
                        {
                            'AdditionalOptions':
                            [
                                # TODO(jmadill): Force include header on other platforms.
                                '<@(vulkan_loader_cflags_win)',
                                '/FIvulkan/angle_loader.h'
                            ],
                        },
                        'VCLinkerTool':
                        {
                            'AdditionalDependencies':
                            [
                                'shlwapi.lib',
                            ],
                        },
                    },
                    'direct_dependent_settings':
                    {
                        'include_dirs':
                        [
                            '<@(vulkan_loader_include_dirs)',
                        ],
                        'msvs_settings':
                        {
                            'VCLinkerTool':
                            {
                                'AdditionalDependencies':
                                [
                                    'shlwapi.lib',
                                ],
                            },
                        },
                        'conditions':
                        [
                            ['OS=="win"',
                            {
                                'defines':
                                [
                                    'VK_USE_PLATFORM_WIN32_KHR',
                                ],
                            }],
                        ],
                    },
                    'conditions':
                    [
                        ['OS=="win"',
                        {
                            'sources':
                            [
                                '<(angle_gen_path)/vulkan/angle_loader.h',
                                '<@(vulkan_loader_win_sources)',
                            ],
                            'defines':
                            [
                                'VK_USE_PLATFORM_WIN32_KHR',
                            ],
                        }],
                    ],
                    'actions':
                    [
                        {
                            # The loader header is force included into the loader and layers. Because
                            # of issues with GYP, we can't use a normal header file, we hav to force
                            # inclue this using compiler-specific flags.
                            'action_name': 'vulkan_loader_gen_angle_header',
                            'message': 'generating Vulkan loader ANGLE header',
                            'msvs_cygwin_shell': 0,
                            'inputs':
                            [
                                '<(angle_path)/scripts/generate_vulkan_header.py',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/angle_loader.h',
                            ],
                            'action':
                            [
                                # TODO(jmadill): Use correct platform path
                                'python', '<(angle_path)/scripts/generate_vulkan_header.py', '<(angle_gen_path)/vulkan/json',
                                '<(angle_gen_path)/vulkan/angle_loader.h', '<(PRODUCT_DIR)',
                            ],
                        },
                    ],
                },

                {
                    'target_name': 'spirv_tools',
                    'type': 'static_library',
                    'sources':
                    [
                        '<@(spirv_tools_sources)',
                    ],
                    'include_dirs':
                    [
                        '<(angle_gen_path)/vulkan',
                        '<(spirv_headers_path)/include',
                        '<(spirv_tools_path)/include',
                        '<(spirv_tools_path)/source',
                    ],
                    'msvs_settings':
                    {
                        'VCCLCompilerTool':
                        {
                            'PreprocessorDefinitions':
                            [
                                '_HAS_EXCEPTIONS=0',
                            ],
                            'AdditionalOptions':
                            [
                                '/wd4127', # Conditional expression is constant, happens in a template in hex_float.h.
                                '/wd4706', # Assignment within conditional expression
                                '/wd4996', # Unsafe stdlib function
                            ],
                        },
                    },
                    'actions':
                    [
                        {
                            'action_name': 'spirv_tools_gen_build_tables_1_0',
                            'message': 'generating info tables for SPIR-V 1.0 instructions and operands',
                            'msvs_cygwin_shell': 0,
                            'inputs':
                            [
                                '<(spirv_headers_path)/include/spirv/1.0/extinst.glsl.std.450.grammar.json',
                                '<(spirv_headers_path)/include/spirv/1.0/spirv.core.grammar.json',
                                '<(spirv_tools_path)/source/extinst-1.0.opencl.std.grammar.json',
                                '<(spirv_tools_path)/utils/generate_grammar_tables.py',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/core.insts-1.0.inc',
                                '<(angle_gen_path)/vulkan/glsl.std.450.insts-1.0.inc',
                                '<(angle_gen_path)/vulkan/opencl.std.insts-1.0.inc',
                                '<(angle_gen_path)/vulkan/operand.kinds-1.0.inc',
                            ],
                            'action':
                            [
                                'python', '<(spirv_tools_path)/utils/generate_grammar_tables.py',
                                '--spirv-core-grammar=<(spirv_headers_path)/include/spirv/1.0/spirv.core.grammar.json',
                                '--extinst-glsl-grammar=<(spirv_headers_path)/include/spirv/1.0/extinst.glsl.std.450.grammar.json',
                                '--extinst-opencl-grammar=<(spirv_tools_path)/source/extinst-1.0.opencl.std.grammar.json',
                                '--core-insts-output=<(angle_gen_path)/vulkan/core.insts-1.0.inc',
                                '--glsl-insts-output=<(angle_gen_path)/vulkan/glsl.std.450.insts-1.0.inc',
                                '--opencl-insts-output=<(angle_gen_path)/vulkan/opencl.std.insts-1.0.inc',
                                '--operand-kinds-output=<(angle_gen_path)/vulkan/operand.kinds-1.0.inc',
                            ],
                        },

                        {
                            'action_name': 'spirv_tools_gen_build_tables_1_1',
                            'message': 'generating info tables for SPIR-V 1.1 instructions and operands',
                            'msvs_cygwin_shell': 0,
                            'inputs':
                            [
                                '<(spirv_tools_path)/utils/generate_grammar_tables.py',
                                '<(spirv_headers_path)/include/spirv/1.1/spirv.core.grammar.json',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/core.insts-1.1.inc',
                                '<(angle_gen_path)/vulkan/operand.kinds-1.1.inc',
                            ],
                            'action':
                            [
                                'python', '<(spirv_tools_path)/utils/generate_grammar_tables.py',
                                '--spirv-core-grammar=<(spirv_headers_path)/include/spirv/1.1/spirv.core.grammar.json',
                                '--core-insts-output=<(angle_gen_path)/vulkan/core.insts-1.1.inc',
                                '--operand-kinds-output=<(angle_gen_path)/vulkan/operand.kinds-1.1.inc',
                            ],
                        },

                        {
                            'action_name': 'spirv_tools_gen_generators_inc',
                            'message': 'generating generators.inc for SPIRV-Tools',
                            'msvs_cygwin_shell': 0,
                            'inputs':
                            [
                                '<(spirv_tools_path)/utils/generate_registry_tables.py',
                                '<(spirv_headers_path)/include/spirv/spir-v.xml',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/generators.inc',
                            ],
                            'action':
                            [
                                'python', '<(spirv_tools_path)/utils/generate_registry_tables.py',
                                '--xml=<(spirv_headers_path)/include/spirv/spir-v.xml',
                                '--generator-output=<(angle_gen_path)/vulkan/generators.inc',
                            ],
                        },
                    ],

                    'direct_dependent_settings':
                    {
                        'include_dirs':
                        [
                            '<(spirv_headers_path)/include',
                            '<(spirv_tools_path)/include',
                        ],
                    },
                },

                {
                    'target_name': 'vulkan_layer_utils_static',
                    'type': 'static_library',
                    'sources':
                    [
                        '<@(vulkan_layer_utils_sources)',
                    ],
                    'include_dirs':
                    [
                        '<@(vulkan_loader_include_dirs)',
                    ],
                    'msvs_settings':
                    {
                        'VCCLCompilerTool':
                        {
                            'PreprocessorDefinitions':
                            [
                                '_HAS_EXCEPTIONS=0',
                            ],
                            'AdditionalOptions':
                            [
                                '/wd4100', # Unreferenced formal parameter
                                '/wd4309', # Truncation of constant value
                                '/wd4505', # Unreferenced local function has been removed
                                '/wd4996', # Unsafe stdlib function
                            ],
                        },
                    },
                    'conditions':
                    [
                        ['OS=="win"',
                        {
                            'defines':
                            [
                                'WIN32',
                                'WIN32_LEAN_AND_MEAN',
                                'VK_USE_PLATFORM_WIN32_KHR',
                            ],
                        }],
                    ],
                    'direct_dependent_settings':
                    {
                        'msvs_cygwin_shell': 0,
                        'sources':
                        [
                            '<(vulkan_layers_path)/layers/vk_layer_table.cpp',
                            '<(vulkan_layers_path)/layers/vk_layer_table.h',
                        ],
                        'include_dirs':
                        [
                            '<(angle_gen_path)/vulkan',
                            '<(glslang_path)',
                            '<(vulkan_layers_path)/layers',
                            '<@(vulkan_loader_include_dirs)',
                        ],
                        'msvs_settings':
                        {
                            'VCCLCompilerTool':
                            {
                                'PreprocessorDefinitions':
                                [
                                    '_HAS_EXCEPTIONS=0',
                                ],
                                'AdditionalOptions':
                                [
                                    '/wd4100', # Unreferenced local parameter
                                    '/wd4456', # declaration hides previous local declaration
                                    '/wd4505', # Unreferenced local function has been removed
                                    '/wd4996', # Unsafe stdlib function
                                ],
                            }
                        },
                        'conditions':
                        [
                            ['OS=="win"',
                            {
                                'defines':
                                [
                                    'WIN32_LEAN_AND_MEAN',
                                    'VK_USE_PLATFORM_WIN32_KHR',
                                ],
                                'configurations':
                                {
                                    'Debug_Base':
                                    {
                                        'msvs_settings':
                                        {
                                            'VCCLCompilerTool':
                                            {
                                                'AdditionalOptions':
                                                [
                                                    '/bigobj',
                                                ],
                                            },
                                        },
                                    },
                                },
                            }],
                        ],
                    },
                },
                {
                    'target_name': 'vulkan_generate_layer_helpers',
                    'type': 'none',
                    'msvs_cygwin_shell': 0,

                    'actions':
                    [
                        # Duplicate everything because of GYP limitations.
                        {
                            'action_name': 'vulkan_run_vk_xml_generate_vk_enum_string_helper_h',
                            'message': 'generating vk_enum_string_helper.h',
                            'inputs':
                            [
                                '<(vulkan_layers_path)/scripts/generator.py',
                                '<(vulkan_layers_path)/scripts/helper_file_generator.py',
                                '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                '<(vulkan_layers_path)/scripts/reg.py',
                                '<(vulkan_layers_path)/scripts/vk.xml',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/vk_enum_string_helper.h'
                            ],
                            'action':
                            [
                                'python', '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                 '-o', '<(angle_gen_path)/vulkan',
                                '-registry', '<(vulkan_layers_path)/scripts/vk.xml',
                                'vk_enum_string_helper.h', '-quiet',
                            ],
                        },

                        {
                            'action_name': 'vulkan_run_vk_xml_generate_vk_struct_size_helper_h',
                            'message': 'generating vk_struct_size_helper.h',
                            'inputs':
                            [
                                '<(vulkan_layers_path)/scripts/generator.py',
                                '<(vulkan_layers_path)/scripts/helper_file_generator.py',
                                '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                '<(vulkan_layers_path)/scripts/reg.py',
                                '<(vulkan_layers_path)/scripts/vk.xml',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/vk_struct_size_helper.h'
                            ],
                            'action':
                            [
                                'python', '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                 '-o', '<(angle_gen_path)/vulkan',
                                '-registry', '<(vulkan_layers_path)/scripts/vk.xml',
                                'vk_struct_size_helper.h', '-quiet',
                            ],
                        },

                        {
                            'action_name': 'vulkan_run_vk_xml_generate_vk_struct_size_helper_c',
                            'message': 'generating vk_struct_size_helper.c',
                            'inputs':
                            [
                                '<(vulkan_layers_path)/scripts/generator.py',
                                '<(vulkan_layers_path)/scripts/helper_file_generator.py',
                                '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                '<(vulkan_layers_path)/scripts/reg.py',
                                '<(vulkan_layers_path)/scripts/vk.xml',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/vk_struct_size_helper.c'
                            ],
                            'action':
                            [
                                'python', '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                 '-o', '<(angle_gen_path)/vulkan',
                                '-registry', '<(vulkan_layers_path)/scripts/vk.xml',
                                'vk_struct_size_helper.c', '-quiet',
                            ],
                        },

                        {
                            'action_name': 'vulkan_run_vk_xml_generate_vk_safe_struct_h',
                            'message': 'generating vk_safe_struct.h',
                            'inputs':
                            [
                                '<(vulkan_layers_path)/scripts/generator.py',
                                '<(vulkan_layers_path)/scripts/helper_file_generator.py',
                                '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                '<(vulkan_layers_path)/scripts/reg.py',
                                '<(vulkan_layers_path)/scripts/vk.xml',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/vk_safe_struct.h'
                            ],
                            'action':
                            [
                                'python', '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                 '-o', '<(angle_gen_path)/vulkan',
                                '-registry', '<(vulkan_layers_path)/scripts/vk.xml',
                                'vk_safe_struct.h', '-quiet',
                            ],
                        },

                        {
                            'action_name': 'vulkan_run_vk_xml_generate_vk_safe_struct_cpp',
                            'message': 'generating vk_safe_struct.cpp',
                            'inputs':
                            [
                                '<(vulkan_layers_path)/scripts/generator.py',
                                '<(vulkan_layers_path)/scripts/helper_file_generator.py',
                                '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                '<(vulkan_layers_path)/scripts/reg.py',
                                '<(vulkan_layers_path)/scripts/vk.xml',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/vk_safe_struct.cpp'
                            ],
                            'action':
                            [
                                'python', '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                 '-o', '<(angle_gen_path)/vulkan',
                                '-registry', '<(vulkan_layers_path)/scripts/vk.xml',
                                'vk_safe_struct.cpp', '-quiet',
                            ],
                        },

                        {
                            'action_name': 'vulkan_generate_dispatch_table_helper',
                            'message': 'generating vk_dispatch_table_helper.h',
                            'inputs':
                            [
                                '<(vulkan_layers_path)/scripts/dispatch_table_generator.py',
                                '<(vulkan_layers_path)/scripts/generator.py',
                                '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                '<(vulkan_layers_path)/scripts/reg.py',
                                '<(vulkan_layers_path)/scripts/vk.xml',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/vk_dispatch_table_helper.h',
                            ],
                            'action':
                            [
                                'python', '<(vulkan_layers_path)/scripts/lvl_genvk.py', '-o', '<(angle_gen_path)/vulkan',
                                '-registry', '<(vulkan_layers_path)/scripts/vk.xml', 'vk_dispatch_table_helper.h', '-quiet',
                            ],
                        },

                        {
                            'action_name': 'vulkan_generate_json_files',
                            'message': 'generating Vulkan json files',
                            'inputs':
                            [
                                '<(angle_path)/scripts/generate_vulkan_layers_json.py',
                            ],
                            'outputs':
                            [
                                '<@(vulkan_gen_json_files_outputs)',
                            ],
                            'conditions':
                            [
                                ['OS=="win"',
                                {
                                    'inputs':
                                    [
                                        '<@(vulkan_gen_json_files_sources_win)',
                                    ],
                                    'action':
                                    [
                                        'python', '<(angle_path)/scripts/generate_vulkan_layers_json.py',
                                        '<(vulkan_layers_path)/layers/windows', '<(angle_gen_path)/vulkan/json', '<(PRODUCT_DIR)',
                                    ],
                                }],
                            ],
                        },
                    ],
                },

                {
                    'target_name': 'VkLayer_core_validation',
                    'type': 'shared_library',
                    'dependencies':
                    [
                        'spirv_tools',
                        'vulkan_generate_layer_helpers',
                        'vulkan_layer_utils_static',
                    ],
                    'sources':
                    [
                        '<@(VkLayer_core_validation_sources)',
                    ],
                    'conditions':
                    [
                        ['OS=="win"',
                        {
                            'sources':
                            [
                                '<(vulkan_layers_path)/layers/VkLayer_core_validation.def',
                            ]
                        }],
                    ],
                },

                {
                    'target_name': 'VkLayer_image',
                    'type': 'shared_library',
                    'dependencies':
                    [
                        'vulkan_generate_layer_helpers',
                        'vulkan_layer_utils_static',
                    ],
                    'sources':
                    [
                        '<@(VkLayer_image_sources)',
                    ],
                    'conditions':
                    [
                        ['OS=="win"',
                        {
                            'sources':
                            [
                                '<(vulkan_layers_path)/layers/VkLayer_image.def',
                            ]
                        }],
                    ],
                },

                {
                    'target_name': 'VkLayer_swapchain',
                    'type': 'shared_library',
                    'dependencies':
                    [
                        'vulkan_generate_layer_helpers',
                        'vulkan_layer_utils_static',
                    ],
                    'sources':
                    [
                        '<@(VkLayer_swapchain_sources)',
                    ],
                    'conditions':
                    [
                        ['OS=="win"',
                        {
                            'sources':
                            [
                                '<(vulkan_layers_path)/layers/VkLayer_swapchain.def',
                            ]
                        }],
                    ],
                },

                {
                    'target_name': 'VkLayer_object_tracker',
                    'type': 'shared_library',
                    'dependencies':
                    [
                        'vulkan_generate_layer_helpers',
                        'vulkan_layer_utils_static',
                    ],
                    'sources':
                    [
                        '<@(VkLayer_object_tracker_sources)',
                    ],
                    'conditions':
                    [
                        ['OS=="win"',
                        {
                            'sources':
                            [
                                '<(vulkan_layers_path)/layers/VkLayer_object_tracker.def',
                            ]
                        }],
                    ],
                },

                {
                    'target_name': 'VkLayer_unique_objects',
                    'type': 'shared_library',
                    'dependencies':
                    [
                        'vulkan_generate_layer_helpers',
                        'vulkan_layer_utils_static',
                    ],
                    'sources':
                    [
                        '<@(VkLayer_unique_objects_sources)',
                    ],
                    'conditions':
                    [
                        ['OS=="win"',
                        {
                            'sources':
                            [
                                '<(vulkan_layers_path)/layers/VkLayer_unique_objects.def',
                            ]
                        }],
                    ],
                    'actions':
                    [
                        {
                            'action_name': 'vulkan_layer_unique_objects_generate',
                            'message': 'generating Vulkan unique_objects helpers',
                            'inputs':
                            [
                                '<(vulkan_layers_path)/scripts/generator.py',
                                '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                '<(vulkan_layers_path)/scripts/reg.py',
                                '<(vulkan_layers_path)/scripts/unique_objects_generator.py',
                                '<(vulkan_layers_path)/scripts/vk.xml',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/unique_objects_wrappers.h',
                            ],
                            'action':
                            [
                                'python', '<(vulkan_layers_path)/scripts/lvl_genvk.py', '-o', '<(angle_gen_path)/vulkan',
                                '-registry', '<(vulkan_layers_path)/scripts/vk.xml', 'unique_objects_wrappers.h', '-quiet',
                            ],
                        },
                    ],
                },

                {
                    'target_name': 'VkLayer_threading',
                    'type': 'shared_library',
                    'dependencies':
                    [
                        'vulkan_generate_layer_helpers',
                        'vulkan_layer_utils_static',
                    ],
                    'sources':
                    [
                        '<@(VkLayer_threading_sources)',
                    ],
                    'conditions':
                    [
                        ['OS=="win"',
                        {
                            'sources':
                            [
                                '<(vulkan_layers_path)/layers/VkLayer_threading.def',
                            ]
                        }],
                    ],
                    'actions':
                    [
                        {
                            'action_name': 'vulkan_layer_threading_generate',
                            'message': 'generating Vulkan threading header',
                            'inputs':
                            [
                                '<(vulkan_layers_path)/scripts/generator.py',
                                '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                '<(vulkan_layers_path)/scripts/reg.py',
                                '<(vulkan_layers_path)/scripts/vk.xml',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/thread_check.h',
                            ],
                            'action':
                            [
                                'python', '<(vulkan_layers_path)/scripts/lvl_genvk.py', '-o',
                                '<(angle_gen_path)/vulkan', '-registry', '<(vulkan_layers_path)/scripts/vk.xml',
                                'thread_check.h', '-quiet',
                            ],
                        },
                    ],
                },

                {
                    'target_name': 'VkLayer_parameter_validation',
                    'type': 'shared_library',
                    'dependencies':
                    [
                        'vulkan_generate_layer_helpers',
                        'vulkan_layer_utils_static',
                    ],
                    'sources':
                    [
                        '<@(VkLayer_parameter_validation_sources)',
                    ],
                    'conditions':
                    [
                        ['OS=="win"',
                        {
                            'sources':
                            [
                                '<(vulkan_layers_path)/layers/VkLayer_parameter_validation.def',
                            ]
                        }],
                    ],
                    'actions':
                    [
                        {
                            'action_name': 'vulkan_layer_parameter_validation_generate',
                            'message': 'generating Vulkan parameter_validation header',
                            'inputs':
                            [
                                '<(vulkan_layers_path)/scripts/generator.py',
                                '<(vulkan_layers_path)/scripts/lvl_genvk.py',
                                '<(vulkan_layers_path)/scripts/reg.py',
                                '<(vulkan_layers_path)/scripts/vk.xml',
                            ],
                            'outputs':
                            [
                                '<(angle_gen_path)/vulkan/parameter_validation.h',
                            ],
                            'action':
                            [
                                'python', '<(vulkan_layers_path)/scripts/lvl_genvk.py', '-o', '<(angle_gen_path)/vulkan',
                                '-registry', '<(vulkan_layers_path)/scripts/vk.xml', 'parameter_validation.h', '-quiet',
                            ],
                        },
                    ],
                },

                {
                    'target_name': 'angle_vulkan',
                    'type': 'none',
                    'dependencies':
                    [
                        'VkLayer_core_validation',
                        'VkLayer_image',
                        'VkLayer_object_tracker',
                        'VkLayer_parameter_validation',
                        'VkLayer_swapchain',
                        'VkLayer_threading',
                        'VkLayer_unique_objects',
                        'vulkan_loader',
                    ],
                    'export_dependent_settings':
                    [
                        'vulkan_loader',
                    ],
                }
            ],
        }],
    ],
}
