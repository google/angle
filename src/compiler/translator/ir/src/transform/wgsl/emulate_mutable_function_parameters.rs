// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// WGSL function parameters are immutable, while GLSL `in` parameters can be written to in the
// function body.  This pass gives every `in` parameter that is written to a mutable local copy,
// initialized from the parameter at the start of the function.  For example:
//
//     float f(float p) { p += 1.0; return p; }
//
// becomes:
//
//     float f(float p_in) { float p = p_in; p += 1.0; return p; }
//
// The function body is not modified to reference the copy.  Instead, the parameter's existing
// variable is turned into the local variable, and a duplicate of it becomes the parameter.
//
// This pass can be removed if WGSL gains mutable function parameters, see
// https://github.com/gpuweb/gpuweb/issues/4113.

use crate::ir::*;
use crate::*;

pub fn run(ir: &mut IR) {
    traverser::transformer::for_each_function(
        &mut ir.meta,
        &mut ir.function_entries,
        &|ir_meta, function_id, entry| {
            let written_params = find_written_input_params(ir_meta, function_id, entry);
            if !written_params.is_empty() {
                copy_params_to_locals(ir_meta, function_id, entry, &written_params);
            }
        },
    );
}

// Returns the indices of the function's `in` parameters that are written to in the function body.
fn find_written_input_params(
    ir_meta: &IRMeta,
    function_id: FunctionId,
    entry: &Block,
) -> Vec<usize> {
    let params = &ir_meta.get_function(function_id).params;
    let mut is_written = vec![false; params.len()];

    traverser::visitor::visit(
        &mut is_written,
        entry,
        traverser::BlockKind::Entry,
        &|is_written, block, _, _| {
            for instruction in &block.instructions {
                let (opcode, _) = instruction.get_op_and_result(ir_meta);
                util::inspect_pointer_access(
                    ir_meta,
                    is_written,
                    opcode,
                    &|is_written, pointer, access| {
                        if access != util::PointerAccess::Read
                            && let Some(variable_id) =
                                util::trace_back_to_variable(ir_meta, pointer.id)
                            && let Some(index) = params.iter().position(|param| {
                                param.variable_id == variable_id
                                    && param.direction == FunctionParamDirection::Input
                            })
                        {
                            is_written[index] = true;
                        }
                    },
                );
            }
            traverser::visitor::VISIT_SUB_BLOCKS
        },
        0,
    );

    is_written.iter().enumerate().filter_map(|(index, &written)| written.then_some(index)).collect()
}

// Turns the given parameters of the function into local variables, each initialized from a new
// parameter that takes its place.
fn copy_params_to_locals(
    ir_meta: &mut IRMeta,
    function_id: FunctionId,
    entry: &mut Block,
    param_indices: &[usize],
) {
    if param_indices.is_empty() {
        return;
    }

    let mut copy_block = Block::new();

    for &index in param_indices {
        let local_id = ir_meta.get_function(function_id).params[index].variable_id;
        let param_id = util::duplicate_variable(ir_meta, local_id);

        ir_meta.get_function_mut(function_id).params[index].variable_id = param_id;
        ir_meta.get_variable_mut(local_id).scope = VariableScope::Local;
        copy_block.add_variable_declaration(local_id);

        // local = param
        let local = TypedId::from_variable_id(ir_meta, local_id);
        let param = TypedId::from_variable_id(ir_meta, param_id);
        let mut builder = instruction::Builder::<&mut Block>::new(&mut copy_block);
        let param_value = builder.load(ir_meta, param);
        builder.store(ir_meta, local, param_value);
    }

    entry.prepend_code(copy_block);
}
