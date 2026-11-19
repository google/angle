// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A set of independent transformations that are applied in one pass.
//
// * rewrite_texel_fetch_offset: transforms TexelFetchOffset built-ins.

use crate::ir::*;
use crate::*;

use crate::transform::rewrite_texel_fetch_offset;

pub struct Options {
    pub rewrite_texel_fetch_offset: bool,
}

pub fn run(ir: &mut IR, options: &Options) {
    traverser::transformer::for_each_instruction(
        &mut ir.meta,
        &mut ir.function_entries,
        &|ir_meta, instruction| {
            let (opcode, result) = instruction.get_op_and_result(ir_meta);
            match *opcode {
                OpCode::BuiltIn(BuiltInOpCode::TexelFetchOffset, ref args)
                    if options.rewrite_texel_fetch_offset =>
                {
                    rewrite_texel_fetch_offset::transform(
                        ir_meta,
                        args[0],
                        args[1],
                        args[2],
                        args[3],
                        result.unwrap(),
                    )
                }
                _ => traverser::Transforms::new(),
            }
        },
    );
}
