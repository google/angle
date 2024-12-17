//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/wgsl/OutputUniformBlocks.h"

#include "angle_gl.h"
#include "common/mathutil.h"
#include "common/utilities.h"
#include "compiler/translator/BaseTypes.h"
#include "compiler/translator/Compiler.h"
#include "compiler/translator/ImmutableStringBuilder.h"
#include "compiler/translator/InfoSink.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/SymbolUniqueId.h"
#include "compiler/translator/tree_util/IntermTraverse.h"
#include "compiler/translator/util.h"
#include "compiler/translator/wgsl/Utils.h"

namespace sh
{

namespace
{

// Traverses the AST and finds all structs that are used in the uniform address space (see the
// UniformBlockMetadata struct).
class FindUniformAddressSpaceStructs : public TIntermTraverser
{
  public:
    FindUniformAddressSpaceStructs(UniformBlockMetadata *uniformBlockMetadata)
        : TIntermTraverser(true, false, false), mUniformBlockMetadata(uniformBlockMetadata)
    {}

    ~FindUniformAddressSpaceStructs() override = default;

    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override
    {
        const TIntermSequence &sequence = *(node->getSequence());

        TIntermTyped *variable = sequence.front()->getAsTyped();
        const TType &type      = variable->getType();

        // TODO(anglebug.com/376553328): should eventually ASSERT that there are no default uniforms
        // here.
        if (type.getQualifier() == EvqUniform)
        {
            recordTypesUsedInUniformAddressSpace(&type);
        }

        return true;
    }

  private:
    // Recurses through the tree of types referred to be `type` (which is used in the uniform
    // address space) and fills in the `mUniformBlockMetadata` struct appropriately.
    void recordTypesUsedInUniformAddressSpace(const TType *type)
    {
        if (type->isArray())
        {
            TType innerType = *type;
            innerType.toArrayBaseType();
            recordTypesUsedInUniformAddressSpace(&innerType);
        }
        else if (type->getStruct() != nullptr)
        {
            mUniformBlockMetadata->structsInUniformAddressSpace.insert(
                type->getStruct()->uniqueId().get());
            // Recurse into the types of the fields of this struct type.
            for (TField *const field : type->getStruct()->fields())
            {
                recordTypesUsedInUniformAddressSpace(field->type());
            }
        }
    }

    UniformBlockMetadata *const mUniformBlockMetadata;
};

}  // namespace

bool RecordUniformBlockMetadata(TIntermBlock *root, UniformBlockMetadata &outMetadata)
{
    FindUniformAddressSpaceStructs traverser(&outMetadata);
    root->traverse(&traverser);
    return true;
}

bool OutputUniformWrapperStructsAndConversions(
    TInfoSinkBase &output,
    const WGSLGenerationMetadataForUniforms &wgslGenerationMetadataForUniforms)
{
    for (const TType &type : wgslGenerationMetadataForUniforms.arrayElementTypesInUniforms)
    {
        // Structs don't need wrapper structs.
        ASSERT(type.getStruct() == nullptr);
        // Multidimensional arrays not currently supported in uniforms
        ASSERT(!type.isArray());

        output << "struct " << MakeUniformWrapperStructName(&type) << "\n{\n";
        output << "  @align(16) " << kWrappedStructFieldName << " : ";
        WriteWgslType(output, type, {});
        output << "\n};\n";
    }

    for (const TType &type :
         wgslGenerationMetadataForUniforms.arrayElementTypesThatNeedUnwrappingConversions)
    {
        // Should be a subset of the types that have had wrapper structs generated above, otherwise
        // it's impossible to unwrap them!
        TType innerType = type;
        innerType.toArrayElementType();
        ASSERT(wgslGenerationMetadataForUniforms.arrayElementTypesInUniforms.count(innerType) != 0);

        // This could take ptr<uniform, typeName>, with the unrestricted_pointer_parameters
        // extension. This is probably fine.
        output << "fn " << MakeUnwrappingArrayConversionFunctionName(&type) << "(wrappedArr : ";
        WriteWgslType(output, type, {WgslAddressSpace::Uniform});
        output << ") -> ";
        WriteWgslType(output, type, {WgslAddressSpace::NonUniform});
        output << "\n{\n";
        output << "  var retVal : ";
        WriteWgslType(output, type, {WgslAddressSpace::NonUniform});
        output << ";\n";
        output << "  for (var i : u32 = 0; i < " << type.getOutermostArraySize() << "; i++) {;\n";
        output << "    retVal[i] = wrappedArr[i]." << kWrappedStructFieldName << ";\n";
        output << "  }\n";
        output << "  return retVal;\n";
        output << "}\n";
    }

    return true;
}

ImmutableString MakeUnwrappingArrayConversionFunctionName(const TType *type)
{
    return BuildConcatenatedImmutableString("ANGLE_Convert_", MakeUniformWrapperStructName(type),
                                            "_ElementsTo_", type->getBuiltInTypeNameString(),
                                            "_Elements");
}

bool OutputUniformBlocks(TCompiler *compiler, TIntermBlock *root)
{
    // TODO(anglebug.com/42267100): This should eventually just be handled the same way as a regular
    // UBO, like in Vulkan which create a block out of the default uniforms with a traverser:
    // https://source.chromium.org/chromium/chromium/src/+/main:third_party/angle/src/compiler/translator/spirv/TranslatorSPIRV.cpp;l=70;drc=451093bbaf7fe812bf67d27d760f3bb64c92830b
    const std::vector<ShaderVariable> &basicUniforms = compiler->getUniforms();
    TInfoSinkBase &output                            = compiler->getInfoSink().obj;
    GlobalVars globalVars                            = FindGlobalVars(root);

    // Only output a struct at all if there are going to be members.
    bool outputStructHeader = false;
    for (const ShaderVariable &shaderVar : basicUniforms)
    {
        if (gl::IsOpaqueType(shaderVar.type))
        {
            continue;
        }
        if (shaderVar.isBuiltIn())
        {
            // gl_DepthRange and also the GLSL 4.2 gl_NumSamples are uniforms.
            // TODO(anglebug.com/42267100): put gl_DepthRange into default uniform block.
            continue;
        }
        if (!outputStructHeader)
        {
            output << "struct ANGLE_DefaultUniformBlock {\n";
            outputStructHeader = true;
        }
        output << "  ";
        // TODO(anglebug.com/42267100): some types will NOT match std140 layout here, namely matCx2,
        // bool, and arrays with stride less than 16.
        // (this check does not cover the unsupported case where there is an array of structs of
        // size < 16).
        if (gl::VariableRowCount(shaderVar.type) == 2 || shaderVar.type == GL_BOOL ||
            (shaderVar.isArray() && !shaderVar.isStruct() &&
             gl::VariableComponentCount(shaderVar.type) < 3))
        {
            return false;
        }
        output << shaderVar.name << " : ";

        TIntermDeclaration *declNode = globalVars.find(shaderVar.name)->second;
        const TVariable *astVar      = &ViewDeclaration(*declNode).symbol.variable();
        WriteWgslType(output, astVar->getType(), {WgslAddressSpace::Uniform});

        output << ",\n";
    }
    // TODO(anglebug.com/42267100): might need string replacement for @group(0) and @binding(0)
    // annotations. All WGSL resources available to shaders share the same (group, binding) ID
    // space.
    if (outputStructHeader)
    {
        ASSERT(compiler->getShaderType() == GL_VERTEX_SHADER ||
               compiler->getShaderType() == GL_FRAGMENT_SHADER);
        const uint32_t bindingIndex = compiler->getShaderType() == GL_VERTEX_SHADER
                                          ? kDefaultVertexUniformBlockBinding
                                          : kDefaultFragmentUniformBlockBinding;
        output << "};\n\n"
               << "@group(" << kDefaultUniformBlockBindGroup << ") @binding(" << bindingIndex
               << ") var<uniform> " << kDefaultUniformBlockVarName << " : "
               << kDefaultUniformBlockVarType << ";\n";
    }

    return true;
}

}  // namespace sh
