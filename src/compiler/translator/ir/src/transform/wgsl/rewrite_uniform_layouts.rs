// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Rewrite the types in uniform blocks that WGSL's uniform address space cannot hold:
// - `bool` / `bvecN` fields are stored as `u32` / `vecN<u32>` and converted back to `bool` /
//   `vecN<bool>` when loaded.
// - A user struct reachable from a uniform block is cloned into a dedicated struct type only when
//   one of its fields needs a different type, so non-uniform uses of the original struct type
//   remain untouched.
// - Whole-composite loads of such arrays or structs are converted back to the original type with
//   `BuiltInOpCode::ConvertFromUniformLayout`, which the WGSL generator implements with
//   `ANGLE_Convert_*` helper functions.
//
// Matrices, arrays and structs otherwise keep their type: with `uniform_buffer_standard_layout`,
// WGSL lays out the uniform address space like std430, the layout of the default uniform block.
//
// TODO(http://anglebug.com/575560156): std140 uniform blocks also need 16-byte struct alignment
// and a 16-byte stride for the columns of matCx2 and the elements of arrays of scalars and
// 2-component vectors.

use crate::ir::*;
use crate::*;
use std::collections::{HashMap, HashSet};

struct State<'a> {
    ir_meta: &'a mut IRMeta,
    /// The type uniform blocks store for each type already visited (the type itself if it needs
    /// no change).
    layout_types: HashMap<TypeId, TypeId>,
    /// Uniform block variables whose fields were changed.
    uniform_vars: HashSet<VariableId>,
    /// The pointer into the changed types that replaces each register pointing into
    /// `uniform_vars`.
    layout_ptrs: HashMap<RegisterId, TypedId>,
}

fn struct_fields(ir_meta: &IRMeta, struct_type_id: TypeId) -> &[Field] {
    match ir_meta.get_type(struct_type_id) {
        Type::Struct(_, fields, _) => fields,
        _ => unreachable!("Internal error: Expected a struct type"),
    }
}

impl<'a> State<'a> {
    fn new(ir_meta: &'a mut IRMeta) -> Self {
        Self {
            ir_meta,
            layout_types: HashMap::new(),
            uniform_vars: HashSet::new(),
            layout_ptrs: HashMap::new(),
        }
    }

    // Uniform data is always provided as 32-bit values, so the stored values are always High
    // precision; values are converted to their original precision when loaded.
    fn layout_precision(&self, layout_type_id: TypeId) -> Precision {
        if util::is_precision_applicable_to_type(self.ir_meta, layout_type_id) {
            Precision::High
        } else {
            Precision::NotApplicable
        }
    }

    /// Returns the fields of a struct with the types uniform blocks store, or None if every field
    /// already has one.
    fn get_layout_fields(&mut self, struct_type_id: TypeId) -> Option<Vec<Field>> {
        // The fields are cloned because `get_layout_type` needs `&mut self`.
        let mut any_changed = false;
        let layout_fields = struct_fields(self.ir_meta, struct_type_id)
            .to_vec()
            .into_iter()
            .map(|mut field| {
                let layout_type_id = self.get_layout_type(field.type_id);
                if layout_type_id == field.type_id {
                    return field;
                }
                any_changed = true;
                field.type_id = layout_type_id;
                field.precision = self.layout_precision(layout_type_id);
                field
            })
            .collect();
        any_changed.then_some(layout_fields)
    }

    fn get_layout_type(&mut self, type_id: TypeId) -> TypeId {
        if let Some(&cached) = self.layout_types.get(&type_id) {
            return cached;
        }

        let layout_type_id = match *self.ir_meta.get_type(type_id) {
            Type::Scalar(BasicType::Bool) => TYPE_ID_UINT,
            Type::Vector(TYPE_ID_BOOL, count) => {
                self.ir_meta.get_vector_type_id(BasicType::Uint, count)
            }
            Type::Array(elem_type_id, count) => {
                let layout_elem = self.get_layout_type(elem_type_id);
                if layout_elem == elem_type_id {
                    type_id
                } else {
                    self.ir_meta.get_array_type_id(layout_elem, count)
                }
            }
            Type::Struct(orig_name, _, StructSpecialization::Struct) => {
                match self.get_layout_fields(type_id) {
                    Some(layout_fields) => self.ir_meta.get_struct_type_id(
                        Name::new_temp(orig_name.name),
                        layout_fields,
                        StructSpecialization::Struct,
                    ),
                    None => type_id,
                }
            }
            _ => type_id,
        };

        self.layout_types.insert(type_id, layout_type_id);
        layout_type_id
    }

    /// Changes the fields of a uniform block to the types the block stores.  Returns whether any
    /// field changed.
    fn rewrite_interface_block_fields(&mut self, block_type_id: TypeId) -> bool {
        let Some(layout_fields) = self.get_layout_fields(block_type_id) else {
            return false;
        };
        // Interface block types are only used in the uniform address space, so their fields are
        // changed in place.  Other structs are cloned by `get_layout_type`.
        let Type::Struct(_, fields, StructSpecialization::InterfaceBlock) =
            self.ir_meta.get_type_mut(block_type_id)
        else {
            unreachable!("Internal error: Expected an interface block type");
        };
        *fields = layout_fields;
        true
    }

    fn resolve_layout_ptr(&self, ptr: TypedId) -> Option<TypedId> {
        match ptr.id {
            Id::Variable(var_id) => self.uniform_vars.contains(&var_id).then_some(ptr),
            Id::Register(reg_id) => self.layout_ptrs.get(&reg_id).copied(),
            Id::Constant(_) => None,
        }
    }

    fn load_and_convert(
        &mut self,
        builder: &mut instruction::Builder<traverser::Transforms>,
        layout_ptr: TypedId,
        orig_val_type_id: TypeId,
        precision: Precision,
    ) -> TypedId {
        let layout_val_type_id = self.ir_meta.get_pointee_type(layout_ptr.type_id);
        let raw_val = builder.load(self.ir_meta, layout_ptr);
        if layout_val_type_id == orig_val_type_id {
            return raw_val;
        }
        match *self.ir_meta.get_type(orig_val_type_id) {
            Type::Scalar(BasicType::Bool) | Type::Vector(TYPE_ID_BOOL, _) => {
                builder.construct(self.ir_meta, orig_val_type_id, vec![raw_val], None)
            }
            Type::Array(..) | Type::Struct(..) => builder.convert_from_uniform_layout(
                self.ir_meta,
                raw_val,
                orig_val_type_id,
                precision,
            ),
            _ => unreachable!("Internal error: Unexpected type requiring uniform value conversion"),
        }
    }
}

// Rewrites the instructions that use pointers into the rewritten uniform blocks:
// - `Access*` instructions and `Alias` produce the equivalent pointer into the changed types.
// - `Load` and calls (for pointer arguments) load the stored value and convert it back to the
//   original type.
// The remaining instructions that take pointers need no change: they write through them (`Store`,
// increments, atomics), which uniforms do not allow, or require unsized arrays (`ArrayLength`),
// which uniform blocks cannot have.  Instructions that take values need no change either, since
// every value read from a uniform comes from a converting load or call argument above.
fn transform_instruction(
    state: &mut State,
    instruction: &BlockInstruction,
) -> traverser::Transforms {
    let mut builder = instruction::Builder::<traverser::Transforms>::new();
    let (opcode, result) = instruction.get_op_and_result(state.ir_meta);

    let layout_ptr = match *opcode {
        // Load and convert the stored values passed to `in` parameters.
        OpCode::Call(function_id, ref args) => {
            if !args.iter().any(|&arg| state.resolve_layout_ptr(arg).is_some()) {
                return builder.finish();
            }
            let args = args.clone();
            return transform_call(state, builder, function_id, args, result);
        }
        // Load the stored value and convert it to the original type.
        OpCode::Load(ptr) => {
            let Some(layout_ptr) = state.resolve_layout_ptr(ptr) else {
                return builder.finish();
            };
            let result = result.unwrap();
            let final_val =
                state.load_and_convert(&mut builder, layout_ptr, result.type_id, result.precision);
            let transforms = builder.finish_with_result(state.ir_meta, result, final_val);
            // The stored value is High precision; keep the original precision of the result.
            state.ir_meta.get_instruction_mut(result.id).result.precision = result.precision;
            return transforms;
        }
        // The changed struct has the same fields, so access the same field.
        OpCode::AccessStructField(base_ptr, field_index) => {
            let Some(layout_base_ptr) = state.resolve_layout_ptr(base_ptr) else {
                return builder.finish();
            };
            builder.struct_field(state.ir_meta, layout_base_ptr, field_index)
        }
        // Index the changed array.  Matrices keep their type, but may be reached through a changed
        // struct.
        OpCode::AccessArrayElement(base_ptr, index_id)
        | OpCode::AccessMatrixColumn(base_ptr, index_id) => {
            let Some(layout_base_ptr) = state.resolve_layout_ptr(base_ptr) else {
                return builder.finish();
            };
            builder.index(state.ir_meta, layout_base_ptr, index_id)
        }
        // Vectors keep their components (`bvecN` is stored as `uvecN`), so access the same ones.
        OpCode::AccessVectorComponent(base_ptr, comp_index) => {
            let Some(layout_base_ptr) = state.resolve_layout_ptr(base_ptr) else {
                return builder.finish();
            };
            builder.vector_component(state.ir_meta, layout_base_ptr, comp_index)
        }
        OpCode::AccessVectorComponentMulti(base_ptr, ref indices) => {
            let indices = indices.clone();
            let Some(layout_base_ptr) = state.resolve_layout_ptr(base_ptr) else {
                return builder.finish();
            };
            builder.vector_component_multi(state.ir_meta, layout_base_ptr, indices)
        }
        OpCode::AccessVectorComponentDynamic(base_ptr, comp_index_id) => {
            let Some(layout_base_ptr) = state.resolve_layout_ptr(base_ptr) else {
                return builder.finish();
            };
            builder.index(state.ir_meta, layout_base_ptr, comp_index_id)
        }
        // An alias of a pointer into the changed types aliases that pointer.
        OpCode::Alias(ptr) => {
            let Some(layout_ptr) = state.resolve_layout_ptr(ptr) else {
                return builder.finish();
            };
            builder.alias(state.ir_meta, layout_ptr)
        }
        _ => return builder.finish(),
    };
    state.layout_ptrs.insert(result.unwrap().id, layout_ptr);
    builder.finish()
}

fn transform_call(
    state: &mut State,
    mut builder: instruction::Builder<traverser::Transforms>,
    function_id: FunctionId,
    args: Vec<TypedId>,
    result: Option<TypedRegisterId>,
) -> traverser::Transforms {
    let mut new_args = Vec::with_capacity(args.len());
    for (index, arg) in args.into_iter().enumerate() {
        if let Some(layout_ptr) = state.resolve_layout_ptr(arg) {
            debug_assert!(
                state.ir_meta.get_function(function_id).params[index].direction
                    == FunctionParamDirection::Input,
                "Internal error: Uniforms cannot be passed to out or inout parameters"
            );
            let orig_val_type_id = state.ir_meta.get_pointee_type(arg.type_id);
            new_args.push(state.load_and_convert(
                &mut builder,
                layout_ptr,
                orig_val_type_id,
                arg.precision,
            ));
        } else {
            new_args.push(arg);
        }
    }
    if let Some(result) = result {
        let call_res = builder.call(state.ir_meta, function_id, new_args);
        builder.finish_with_result(state.ir_meta, result, call_res)
    } else {
        builder.call_void(state.ir_meta, function_id, new_args);
        builder.finish()
    }
}

pub fn run(ir: &mut IR) {
    let mut state = State::new(&mut ir.meta);

    // Iterate over a copy of the ids: rewriting a block's fields adds types to `ir_meta`.
    for var_id in state.ir_meta.all_global_variables().clone() {
        let var = state.ir_meta.get_variable(var_id);
        if !var.decorations.has(Decoration::Uniform)
            || !has_decoration!(var.decorations, Decoration::Block)
        {
            continue;
        }
        let block_type_id =
            state.ir_meta.get_base_element_type(state.ir_meta.get_pointee_type(var.type_id));
        if state.rewrite_interface_block_fields(block_type_id) {
            state.uniform_vars.insert(var_id);
        }
    }

    if state.uniform_vars.is_empty() {
        return;
    }

    traverser::transformer::for_each_instruction(
        &mut state,
        &mut ir.function_entries,
        &|state, instruction| transform_instruction(state, instruction),
    );
}
