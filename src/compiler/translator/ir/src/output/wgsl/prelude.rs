// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// WGSL helper functions that are declared on demand at the top of the generated shader, for GLSL
// built-ins that WGSL lacks.  Each helper is a fixed piece of WGSL text, declared at most once.

use crate::ir::*;

const INVERSE_MAT2: &str = "fn ANGLE_inverse_mat2(m : mat2x2<f32>) -> mat2x2<f32>
{
  let invDet = 1.0 / (m[0][0] * m[1][1] - m[0][1] * m[1][0]);
  return mat2x2<f32>(vec2<f32>(m[1][1], -m[0][1]), vec2<f32>(-m[1][0], m[0][0])) * invDet;
}

";

const INVERSE_MAT3: &str = "fn ANGLE_inverse_mat3(m : mat3x3<f32>) -> mat3x3<f32>
{
  let a00 = m[0][0]; let a01 = m[0][1]; let a02 = m[0][2];
  let a10 = m[1][0]; let a11 = m[1][1]; let a12 = m[1][2];
  let a20 = m[2][0]; let a21 = m[2][1]; let a22 = m[2][2];
  let b01 = a22 * a11 - a12 * a21;
  let b11 = -a22 * a10 + a12 * a20;
  let b21 = a21 * a10 - a11 * a20;
  let invDet = 1.0 / (a00 * b01 + a01 * b11 + a02 * b21);
  return mat3x3<f32>(
    vec3<f32>(b01, -a22 * a01 + a02 * a21, a12 * a01 - a02 * a11),
    vec3<f32>(b11, a22 * a00 - a02 * a20, -a12 * a00 + a02 * a10),
    vec3<f32>(b21, -a21 * a00 + a01 * a20, a11 * a00 - a01 * a10)
  ) * invDet;
}

";

const INVERSE_MAT4: &str = "fn ANGLE_inverse_mat4(m : mat4x4<f32>) -> mat4x4<f32>
{
  let s0 = m[0][0] * m[1][1] - m[1][0] * m[0][1];
  let s1 = m[0][0] * m[1][2] - m[1][0] * m[0][2];
  let s2 = m[0][0] * m[1][3] - m[1][0] * m[0][3];
  let s3 = m[0][1] * m[1][2] - m[1][1] * m[0][2];
  let s4 = m[0][1] * m[1][3] - m[1][1] * m[0][3];
  let s5 = m[0][2] * m[1][3] - m[1][2] * m[0][3];
  let c5 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
  let c4 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
  let c3 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
  let c2 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
  let c1 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
  let c0 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
  let invDet = 1.0 / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);
  return mat4x4<f32>(
    vec4<f32>(
      m[1][1] * c5 - m[1][2] * c4 + m[1][3] * c3,
      -m[0][1] * c5 + m[0][2] * c4 - m[0][3] * c3,
      m[3][1] * s5 - m[3][2] * s4 + m[3][3] * s3,
      -m[2][1] * s5 + m[2][2] * s4 - m[2][3] * s3),
    vec4<f32>(
      -m[1][0] * c5 + m[1][2] * c2 - m[1][3] * c1,
      m[0][0] * c5 - m[0][2] * c2 + m[0][3] * c1,
      -m[3][0] * s5 + m[3][2] * s2 - m[3][3] * s1,
      m[2][0] * s5 - m[2][2] * s2 + m[2][3] * s1),
    vec4<f32>(
      m[1][0] * c4 - m[1][1] * c2 + m[1][3] * c0,
      -m[0][0] * c4 + m[0][1] * c2 - m[0][3] * c0,
      m[3][0] * s4 - m[3][1] * s2 + m[3][3] * s0,
      -m[2][0] * s4 + m[2][1] * s2 - m[2][3] * s0),
    vec4<f32>(
      -m[1][0] * c3 + m[1][1] * c1 - m[1][2] * c0,
      m[0][0] * c3 - m[0][1] * c1 + m[0][2] * c0,
      -m[3][0] * s3 + m[3][1] * s1 - m[3][2] * s0,
      m[2][0] * s3 - m[2][1] * s1 + m[2][2] * s0)
  ) * invDet;
}

";

const REFLECT_F32: &str = "fn ANGLE_reflect_f32(i : f32, n : f32) -> f32
{
  return i - 2.0 * n * i * n;
}

";

const FACEFORWARD_F32: &str = "fn ANGLE_faceforward_f32(n : f32, i : f32, nref : f32) -> f32
{
  return select(-n, n, nref * i < 0.0);
}

";

const REFRACT_F32: &str = "fn ANGLE_refract_f32(i : f32, n : f32, eta : f32) -> f32
{
  let d = n * i;
  let k = 1.0 - eta * eta * (1.0 - d * d);
  return select(eta * i - (eta * d + sqrt(max(k, 0.0))) * n, 0.0, k < 0.0);
}

";

#[derive(Default)]
struct Emitted {
    inverse_mat2: bool,
    inverse_mat3: bool,
    inverse_mat4: bool,
    reflect_f32: bool,
    faceforward_f32: bool,
    refract_f32: bool,
}

/// Declares the helper `name` with the WGSL text `body` unless already declared, and returns a
/// call to it with `args`.
fn call(
    text: &mut String,
    emitted: &mut bool,
    name: &'static str,
    body: &'static str,
    args: &[&str],
) -> String {
    if !*emitted {
        *emitted = true;
        text.push_str(body);
    }
    format!("{name}({})", args.join(", "))
}

pub struct Prelude {
    text: String,
    emitted: Emitted,
}

impl Prelude {
    pub fn new() -> Prelude {
        Prelude { text: String::new(), emitted: Emitted::default() }
    }

    /// Returns a call to the `ANGLE_inverse_mat{2,3,4}` helper that inverts `arg` of the square
    /// matrix type `type_id`.  WGSL has no built-in matrix `inverse()`.
    pub fn inverse_call(&mut self, type_id: TypeId, arg: &str) -> String {
        match type_id {
            TYPE_ID_MAT2 => call(
                &mut self.text,
                &mut self.emitted.inverse_mat2,
                "ANGLE_inverse_mat2",
                INVERSE_MAT2,
                &[arg],
            ),
            TYPE_ID_MAT3 => call(
                &mut self.text,
                &mut self.emitted.inverse_mat3,
                "ANGLE_inverse_mat3",
                INVERSE_MAT3,
                &[arg],
            ),
            TYPE_ID_MAT4 => call(
                &mut self.text,
                &mut self.emitted.inverse_mat4,
                "ANGLE_inverse_mat4",
                INVERSE_MAT4,
                &[arg],
            ),
            _ => unreachable!("Internal error: Unexpected type for inverse"),
        }
    }

    /// Returns a call to the `ANGLE_reflect_f32` helper.  WGSL defines `reflect` only for
    /// vectors.
    pub fn scalar_reflect_call(&mut self, i: &str, n: &str) -> String {
        call(
            &mut self.text,
            &mut self.emitted.reflect_f32,
            "ANGLE_reflect_f32",
            REFLECT_F32,
            &[i, n],
        )
    }

    /// Returns a call to the `ANGLE_faceforward_f32` helper.  WGSL defines `faceForward` only for
    /// vectors.
    pub fn scalar_faceforward_call(&mut self, n: &str, i: &str, nref: &str) -> String {
        call(
            &mut self.text,
            &mut self.emitted.faceforward_f32,
            "ANGLE_faceforward_f32",
            FACEFORWARD_F32,
            &[n, i, nref],
        )
    }

    /// Returns a call to the `ANGLE_refract_f32` helper.  WGSL defines `refract` only for
    /// vectors.
    pub fn scalar_refract_call(&mut self, i: &str, n: &str, eta: &str) -> String {
        call(
            &mut self.text,
            &mut self.emitted.refract_f32,
            "ANGLE_refract_f32",
            REFRACT_F32,
            &[i, n, eta],
        )
    }

    /// Returns the WGSL text of all declared helpers.
    pub fn text(&self) -> &str {
        &self.text
    }
}
