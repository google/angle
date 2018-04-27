#!/usr/bin/python2
#
# Copyright 2017 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# run_code_reneration.py:
#   Runs  ANGLE format table and other script run_code_renerationgeneration.

import os, subprocess, sys

root_dir = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

# auto_script is a standard way for scripts to return their inputs and outputs.

def grab_from_script(script, param):
    res = subprocess.check_output(['python', script, param]).strip()
    return [os.path.abspath(os.path.join(os.path.dirname(script), name)) for name in res.split(',')]

def auto_script(script):
    # Set the CWD to the script directory.
    os.chdir(os.path.dirname(os.path.abspath(script)))
    base_script = os.path.basename(script)
    return {
        'script': script,
        'inputs': grab_from_script(base_script, 'inputs'),
        'outputs': grab_from_script(base_script, 'outputs'),
    }

# TODO(jmadill): Convert everyting to auto-script.
generators = {
    'ANGLE format': {
        'inputs': [
            'src/libANGLE/renderer/angle_format.py',
            'src/libANGLE/renderer/angle_format_data.json',
            'src/libANGLE/renderer/angle_format_map.json',
        ],
        'outputs': [
            'src/libANGLE/renderer/Format_table_autogen.cpp',
            'src/libANGLE/renderer/Format_ID_autogen.inl',
        ],
        'script': 'src/libANGLE/renderer/gen_angle_format_table.py',
    },
    'ANGLE load functions table': {
        'inputs': [
            'src/libANGLE/renderer/load_functions_data.json',
        ],
        'outputs': [
            'src/libANGLE/renderer/load_functions_table_autogen.cpp',
        ],
        'script': 'src/libANGLE/renderer/gen_load_functions_table.py',
    },
    'D3D11 format': {
        'inputs': [
            'src/libANGLE/renderer/angle_format.py',
            'src/libANGLE/renderer/d3d/d3d11/texture_format_data.json',
            'src/libANGLE/renderer/d3d/d3d11/texture_format_map.json',
        ],
        'outputs': [
            'src/libANGLE/renderer/d3d/d3d11/texture_format_table_autogen.cpp',
        ],
        'script': 'src/libANGLE/renderer/d3d/d3d11/gen_texture_format_table.py',
    },
    'DXGI format': {
        'inputs': [
            'src/libANGLE/renderer/angle_format.py',
            'src/libANGLE/renderer/angle_format_map.json',
            'src/libANGLE/renderer/d3d/d3d11/dxgi_format_data.json',
        ],
        'outputs': [
            'src/libANGLE/renderer/d3d/d3d11/dxgi_format_map_autogen.cpp',
        ],
        'script': 'src/libANGLE/renderer/d3d/d3d11/gen_dxgi_format_table.py',
    },
    'DXGI format support': {
        'inputs': [
            'src/libANGLE/renderer/d3d/d3d11/dxgi_support_data.json',
        ],
        'outputs': [
            'src/libANGLE/renderer/d3d/d3d11/dxgi_support_table.cpp',
        ],
        'script': 'src/libANGLE/renderer/d3d/d3d11/gen_dxgi_support_tables.py',
    },
    'GL copy conversion table': {
        'inputs': [
            'src/libANGLE/es3_copy_conversion_formats.json',
        ],
        'outputs': [
            'src/libANGLE/es3_copy_conversion_table_autogen.cpp',
        ],
        'script': 'src/libANGLE/gen_copy_conversion_table.py',
    },
    'GL entry point': {
        'inputs': [
            'scripts/entry_point_packed_gl_enums.json',
            'scripts/gl.xml',
        ],
        'outputs': [
            'src/libGLESv2/entry_points_gles_2_0_autogen.cpp',
            'src/libGLESv2/entry_points_gles_2_0_autogen.h',
            'src/libGLESv2/entry_points_gles_3_0_autogen.cpp',
            'src/libGLESv2/entry_points_gles_3_0_autogen.h',
        ],
        'script': 'scripts/generate_entry_points.py',
    },
    'GL format map': {
        'inputs': [
            'src/libANGLE/es3_format_type_combinations.json',
            'src/libANGLE/format_map_data.json',
        ],
        'outputs': [
            'src/libANGLE/format_map_autogen.cpp',
        ],
        'script': 'src/libANGLE/gen_format_map.py',
    },
    'uniform type': {
        'inputs': [],
        'outputs': [
            'src/common/uniform_type_info_autogen.cpp',
        ],
        'script': 'src/common/gen_uniform_type_table.py',
    },
    'OpenGL dispatch table': {
        'inputs': [
            'scripts/gl.xml',
        ],
        'outputs': [
            'src/libANGLE/renderer/gl/DispatchTableGL_autogen.cpp',
            'src/libANGLE/renderer/gl/DispatchTableGL_autogen.h',
            'src/libANGLE/renderer/gl/null_functions.h',
            'src/libANGLE/renderer/gl/null_functions.cpp',
        ],
        'script': 'src/libANGLE/renderer/gl/generate_gl_dispatch_table.py',
    },
    'packed enum': {
        'inputs': [
            'src/libANGLE/packed_gl_enums.json',
            'src/libANGLE/packed_egl_enums.json',
        ],
        'outputs': [
            'src/libANGLE/PackedEGLEnums_autogen.cpp',
            'src/libANGLE/PackedEGLEnums_autogen.h',
            'src/libANGLE/PackedGLEnums_autogen.cpp',
            'src/libANGLE/PackedGLEnums_autogen.h',
        ],
        'script': 'src/libANGLE/gen_packed_gl_enums.py',
    },
    'proc table': {
        'inputs': [
            'src/libGLESv2/proc_table_data.json',
        ],
        'outputs': [
            'src/libGLESv2/proc_table_autogen.cpp',
        ],
        'script': 'src/libGLESv2/gen_proc_table.py',
    },
    'Vulkan format': {
        'inputs': [
            'src/libANGLE/renderer/angle_format.py',
            'src/libANGLE/renderer/angle_format_map.json',
            'src/libANGLE/renderer/vulkan/vk_format_map.json',
        ],
        'outputs': [
            'src/libANGLE/renderer/vulkan/vk_format_table_autogen.cpp',
        ],
        'script': 'src/libANGLE/renderer/vulkan/gen_vk_format_table.py',
    },
    'Vulkan mandatory format support table': {
        'inputs': [
            'src/libANGLE/renderer/angle_format.py',
            'third_party/vulkan-validation-layers/src/scripts/vk.xml',
            'src/libANGLE/renderer/vulkan/vk_mandatory_format_support_data.json',
        ],
        'outputs': [
            'src/libANGLE/renderer/vulkan/vk_mandatory_format_support_table_autogen.cpp',
        ],
        'script': 'src/libANGLE/renderer/vulkan/gen_vk_mandatory_format_support_table.py',
    },
    'Vulkan internal shader programs':
        auto_script('src/libANGLE/renderer/vulkan/gen_vk_internal_shaders.py'),
    'Emulated HLSL functions': {
        'inputs': [
            'src/compiler/translator/emulated_builtin_function_data_hlsl.json'
        ],
        'outputs': [
            'src/compiler/translator/emulated_builtin_functions_hlsl_autogen.cpp'
        ],
        'script': 'src/compiler/translator/gen_emulated_builtin_function_tables.py'
    },
    'ESSL static builtins': {
        'inputs': [
            'src/compiler/translator/builtin_function_declarations.txt',
            'src/compiler/translator/builtin_variables.json',
        ],
        'outputs': [
            'src/compiler/translator/tree_util/BuiltIn_autogen.h',
            'src/compiler/translator/builtin_symbols_hash_autogen.txt',
            'src/compiler/translator/ParseContext_autogen.h',
            'src/compiler/translator/SymbolTable_autogen.cpp',
            'src/compiler/translator/SymbolTable_autogen.h',
            'src/tests/compiler_tests/ImmutableString_test_autogen.cpp',
        ],
        'script': 'src/compiler/translator/gen_builtin_symbols.py',
    },
}

any_dirty = False

for name, info in sorted(generators.iteritems()):

    # Set the CWD to the root ANGLE directory.
    os.chdir(root_dir)

    script = info['script']
    dirty = False

    for finput in info['inputs'] + [script]:
        input_mtime = os.path.getmtime(finput)
        for foutput in info['outputs']:
            if not os.path.exists(foutput):
                print('Output ' + foutput + ' not found for ' + name + ' table')
                dirty = True
            else:
                output_mtime = os.path.getmtime(foutput)
                # Use a fuzzy comparison to avoid tiny time delta errors.
                if input_mtime - output_mtime > 0.1:
                    dirty = True

    if dirty:
        any_dirty = True

        # Set the CWD to the script directory.
        os.chdir(os.path.dirname(os.path.abspath(script)))

        print('Running ' + name + ' code generator')
        if subprocess.call(['python', os.path.basename(script)]) != 0:
            sys.exit(1)

if any_dirty:
    args = []
    if os.name == 'nt':
        args += ['git.bat']
    else:
        args += ['git']
    args += ['cl', 'format']
    print('Calling git cl format')
    subprocess.call(args)
