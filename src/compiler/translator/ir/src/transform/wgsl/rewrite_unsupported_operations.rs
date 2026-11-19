// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Operations that GLSL supports but WGSL either lacks or defines differently are handled in one of
// three places depending on how they map to WGSL:
//
// - Direct translation in `output/wgsl.rs`: Used when an operation maps to a simple single WGSL
//   expression
// - WGSL helper functions (`output/wgsl/prelude.rs`): Used when a self-contained helper function is
//   clearer and more compact as WGSL text than built instruction-by-instruction in IR. Some
//   existing IR opcodes (`UnaryOpCode::Inverse`, scalar `Reflect` / `Faceforward` / `Refract`) emit
//   calls to fixed helpers in `output/wgsl/prelude.rs` directly; when an IR transformation needs to
//   introduce a call to a generated WGSL helper (such as
//   `BuiltInOpCode::ConvertFromUniformLayout`), a WGSL-specific `BuiltInOpCode` can be added in
//   `ir.rs` and implemented in the WGSL backend.
// - IR transformations (this pass): transformations more complex than a single line but not worth
//   bloating the resulting WGSL with a prelude function.
//
// Specifically, this pass rewrites:
// 1. Prefix and postfix increment/decrement (`++`, `--`) -> load + add/sub + store.
// 2. Equality and inequality (`==`, `!=`) on vectors -> `all(a == b)` (negated for `!=`), as WGSL
//    `==` and `!=` on vectors return vectors.  On matrices and structs -> per-column and per-field
//    comparisons combined with `all(vecN<bool>(...))`, and on arrays -> a loop over the elements.
//    WGSL forbids `==` and `!=` on composite types (see
//    https://github.com/gpuweb/gpuweb/issues/2128).
// 3. Mixed vector-scalar `&`, `|`, `^`, `min`, `max`, `step`, `clamp` and `smoothstep` -> the
//    scalar operands are splatted to the vector type.  WGSL has no mixed overloads for these
//    (see https://github.com/gpuweb/gpuweb/issues/1865).
// 4. Shifts (`<<`, `>>`) -> the shift amount is converted to unsigned and splatted if the shifted
//    value is a vector.  Constant shift amounts are masked with `31u`, as WGSL rejects constant
//    amounts >= 32 (and takes runtime amounts modulo 32).
// 5. Float `mod(x, y)` -> `x - y * floor(x / y)`.  WGSL `%` is a truncated remainder, whereas GLSL
//    `mod` requires a floored remainder.
// 6. Matrix `+` and `-` with a scalar, matrix `/` (matrix or scalar operands) and
//    `matrixCompMult` -> per-column operations.  WGSL has no such matrix operators (see
//    https://github.com/gpuweb/gpuweb/issues/963).
// 7. `outerProduct(c, r)` -> a matrix whose column `j` is `c * r[j]`.
// 8. Vector and matrix constructors rejected by WGSL: `matCxR(scalar)`, mixed-type `vecN(args...)`,
//    and mixed or non-float `matCxR(args...)` are built from scalar components via `util` helpers.
// 9. `vecN(scalar)` with a scalar of a different basic type -> the scalar is converted first, as
//    WGSL vector splats require the scalar to have the vector's component type.

use crate::ir::*;
use crate::*;
pub fn run(ir: &mut IR) {
    traverser::transformer::for_each_instruction(
        &mut ir.meta,
        &mut ir.function_entries,
        &rewrite_math_op,
    );
}

fn rewrite_math_op(ir_meta: &mut IRMeta, instruction: &BlockInstruction) -> traverser::Transforms {
    let (opcode, result) = instruction.get_op_and_result(ir_meta);
    match *opcode {
        OpCode::Unary(
            op @ (UnaryOpCode::PrefixIncrement
            | UnaryOpCode::PostfixIncrement
            | UnaryOpCode::PrefixDecrement
            | UnaryOpCode::PostfixDecrement),
            pointer,
        ) => rewrite_unary_increment_decrement(ir_meta, op, pointer, result.unwrap()),
        OpCode::Binary(op @ (BinaryOpCode::Equal | BinaryOpCode::NotEqual), lhs, rhs) => {
            rewrite_equality(ir_meta, op, lhs, rhs, result.unwrap())
        }
        OpCode::Binary(
            op @ (BinaryOpCode::BitwiseAnd
            | BinaryOpCode::BitwiseOr
            | BinaryOpCode::BitwiseXor
            | BinaryOpCode::Min
            | BinaryOpCode::Max
            | BinaryOpCode::Step),
            lhs,
            rhs,
        ) => rewrite_mixed_vector_scalar_binary(ir_meta, op, lhs, rhs, result.unwrap()),
        OpCode::Binary(
            op @ (BinaryOpCode::BitShiftLeft | BinaryOpCode::BitShiftRight),
            lhs,
            rhs,
        ) => rewrite_shift_amount(ir_meta, op, lhs, rhs, result.unwrap()),
        OpCode::Binary(BinaryOpCode::Mod, lhs, rhs) => {
            rewrite_float_mod(ir_meta, lhs, rhs, result.unwrap())
        }
        OpCode::Binary(
            op @ (BinaryOpCode::Add
            | BinaryOpCode::Sub
            | BinaryOpCode::Div
            | BinaryOpCode::MatrixCompMult),
            lhs,
            rhs,
        ) => rewrite_matrix_componentwise_binary(ir_meta, op, lhs, rhs, result.unwrap()),
        OpCode::Binary(BinaryOpCode::OuterProduct, lhs, rhs) => {
            rewrite_outer_product(ir_meta, lhs, rhs, result.unwrap())
        }
        // The vector operand is `x`, which is first in `clamp(x, minVal, maxVal)` and last in
        // `smoothstep(edge0, edge1, x)`.
        OpCode::BuiltIn(BuiltInOpCode::Clamp, ref args) => rewrite_mixed_vector_scalar_built_in(
            ir_meta,
            BuiltInOpCode::Clamp,
            args.clone(),
            0,
            result.unwrap(),
        ),
        OpCode::BuiltIn(BuiltInOpCode::Smoothstep, ref args) => {
            rewrite_mixed_vector_scalar_built_in(
                ir_meta,
                BuiltInOpCode::Smoothstep,
                args.clone(),
                2,
                result.unwrap(),
            )
        }
        OpCode::ConstructVectorFromScalar(arg) => {
            cast_vector_from_scalar_arg(ir_meta, arg, result.unwrap())
        }
        OpCode::ConstructVectorFromMultiple(ref args) => {
            let result = result.unwrap();
            let component_type_id = ir_meta.get_scalar_type(result.type_id);
            if args.len() <= 1
                || args.iter().all(|arg| ir_meta.get_scalar_type(arg.type_id) == component_type_id)
            {
                traverser::Transforms::new()
            } else {
                let args = args.clone();
                crate::util::build_vector_from_multiple(ir_meta, &args, result)
            }
        }
        OpCode::ConstructMatrixFromScalar(arg) => {
            crate::util::build_matrix_from_scalar(ir_meta, arg, result.unwrap())
        }
        OpCode::ConstructMatrixFromMatrix(arg) => {
            let result = result.unwrap();
            if arg.type_id == result.type_id {
                traverser::Transforms::new()
            } else {
                rewrite_matrix_from_matrix(ir_meta, arg, result)
            }
        }
        OpCode::ConstructMatrixFromMultiple(ref args) => {
            let result = result.unwrap();
            let &Type::Matrix(col_vec_type_id, _) = ir_meta.get_type(result.type_id) else {
                unreachable!("Internal error: Expected matrix result type");
            };
            let scalar_type_id = ir_meta.get_scalar_type(col_vec_type_id);
            if args.iter().all(|arg| arg.type_id == col_vec_type_id)
                || args.iter().all(|arg| arg.type_id == scalar_type_id)
            {
                traverser::Transforms::new()
            } else {
                let args = args.clone();
                crate::util::build_matrix_from_multiple(ir_meta, &args, result)
            }
        }
        _ => traverser::Transforms::new(),
    }
}

fn rewrite_unary_increment_decrement(
    ir_meta: &mut IRMeta,
    op: UnaryOpCode,
    pointer: TypedId,
    result: TypedRegisterId,
) -> traverser::Transforms {
    let (delta_is_add, is_prefix) = match op {
        UnaryOpCode::PrefixIncrement => (true, true),
        UnaryOpCode::PostfixIncrement => (true, false),
        UnaryOpCode::PrefixDecrement => (false, true),
        UnaryOpCode::PostfixDecrement => (false, false),
        _ => unreachable!("Internal error: Expected increment or decrement opcode"),
    };

    // Multi-element swizzle lvalues (e.g. `v.xy++`) cannot be directly stored through a WGSL
    // reference and are handled by `rewrite_multielement_swizzle_assignment` in a later pass.
    if let Id::Register(ptr_reg) = pointer.id
        && let Id::Register(aliased_reg) = ir_meta.get_aliased_id(ptr_reg).id
        && let OpCode::AccessVectorComponentMulti(..) = &ir_meta.get_instruction(aliased_reg).op
    {
        return traverser::Transforms::new();
    }

    let pointee_type_id = ir_meta.get_pointee_type(pointer.type_id);
    let target_precision = pointer.precision;

    let scalar_one = match ir_meta.get_scalar_type(pointee_type_id) {
        TYPE_ID_FLOAT => CONSTANT_ID_FLOAT_ONE,
        TYPE_ID_INT => CONSTANT_ID_INT_ONE,
        TYPE_ID_UINT => CONSTANT_ID_UINT_ONE,
        _ => unreachable!("Internal error: Unexpected scalar type for increment/decrement"),
    };

    let one = match *ir_meta.get_type(pointee_type_id) {
        Type::Scalar(_) => TypedId::from_constant_id(scalar_one, pointee_type_id, target_precision),
        Type::Vector(_, count) => ir_meta.get_constant_composite_typed(
            pointee_type_id,
            vec![scalar_one; count as usize],
            target_precision,
        ),
        Type::Matrix(col_type_id, cols) => {
            let rows = ir_meta.get_type(col_type_id).get_vector_size().unwrap();
            let col_one =
                ir_meta.get_constant_composite(col_type_id, vec![scalar_one; rows as usize]);
            ir_meta.get_constant_composite_typed(
                pointee_type_id,
                vec![col_one; cols as usize],
                target_precision,
            )
        }
        _ => unreachable!("Internal error: Unexpected pointee type for increment/decrement"),
    };

    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    let old = builder.load(ir_meta, pointer);
    let new_val =
        if delta_is_add { builder.add(ir_meta, old, one) } else { builder.sub(ir_meta, old, one) };
    builder.store(ir_meta, pointer, new_val);
    let ret_val = if is_prefix { new_val } else { old };
    builder.finish_with_result(ir_meta, result, ret_val)
}

fn rewrite_equality(
    ir_meta: &mut IRMeta,
    op: BinaryOpCode,
    lhs: TypedId,
    rhs: TypedId,
    result: TypedRegisterId,
) -> traverser::Transforms {
    if ir_meta.get_type(lhs.type_id).is_scalar() {
        return traverser::Transforms::new();
    }
    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    let eq = compare_values(ir_meta, &mut builder, lhs, rhs);
    let res = if op == BinaryOpCode::NotEqual { builder.logical_not(ir_meta, eq) } else { eq };
    builder.finish_with_result(ir_meta, result, res)
}

/// Compares two values of the same type for equality, returning a scalar `bool`.  Vectors are
/// compared with `all(lhs == rhs)`, matrices per column, structs per field, and arrays per element
/// in a loop.
fn compare_values<C: instruction::InstructionContainer>(
    ir_meta: &mut IRMeta,
    builder: &mut instruction::Builder<C>,
    lhs: TypedId,
    rhs: TypedId,
) -> TypedId {
    match *ir_meta.get_type(lhs.type_id) {
        Type::Array(_, count) => compare_arrays_in_loop(ir_meta, builder, lhs, rhs, count),
        Type::Matrix(_, count) => {
            let conds = (0..count)
                .map(|i| {
                    let index = ir_meta.get_constant_uint_typed(i, Precision::Low);
                    let l = builder.index(ir_meta, lhs, index);
                    let r = builder.index(ir_meta, rhs, index);
                    compare_values(ir_meta, builder, l, r)
                })
                .collect();
            all_of(ir_meta, builder, conds)
        }
        Type::Struct(_, ref fields, _) => {
            let field_count = fields.len() as u32;
            let conds = (0..field_count)
                .map(|field| {
                    let l = builder.struct_field(ir_meta, lhs, field);
                    let r = builder.struct_field(ir_meta, rhs, field);
                    compare_values(ir_meta, builder, l, r)
                })
                .collect();
            all_of(ir_meta, builder, conds)
        }
        Type::Vector(..) => {
            let eq = builder.built_in_binary(ir_meta, BinaryOpCode::EqualVec, lhs, rhs);
            builder.built_in_unary(ir_meta, UnaryOpCode::All, eq)
        }
        _ => builder.equal(ir_meta, lhs, rhs),
    }
}

/// Compares two arrays of `count` elements in a loop that breaks at the first unequal element:
///
///     var equal = true;
///     for (var i = 0u; i < count; i += 1u) {
///         if (!(lhs[i] == rhs[i])) {
///             equal = false;
///             break;
///         }
///     }
fn compare_arrays_in_loop<C: instruction::InstructionContainer>(
    ir_meta: &mut IRMeta,
    builder: &mut instruction::Builder<C>,
    lhs: TypedId,
    rhs: TypedId,
    count: u32,
) -> TypedId {
    let (equal_id, equal) = ir_meta.declare_private_variable(
        Name::new_temp("equal"),
        TYPE_ID_BOOL,
        Precision::NotApplicable,
        Some(CONSTANT_ID_TRUE),
        VariableScope::Local,
    );
    let (index_id, index) = ir_meta.declare_private_variable(
        Name::new_temp("i"),
        TYPE_ID_UINT,
        Precision::High,
        Some(CONSTANT_ID_UINT_ZERO),
        VariableScope::ForLoopVariable,
    );
    builder.declare(equal_id);
    builder.for_loop(
        ir_meta,
        |init_builder, _| init_builder.declare(index_id),
        |cond_builder, ir_meta| {
            let i = cond_builder.load(ir_meta, index);
            let count = ir_meta.get_constant_uint_typed(count, Precision::High);
            let in_range = cond_builder.less_than(ir_meta, i, count);
            cond_builder.branch_loop_if(in_range);
        },
        |step_builder, ir_meta| {
            let i = step_builder.load(ir_meta, index);
            let one = TYPED_CONSTANT_ID_UINT_ONE;
            let next = step_builder.add(ir_meta, i, one);
            step_builder.store(ir_meta, index, next);
            step_builder.branch_continue();
        },
        |body_builder, ir_meta| {
            let i = body_builder.load(ir_meta, index);
            let l = body_builder.index(ir_meta, lhs, i);
            let r = body_builder.index(ir_meta, rhs, i);
            let element_equal = compare_values(ir_meta, body_builder, l, r);
            let element_not_equal = body_builder.logical_not(ir_meta, element_equal);
            body_builder.if_then(ir_meta, element_not_equal, |then_builder, ir_meta| {
                then_builder.store(ir_meta, equal, TYPED_CONSTANT_ID_FALSE);
                then_builder.branch_break();
            });
            body_builder.branch_continue();
        },
    );
    builder.load(ir_meta, equal)
}

/// Returns the conjunction of `conds` by packing up to four scalar `bool`s at a time into a
/// `bvecN` and applying `all`.
fn all_of<C: instruction::InstructionContainer>(
    ir_meta: &mut IRMeta,
    builder: &mut instruction::Builder<C>,
    mut conds: Vec<TypedId>,
) -> TypedId {
    if conds.is_empty() {
        return TYPED_CONSTANT_ID_TRUE;
    }
    while conds.len() > 1 {
        conds = conds
            .chunks(4)
            .map(|chunk| match chunk {
                [only] => *only,
                _ => {
                    let bvec_type_id =
                        ir_meta.get_vector_type_id(BasicType::Bool, chunk.len() as u32);
                    let bvec = builder.construct(ir_meta, bvec_type_id, chunk.to_vec(), None);
                    builder.built_in_unary(ir_meta, UnaryOpCode::All, bvec)
                }
            })
            .collect();
    }
    conds[0]
}

/// Splats the scalar `value` to `vector_type_id`, whose component type must match its type.
fn splat_vec(
    ir_meta: &mut IRMeta,
    builder: &mut instruction::Builder<traverser::Transforms>,
    value: TypedId,
    vector_type_id: TypeId,
) -> TypedId {
    builder.construct(ir_meta, vector_type_id, vec![value], None)
}

fn rewrite_mixed_vector_scalar_binary(
    ir_meta: &mut IRMeta,
    op: BinaryOpCode,
    lhs: TypedId,
    rhs: TypedId,
    result: TypedRegisterId,
) -> traverser::Transforms {
    let lhs_is_vector = ir_meta.get_type(lhs.type_id).is_vector();
    let rhs_is_vector = ir_meta.get_type(rhs.type_id).is_vector();
    if lhs_is_vector == rhs_is_vector {
        return traverser::Transforms::new();
    }
    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    let (lhs, rhs) = if lhs_is_vector {
        (lhs, splat_vec(ir_meta, &mut builder, rhs, lhs.type_id))
    } else {
        (splat_vec(ir_meta, &mut builder, lhs, rhs.type_id), rhs)
    };
    let res = builder.binary(ir_meta, op, lhs, rhs);
    builder.finish_with_result(ir_meta, result, res)
}

fn rewrite_mixed_vector_scalar_built_in(
    ir_meta: &mut IRMeta,
    op: BuiltInOpCode,
    mut args: Vec<TypedId>,
    vector_index: usize,
    result: TypedRegisterId,
) -> traverser::Transforms {
    let vector_type_id = args[vector_index].type_id;
    if !ir_meta.get_type(vector_type_id).is_vector()
        || args.iter().all(|arg| ir_meta.get_type(arg.type_id).is_vector())
    {
        return traverser::Transforms::new();
    }
    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    for arg in args.iter_mut() {
        if !ir_meta.get_type(arg.type_id).is_vector() {
            *arg = splat_vec(ir_meta, &mut builder, *arg, vector_type_id);
        }
    }
    let res = builder.built_in(ir_meta, op, args);
    builder.finish_with_result(ir_meta, result, res)
}

fn rewrite_shift_amount(
    ir_meta: &mut IRMeta,
    op: BinaryOpCode,
    lhs: TypedId,
    rhs: TypedId,
    result: TypedRegisterId,
) -> traverser::Transforms {
    let rhs_size = ir_meta.get_type(rhs.type_id).get_vector_size();
    let lhs_size = ir_meta.get_type(lhs.type_id).get_vector_size();
    let needs_splat = rhs_size.is_none() && lhs_size.is_some();
    if ir_meta.get_scalar_type(rhs.type_id) == TYPE_ID_UINT && !rhs.id.is_constant() && !needs_splat
    {
        return traverser::Transforms::new();
    }
    let uint_type_id = match rhs_size {
        Some(size) => ir_meta.get_vector_type_id_from_element_id(TYPE_ID_UINT, size),
        None => TYPE_ID_UINT,
    };

    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    let mut amount = crate::util::cast(ir_meta, &mut builder, rhs, uint_type_id, rhs.precision);
    // WGSL takes runtime shift amounts modulo 32, but rejects constant amounts >= 32.  Masking
    // constant amounts gives them the same result (and folds).
    if amount.id.is_constant() {
        let mut mask = ir_meta.get_constant_uint_typed(31, rhs.precision);
        if rhs_size.is_some() {
            mask = splat_vec(ir_meta, &mut builder, mask, uint_type_id);
        }
        amount = builder.bitwise_and(ir_meta, amount, mask);
    }
    if needs_splat {
        let uvec_type_id =
            ir_meta.get_vector_type_id_from_element_id(TYPE_ID_UINT, lhs_size.unwrap());
        amount = splat_vec(ir_meta, &mut builder, amount, uvec_type_id);
    }
    let res = builder.binary(ir_meta, op, lhs, amount);
    builder.finish_with_result(ir_meta, result, res)
}

fn rewrite_float_mod(
    ir_meta: &mut IRMeta,
    lhs: TypedId,
    rhs: TypedId,
    result: TypedRegisterId,
) -> traverser::Transforms {
    debug_assert!(ir_meta.get_scalar_type(lhs.type_id) == TYPE_ID_FLOAT);
    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    let quotient = builder.div(ir_meta, lhs, rhs);
    let floored = builder.unary(ir_meta, UnaryOpCode::Floor, quotient);
    let product = builder.mul(ir_meta, rhs, floored);
    let res = builder.sub(ir_meta, lhs, product);
    builder.finish_with_result(ir_meta, result, res)
}

fn rewrite_matrix_componentwise_binary(
    ir_meta: &mut IRMeta,
    op: BinaryOpCode,
    lhs: TypedId,
    rhs: TypedId,
    result: TypedRegisterId,
) -> traverser::Transforms {
    let lhs_is_matrix = ir_meta.get_type(lhs.type_id).is_matrix();
    let rhs_is_matrix = ir_meta.get_type(rhs.type_id).is_matrix();
    // WGSL supports non-matrix `+`/`-`/`/`, and `+` and `-` on two matrices.
    if (!lhs_is_matrix && !rhs_is_matrix)
        || (lhs_is_matrix && rhs_is_matrix && matches!(op, BinaryOpCode::Add | BinaryOpCode::Sub))
    {
        return traverser::Transforms::new();
    }
    let column_op = if op == BinaryOpCode::MatrixCompMult { BinaryOpCode::Mul } else { op };
    let matrix_type_id = if lhs_is_matrix { lhs.type_id } else { rhs.type_id };
    let column_count = ir_meta.get_type(matrix_type_id).get_matrix_size().unwrap();
    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    let columns = (0..column_count)
        .map(|column| {
            let index = ir_meta.get_constant_uint_typed(column, Precision::Low);
            let l = if lhs_is_matrix { builder.index(ir_meta, lhs, index) } else { lhs };
            let r = if rhs_is_matrix { builder.index(ir_meta, rhs, index) } else { rhs };
            builder.binary(ir_meta, column_op, l, r)
        })
        .collect();
    let res = builder.construct(ir_meta, matrix_type_id, columns, Some(result.precision));
    builder.finish_with_result(ir_meta, result, res)
}

fn rewrite_outer_product(
    ir_meta: &mut IRMeta,
    lhs: TypedId,
    rhs: TypedId,
    result: TypedRegisterId,
) -> traverser::Transforms {
    let column_count = ir_meta.get_type(rhs.type_id).get_vector_size().unwrap();
    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    let columns = (0..column_count)
        .map(|column| {
            let r = builder.vector_component(ir_meta, rhs, column);
            builder.vector_times_scalar(ir_meta, lhs, r)
        })
        .collect();
    let res = builder.construct(ir_meta, result.type_id, columns, Some(result.precision));
    builder.finish_with_result(ir_meta, result, res)
}

/// Rewrites a cross-dimension `matCxR(matAxB)` constructor into column vectors (`m[c]`,
/// `m[c].xyz`, or `vecN(m[c], 0.0, 1.0)` plus constant identity columns), since WGSL only allows
/// matrix-from-matrix construction when dimensions match.
///
/// Unlike `scalarize_vec_and_mat_constructor_args` (which uses `util::construct_matrix_from_matrix`
/// to unpack every column into individual scalar components for OpenGL drivers that reject vector
/// arguments in matrix constructors), this keeps whole columns and swizzles instead of multiplying
/// per-component loads.  In some cases scalarizing causes slowdown of shaders compiled by
/// Tint/native drivers.
fn rewrite_matrix_from_matrix(
    ir_meta: &mut IRMeta,
    arg: TypedId,
    result: TypedRegisterId,
) -> traverser::Transforms {
    let &Type::Matrix(col_vec_type_id, col_count) = ir_meta.get_type(result.type_id) else {
        unreachable!("Internal error: Expected matrix result type");
    };
    let row_count = ir_meta.get_type(col_vec_type_id).get_vector_size().unwrap();
    let &Type::Matrix(in_col_vec_type_id, in_cols) = ir_meta.get_type(arg.type_id) else {
        unreachable!("Internal error: Expected matrix operand type");
    };
    let in_rows = ir_meta.get_type(in_col_vec_type_id).get_vector_size().unwrap();

    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    let cols = (0..col_count)
        .map(|col| {
            if col < in_cols {
                let col_idx = ir_meta.get_constant_uint_typed(col, Precision::Low);
                let in_col = builder.index(ir_meta, arg, col_idx);
                if row_count == in_rows {
                    in_col
                } else if row_count < in_rows {
                    builder.vector_component_multi(ir_meta, in_col, (0..row_count).collect())
                } else {
                    let mut col_args = Vec::with_capacity(1 + (row_count - in_rows) as usize);
                    col_args.push(in_col);
                    for row in in_rows..row_count {
                        col_args.push(if row == col {
                            TYPED_CONSTANT_ID_FLOAT_ONE
                        } else {
                            TYPED_CONSTANT_ID_FLOAT_ZERO
                        });
                    }
                    builder.construct(ir_meta, col_vec_type_id, col_args, Some(result.precision))
                }
            } else {
                let components =
                    (0..row_count)
                        .map(|row| {
                            if row == col { CONSTANT_ID_FLOAT_ONE } else { CONSTANT_ID_FLOAT_ZERO }
                        })
                        .collect();
                ir_meta.get_constant_composite_typed(col_vec_type_id, components, result.precision)
            }
        })
        .collect();
    let res = builder.construct(ir_meta, result.type_id, cols, Some(result.precision));
    builder.finish_with_result(ir_meta, result, res)
}

fn cast_vector_from_scalar_arg(
    ir_meta: &mut IRMeta,
    arg: TypedId,
    result: TypedRegisterId,
) -> traverser::Transforms {
    let component_type_id = ir_meta.get_scalar_type(result.type_id);
    if arg.type_id == component_type_id {
        return traverser::Transforms::new();
    }
    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    let arg = crate::util::cast(ir_meta, &mut builder, arg, component_type_id, result.precision);
    let res = builder.construct(ir_meta, result.type_id, vec![arg], Some(result.precision));
    builder.finish_with_result(ir_meta, result, res)
}
