// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A helper to validate the rules of IR.  This is useful particularly to be run after
// transformations, to ensure they generate valid IR.
//
// Validations implemented:
//   - Every ID must be present in the respective map: validate_all_ids_are_present()
//   - Every variable must be defined somewhere, either in global block or in a block:
//     validate_all_variables_are_declared_in_scope()
//   - Every accessed variable must be declared in an accessible block:
//     validate_all_variables_are_declared_in_scope()
//   - Every accessed register must be declared in an accessible block:
//     validate_all_registers_are_declared_in_scope()

// TODO(http://anglebug.com/349994211): to validate:
//   - Branches must have the appropriate targets set (merge, trueblock for if, etc).
//   - No branches inside a block, every block ends in branch. (i.e. no dead code)
//   - For merge blocks that have an input, the branch instruction of blocks that jump to it have an
//     output.
//   - No identical types with different IDs.
//   - If there's a cached "has side effect", that it's correct.
//   - Validate that ImageType fields are valid in combination with ImageDimension
//   - No operations should have entirely constant arguments, that should be folded (and
//     transformations shouldn't retintroduce it)
//   - Catch misuse of built-in names.
//   - Precision is not applied to types that don't are not applicable.  It _is_ applied to types
//     that are applicable (including uniforms and samplers for example).  Needs to work to make
//     sure precision is always assigned.
//   - Case values are always ConstantId (int/uint only too?)
//   - Variables are Pointers
//   - Pointers only valid in the left arg of load/store/access/call
//   - Loop blocks ends in the appropriate instructions.
//   - Do blocks end in DoLoop (unless already terminated by something else, like Return)
//   - If condition is a bool.
//   - Maximum one default case for Switch instructions.
//   - No pointer->pointer type.
//   - Interface variables with NameSource::Internal are unique.
//   - NameSource::Internal names don't start with the user and temporary name prefixes (_u, t and f
//     respectively).
//   - Interface variables with NameSource::ShaderInterface are unique.
//   - NameSource::ShaderInterface and NameSource::Internal are never found inside body
//   - blocks, those should always be Temporary.
//   - No identity swizzles.
//   - Type matches?
//   - Block inputs have MergeInput opcode, nothing else has that opcode.
//   - Block inputs are not pointers.  AST-ifier does not handle that.
//   - Whatever else is in the AST validation currently.
//   - Validate built-ins that accept an out or inout parameter, that the corresponding parameter is
//     passed a Pointer at the call site.  For that matter, do the same for user function calls too.
//   - Check for invalid texture* combinations, like non-shadow samplers with TextureCompare ops, or
//     is_proj is false for cubemaps.
//   - Check that function parameter variables don't have an initializer.
//   - Check that returned values from functions match the type of the function's return value.
//   - Validate that if there's a merge variable in an if block, both true and false blocks exist
//     and they both end in a Merge with an ID.  (Technically, we should be able to also support one
//     block being merge and the other discard/return/break/continue, but no such code can be
//     generated right now).

use crate::debug;
use crate::ir::*;
use crate::traverser;
use std::collections::HashSet;
use std::fmt;

pub fn validate(ir: &IR) {
    let validator = Validator::new(ir);
    validator.validate();
}

#[derive(Copy, Clone, PartialEq)]
enum TypedIdValidationCategory {
    // check id does not exceed max constant_id, max register_id, max variable_id
    IdInBound,
    // check variabld_id is declared in the current accessible scope
    VariableDeclared,
    // check register_id is declared in the current accessible scope
    RegisterDeclared,
}

struct DeclaredVarTracker {
    declared_vars_in_current_scope: Vec<HashSet<u32>>,
}

impl DeclaredVarTracker {
    fn new() -> DeclaredVarTracker {
        DeclaredVarTracker { declared_vars_in_current_scope: Vec::new() }
    }

    fn set_global_declared_vars(&mut self, ir_meta_global_vars: &Vec<VariableId>) {
        // global_vars should be the first hash set to be added into the
        // declared_vars_in_current_scope
        debug_assert!(self.declared_vars_in_current_scope.is_empty());
        let mut global_vars = HashSet::new();
        for global_var in ir_meta_global_vars {
            global_vars.insert(global_var.id);
        }
        self.declared_vars_in_current_scope.push(global_vars);
    }

    // Some variables are declared in the function parameters.
    // For example:
    // void my_function(int function_param_var)
    // {
    //   // do something with function_param_var
    // }
    // function_param_var is declared in the function parameter
    fn add_function_param_vars_upon_enter_function(
        &mut self,
        function_parameters: &Vec<FunctionParam>,
    ) {
        // global_vars should be the only hash set in declared_vars_in_current_scope before we add
        // current function param variables
        debug_assert!(self.declared_vars_in_current_scope.len() == 1);
        let mut function_param_vars = HashSet::new();
        for function_param in function_parameters {
            function_param_vars.insert(function_param.variable_id.id);
        }
        self.declared_vars_in_current_scope.push(function_param_vars);
    }

    fn remove_function_param_vars_upon_exit_function(&mut self) {
        self.declared_vars_in_current_scope.pop().unwrap();
        // global_vars should be the only hash set in declared_vars_in_current_scope after we pop
        // current function param variables
        debug_assert!(self.declared_vars_in_current_scope.len() == 1);
    }

    fn add_current_scope_declared_vars_upon_enter_scope(
        &mut self,
        parent_declared_vars: &Vec<VariableId>,
    ) {
        let mut parent_declared_var_map = HashSet::new();

        for parent_var in parent_declared_vars {
            parent_declared_var_map.insert(parent_var.id);
        }

        self.declared_vars_in_current_scope.push(parent_declared_var_map);
    }

    fn remove_current_scope_declared_vars_upon_exit_scope(&mut self) {
        self.declared_vars_in_current_scope.pop().unwrap();
    }

    fn is_variable_declared(&self, variable_id: VariableId) -> bool {
        for declared_var_map in &self.declared_vars_in_current_scope {
            if declared_var_map.contains(&variable_id.id) {
                return true;
            }
        }
        return false;
    }
}

struct DeclaredRegisterTracker {
    declared_registers_in_current_scope: Vec<HashSet<RegisterId>>,
}

impl DeclaredRegisterTracker {
    fn new() -> DeclaredRegisterTracker {
        DeclaredRegisterTracker { declared_registers_in_current_scope: Vec::new() }
    }

    fn add_scope(&mut self) {
        self.declared_registers_in_current_scope.push(HashSet::new());
    }

    fn remove_scope(&mut self) {
        self.declared_registers_in_current_scope.pop().unwrap();
    }

    fn declare_register<'a>(&mut self, register_id: RegisterId, validator: &Validator<'a>) {
        // first check we have not declared this register yet
        if self.is_declared(register_id) {
            validator.on_error(format_args!(
                "register {} is already declared, can't declare the same register twice",
                register_id.id
            ));
        }
        // add the register to the declaration map
        self.declared_registers_in_current_scope.last_mut().unwrap().insert(register_id);
    }

    fn is_declared(&self, register_id: RegisterId) -> bool {
        for declared_registers in self.declared_registers_in_current_scope.iter().rev() {
            if declared_registers.contains(&register_id) {
                return true;
            }
        }
        false
    }
}

// Validator takes a reference of IR object, and its' lifetime is the same as the lifetime of IR
// object
struct Validator<'a> {
    ir: &'a IR,
    max_type_count: u32,
    max_variable_count: u32,
    max_constant_count: u32,
    max_register_count: u32,
}

impl<'a> Validator<'a> {
    // Validator constructor
    fn new(ir: &'a IR) -> Validator<'a> {
        Validator {
            ir,
            max_type_count: ir.meta.all_types().len() as u32,
            max_variable_count: ir.meta.all_variables().len() as u32,
            max_constant_count: ir.meta.all_constants().len() as u32,
            max_register_count: ir.meta.total_register_count(),
        }
    }

    // ANGLE IR validation entry point
    fn validate(&self) {
        self.validate_all_ids_are_present();
        self.validate_all_variables_are_declared_in_scope();
        self.validate_all_registers_are_declared_in_scope();
    }

    fn validate_all_ids_are_present(&self) {
        self.validate_all_ids_are_present_in_ir_meta();
        self.validate_all_ids_are_present_in_ir_function_entries();
    }

    fn validate_all_ids_are_present_in_ir_meta(&self) {
        // validate IRMeta.constants
        for (constant_index, constant) in self.ir.meta.all_constants().iter().enumerate() {
            self.validate_all_ids_in_a_constant_are_present(
                constant_index.try_into().unwrap(),
                constant,
            );
        }
        // validate IRMeta.variables
        for (variable_index, variable) in self.ir.meta.all_variables().iter().enumerate() {
            self.validate_all_ids_in_a_variable_are_present(
                variable_index.try_into().unwrap(),
                variable,
            );
        }
        // validate IRMeta.functions
        for (function_index, function) in self.ir.meta.all_functions().iter().enumerate() {
            self.validate_all_ids_in_a_function_are_present(
                function_index.try_into().unwrap(),
                function,
            );
        }

        // Validate IRMeta.global_variables
        for (global_variable_index, global_variable) in
            self.ir.meta.all_global_variables().iter().enumerate()
        {
            if global_variable.id >= self.max_variable_count {
                self.on_error(format_args!(
                    "invalid IRMeta: global_variable {global_variable_index} uses invalid \
                     variable id {}",
                    global_variable.id
                ));
            }
        }

        // validate IRMeta.variables_pending_zero_initialization
        for (variable_pending_zero_initialization_index, variable_id) in
            self.ir.meta.get_variables_pending_zero_initialization().iter().enumerate()
        {
            if variable_id.id >= self.max_variable_count {
                self.on_error(format_args!(
                    "invalid IRMeta: variable_pending_zero_initialization \
                     {variable_pending_zero_initialization_index} uses invalid variable id {}",
                    variable_id.id
                ));
            }
        }
    }

    fn validate_all_ids_are_present_in_ir_function_entries(&self) {
        traverser::visitor::for_each_function(
            &mut (), // no state to track while traversing all functions
            &self.ir.function_entries,
            |_, _| {}, // do nothing in pre_visit
            |_, block, _, _| {
                self.validate_all_ids_are_present_in_block(block);
                traverser::visitor::VISIT_SUB_BLOCKS
            }, // validate ids in a block during visit
            |_, _| {}, // do nothing in post_visit
        );
    }

    // Helper function to check Constant member type_id is valid
    fn validate_all_ids_in_a_constant_are_present(&self, constant_id: u32, constant: &Constant) {
        if constant.type_id.id >= self.max_type_count {
            self.on_error(format_args!(
                "Constant id {constant_id} has invalid type id {}",
                constant.type_id.id
            ));
        }
    }

    // Helper function to check Variable members type_id, initializer are valid
    fn validate_all_ids_in_a_variable_are_present(&self, variable_id: u32, variable: &Variable) {
        // Check type_id
        if variable.type_id.id >= self.max_type_count {
            self.on_error(format_args!(
                "Variable id {variable_id} has invalid type id {}",
                variable.type_id.id
            ));
        }
        // Check initializer
        if let Some(valid_initializer) = variable.initializer
            && valid_initializer.id >= self.max_constant_count
        {
            self.on_error(format_args!(
                "Variable id {variable_id} has invalid constant initializer {}",
                valid_initializer.id
            ));
        }
    }

    // Helper function to check Function members return_type_id, params are valid
    fn validate_all_ids_in_a_function_are_present(&self, function_id: u32, function: &Function) {
        // Check return type
        if function.return_type_id.id >= self.max_type_count {
            self.on_error(format_args!(
                "Function id {function_id} has invalid return type {}",
                function.return_type_id.id
            ));
        }

        // Check function parameters
        for param in &function.params {
            if param.variable_id.id >= self.max_variable_count {
                self.on_error(format_args!(
                    "Function id {function_id} has invalid parameters {}",
                    param.variable_id.id
                ));
            }
        }
    }

    // TODO yuxinhu@google.com
    // Write a helper function that walks through a block once and collect all information
    // needed for validations

    fn validate_all_ids_are_present_in_block(&self, block: &Block) {
        // validate block variables
        for variable in &block.variables {
            if variable.id >= self.max_variable_count {
                self.on_error(format_args!(
                    "invalid variable id {} found in block variabls",
                    variable.id
                ));
            }
        }
        // validate input
        if let Some(valid_input) = block.input {
            self.validate_block_input_has_valid_ids(&valid_input);
        }
        // validate instructions
        for instruction in &block.instructions {
            let (opcode, result) = instruction.get_op_and_result(&self.ir.meta);
            self.validate_instruction_op_code_typed_id_parameters(
                opcode,
                TypedIdValidationCategory::IdInBound,
                None,
                None,
            );
            if let Some(instruction_result) = result {
                self.validate_opcode_instruction_result_has_valid_ids(opcode, &instruction_result);
            }
        }
    }

    // Validate OpCode parameters
    fn validate_instruction_op_code_typed_id_parameters(
        &self,
        op_code: &OpCode,
        category: TypedIdValidationCategory,
        declared_variables: Option<&DeclaredVarTracker>,
        declared_registers: Option<&DeclaredRegisterTracker>,
    ) {
        match op_code {
            // OpCode that does not take any parameters: do nothing
            OpCode::MergeInput
            | OpCode::Discard
            | OpCode::Break
            | OpCode::Continue
            | OpCode::Passthrough
            | OpCode::NextBlock
            | OpCode::Loop
            | OpCode::DoLoop
            | OpCode::Return(None)
            | OpCode::Merge(None) => (),
            // OpCode that takes in Vec<TypedId> params, verify Vec<TypedId> params
            OpCode::Call(_, params)
            | OpCode::ConstructVectorFromMultiple(params)
            | OpCode::ConstructMatrixFromMultiple(params)
            | OpCode::ConstructStruct(params)
            | OpCode::ConstructArray(params)
            | OpCode::BuiltIn(_, params) => {
                for param in params {
                    self.validate_typed_id_params(
                        op_code,
                        param,
                        category,
                        declared_variables,
                        declared_registers,
                    );
                }
            }
            // OpCode that takes in TypedId params, verify TypedId
            OpCode::Return(Some(id))
            | OpCode::Merge(Some(id))
            | OpCode::If(id)
            | OpCode::LoopIf(id)
            | OpCode::Switch(id, _)
            | OpCode::ExtractVectorComponent(id, _)
            | OpCode::ExtractVectorComponentMulti(id, _)
            | OpCode::ExtractStructField(id, _)
            | OpCode::ConstructScalarFromScalar(id)
            | OpCode::ConstructVectorFromScalar(id)
            | OpCode::ConstructMatrixFromScalar(id)
            | OpCode::ConstructMatrixFromMatrix(id)
            | OpCode::AccessVectorComponent(id, _)
            | OpCode::AccessVectorComponentMulti(id, _)
            | OpCode::AccessStructField(id, _)
            | OpCode::Load(id)
            | OpCode::Alias(id)
            | OpCode::Unary(_, id) => {
                self.validate_typed_id_params(
                    op_code,
                    id,
                    category,
                    declared_variables,
                    declared_registers,
                );
            }
            // OpCode that takes two TypedId, verify both TypedId
            OpCode::ExtractVectorComponentDynamic(lhs, rhs)
            | OpCode::ExtractMatrixColumn(lhs, rhs)
            | OpCode::ExtractArrayElement(lhs, rhs)
            | OpCode::AccessVectorComponentDynamic(lhs, rhs)
            | OpCode::AccessMatrixColumn(lhs, rhs)
            | OpCode::AccessArrayElement(lhs, rhs)
            | OpCode::Store(lhs, rhs)
            | OpCode::Binary(_, lhs, rhs) => {
                self.validate_typed_id_params(
                    op_code,
                    lhs,
                    category,
                    declared_variables,
                    declared_registers,
                );
                self.validate_typed_id_params(
                    op_code,
                    rhs,
                    category,
                    declared_variables,
                    declared_registers,
                );
            }
            // OpCode that takes Another OpCode (texture_op) as Parameter
            OpCode::Texture(texture_op, sampler, coord) => {
                self.validate_typed_id_params(
                    texture_op,
                    sampler,
                    category,
                    declared_variables,
                    declared_registers,
                );
                self.validate_typed_id_params(
                    texture_op,
                    coord,
                    category,
                    declared_variables,
                    declared_registers,
                );
                match texture_op {
                    TextureOpCode::Implicit { is_proj: _, offset }
                    | TextureOpCode::Gather { offset } => {
                        if let Some(valid_offset) = offset {
                            self.validate_typed_id_params(
                                texture_op,
                                valid_offset,
                                category,
                                declared_variables,
                                declared_registers,
                            );
                        }
                    }
                    TextureOpCode::Compare { compare } => {
                        self.validate_typed_id_params(
                            texture_op,
                            compare,
                            category,
                            declared_variables,
                            declared_registers,
                        );
                    }
                    TextureOpCode::Lod { is_proj: _, lod, offset } => {
                        self.validate_typed_id_params(
                            texture_op,
                            lod,
                            category,
                            declared_variables,
                            declared_registers,
                        );

                        if let Some(valid_offset) = offset {
                            self.validate_typed_id_params(
                                texture_op,
                                valid_offset,
                                category,
                                declared_variables,
                                declared_registers,
                            );
                        }
                    }
                    TextureOpCode::CompareLod { compare, lod } => {
                        self.validate_typed_id_params(
                            texture_op,
                            compare,
                            category,
                            declared_variables,
                            declared_registers,
                        );
                        self.validate_typed_id_params(
                            texture_op,
                            lod,
                            category,
                            declared_variables,
                            declared_registers,
                        );
                    }
                    TextureOpCode::Bias { is_proj: _, bias, offset } => {
                        self.validate_typed_id_params(
                            texture_op,
                            bias,
                            category,
                            declared_variables,
                            declared_registers,
                        );
                        if let Some(valid_offset) = offset {
                            self.validate_typed_id_params(
                                texture_op,
                                valid_offset,
                                category,
                                declared_variables,
                                declared_registers,
                            );
                        }
                    }
                    TextureOpCode::CompareBias { compare, bias } => {
                        self.validate_typed_id_params(
                            texture_op,
                            compare,
                            category,
                            declared_variables,
                            declared_registers,
                        );
                        self.validate_typed_id_params(
                            texture_op,
                            bias,
                            category,
                            declared_variables,
                            declared_registers,
                        );
                    }
                    TextureOpCode::Grad { is_proj: _, dx, dy, offset } => {
                        self.validate_typed_id_params(
                            texture_op,
                            dx,
                            category,
                            declared_variables,
                            declared_registers,
                        );
                        self.validate_typed_id_params(
                            texture_op,
                            dy,
                            category,
                            declared_variables,
                            declared_registers,
                        );
                        if let Some(valid_offset) = offset {
                            self.validate_typed_id_params(
                                texture_op,
                                valid_offset,
                                category,
                                declared_variables,
                                declared_registers,
                            );
                        }
                    }
                    TextureOpCode::GatherComponent { component, offset } => {
                        self.validate_typed_id_params(
                            texture_op,
                            component,
                            category,
                            declared_variables,
                            declared_registers,
                        );
                        if let Some(valid_offset) = offset {
                            self.validate_typed_id_params(
                                texture_op,
                                valid_offset,
                                category,
                                declared_variables,
                                declared_registers,
                            );
                        }
                    }
                    TextureOpCode::GatherRef { refz, offset } => {
                        self.validate_typed_id_params(
                            texture_op,
                            refz,
                            category,
                            declared_variables,
                            declared_registers,
                        );
                        if let Some(valid_offset) = offset {
                            self.validate_typed_id_params(
                                texture_op,
                                valid_offset,
                                category,
                                declared_variables,
                                declared_registers,
                            );
                        }
                    }
                }
            }
        };
    }

    // Helper function to check the OpCode instruction result TypedRegisterId contains valid
    // RegisterId and TypeId members
    fn validate_opcode_instruction_result_has_valid_ids(
        &self,
        op_code: &OpCode,
        result: &TypedRegisterId,
    ) {
        if result.id.id >= self.max_register_count {
            self.on_error(format_args!(
                "invalid {:?} instruction result register id {}",
                op_code, result.id.id
            ));
        }
        if result.type_id.id >= self.max_type_count {
            self.on_error(format_args!(
                "invalid {:?} instruction result type {}",
                op_code, result.type_id.id
            ));
        }
    }

    // Helper function to check Block input TypedRegisterId contains valid RegisterId and TypeId
    // members
    fn validate_block_input_has_valid_ids(&self, input: &TypedRegisterId) {
        if input.id.id >= self.max_register_count {
            self.on_error(format_args!(
                "invalid block input found, invalid input register id {}",
                input.id.id
            ));
        }
        if input.type_id.id >= self.max_type_count {
            self.on_error(format_args!(
                "invalid block input found, invalid input type {}",
                input.type_id.id
            ));
        }
    }

    // Helper function to check OpCode instruction TypedId parameters contain valid id and
    // type_id members
    fn validate_typed_id_params(
        &self,
        op_code: &dyn fmt::Debug,
        typed_id: &TypedId,
        category: TypedIdValidationCategory,
        declared_variables: Option<&DeclaredVarTracker>,
        declared_registers: Option<&DeclaredRegisterTracker>,
    ) {
        // validate id
        match typed_id.id {
            Id::Register(register_id) => {
                match category {
                    TypedIdValidationCategory::IdInBound => {
                        if register_id.id >= self.max_register_count {
                            self.on_error(format_args!(
                                "invalid {:?} instruction: invalid register id {}",
                                op_code, register_id.id
                            ));
                        }
                    }

                    TypedIdValidationCategory::VariableDeclared => {
                        // Do nothing
                    }
                    TypedIdValidationCategory::RegisterDeclared => {
                        let declared_register_tracker = declared_registers.expect(
                            "Expecting valid DeclaredRegisterTracker provided for \
                             RegisterDeclared category",
                        );
                        if !declared_register_tracker.is_declared(register_id) {
                            self.on_error(format_args!(
                                "invalid {:?} instruction: undeclared register id {}",
                                op_code, register_id.id
                            ));
                        }
                    }
                }
            }
            Id::Constant(constant_id) => {
                match category {
                    TypedIdValidationCategory::IdInBound => {
                        if constant_id.id >= self.max_constant_count {
                            self.on_error(format_args!(
                                "invalid {:?} instruction: invalid constant id {}",
                                op_code, constant_id.id
                            ));
                        }
                    }

                    TypedIdValidationCategory::VariableDeclared => {
                        // Do nothing
                    }
                    TypedIdValidationCategory::RegisterDeclared => {
                        // Do nothing
                    }
                }
            }
            Id::Variable(variable_id) => match category {
                TypedIdValidationCategory::IdInBound => {
                    if variable_id.id >= self.max_variable_count {
                        self.on_error(format_args!(
                            "invalid {:?} instruction: invalid variable id {}",
                            op_code, variable_id.id
                        ));
                    }
                }

                TypedIdValidationCategory::VariableDeclared => {
                    let declared_variables_tracker = declared_variables.expect(
                        "Expecting valid DeclaredVarTracker provided for VariableDeclared category",
                    );
                    if !declared_variables_tracker.is_variable_declared(variable_id) {
                        self.on_error(format_args!(
                            "invalid {:?} instruction: undeclared variable id {}",
                            op_code, variable_id.id
                        ));
                    }
                }
                TypedIdValidationCategory::RegisterDeclared => {
                    // Do nothing
                }
            },
        }
        // validate typed_id
        if typed_id.type_id.id >= self.max_type_count {
            self.on_error(format_args!(
                "invalid {:?} instruction: invalid type id {}",
                op_code, typed_id.type_id.id
            ));
        }
    }

    // Helper Function to print the invalid IR and then panic!
    fn on_error(&self, validation_error_msg: fmt::Arguments) {
        println!("Internal error: Invalid ANGLE IR! {}", validation_error_msg);
        debug::dump(self.ir);
        panic!();
    }

    fn validate_all_variables_are_declared_in_scope(&self) {
        let mut vars_declared_map = DeclaredVarTracker::new();
        vars_declared_map.set_global_declared_vars(self.ir.meta.all_global_variables());

        for (function_entry_index, entry) in self.ir.function_entries.iter().enumerate() {
            if entry.is_none() {
                // Skip over functions that have been dead-code eliminated.
                continue;
            }
            let function_signature = &self.ir.meta.all_functions()[function_entry_index];
            vars_declared_map
                .add_function_param_vars_upon_enter_function(&function_signature.params);
            self.validate_all_variables_in_a_block_are_declared_in_scope(
                &mut vars_declared_map,
                entry.as_ref().unwrap(),
            );
            vars_declared_map.remove_function_param_vars_upon_exit_function();
        }
    }

    fn validate_all_variables_in_a_block_are_declared_in_scope(
        &self,
        vars_declared_map: &mut DeclaredVarTracker,
        block: &Block,
    ) {
        // push the block variable to vars_declared
        vars_declared_map.add_current_scope_declared_vars_upon_enter_scope(&block.variables);

        // Validate variable used in each instructions
        for instruction in &block.instructions {
            let (opcode, _result) = instruction.get_op_and_result(&self.ir.meta);
            self.validate_instruction_op_code_typed_id_parameters(
                opcode,
                TypedIdValidationCategory::VariableDeclared,
                Some(vars_declared_map),
                None,
            );
        }

        // Check sub blocks, excluding merge_block
        block.for_each_sub_block(vars_declared_map, &|vars_declared_map, sub_block| {
            self.validate_all_variables_in_a_block_are_declared_in_scope(
                vars_declared_map,
                sub_block,
            )
        });

        // Continue check merge_block
        if let Some(valid_merge_block) = &block.merge_block {
            self.validate_all_variables_in_a_block_are_declared_in_scope(
                vars_declared_map,
                valid_merge_block,
            );
        }

        // pop the block variable from vars_declared_map
        vars_declared_map.remove_current_scope_declared_vars_upon_exit_scope();
    }

    fn validate_all_registers_are_declared_in_scope(&self) {
        let mut registers_declared_map = DeclaredRegisterTracker::new();
        for entry in &self.ir.function_entries {
            if entry.is_none() {
                // Skip over functions that have been dead-code eliminated.
                continue;
            }
            self.validate_block_registers(entry.as_ref().unwrap(), &mut registers_declared_map);
        }
    }

    fn validate_block_registers(
        &self,
        block: &Block,
        registers_declared_map: &mut DeclaredRegisterTracker,
    ) {
        registers_declared_map.add_scope();

        block.input.inspect(|input| {
            registers_declared_map.declare_register(input.id, self);
        });

        for instruction in &block.instructions {
            let (opcode, result) = instruction.get_op_and_result(&self.ir.meta);
            self.validate_instruction_op_code_typed_id_parameters(
                opcode,
                TypedIdValidationCategory::RegisterDeclared,
                None,
                Some(registers_declared_map),
            );
            result.inspect(|result| {
                registers_declared_map.declare_register(result.id, self);
            });
        }

        block.for_each_sub_block(registers_declared_map, &|registers_declared_map, sub_block| {
            self.validate_block_registers(sub_block, registers_declared_map);
        });

        block.merge_block.as_ref().inspect(|merge_block| {
            self.validate_block_registers(merge_block, registers_declared_map);
        });

        registers_declared_map.remove_scope();
    }
}
