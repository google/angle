// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Transform gl_DrawID from ANGLE_multi_draw to a user uniform.
use crate::ir::*;

pub fn run(ir: &mut IR) {
    if let Some(draw_id) = ir.meta.get_built_in_variable(BuiltIn::DrawID) {
        let draw_id = ir.meta.get_variable_mut(draw_id);
        draw_id.name = Name::new_exact("angle_DrawID");
        draw_id.built_in = None;
        debug_assert!(draw_id.decorations.decorations.is_empty());
        draw_id.decorations.decorations.push(Decoration::Uniform);
    }
}
