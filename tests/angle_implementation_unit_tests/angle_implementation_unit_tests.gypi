# Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'sources':
    [
        'Config_unittest.cpp',
        'Fence_unittest.cpp',
        'ImageIndexIterator_unittest.cpp',
        'Surface_unittest.cpp',
        'TransformFeedback_unittest.cpp'
    ],
    'conditions':
    [
        ['angle_build_winrt==1',
        {
            'sources':
            [
                'CoreWindowNativeWindow_unittest.cpp',
                'SwapChainPanelNativeWindow_unittest.cpp',
            ],
            'defines':
            [
                'ANGLE_ENABLE_D3D11',
            ],
            'msvs_settings':
            {
                'VCLinkerTool':
                {
                    'AdditionalDependencies':
                    [
                        'runtimeobject.lib',
                    ],
                },
            },
        }],
    ],
}
