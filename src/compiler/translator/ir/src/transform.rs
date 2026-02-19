// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
pub mod astify;
pub mod broadcast_fragcolor;
pub mod clamp_point_size;
pub mod dealias;
pub mod emulate_instanced_multiview;
pub mod emulate_multi_draw;
pub mod initialize_uninitialized_variables;
pub mod monomorphize_unsupported_functions;
pub mod propagate_precision;
pub mod prune_unused_variables;
pub mod remove_unused_framebuffer_fetch;
pub mod rewrite_pixel_local_storage;
pub mod sort_uniforms;

// Helper macro to run a transformation and automatically validate the IR afterwards
macro_rules! run {
    ($func:ident $(::$path:ident)*, $ir:expr $(, $params:expr)*) => {
        $crate::transform::$func$(::$path)*::run($ir $(, $params)*);
        $crate::ir::validate!($ir);
    };
}
pub(crate) use run;
