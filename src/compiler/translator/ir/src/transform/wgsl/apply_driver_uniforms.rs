// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Applies driver uniform values to the shader, e.g. flipping Y or adjusting depth range
// as specified by the driver uniforms.

use crate::ir::*;
use crate::transform::add_driver_uniforms::DriverUniforms;
use crate::*;

pub fn run(ir: &mut IR, driver_uniforms: &DriverUniforms) {
    match ir.meta.get_shader_type() {
        ShaderType::Vertex => transform_vertex_position(ir, driver_uniforms),
        ShaderType::Fragment => {
            flip_frag_coord(ir, driver_uniforms);
            flip_dfdy(ir, driver_uniforms);
        }
        _ => {}
    }
}

fn transform_vertex_position(ir: &mut IR, driver_uniforms: &DriverUniforms) {
    let Some(position_var_id) = ir.meta.get_built_in_variable(BuiltIn::Position) else {
        return;
    };
    let position = TypedId::from_variable_id(&ir.meta, position_var_id);

    let mut postamble = Block::new();
    let transform_depth = {
        let mut builder = instruction::Builder::<&mut Block>::new(&mut postamble);

        // Flip `gl_Position.y` for WebGPU's clip-space orientation if necessary.
        let flip_xy = driver_uniforms.flip_xy(&mut ir.meta, &mut builder);
        let flip_y = builder.vector_component(&mut ir.meta, flip_xy, 1);
        let pos_y_ptr = builder.vector_component(&mut ir.meta, position, 1);
        let pos_y = builder.load(&mut ir.meta, pos_y_ptr);
        let flipped_y = builder.mul(&mut ir.meta, pos_y, flip_y);
        builder.store(&mut ir.meta, pos_y_ptr, flipped_y);

        // Conditionally remap `gl_Position.z` from GL's `[-w, w]` to WebGPU's `[0, w]`.
        driver_uniforms.transform_depth(&mut ir.meta, &mut builder)
    };

    let mut remap_block = Block::new();
    {
        // z = ((z + w) * 0.5)
        let mut builder = instruction::Builder::<&mut Block>::new(&mut remap_block);
        let pos_z_ptr = builder.vector_component(&mut ir.meta, position, 2);
        let pos_w_ptr = builder.vector_component(&mut ir.meta, position, 3);
        let pos_z = builder.load(&mut ir.meta, pos_z_ptr);
        let pos_w = builder.load(&mut ir.meta, pos_w_ptr);
        let z_plus_w = builder.add(&mut ir.meta, pos_z, pos_w);
        let remapped_z = builder.mul(&mut ir.meta, z_plus_w, TYPED_CONSTANT_ID_FLOAT_HALF);
        builder.store(&mut ir.meta, pos_z_ptr, remapped_z);
        builder.branch_merge(None);
    }

    postamble.terminate(OpCode::If(transform_depth));
    postamble.set_if_true_block(remap_block);
    // `append_to_main` requires the merge chain's last block to be unterminated so it can attach
    // the original `main()` terminator.
    postamble.set_merge_block(Block::new());

    ir.append_to_main(postamble);
}

fn flip_frag_coord(ir: &mut IR, driver_uniforms: &DriverUniforms) {
    let Some(frag_coord_var_id) = ir.meta.get_built_in_variable(BuiltIn::FragCoord) else {
        return;
    };

    // Replace `gl_FragCoord` with a private global so existing reads use the adjusted value,
    // and initialize it from the built-in input at the start of `main()`.
    let built_in_frag_coord =
        ir.meta.declare_cached_global_for_variable(frag_coord_var_id, "FragCoord", None).1;
    let cached_frag_coord = TypedId::from_variable_id(&ir.meta, frag_coord_var_id);

    let mut preamble = Block::new();
    let mut builder = instruction::Builder::<&mut Block>::new(&mut preamble);

    // WebGPU's origin is top-left; mirror `.xy` around the render-area center while keeping `.zw`:
    //   cached = vec4((gl_FragCoord.xy - halfArea) * flipXY + halfArea,
    //                 gl_FragCoord.z, gl_FragCoord.w)
    let orig = builder.load(&mut ir.meta, built_in_frag_coord);
    let orig_xy = builder.vector_component_multi(&mut ir.meta, orig, vec![0, 1]);
    let orig_z = builder.vector_component(&mut ir.meta, orig, 2);
    let orig_w = builder.vector_component(&mut ir.meta, orig, 3);

    let half_area = driver_uniforms.half_render_area(&mut ir.meta, &mut builder);
    let flip_xy = driver_uniforms.flip_xy(&mut ir.meta, &mut builder);
    let centered = builder.sub(&mut ir.meta, orig_xy, half_area);
    let flipped_xy = builder.mul(&mut ir.meta, centered, flip_xy);
    let adjusted_xy = builder.add(&mut ir.meta, flipped_xy, half_area);

    let adjusted =
        builder.construct(&mut ir.meta, TYPE_ID_VEC4, vec![adjusted_xy, orig_z, orig_w], None);
    builder.store(&mut ir.meta, cached_frag_coord, adjusted);

    ir.prepend_to_main(preamble);
}

// TODO(http://anglebug.com/575560156): Fold into `rewrite_unsupported_operations` (or a combined
// WGSL instruction-walking pass) to avoid an extra IR traversal on fragment shaders without `dFdy`.
fn flip_dfdy(ir: &mut IR, driver_uniforms: &DriverUniforms) {
    traverser::transformer::for_each_instruction(
        &mut ir.meta,
        &mut ir.function_entries,
        &|ir_meta, instruction| {
            let (opcode, result) = instruction.get_op_and_result(ir_meta);
            let OpCode::Unary(UnaryOpCode::DFdy, operand) = *opcode else {
                return traverser::Transforms::new();
            };

            // WebGPU's `dpdy` is top-down, which is inverted relative to GL's on y-flipped
            // framebuffers (`flipXY.y == -1`).
            let mut builder = instruction::Builder::<traverser::Transforms>::new();
            let dfdy = builder.built_in_unary(ir_meta, UnaryOpCode::DFdy, operand);
            let flip_xy = driver_uniforms.flip_xy(ir_meta, &mut builder);
            let flip_y = builder.vector_component(ir_meta, flip_xy, 1);
            let flipped = if operand.type_id.is_vector() {
                builder.vector_times_scalar(ir_meta, dfdy, flip_y)
            } else {
                builder.mul(ir_meta, dfdy, flip_y)
            };
            builder.finish_with_result(ir_meta, result.unwrap(), flipped)
        },
    );
}
