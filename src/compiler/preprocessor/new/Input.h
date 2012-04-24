//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_INPUT_H_
#define COMPILER_PREPROCESSOR_INPUT_H_

namespace pp
{

// Holds lexer input.
struct Input
{
    // Input.
    int count;
    const char* const* string;
    const int* length;

    // Current read position.
    int index;  // Index of string currently being scanned.
    void* buffer;  // Current buffer handle.

    Input();
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_INPUT_H_

