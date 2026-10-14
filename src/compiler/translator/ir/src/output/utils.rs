// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Output utilities for code generators.

pub fn get_swizzle(index: u32) -> &'static str {
    match index {
        0 => "x",
        1 => "y",
        2 => "z",
        3 => "w",
        _ => unreachable!("Internal error: Unexpected swizzle index"),
    }
}

pub fn get_swizzle_multi(indices: &[u32]) -> String {
    indices.iter().map(|&index| get_swizzle(index)).collect::<Vec<_>>().join("")
}
