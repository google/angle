// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Rewrite texelFetchOffset as texelFetch with the offset added to the coordinates.

use crate::ir::*;
use crate::*;

pub fn transform(
    ir_meta: &mut IRMeta,
    sampler: TypedId,
    coord: TypedId,
    lod: TypedId,
    offset: TypedId,
    result: TypedRegisterId,
) -> traverser::Transforms {
    let mut builder = instruction::Builder::<traverser::Transforms>::new();

    // For 2D and 3D samplers, coord and offset have matching types, so offset' = offset.
    // For 2D array samplers, coord is ivec3 and offset is ivec2. Keep the layer unchanged:
    //
    //     offset' = ConstructVectorFromMultiple offset 0
    let offset = if coord.type_id != offset.type_id {
        debug_assert!(coord.type_id == TYPE_ID_IVEC3 && offset.type_id == TYPE_ID_IVEC2);
        builder.construct(ir_meta, coord.type_id, vec![offset, TYPED_CONSTANT_ID_INT_ZERO], None)
    } else {
        offset
    };

    // Transform:
    //
    //     result = TexelFetchOffset sampler coord lod offset
    //
    // Into:
    //
    //     coord' = Add coord offset'
    //     result = TexelFetch sampler coord' lod
    let coord = builder.add(ir_meta, coord, offset);
    let fetched = builder.built_in(ir_meta, BuiltInOpCode::TexelFetch, vec![sampler, coord, lod]);
    builder.finish_with_result(ir_meta, result, fetched)
}
