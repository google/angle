//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BuildSPIRV: Helper for OutputSPIRV to build SPIR-V.
//

#include "compiler/translator/BuildSPIRV.h"

#include "common/spirv/spirv_instruction_builder_autogen.h"
#include "compiler/translator/ValidateVaryingLocations.h"
#include "compiler/translator/util.h"

namespace sh
{
bool operator==(const SpirvType &a, const SpirvType &b)
{
    if (a.block != b.block)
    {
        return false;
    }

    if (a.arraySizes != b.arraySizes)
    {
        return false;
    }

    // If structure or interface block, they should match by pointer (i.e. be the same block).  The
    // AST transformations are expected to keep the AST consistent by using the same structure and
    // interface block pointer between declarations and usages.  This is validated by
    // ValidateASTOptions::validateVariableReferences.
    if (a.block != nullptr)
    {
        return a.blockStorage == b.blockStorage && a.isInvariant == b.isInvariant;
    }

    // Otherwise, match by the type contents.  The AST transformations sometimes recreate types that
    // are already defined, so we can't rely on pointers being unique.
    return a.type == b.type && a.primarySize == b.primarySize &&
           a.secondarySize == b.secondarySize && a.imageInternalFormat == b.imageInternalFormat &&
           a.isSamplerBaseImage == b.isSamplerBaseImage &&
           (a.arraySizes.empty() || a.blockStorage == b.blockStorage);
}

uint32_t GetTotalArrayElements(const SpirvType &type)
{
    uint32_t arraySizeProduct = 1;
    for (uint32_t arraySize : type.arraySizes)
    {
        // For runtime arrays, arraySize will be 0 and should be excluded.
        arraySizeProduct *= arraySize > 0 ? arraySize : 1;
    }

    return arraySizeProduct;
}

uint32_t GetOutermostArraySize(const SpirvType &type)
{
    uint32_t size = type.arraySizes.back();
    return size ? size : 1;
}

spirv::IdRef SPIRVBuilder::getNewId(const SpirvDecorations &decorations)
{
    spirv::IdRef newId = mNextAvailableId;
    mNextAvailableId   = spirv::IdRef(mNextAvailableId + 1);

    for (const spv::Decoration decoration : decorations)
    {
        spirv::WriteDecorate(&mSpirvDecorations, newId, decoration, {});
    }

    return newId;
}

TLayoutBlockStorage SPIRVBuilder::getBlockStorage(const TType &type) const
{
    // Default to std140 for uniform and std430 for buffer blocks.
    TLayoutBlockStorage blockStorage = type.getLayoutQualifier().blockStorage;
    if (IsShaderIoBlock(type.getQualifier()) || blockStorage == EbsStd140 ||
        blockStorage == EbsStd430)
    {
        return blockStorage;
    }

    if (type.getQualifier() == EvqBuffer)
    {
        return EbsStd430;
    }

    return EbsStd140;
}

SpirvType SPIRVBuilder::getSpirvType(const TType &type, TLayoutBlockStorage blockStorage) const
{
    SpirvType spirvType;
    spirvType.type                = type.getBasicType();
    spirvType.primarySize         = static_cast<uint8_t>(type.getNominalSize());
    spirvType.secondarySize       = static_cast<uint8_t>(type.getSecondarySize());
    spirvType.arraySizes          = type.getArraySizes();
    spirvType.imageInternalFormat = type.getLayoutQualifier().imageInternalFormat;
    spirvType.blockStorage        = blockStorage;

    if (type.getStruct() != nullptr)
    {
        spirvType.block       = type.getStruct();
        spirvType.isInvariant = isInvariantOutput(type);
    }
    else if (type.isInterfaceBlock())
    {
        spirvType.block = type.getInterfaceBlock();

        // Calculate the block storage from the interface block automatically.  The fields inherit
        // from this.
        if (spirvType.blockStorage == EbsUnspecified)
        {
            spirvType.blockStorage = getBlockStorage(type);
        }
    }
    else if (spirvType.arraySizes.empty())
    {
        // No difference in type for non-block non-array types in std140 and std430 block storage.
        spirvType.blockStorage = EbsUnspecified;
    }

    return spirvType;
}

const SpirvTypeData &SPIRVBuilder::getTypeData(const TType &type, TLayoutBlockStorage blockStorage)
{
    SpirvType spirvType = getSpirvType(type, blockStorage);

    const TSymbol *block = nullptr;
    if (type.getStruct() != nullptr)
    {
        block = type.getStruct();
    }
    else if (type.isInterfaceBlock())
    {
        block = type.getInterfaceBlock();
    }

    return getSpirvTypeData(spirvType, block);
}

const SpirvTypeData &SPIRVBuilder::getSpirvTypeData(const SpirvType &type, const TSymbol *block)
{
    auto iter = mTypeMap.find(type);
    if (iter == mTypeMap.end())
    {
        SpirvTypeData newTypeData = declareType(type, block);

        iter = mTypeMap.insert({type, newTypeData}).first;
    }

    return iter->second;
}

spirv::IdRef SPIRVBuilder::getBasicTypeId(TBasicType basicType, size_t size)
{
    SpirvType type;
    type.type        = basicType;
    type.primarySize = static_cast<uint8_t>(size);
    return getSpirvTypeData(type, nullptr).id;
}

spirv::IdRef SPIRVBuilder::getTypePointerId(spirv::IdRef typeId, spv::StorageClass storageClass)
{
    SpirvIdAndStorageClass key{typeId, storageClass};

    auto iter = mTypePointerIdMap.find(key);
    if (iter == mTypePointerIdMap.end())
    {
        const spirv::IdRef typePointerId = getNewId({});

        spirv::WriteTypePointer(&mSpirvTypePointerDecls, typePointerId, storageClass, typeId);

        iter = mTypePointerIdMap.insert({key, typePointerId}).first;
    }

    return iter->second;
}

spirv::IdRef SPIRVBuilder::getFunctionTypeId(spirv::IdRef returnTypeId,
                                             const spirv::IdRefList &paramTypeIds)
{
    SpirvIdAndIdList key{returnTypeId, paramTypeIds};

    auto iter = mFunctionTypeIdMap.find(key);
    if (iter == mFunctionTypeIdMap.end())
    {
        const spirv::IdRef functionTypeId = getNewId({});

        spirv::WriteTypeFunction(&mSpirvFunctionTypeDecls, functionTypeId, returnTypeId,
                                 paramTypeIds);

        iter = mFunctionTypeIdMap.insert({key, functionTypeId}).first;
    }

    return iter->second;
}

SpirvDecorations SPIRVBuilder::getDecorations(const TType &type)
{
    const bool enablePrecision = (mCompileOptions & SH_IGNORE_PRECISION_QUALIFIERS) == 0;
    const TPrecision precision = type.getPrecision();

    SpirvDecorations decorations;

    // Handle precision.
    if (enablePrecision && !mDisableRelaxedPrecision &&
        (precision == EbpMedium || precision == EbpLow))
    {
        decorations.push_back(spv::DecorationRelaxedPrecision);
    }

    // TODO: Handle |precise|.  http://anglebug.com/4889.

    return decorations;
}

spirv::IdRef SPIRVBuilder::getExtInstImportIdStd()
{
    if (!mExtInstImportIdStd.valid())
    {
        mExtInstImportIdStd = getNewId({});
    }
    return mExtInstImportIdStd;
}

SpirvTypeData SPIRVBuilder::declareType(const SpirvType &type, const TSymbol *block)
{
    // Recursively declare the type.  Type id is allocated afterwards purely for better id order in
    // output.
    spirv::IdRef typeId;

    if (!type.arraySizes.empty())
    {
        // Declaring an array.  First, declare the type without the outermost array size, then
        // declare a new array type based on that.

        SpirvType subType  = type;
        subType.arraySizes = type.arraySizes.first(type.arraySizes.size() - 1);
        if (subType.arraySizes.empty() && subType.block == nullptr)
        {
            subType.blockStorage = EbsUnspecified;
        }

        const spirv::IdRef subTypeId = getSpirvTypeData(subType, block).id;

        const unsigned int length = type.arraySizes.back();
        typeId                    = getNewId({});

        if (length == 0)
        {
            // Storage buffers may include a dynamically-sized array, which is identified by it
            // having a length of 0.
            spirv::WriteTypeRuntimeArray(&mSpirvTypeAndConstantDecls, typeId, subTypeId);
        }
        else
        {
            const spirv::IdRef lengthId = getUintConstant(length);
            spirv::WriteTypeArray(&mSpirvTypeAndConstantDecls, typeId, subTypeId, lengthId);
        }
    }
    else if (type.block != nullptr)
    {
        // Declaring a block.  First, declare all the fields, then declare a struct based on the
        // list of field types.

        spirv::IdRefList fieldTypeIds;
        for (const TField *field : type.block->fields())
        {
            const TType &fieldType   = *field->type();
            SpirvType fieldSpirvType = getSpirvType(fieldType, type.blockStorage);
            const TSymbol *structure = fieldType.getStruct();
            // Propagate invariant to struct members.
            if (structure != nullptr)
            {
                fieldSpirvType.isInvariant = type.isInvariant;
            }

            spirv::IdRef fieldTypeId = getSpirvTypeData(fieldSpirvType, structure).id;
            fieldTypeIds.push_back(fieldTypeId);
        }

        typeId = getNewId({});
        spirv::WriteTypeStruct(&mSpirvTypeAndConstantDecls, typeId, fieldTypeIds);
    }
    else if (IsSampler(type.type) && !type.isSamplerBaseImage)
    {
        // Declaring a sampler.  First, declare the non-sampled image and then a combined
        // image-sampler.

        SpirvType imageType          = type;
        imageType.isSamplerBaseImage = true;
        imageType.blockStorage       = EbsUnspecified;

        const spirv::IdRef nonSampledId = getSpirvTypeData(imageType, nullptr).id;

        typeId = getNewId({});
        spirv::WriteTypeSampledImage(&mSpirvTypeAndConstantDecls, typeId, nonSampledId);
    }
    else if (IsImage(type.type) || type.isSamplerBaseImage)
    {
        // Declaring an image.

        spirv::IdRef sampledType;
        spv::Dim dim;
        spirv::LiteralInteger depth;
        spirv::LiteralInteger arrayed;
        spirv::LiteralInteger multisampled;
        spirv::LiteralInteger sampled;

        getImageTypeParameters(type.type, &sampledType, &dim, &depth, &arrayed, &multisampled,
                               &sampled);
        spv::ImageFormat imageFormat = getImageFormat(type.imageInternalFormat);

        typeId = getNewId({});
        spirv::WriteTypeImage(&mSpirvTypeAndConstantDecls, typeId, sampledType, dim, depth, arrayed,
                              multisampled, sampled, imageFormat, nullptr);
    }
    else if (IsSubpassInputType(type.type))
    {
        // TODO: add support for framebuffer fetch. http://anglebug.com/4889
        UNIMPLEMENTED();
    }
    else if (type.secondarySize > 1)
    {
        // Declaring a matrix.  Declare the column type first, then create a matrix out of it.

        SpirvType columnType     = type;
        columnType.primarySize   = columnType.secondarySize;
        columnType.secondarySize = 1;
        columnType.blockStorage  = EbsUnspecified;

        const spirv::IdRef columnTypeId = getSpirvTypeData(columnType, nullptr).id;

        typeId = getNewId({});
        spirv::WriteTypeMatrix(&mSpirvTypeAndConstantDecls, typeId, columnTypeId,
                               spirv::LiteralInteger(type.primarySize));
    }
    else if (type.primarySize > 1)
    {
        // Declaring a vector.  Declare the component type first, then create a vector out of it.

        SpirvType componentType    = type;
        componentType.primarySize  = 1;
        componentType.blockStorage = EbsUnspecified;

        const spirv::IdRef componentTypeId = getSpirvTypeData(componentType, nullptr).id;

        typeId = getNewId({});
        spirv::WriteTypeVector(&mSpirvTypeAndConstantDecls, typeId, componentTypeId,
                               spirv::LiteralInteger(type.primarySize));
    }
    else
    {
        typeId = getNewId({});

        // Declaring a basic type.  There's a different instruction for each.
        switch (type.type)
        {
            case EbtVoid:
                spirv::WriteTypeVoid(&mSpirvTypeAndConstantDecls, typeId);
                break;
            case EbtFloat:
                spirv::WriteTypeFloat(&mSpirvTypeAndConstantDecls, typeId,
                                      spirv::LiteralInteger(32));
                break;
            case EbtDouble:
                // TODO: support desktop GLSL.  http://anglebug.com/4889
                UNIMPLEMENTED();
                break;
            case EbtInt:
                spirv::WriteTypeInt(&mSpirvTypeAndConstantDecls, typeId, spirv::LiteralInteger(32),
                                    spirv::LiteralInteger(1));
                break;
            case EbtUInt:
                spirv::WriteTypeInt(&mSpirvTypeAndConstantDecls, typeId, spirv::LiteralInteger(32),
                                    spirv::LiteralInteger(0));
                break;
            case EbtBool:
                // TODO: In SPIR-V, it's invalid to have a bool type in an interface block.  An AST
                // transformation should be written to rewrite the blocks to use a uint type with
                // appropriate casts where used.  Need to handle:
                //
                // - Store: cast the rhs of assignment
                // - Non-array load: cast the expression
                // - Array load (for example to use in a struct constructor): reconstruct the array
                //   with elements cast.
                // - Pass to function as out parameter: Use
                //   MonomorphizeUnsupportedFunctionsInVulkanGLSL to avoid it, as there's no easy
                //   way to handle such function calls inside if conditions and such.
                //
                // It might be simplest to do this for bools in structs as well, to avoid having to
                // convert between an old and new struct type if the struct is used both inside and
                // outside an interface block.
                //
                // http://anglebug.com/4889.
                spirv::WriteTypeBool(&mSpirvTypeAndConstantDecls, typeId);
                break;
            default:
                UNREACHABLE();
        }
    }

    // If this was a block declaration, add debug information for its type and field names.
    //
    // TODO: make this conditional to a compiler flag.  Instead of outputting the debug info
    // unconditionally and having the SPIR-V transformer remove them, it's better to avoid
    // generating them in the first place.  This both simplifies the transformer and reduces SPIR-V
    // binary size that gets written to disk cache.  http://anglebug.com/4889
    if (type.block != nullptr && type.arraySizes.empty())
    {
        spirv::WriteName(&mSpirvDebug, typeId, hashName(block).data());

        uint32_t fieldIndex = 0;
        for (const TField *field : type.block->fields())
        {
            spirv::WriteMemberName(&mSpirvDebug, typeId, spirv::LiteralInteger(fieldIndex++),
                                   hashFieldName(field).data());
        }
    }

    uint32_t baseAlignment      = 4;
    uint32_t sizeInStorageBlock = 0;

    // Calculate base alignment and sizes for types.  Size for blocks are not calculated, as they
    // are done later at the same time Offset decorations are written.
    const bool isOpaqueType = IsOpaqueType(type.type);
    if (!isOpaqueType)
    {
        baseAlignment = calculateBaseAlignmentAndSize(type, &sizeInStorageBlock);
    }

    // Write decorations for interface block fields.
    if (type.blockStorage != EbsUnspecified)
    {
        // Cannot have opaque uniforms inside interface blocks.
        ASSERT(!isOpaqueType);

        const bool isInterfaceBlock = block != nullptr && block->isInterfaceBlock();

        if (!type.arraySizes.empty() && !isInterfaceBlock)
        {
            // Write the ArrayStride decoration for arrays inside interface blocks.  An array of
            // interface blocks doesn't need a stride.
            spirv::WriteDecorate(
                &mSpirvDecorations, typeId, spv::DecorationArrayStride,
                {spirv::LiteralInteger(sizeInStorageBlock / GetOutermostArraySize(type))});
        }
        else if (type.arraySizes.empty() && type.block != nullptr)
        {
            // Write the Offset decoration for interface blocks and structs in them.
            sizeInStorageBlock = calculateSizeAndWriteOffsetDecorations(type, typeId);
        }
    }

    // Write other member decorations.
    if (type.block != nullptr && type.arraySizes.empty())
    {
        writeMemberDecorations(type, typeId);
    }

    return {typeId, baseAlignment, sizeInStorageBlock};
}

void SPIRVBuilder::getImageTypeParameters(TBasicType type,
                                          spirv::IdRef *sampledTypeOut,
                                          spv::Dim *dimOut,
                                          spirv::LiteralInteger *depthOut,
                                          spirv::LiteralInteger *arrayedOut,
                                          spirv::LiteralInteger *multisampledOut,
                                          spirv::LiteralInteger *sampledOut)
{
    TBasicType sampledType = EbtFloat;
    *dimOut                = spv::Dim2D;
    bool isDepth           = false;
    bool isArrayed         = false;
    bool isMultisampled    = false;

    // Decompose the basic type into image properties
    switch (type)
    {
        // Float 2D Images
        case EbtSampler2D:
        case EbtImage2D:
        case EbtSamplerExternalOES:
        case EbtSamplerExternal2DY2YEXT:
        case EbtSamplerVideoWEBGL:
            break;
        case EbtSampler2DArray:
        case EbtImage2DArray:
            isArrayed = true;
            break;
        case EbtSampler2DMS:
        case EbtImage2DMS:
            isMultisampled = true;
            break;
        case EbtSampler2DMSArray:
        case EbtImage2DMSArray:
            isArrayed      = true;
            isMultisampled = true;
            break;
        case EbtSampler2DShadow:
            isDepth = true;
            break;
        case EbtSampler2DArrayShadow:
            isDepth   = true;
            isArrayed = true;
            break;

        // Integer 2D images
        case EbtISampler2D:
        case EbtIImage2D:
            sampledType = EbtInt;
            break;
        case EbtISampler2DArray:
        case EbtIImage2DArray:
            sampledType = EbtInt;
            isArrayed   = true;
            break;
        case EbtISampler2DMS:
        case EbtIImage2DMS:
            sampledType    = EbtInt;
            isMultisampled = true;
            break;
        case EbtISampler2DMSArray:
        case EbtIImage2DMSArray:
            sampledType    = EbtInt;
            isArrayed      = true;
            isMultisampled = true;
            break;

        // Unsinged integer 2D images
        case EbtUSampler2D:
        case EbtUImage2D:
            sampledType = EbtUInt;
            break;
        case EbtUSampler2DArray:
        case EbtUImage2DArray:
            sampledType = EbtUInt;
            isArrayed   = true;
            break;
        case EbtUSampler2DMS:
        case EbtUImage2DMS:
            sampledType    = EbtUInt;
            isMultisampled = true;
            break;
        case EbtUSampler2DMSArray:
        case EbtUImage2DMSArray:
            sampledType    = EbtUInt;
            isArrayed      = true;
            isMultisampled = true;
            break;

        // 3D images
        case EbtSampler3D:
        case EbtImage3D:
            *dimOut = spv::Dim3D;
            break;
        case EbtISampler3D:
        case EbtIImage3D:
            sampledType = EbtInt;
            *dimOut     = spv::Dim3D;
            break;
        case EbtUSampler3D:
        case EbtUImage3D:
            sampledType = EbtUInt;
            *dimOut     = spv::Dim3D;
            break;

        // Float cube images
        case EbtSamplerCube:
        case EbtImageCube:
            *dimOut = spv::DimCube;
            break;
        case EbtSamplerCubeArray:
        case EbtImageCubeArray:
            *dimOut   = spv::DimCube;
            isArrayed = true;
            break;
        case EbtSamplerCubeArrayShadow:
            *dimOut   = spv::DimCube;
            isDepth   = true;
            isArrayed = true;
            break;
        case EbtSamplerCubeShadow:
            *dimOut = spv::DimCube;
            isDepth = true;
            break;

        // Integer cube images
        case EbtISamplerCube:
        case EbtIImageCube:
            sampledType = EbtInt;
            *dimOut     = spv::DimCube;
            break;
        case EbtISamplerCubeArray:
        case EbtIImageCubeArray:
            sampledType = EbtInt;
            *dimOut     = spv::DimCube;
            isArrayed   = true;
            break;

        // Unsigned integer cube images
        case EbtUSamplerCube:
        case EbtUImageCube:
            sampledType = EbtUInt;
            *dimOut     = spv::DimCube;
            break;
        case EbtUSamplerCubeArray:
        case EbtUImageCubeArray:
            sampledType = EbtUInt;
            *dimOut     = spv::DimCube;
            isArrayed   = true;
            break;

        // Float 1D images
        case EbtSampler1D:
        case EbtImage1D:
            *dimOut = spv::Dim1D;
            break;
        case EbtSampler1DArray:
        case EbtImage1DArray:
            *dimOut   = spv::Dim1D;
            isArrayed = true;
            break;
        case EbtSampler1DShadow:
            *dimOut = spv::Dim1D;
            isDepth = true;
            break;
        case EbtSampler1DArrayShadow:
            *dimOut   = spv::Dim1D;
            isDepth   = true;
            isArrayed = true;
            break;

        // Integer 1D images
        case EbtISampler1D:
        case EbtIImage1D:
            sampledType = EbtInt;
            *dimOut     = spv::Dim1D;
            break;
        case EbtISampler1DArray:
        case EbtIImage1DArray:
            sampledType = EbtInt;
            *dimOut     = spv::Dim1D;
            isArrayed   = true;
            break;

        // Unsigned integer 1D images
        case EbtUSampler1D:
        case EbtUImage1D:
            sampledType = EbtUInt;
            *dimOut     = spv::Dim1D;
            break;
        case EbtUSampler1DArray:
        case EbtUImage1DArray:
            sampledType = EbtUInt;
            *dimOut     = spv::Dim1D;
            isArrayed   = true;
            break;

        // Rect images
        case EbtSampler2DRect:
        case EbtImageRect:
            *dimOut = spv::DimRect;
            break;
        case EbtSampler2DRectShadow:
            *dimOut = spv::DimRect;
            isDepth = true;
            break;
        case EbtISampler2DRect:
        case EbtIImageRect:
            sampledType = EbtInt;
            *dimOut     = spv::DimRect;
            break;
        case EbtUSampler2DRect:
        case EbtUImageRect:
            sampledType = EbtUInt;
            *dimOut     = spv::DimRect;
            break;

        // Image buffers
        case EbtSamplerBuffer:
        case EbtImageBuffer:
            *dimOut = spv::DimBuffer;
            break;
        case EbtISamplerBuffer:
        case EbtIImageBuffer:
            sampledType = EbtInt;
            *dimOut     = spv::DimBuffer;
            break;
        case EbtUSamplerBuffer:
        case EbtUImageBuffer:
            sampledType = EbtUInt;
            *dimOut     = spv::DimBuffer;
            break;
        default:
            // TODO: support framebuffer fetch.  http://anglebug.com/4889
            UNREACHABLE();
    }

    // Get id of the component type of the image
    SpirvType sampledSpirvType;
    sampledSpirvType.type = sampledType;

    *sampledTypeOut = getSpirvTypeData(sampledSpirvType, nullptr).id;

    const bool isSampledImage = IsSampler(type);

    // Set flags based on SPIR-V required values.  See OpTypeImage:
    //
    // - For depth:        0 = non-depth,      1 = depth
    // - For arrayed:      0 = non-arrayed,    1 = arrayed
    // - For multisampled: 0 = single-sampled, 1 = multisampled
    // - For sampled:      1 = sampled,        2 = storage
    //
    *depthOut        = spirv::LiteralInteger(isDepth ? 1 : 0);
    *arrayedOut      = spirv::LiteralInteger(isArrayed ? 1 : 0);
    *multisampledOut = spirv::LiteralInteger(isMultisampled ? 1 : 0);
    *sampledOut      = spirv::LiteralInteger(isSampledImage ? 1 : 2);

    // Add the necessary capability based on parameters.  The SPIR-V spec section 3.8 Dim specfies
    // the required capabilities:
    //
    //     Dim          Sampled         Storage            Storage Array
    //     --------------------------------------------------------------
    //     1D           Sampled1D       Image1D
    //     2D           Shader                             ImageMSArray
    //     3D
    //     Cube         Shader                             ImageCubeArray
    //     Rect         SampledRect     ImageRect
    //     Buffer       SampledBuffer   ImageBuffer
    //
    // Note that the Shader capability is always unconditionally added.
    //
    switch (*dimOut)
    {
        case spv::Dim1D:
            addCapability(isSampledImage ? spv::CapabilitySampled1D : spv::CapabilityImage1D);
            break;
        case spv::Dim2D:
            if (!isSampledImage && isArrayed && isMultisampled)
            {
                addCapability(spv::CapabilityImageMSArray);
            }
            break;
        case spv::Dim3D:
            break;
        case spv::DimCube:
            if (!isSampledImage && isArrayed && isMultisampled)
            {
                addCapability(spv::CapabilityImageCubeArray);
            }
            break;
        case spv::DimRect:
            addCapability(isSampledImage ? spv::CapabilitySampledRect : spv::CapabilityImageRect);
            break;
        case spv::DimBuffer:
            addCapability(isSampledImage ? spv::CapabilitySampledBuffer
                                         : spv::CapabilityImageBuffer);
            break;
        default:
            // TODO: support framebuffer fetch.  http://anglebug.com/4889
            UNREACHABLE();
    }
}

spv::ImageFormat SPIRVBuilder::getImageFormat(TLayoutImageInternalFormat imageInternalFormat)
{
    switch (imageInternalFormat)
    {
        case EiifUnspecified:
            return spv::ImageFormatUnknown;
        case EiifRGBA32F:
            return spv::ImageFormatRgba32f;
        case EiifRGBA16F:
            return spv::ImageFormatRgba16f;
        case EiifR32F:
            return spv::ImageFormatR32f;
        case EiifRGBA32UI:
            return spv::ImageFormatRgba32ui;
        case EiifRGBA16UI:
            return spv::ImageFormatRgba16ui;
        case EiifRGBA8UI:
            return spv::ImageFormatRgba8ui;
        case EiifR32UI:
            return spv::ImageFormatR32ui;
        case EiifRGBA32I:
            return spv::ImageFormatRgba32i;
        case EiifRGBA16I:
            return spv::ImageFormatRgba16i;
        case EiifRGBA8I:
            return spv::ImageFormatRgba8i;
        case EiifR32I:
            return spv::ImageFormatR32i;
        case EiifRGBA8:
            return spv::ImageFormatRgba8;
        case EiifRGBA8_SNORM:
            return spv::ImageFormatRgba8Snorm;
        default:
            UNREACHABLE();
            return spv::ImageFormatUnknown;
    }
}

spirv::IdRef SPIRVBuilder::getBoolConstant(bool value)
{
    uint32_t asInt = static_cast<uint32_t>(value);

    spirv::IdRef constantId = mBoolConstants[asInt];

    if (!constantId.valid())
    {
        SpirvType boolType;
        boolType.type = EbtBool;

        const spirv::IdRef boolTypeId = getSpirvTypeData(boolType, nullptr).id;

        mBoolConstants[asInt] = constantId = getNewId({});
        if (value)
        {
            spirv::WriteConstantTrue(&mSpirvTypeAndConstantDecls, boolTypeId, constantId);
        }
        else
        {
            spirv::WriteConstantFalse(&mSpirvTypeAndConstantDecls, boolTypeId, constantId);
        }
    }

    return constantId;
}

spirv::IdRef SPIRVBuilder::getBasicConstantHelper(uint32_t value,
                                                  TBasicType type,
                                                  angle::HashMap<uint32_t, spirv::IdRef> *constants)
{
    auto iter = constants->find(value);
    if (iter == constants->end())
    {
        SpirvType spirvType;
        spirvType.type = type;

        const spirv::IdRef typeId     = getSpirvTypeData(spirvType, nullptr).id;
        const spirv::IdRef constantId = getNewId({});

        spirv::WriteConstant(&mSpirvTypeAndConstantDecls, typeId, constantId,
                             spirv::LiteralContextDependentNumber(value));

        iter = constants->insert({value, constantId}).first;
    }

    return iter->second;
}

spirv::IdRef SPIRVBuilder::getUintConstant(uint32_t value)
{
    return getBasicConstantHelper(value, EbtUInt, &mUintConstants);
}

spirv::IdRef SPIRVBuilder::getIntConstant(int32_t value)
{
    uint32_t asUint = static_cast<uint32_t>(value);
    return getBasicConstantHelper(asUint, EbtInt, &mIntConstants);
}

spirv::IdRef SPIRVBuilder::getFloatConstant(float value)
{
    union
    {
        float f;
        uint32_t u;
    } asUint;
    asUint.f = value;
    return getBasicConstantHelper(asUint.u, EbtFloat, &mFloatConstants);
}

spirv::IdRef SPIRVBuilder::getVectorConstantHelper(spirv::IdRef valueId, TBasicType type, int size)
{
    if (size == 1)
    {
        return valueId;
    }

    SpirvType vecType;
    vecType.type        = type;
    vecType.primarySize = static_cast<uint8_t>(size);

    const spirv::IdRef typeId = getSpirvTypeData(vecType, nullptr).id;
    const spirv::IdRefList valueIds(size, valueId);

    return getCompositeConstant(typeId, valueIds);
}

spirv::IdRef SPIRVBuilder::getUvecConstant(uint32_t value, int size)
{
    const spirv::IdRef valueId = getUintConstant(value);
    return getVectorConstantHelper(valueId, EbtUInt, size);
}

spirv::IdRef SPIRVBuilder::getIvecConstant(int32_t value, int size)
{
    const spirv::IdRef valueId = getIntConstant(value);
    return getVectorConstantHelper(valueId, EbtInt, size);
}

spirv::IdRef SPIRVBuilder::getVecConstant(float value, int size)
{
    const spirv::IdRef valueId = getFloatConstant(value);
    return getVectorConstantHelper(valueId, EbtFloat, size);
}

spirv::IdRef SPIRVBuilder::getCompositeConstant(spirv::IdRef typeId, const spirv::IdRefList &values)
{
    SpirvIdAndIdList key{typeId, values};

    auto iter = mCompositeConstants.find(key);
    if (iter == mCompositeConstants.end())
    {
        const spirv::IdRef constantId = getNewId({});

        spirv::WriteConstantComposite(&mSpirvTypeAndConstantDecls, typeId, constantId, values);

        iter = mCompositeConstants.insert({key, constantId}).first;
    }

    return iter->second;
}

void SPIRVBuilder::startNewFunction(spirv::IdRef functionId, const TFunction *func)
{
    ASSERT(mSpirvCurrentFunctionBlocks.empty());

    // Add the first block of the function.
    mSpirvCurrentFunctionBlocks.emplace_back();
    mSpirvCurrentFunctionBlocks.back().labelId = getNewId({});

    // Output debug information.
    spirv::WriteName(&mSpirvDebug, functionId, hashFunctionName(func).data());
}

void SPIRVBuilder::assembleSpirvFunctionBlocks()
{
    // Take all the blocks and place them in the functions section of SPIR-V in sequence.
    for (const SpirvBlock &block : mSpirvCurrentFunctionBlocks)
    {
        // Every block must be properly terminated.
        ASSERT(block.isTerminated);

        // Generate the OpLabel instruction for the block.
        spirv::WriteLabel(&mSpirvFunctions, block.labelId);

        // Add the variable declarations if any.
        mSpirvFunctions.insert(mSpirvFunctions.end(), block.localVariables.begin(),
                               block.localVariables.end());

        // Add the body of the block.
        mSpirvFunctions.insert(mSpirvFunctions.end(), block.body.begin(), block.body.end());
    }

    // Clean up.
    mSpirvCurrentFunctionBlocks.clear();
}

spirv::IdRef SPIRVBuilder::declareVariable(spirv::IdRef typeId,
                                           spv::StorageClass storageClass,
                                           const SpirvDecorations &decorations,
                                           spirv::IdRef *initializerId,
                                           const char *name)
{
    const bool isFunctionLocal = storageClass == spv::StorageClassFunction;

    // Make sure storage class is consistent with where the variable is declared.
    ASSERT(!isFunctionLocal || !mSpirvCurrentFunctionBlocks.empty());

    // Function-local variables go in the first block of the function, while the rest are in the
    // global variables section.
    spirv::Blob *spirvSection = isFunctionLocal
                                    ? &mSpirvCurrentFunctionBlocks.front().localVariables
                                    : &mSpirvVariableDecls;

    const spirv::IdRef variableId    = getNewId(decorations);
    const spirv::IdRef typePointerId = getTypePointerId(typeId, storageClass);

    spirv::WriteVariable(spirvSection, typePointerId, variableId, storageClass, initializerId);

    // Output debug information.
    if (name)
    {
        spirv::WriteName(&mSpirvDebug, variableId, name);
    }

    return variableId;
}

spirv::IdRef SPIRVBuilder::declareSpecConst(TBasicType type, int id, const char *name)
{
    SpirvType spirvType;
    spirvType.type = type;

    const spirv::IdRef typeId      = getSpirvTypeData(spirvType, nullptr).id;
    const spirv::IdRef specConstId = getNewId({});

    // Note: all spec constants are 0 initialized by the translator.
    if (type == EbtBool)
    {
        spirv::WriteSpecConstantFalse(&mSpirvTypeAndConstantDecls, typeId, specConstId);
    }
    else
    {
        spirv::WriteSpecConstant(&mSpirvTypeAndConstantDecls, typeId, specConstId,
                                 spirv::LiteralContextDependentNumber(0));
    }

    // Add the SpecId decoration
    spirv::WriteDecorate(&mSpirvDecorations, specConstId, spv::DecorationSpecId,
                         {spirv::LiteralInteger(id)});

    // Output debug information.
    if (name)
    {
        spirv::WriteName(&mSpirvDebug, specConstId, name);
    }

    return specConstId;
}

void SPIRVBuilder::startConditional(size_t blockCount, bool isContinuable, bool isBreakable)
{
    mConditionalStack.emplace_back();
    SpirvConditional &conditional = mConditionalStack.back();

    // Create the requested number of block ids.
    conditional.blockIds.resize(blockCount);
    for (spirv::IdRef &blockId : conditional.blockIds)
    {
        blockId = getNewId({});
    }

    conditional.isContinuable = isContinuable;
    conditional.isBreakable   = isBreakable;

    // Don't automatically start the next block.  The caller needs to generate instructions based on
    // the ids that were just generated above.
}

void SPIRVBuilder::nextConditionalBlock()
{
    ASSERT(!mConditionalStack.empty());
    SpirvConditional &conditional = mConditionalStack.back();

    ASSERT(conditional.nextBlockToWrite < conditional.blockIds.size());
    const spirv::IdRef blockId = conditional.blockIds[conditional.nextBlockToWrite++];

    // The previous block must have properly terminated.
    ASSERT(isCurrentFunctionBlockTerminated());

    // Generate a new block.
    mSpirvCurrentFunctionBlocks.emplace_back();
    mSpirvCurrentFunctionBlocks.back().labelId = blockId;
}

void SPIRVBuilder::endConditional()
{
    ASSERT(!mConditionalStack.empty());

    // No blocks should be left.
    ASSERT(mConditionalStack.back().nextBlockToWrite == mConditionalStack.back().blockIds.size());

    mConditionalStack.pop_back();
}

bool SPIRVBuilder::isInLoop() const
{
    for (const SpirvConditional &conditional : mConditionalStack)
    {
        if (conditional.isContinuable)
        {
            return true;
        }
    }

    return false;
}

spirv::IdRef SPIRVBuilder::getBreakTargetId() const
{
    for (size_t index = mConditionalStack.size(); index > 0; --index)
    {
        const SpirvConditional &conditional = mConditionalStack[index - 1];

        if (conditional.isBreakable)
        {
            // The target of break; is always the merge block, and the merge block is always the
            // last block.
            return conditional.blockIds.back();
        }
    }

    UNREACHABLE();
    return spirv::IdRef{};
}

spirv::IdRef SPIRVBuilder::getContinueTargetId() const
{
    for (size_t index = mConditionalStack.size(); index > 0; --index)
    {
        const SpirvConditional &conditional = mConditionalStack[index - 1];

        if (conditional.isContinuable)
        {
            // The target of continue; is always the block before merge, so it's the one before
            // last.
            ASSERT(conditional.blockIds.size() > 2);
            return conditional.blockIds[conditional.blockIds.size() - 2];
        }
    }

    UNREACHABLE();
    return spirv::IdRef{};
}

uint32_t SPIRVBuilder::nextUnusedBinding()
{
    return mNextUnusedBinding++;
}

uint32_t SPIRVBuilder::nextUnusedInputLocation(uint32_t consumedCount)
{
    uint32_t nextUnused = mNextUnusedInputLocation;
    mNextUnusedInputLocation += consumedCount;
    return nextUnused;
}

uint32_t SPIRVBuilder::nextUnusedOutputLocation(uint32_t consumedCount)
{
    uint32_t nextUnused = mNextUnusedOutputLocation;
    mNextUnusedOutputLocation += consumedCount;
    return nextUnused;
}

bool SPIRVBuilder::isInvariantOutput(const TType &type) const
{
    // The Invariant decoration is applied to output variables if specified or if globally enabled.
    return type.isInvariant() ||
           (IsShaderOut(type.getQualifier()) && mCompiler->getPragma().stdgl.invariantAll);
}

void SPIRVBuilder::addCapability(spv::Capability capability)
{
    mCapabilities.insert(capability);
}

void SPIRVBuilder::setEntryPointId(spirv::IdRef id)
{
    ASSERT(!mEntryPointId.valid());
    mEntryPointId = id;
}

void SPIRVBuilder::addEntryPointInterfaceVariableId(spirv::IdRef id)
{
    mEntryPointInterfaceList.push_back(id);
}

void SPIRVBuilder::writePerVertexBuiltIns(const TType &type, spirv::IdRef typeId)
{
    ASSERT(type.isInterfaceBlock());
    const TInterfaceBlock *block = type.getInterfaceBlock();

    uint32_t fieldIndex = 0;
    for (const TField *field : block->fields())
    {
        spv::BuiltIn decorationValue = spv::BuiltInPosition;
        switch (field->type()->getQualifier())
        {
            case EvqPosition:
                decorationValue = spv::BuiltInPosition;
                break;
            case EvqPointSize:
                decorationValue = spv::BuiltInPointSize;
                break;
            case EvqClipDistance:
                decorationValue = spv::BuiltInClipDistance;
                break;
            case EvqCullDistance:
                decorationValue = spv::BuiltInCullDistance;
                break;
            default:
                UNREACHABLE();
        }

        spirv::WriteMemberDecorate(&mSpirvDecorations, typeId, spirv::LiteralInteger(fieldIndex++),
                                   spv::DecorationBuiltIn,
                                   {spirv::LiteralInteger(decorationValue)});
    }
}

void SPIRVBuilder::writeInterfaceVariableDecorations(const TType &type, spirv::IdRef variableId)
{
    const TLayoutQualifier &layoutQualifier = type.getLayoutQualifier();

    const bool needsSetBinding =
        IsSampler(type.getBasicType()) ||
        (type.isInterfaceBlock() &&
         (type.getQualifier() == EvqUniform || type.getQualifier() == EvqBuffer)) ||
        IsImage(type.getBasicType()) || IsSubpassInputType(type.getBasicType());
    const bool needsLocation =
        type.getQualifier() == EvqAttribute || type.getQualifier() == EvqVertexIn ||
        type.getQualifier() == EvqFragmentOut || IsVarying(type.getQualifier());
    const bool needsInputAttachmentIndex = IsSubpassInputType(type.getBasicType());
    const bool needsBlendIndex =
        type.getQualifier() == EvqFragmentOut && layoutQualifier.index >= 0;

    // TODO: handle row-major matrixes.  http://anglebug.com/4889.
    // TODO: handle invariant (spv::DecorationInvariant).

    // If the resource declaration requires set & binding, add the DescriptorSet and Binding
    // decorations.
    if (needsSetBinding)
    {
        spirv::WriteDecorate(&mSpirvDecorations, variableId, spv::DecorationDescriptorSet,
                             {spirv::LiteralInteger(0)});
        spirv::WriteDecorate(&mSpirvDecorations, variableId, spv::DecorationBinding,
                             {spirv::LiteralInteger(nextUnusedBinding())});
    }

    if (needsLocation)
    {
        const unsigned int locationCount =
            CalculateVaryingLocationCount(type, gl::ToGLenum(mShaderType));
        const uint32_t location = IsShaderIn(type.getQualifier())
                                      ? nextUnusedInputLocation(locationCount)
                                      : nextUnusedOutputLocation(locationCount);

        spirv::WriteDecorate(&mSpirvDecorations, variableId, spv::DecorationLocation,
                             {spirv::LiteralInteger(location)});
    }

    // If the resource declaration is an input attachment, add the InputAttachmentIndex decoration.
    if (needsInputAttachmentIndex)
    {
        spirv::WriteDecorate(&mSpirvDecorations, variableId, spv::DecorationInputAttachmentIndex,
                             {spirv::LiteralInteger(layoutQualifier.inputAttachmentIndex)});
    }

    if (needsBlendIndex)
    {
        spirv::WriteDecorate(&mSpirvDecorations, variableId, spv::DecorationIndex,
                             {spirv::LiteralInteger(layoutQualifier.index)});
    }
}

void SPIRVBuilder::writeBranchConditional(spirv::IdRef conditionValue,
                                          spirv::IdRef trueBlock,
                                          spirv::IdRef falseBlock,
                                          spirv::IdRef mergeBlock)
{
    // Generate the following:
    //
    //     OpSelectionMerge %mergeBlock None
    //     OpBranchConditional %conditionValue %trueBlock %falseBlock
    //
    spirv::WriteSelectionMerge(getSpirvCurrentFunctionBlock(), mergeBlock,
                               spv::SelectionControlMaskNone);
    spirv::WriteBranchConditional(getSpirvCurrentFunctionBlock(), conditionValue, trueBlock,
                                  falseBlock, {});
    terminateCurrentFunctionBlock();

    // Start the true or false block, whichever exists.
    nextConditionalBlock();
}

void SPIRVBuilder::writeBranchConditionalBlockEnd()
{
    if (!isCurrentFunctionBlockTerminated())
    {
        // Insert a branch to the merge block at the end of each if-else block, unless the block is
        // already terminated, such as with a return or discard.
        const spirv::IdRef mergeBlock = getCurrentConditional()->blockIds.back();

        spirv::WriteBranch(getSpirvCurrentFunctionBlock(), mergeBlock);
        terminateCurrentFunctionBlock();
    }

    // Move on to the next block.
    nextConditionalBlock();
}

void SPIRVBuilder::writeLoopHeader(spirv::IdRef branchToBlock,
                                   spirv::IdRef continueBlock,
                                   spirv::IdRef mergeBlock)
{
    // First, jump to the header block:
    //
    //     OpBranch %header
    //
    const spirv::IdRef headerBlock = mConditionalStack.back().blockIds[0];
    spirv::WriteBranch(getSpirvCurrentFunctionBlock(), headerBlock);
    terminateCurrentFunctionBlock();

    // Start the header block.
    nextConditionalBlock();

    // Generate the following:
    //
    //     OpLoopMerge %mergeBlock %continueBlock None
    //     OpBranch %branchToBlock (%cond or if do-while, %body)
    //
    spirv::WriteLoopMerge(getSpirvCurrentFunctionBlock(), mergeBlock, continueBlock,
                          spv::LoopControlMaskNone);
    spirv::WriteBranch(getSpirvCurrentFunctionBlock(), branchToBlock);
    terminateCurrentFunctionBlock();

    // Start the next block, which is either %cond or %body.
    nextConditionalBlock();
}

void SPIRVBuilder::writeLoopConditionEnd(spirv::IdRef conditionValue,
                                         spirv::IdRef branchToBlock,
                                         spirv::IdRef mergeBlock)
{
    // Generate the following:
    //
    //     OpBranchConditional %conditionValue %branchToBlock %mergeBlock
    //
    // %branchToBlock is either %body or if do-while, %header
    //
    spirv::WriteBranchConditional(getSpirvCurrentFunctionBlock(), conditionValue, branchToBlock,
                                  mergeBlock, {});
    terminateCurrentFunctionBlock();

    // Start the next block, which is either %continue or %body.
    nextConditionalBlock();
}

void SPIRVBuilder::writeLoopContinueEnd(spirv::IdRef headerBlock)
{
    // Generate the following:
    //
    //     OpBranch %headerBlock
    //
    spirv::WriteBranch(getSpirvCurrentFunctionBlock(), headerBlock);
    terminateCurrentFunctionBlock();

    // Start the next block, which is %body.
    nextConditionalBlock();
}

void SPIRVBuilder::writeLoopBodyEnd(spirv::IdRef continueBlock)
{
    // Generate the following:
    //
    //     OpBranch %continueBlock
    //
    // This is only done if the block isn't already terminated in another way, such as with an
    // unconditional continue/etc at the end of the loop.
    if (!isCurrentFunctionBlockTerminated())
    {
        spirv::WriteBranch(getSpirvCurrentFunctionBlock(), continueBlock);
        terminateCurrentFunctionBlock();
    }

    // Start the next block, which is %merge or if while, %continue.
    nextConditionalBlock();
}

void SPIRVBuilder::writeSwitch(spirv::IdRef conditionValue,
                               spirv::IdRef defaultBlock,
                               const spirv::PairLiteralIntegerIdRefList &targetPairList,
                               spirv::IdRef mergeBlock)
{
    // Generate the following:
    //
    //     OpSelectionMerge %mergeBlock None
    //     OpSwitch %conditionValue %defaultBlock A %ABlock B %BBlock ...
    //
    spirv::WriteSelectionMerge(getSpirvCurrentFunctionBlock(), mergeBlock,
                               spv::SelectionControlMaskNone);
    spirv::WriteSwitch(getSpirvCurrentFunctionBlock(), conditionValue, defaultBlock,
                       targetPairList);
    terminateCurrentFunctionBlock();

    // Start the next case block.
    nextConditionalBlock();
}

void SPIRVBuilder::writeSwitchCaseBlockEnd()
{
    if (!isCurrentFunctionBlockTerminated())
    {
        // If a case does not end in branch, insert a branch to the next block, implementing
        // fallthrough.  For the last block, the branch target would automatically be the merge
        // block.
        const SpirvConditional *conditional = getCurrentConditional();
        const spirv::IdRef nextBlock        = conditional->blockIds[conditional->nextBlockToWrite];

        spirv::WriteBranch(getSpirvCurrentFunctionBlock(), nextBlock);
        terminateCurrentFunctionBlock();
    }

    // Move on to the next block.
    nextConditionalBlock();
}

// This function is nearly identical to getTypeData(), except for row-major matrices.  For the
// purposes of base alignment and size calculations, it swaps the primary and secondary sizes such
// that the look up always assumes column-major matrices.  Row-major matrices are only applicable to
// interface block fields, so this function is only called on those.
const SpirvTypeData &SPIRVBuilder::getFieldTypeDataForAlignmentAndSize(
    const TType &type,
    TLayoutBlockStorage blockStorage)
{
    SpirvType fieldSpirvType = getSpirvType(type, blockStorage);

    // If the field is row-major, swap the rows and columns for the purposes of base alignment
    // calculation.
    const bool isRowMajor = type.getLayoutQualifier().matrixPacking == EmpRowMajor;
    if (isRowMajor)
    {
        std::swap(fieldSpirvType.primarySize, fieldSpirvType.secondarySize);
    }

    return getSpirvTypeData(fieldSpirvType, nullptr);
}

uint32_t SPIRVBuilder::calculateBaseAlignmentAndSize(const SpirvType &type,
                                                     uint32_t *sizeInStorageBlockOut)
{
    // Calculate the base alignment of a type according to the rules of std140 and std430 packing.
    //
    // See GLES3.2 Section 7.6.2.2 Standard Uniform Block Layout.

    if (!type.arraySizes.empty())
    {
        // > Rule 4. If the member is an array of scalars or vectors, the base alignment and array
        // > stride are set to match the base alignment of a single array element, according to
        // > rules (1), (2), and (3), ...
        //
        // > Rule 10. If the member is an array of S structures, the S elements of the array are
        // > laid out in order, according to rule (9).
        SpirvType baseType  = type;
        baseType.arraySizes = {};
        if (baseType.arraySizes.empty() && baseType.block == nullptr)
        {
            baseType.blockStorage = EbsUnspecified;
        }

        const SpirvTypeData &baseTypeData = getSpirvTypeData(baseType, nullptr);
        uint32_t baseAlignment            = baseTypeData.baseAlignment;
        uint32_t baseSizeInStorageBlock   = baseTypeData.sizeInStorageBlock;

        // For std140 only:
        // > Rule 4. ... and rounded up to the base alignment of a vec4.
        // > Rule 9. ... If none of the structure members are larger than a vec4, the base alignment
        // of the structure is vec4.
        if (type.blockStorage != EbsStd430)
        {
            baseAlignment          = std::max(baseAlignment, 16u);
            baseSizeInStorageBlock = std::max(baseSizeInStorageBlock, 16u);
        }
        // Note that matrix arrays follow a similar rule (rules 6 and 8).  The matrix base alignment
        // is the same as its column or row base alignment, and arrays of that matrix don't change
        // the base alignment.

        // The size occupied by the array is simply the size of each element (which is already
        // aligned to baseAlignment) multiplied by the number of elements.
        *sizeInStorageBlockOut = baseSizeInStorageBlock * GetTotalArrayElements(type);

        return baseAlignment;
    }

    if (type.block != nullptr)
    {
        // > Rule 9. If the member is a structure, the base alignment of the structure is N, where N
        // > is the largest base alignment value of any of its members, and rounded up to the base
        // > alignment of a vec4.

        uint32_t baseAlignment = 4;
        for (const TField *field : type.block->fields())
        {
            const SpirvTypeData &fieldTypeData =
                getFieldTypeDataForAlignmentAndSize(*field->type(), type.blockStorage);
            baseAlignment = std::max(baseAlignment, fieldTypeData.baseAlignment);
        }

        // For std140 only:
        // > If none of the structure members are larger than a vec4, the base alignment of the
        // structure is vec4.
        if (type.blockStorage != EbsStd430)
        {
            baseAlignment = std::max(baseAlignment, 16u);
        }

        // Note: sizeInStorageBlockOut is not calculated here, it's done in
        // calculateSizeAndWriteOffsetDecorations at the same time offsets are calculated.
        *sizeInStorageBlockOut = 0;

        return baseAlignment;
    }

    if (type.secondarySize > 1)
    {
        SpirvType vectorType = type;

        // > Rule 5. If the member is a column-major matrix with C columns and R rows, the matrix is
        // > stored identically to an array of C column vectors with R components each, according to
        // > rule (4).
        //
        // > Rule 7. If the member is a row-major matrix with C columns and R rows, the matrix is
        // > stored identically to an array of R row vectors with C components each, according to
        // > rule (4).
        //
        // For example, given a mat3x4 (3 columns, 4 rows), the base alignment is the same as the
        // base alignment of a vec4 (secondary size) if column-major, and a vec3 (primary size) if
        // row-major.
        //
        // Here, we always calculate the base alignment and size for column-major matrices.  If a
        // row-major matrix is used in a block, the columns and rows are simply swapped before
        // looking up the base alignment and size.

        vectorType.primarySize   = vectorType.secondarySize;
        vectorType.secondarySize = 1;

        const SpirvTypeData &vectorTypeData = getSpirvTypeData(vectorType, nullptr);
        uint32_t baseAlignment              = vectorTypeData.baseAlignment;

        // For std140 only:
        // > Rule 4. ... and rounded up to the base alignment of a vec4.
        if (type.blockStorage != EbsStd430)
        {
            baseAlignment = std::max(baseAlignment, 16u);
        }

        // The size occupied by the matrix is the size of each vector multiplied by the number of
        // vectors.
        *sizeInStorageBlockOut = vectorTypeData.sizeInStorageBlock * vectorType.primarySize;

        return baseAlignment;
    }

    if (type.primarySize > 1)
    {
        // > Rule 2. If the member is a two- or four-component vector with components consuming N
        // > basic machine units, the base alignment is 2N or 4N, respectively.
        //
        // > Rule 3. If the member is a three-component vector with components consuming N basic
        // > machine units, the base alignment is 4N.

        SpirvType baseType   = type;
        baseType.primarySize = 1;

        const SpirvTypeData &baseTypeData = getSpirvTypeData(baseType, nullptr);
        uint32_t baseAlignment            = baseTypeData.baseAlignment;

        uint32_t multiplier = type.primarySize != 3 ? type.primarySize : 4;
        baseAlignment *= multiplier;

        // The size occupied by the vector is the same as its alignment.
        *sizeInStorageBlockOut = baseAlignment;

        return baseAlignment;
    }

    // TODO: support desktop GLSL.  http://anglebug.com/4889.  Except for double (desktop GLSL),
    // every other type occupies 4 bytes.
    constexpr uint32_t kBasicAlignment = 4;
    *sizeInStorageBlockOut             = kBasicAlignment;
    return kBasicAlignment;
}

uint32_t SPIRVBuilder::calculateSizeAndWriteOffsetDecorations(const SpirvType &type,
                                                              spirv::IdRef typeId)
{
    ASSERT(type.block != nullptr);

    uint32_t fieldIndex = 0;
    uint32_t nextOffset = 0;

    // Get the storage size for each field, align them based on block storage rules, and sum them
    // up.  In the process, write Offset decorations for the block.
    //
    // See GLES3.2 Section 7.6.2.2 Standard Uniform Block Layout.

    for (const TField *field : type.block->fields())
    {
        const TType &fieldType = *field->type();

        // Round the offset up to the field's alignment.  The spec says:
        //
        // > A structure and each structure member have a base offset and a base alignment, from
        // > which an aligned offset is computed by rounding the base offset up to a multiple of the
        // > base alignment.
        const SpirvTypeData &fieldTypeData =
            getFieldTypeDataForAlignmentAndSize(fieldType, type.blockStorage);
        nextOffset = rx::roundUp(nextOffset, fieldTypeData.baseAlignment);

        // Write the Offset decoration.
        spirv::WriteMemberDecorate(&mSpirvDecorations, typeId, spirv::LiteralInteger(fieldIndex),
                                   spv::DecorationOffset, {spirv::LiteralInteger(nextOffset)});

        // Calculate the next offset.  The next offset is the current offset plus the size of the
        // field, aligned to its base alignment.
        //
        // > Rule 4. ... the base offset of the member following the array is rounded up to the next
        // > multiple of the base alignment.
        //
        // > Rule 9. ... the base offset of the member following the sub-structure is rounded up to
        // > the next multiple of the base alignment of the structure.
        nextOffset = nextOffset + fieldTypeData.sizeInStorageBlock;
        nextOffset = rx::roundUp(nextOffset, fieldTypeData.baseAlignment);

        ++fieldIndex;
    }

    return nextOffset;
}

void SPIRVBuilder::writeMemberDecorations(const SpirvType &type, spirv::IdRef typeId)
{
    ASSERT(type.block != nullptr);

    uint32_t fieldIndex = 0;

    for (const TField *field : type.block->fields())
    {
        const TType &fieldType = *field->type();
        const SpirvTypeData &fieldTypeData =
            getFieldTypeDataForAlignmentAndSize(fieldType, type.blockStorage);

        // Add invariant decoration if any.
        if (type.isInvariant || fieldType.isInvariant())
        {
            spirv::WriteMemberDecorate(&mSpirvDecorations, typeId,
                                       spirv::LiteralInteger(fieldIndex), spv::DecorationInvariant,
                                       {});
        }

        // Add matrix decorations if any.
        if (fieldType.isMatrix())
        {
            // The matrix stride is simply the alignment of the vector constituting a column or row.
            const uint32_t matrixStride = fieldTypeData.baseAlignment;

            // MatrixStride
            spirv::WriteMemberDecorate(
                &mSpirvDecorations, typeId, spirv::LiteralInteger(fieldIndex),
                spv::DecorationMatrixStride, {spirv::LiteralInteger(matrixStride)});

            // ColMajor or RowMajor
            const bool isRowMajor = fieldType.getLayoutQualifier().matrixPacking == EmpRowMajor;
            spirv::WriteMemberDecorate(
                &mSpirvDecorations, typeId, spirv::LiteralInteger(fieldIndex),
                isRowMajor ? spv::DecorationRowMajor : spv::DecorationColMajor, {});
        }

        // Add other decorations.
        SpirvDecorations decorations = getDecorations(fieldType);
        for (const spv::Decoration decoration : decorations)
        {
            spirv::WriteMemberDecorate(&mSpirvDecorations, typeId,
                                       spirv::LiteralInteger(fieldIndex), decoration, {});
        }

        ++fieldIndex;
    }
}

ImmutableString SPIRVBuilder::hashName(const TSymbol *symbol)
{
    return HashName(symbol, mHashFunction, &mNameMap);
}

ImmutableString SPIRVBuilder::hashTypeName(const TType &type)
{
    return GetTypeName(type, mHashFunction, &mNameMap);
}

ImmutableString SPIRVBuilder::hashFieldName(const TField *field)
{
    ASSERT(field->symbolType() != SymbolType::Empty);
    if (field->symbolType() == SymbolType::UserDefined)
    {
        return HashName(field->name(), mHashFunction, &mNameMap);
    }

    return field->name();
}

ImmutableString SPIRVBuilder::hashFunctionName(const TFunction *func)
{
    if (func->isMain())
    {
        return func->name();
    }

    return hashName(func);
}

spirv::Blob SPIRVBuilder::getSpirv()
{
    ASSERT(mConditionalStack.empty());

    spirv::Blob result;

    // Reserve a minimum amount of memory.
    //
    //   5 for header +
    //   a number of capabilities +
    //   size of already generated instructions.
    //
    // The actual size is larger due to other metadata instructions such as extensions,
    // OpExtInstImport, OpEntryPoint, OpExecutionMode etc.
    result.reserve(5 + mCapabilities.size() * 2 + mSpirvDebug.size() + mSpirvDecorations.size() +
                   mSpirvTypeAndConstantDecls.size() + mSpirvTypePointerDecls.size() +
                   mSpirvFunctionTypeDecls.size() + mSpirvVariableDecls.size() +
                   mSpirvFunctions.size());

    // Generate the SPIR-V header.
    spirv::WriteSpirvHeader(&result, mNextAvailableId);

    // Generate metadata in the following order:
    //
    // - OpCapability instructions.  The Shader capability is always defined.
    spirv::WriteCapability(&result, spv::CapabilityShader);
    for (spv::Capability capability : mCapabilities)
    {
        spirv::WriteCapability(&result, capability);
    }

    // - OpExtension instructions (TODO: http://anglebug.com/4889)

    // - OpExtInstImport
    if (mExtInstImportIdStd.valid())
    {
        spirv::WriteExtInstImport(&result, mExtInstImportIdStd, "GLSL.std.450");
    }

    // - OpMemoryModel
    spirv::WriteMemoryModel(&result, spv::AddressingModelLogical, spv::MemoryModelGLSL450);

    // - OpEntryPoint
    constexpr gl::ShaderMap<spv::ExecutionModel> kExecutionModels = {
        {gl::ShaderType::Vertex, spv::ExecutionModelVertex},
        {gl::ShaderType::TessControl, spv::ExecutionModelTessellationControl},
        {gl::ShaderType::TessEvaluation, spv::ExecutionModelTessellationEvaluation},
        {gl::ShaderType::Geometry, spv::ExecutionModelGeometry},
        {gl::ShaderType::Fragment, spv::ExecutionModelFragment},
        {gl::ShaderType::Compute, spv::ExecutionModelGLCompute},
    };
    spirv::WriteEntryPoint(&result, kExecutionModels[mShaderType], mEntryPointId, "main",
                           mEntryPointInterfaceList);

    // - OpExecutionMode instructions
    generateExecutionModes(&result);

    // - OpSource instruction.
    //
    // This is to support debuggers and capture/replay tools and isn't strictly necessary.
    spirv::WriteSource(&result, spv::SourceLanguageGLSL, spirv::LiteralInteger(450), nullptr,
                       nullptr);

    // Append the already generated sections in order
    result.insert(result.end(), mSpirvDebug.begin(), mSpirvDebug.end());
    result.insert(result.end(), mSpirvDecorations.begin(), mSpirvDecorations.end());
    result.insert(result.end(), mSpirvTypeAndConstantDecls.begin(),
                  mSpirvTypeAndConstantDecls.end());
    result.insert(result.end(), mSpirvTypePointerDecls.begin(), mSpirvTypePointerDecls.end());
    result.insert(result.end(), mSpirvFunctionTypeDecls.begin(), mSpirvFunctionTypeDecls.end());
    result.insert(result.end(), mSpirvVariableDecls.begin(), mSpirvVariableDecls.end());
    result.insert(result.end(), mSpirvFunctions.begin(), mSpirvFunctions.end());

    result.shrink_to_fit();
    return result;
}

void SPIRVBuilder::generateExecutionModes(spirv::Blob *blob)
{
    switch (mShaderType)
    {
        case gl::ShaderType::Fragment:
            spirv::WriteExecutionMode(blob, mEntryPointId, spv::ExecutionModeOriginUpperLeft, {});
            break;

        case gl::ShaderType::Compute:
        {
            const sh::WorkGroupSize &localSize = mCompiler->getComputeShaderLocalSize();
            spirv::WriteExecutionMode(
                blob, mEntryPointId, spv::ExecutionModeLocalSize,
                {spirv::LiteralInteger(localSize[0]), spirv::LiteralInteger(localSize[1]),
                 spirv::LiteralInteger(localSize[2])});
            break;
        }
        default:
            // TODO: other shader types.  http://anglebug.com/4889
            break;
    }
}

}  // namespace sh
