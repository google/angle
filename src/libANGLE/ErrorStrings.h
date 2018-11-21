//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ErrorStrings.h: Contains mapping of commonly used error messages

#ifndef LIBANGLE_ERRORSTRINGS_H_
#define LIBANGLE_ERRORSTRINGS_H_

namespace gl
{
constexpr const char *kError3DDepthStencil =
    "Format cannot be GL_DEPTH_COMPONENT or GL_DEPTH_STENCIL if target is GL_TEXTURE_3D";
constexpr const char *kErrorANGLECopyTexture3DUnavailable =
    "GL_ANGLE_copy_texture_3d extension not available.";
constexpr const char *kErrorBaseLevelOutOfRange = "Texture base level out of range";
constexpr const char *kErrorBlitDimensionsOutOfRange =
    "BlitFramebuffer dimensions out of 32-bit integer range.";
constexpr const char *kErrorBlitExtensionDepthStencilWholeBufferBlit =
    "Only whole-buffer depth and stencil blits are supported by this extension.";
constexpr const char *kErrorBlitExtensionFormatMismatch =
    "Attempting to blit and the read and draw buffer formats don't match.";
constexpr const char *kErrorBlitExtensionFromInvalidAttachmentType =
    "Blits are only supported from 2D texture = renderbuffer or default framebuffer attachments "
    "in this extension.";
constexpr const char *kErrorBlitExtensionLinear = "Linear blit not supported in this extension.";
constexpr const char *kErrorBlitExtensionMultisampledDepthOrStencil =
    "Multisampled depth/stencil blit is not supported by this extension.";
constexpr const char *kErrorBlitExtensionMultisampledWholeBufferBlit =
    "Only whole-buffer blit is supported from a multisampled read buffer in this extension.";
constexpr const char *kErrorBlitExtensionNotAvailable = "Blit extension not available.";
constexpr const char *kErrorBlitExtensionScaleOrFlip =
    "Scaling and flipping in BlitFramebufferANGLE not supported by this implementation.";
constexpr const char *kErrorBlitExtensionToInvalidAttachmentType =
    "Blits are only supported to 2D texture = renderbuffer or default framebuffer attachments in "
    "this extension.";
constexpr const char *kErrorBlitFeedbackLoop =
    "Blit feedback loop: the read and draw framebuffers are the same.";
constexpr const char *kErrorBlitFramebufferMissing =
    "Read and draw framebuffers must both exist for a blit to succeed.";
constexpr const char *kErrorBlitFromMultiview = "Attempt to read from a multi-view framebuffer.";
constexpr const char *kErrorBlitDepthOrStencilFormatMismatch =
    "Depth/stencil buffer format combination not allowed for blit.";
constexpr const char *kErrorBlitIntegerWithLinearFilter =
    "Cannot use GL_LINEAR filter when blitting a integer framebuffer.";
constexpr const char *kErrorBlitInvalidFilter = "Invalid blit filter.";
constexpr const char *kErrorBlitInvalidMask   = "Invalid blit mask.";
constexpr const char *kErrorBlitMissingColor =
    "Attempt to read from a missing color attachment of a complete framebuffer.";
constexpr const char *kErrorBlitMissingDepthOrStencil =
    "Attempt to read from a missing depth/stencil attachment of a complete framebuffer.";
constexpr const char *kErrorBlitOnlyNearestForNonColor =
    "Only nearest filtering can be used when blitting buffers other than the color buffer.";
constexpr const char *kErrorBlitToMultiview = "Attempt to write to a multi-view framebuffer.";
constexpr const char *kErrorBlitTypeMismatchFixedOrFloat =
    "If the read buffer contains fixed-point or floating-point values = the draw buffer must as "
    "well.";
constexpr const char *kErrorBlitTypeMismatchFixedPoint =
    "If the read buffer contains fixed-point values = the draw buffer must as well.";
constexpr const char *kErrorBlitTypeMismatchSignedInteger =
    "If the read buffer contains signed integer values the draw buffer must as well.";
constexpr const char *kErrorBlitTypeMismatchUnsignedInteger =
    "If the read buffer contains unsigned integer values the draw buffer must as well.";
constexpr const char *kErrorBlitMultisampledBoundsMismatch =
    "Attempt to blit from a multisampled framebuffer and the bounds don't match with the draw "
    "framebuffer.";
constexpr const char *kErrorBlitMultisampledFormatOrBoundsMismatch =
    "Attempt to blit from a multisampled framebuffer and the bounds or format of the color "
    "buffer don't match with the draw framebuffer.";
constexpr const char *kErrorBlitSameImageColor =
    "Read and write color attachments cannot be the same image.";
constexpr const char *kErrorBlitSameImageDepthOrStencil =
    "Read and write depth stencil attachments cannot be the same image.";
constexpr const char *kErrorBufferBoundForTransformFeedback =
    "Buffer is bound for transform feedback.";
constexpr const char *kErrorBufferNotBound = "A buffer must be bound.";
constexpr const char *kErrorBufferMapped   = "An active buffer is mapped";
constexpr const char *kErrorColorNumberGreaterThanMaxDualSourceDrawBuffers =
    "Color number for secondary color greater than or equal to MAX_DUAL_SOURCE_DRAW_BUFFERS";
constexpr const char *kErrorColorNumberGreaterThanMaxDrawBuffers =
    "Color number for primary color greater than or equal to MAX_DRAW_BUFFERS";
constexpr const char *kErrorCompressedMismatch =
    "Compressed data is valid if-and-only-if the texture is compressed.";
constexpr const char *kErrorCompressedTextureDimensionsMustMatchData =
    "Compressed texture dimensions must exactly match the dimensions of the data passed in.";
constexpr const char *kErrorCompressedTexturesNotAttachable =
    "Compressed textures cannot be attached to a framebuffer.";
constexpr const char *kErrorCopyAlias = "The read and write copy regions alias memory.";
constexpr const char *kErrorCubemapFacesEqualDimensions =
    "Each cubemap face must have equal width and height.";
constexpr const char *kErrorCubemapIncomplete =
    "Texture is not cubemap complete. All cubemaps faces must be defined and be the same size.";
constexpr const char *kErrorDefaultFramebufferInvalidAttachment =
    "Invalid attachment when the default framebuffer is bound.";
constexpr const char *kErrorDefaultFramebufferTarget =
    "It is invalid to change default FBO's attachments";
constexpr const char *kErrorDefaultVertexArray   = "Default vertex array object is bound.";
constexpr const char *kErrorDestinationImmutable = "Destination texture cannot be immutable.";
constexpr const char *kErrorDestinationLevelNotDefined =
    "The destination level of the destination texture must be defined.";
constexpr const char *kErrorDestinationTextureTooSmall = "Destination texture too small.";
constexpr const char *kErrorDimensionsMustBePow2       = "Texture dimensions must be power-of-two.";
constexpr const char *kErrorDispatchIndirectBufferNotBound =
    "Dispatch indirect buffer must be bound.";
constexpr const char *kErrorDoubleBoundTransformFeedbackBuffer =
    "Transform feedback has a buffer bound to multiple outputs.";
constexpr const char *kErrorDrawIndirectBufferNotBound = "Draw indirect buffer must be bound.";
constexpr const char *kErrorDrawBufferTypeMismatch =
    "Fragment shader output type does not match the bound framebuffer attachment type.";
constexpr const char *kErrorDrawFramebufferIncomplete = "Draw framebuffer is incomplete";
constexpr const char *kErrorElementArrayBufferBoundForTransformFeedback =
    "It is undefined behavior to use an element array buffer that is bound for transform "
    "feedback.";
constexpr const char *kErrorEnumNotSupported   = "Enum is not currently supported.";
constexpr const char *kErrorEnumRequiresGLES31 = "Enum requires GLES 3.1";
constexpr const char *kErrorES31Required       = "OpenGL ES 3.1 Required";
constexpr const char *kErrorES3Required        = "OpenGL ES 3.0 Required.";
constexpr const char *kErrorExceedsMaxElement  = "Element value exceeds maximum element index.";
constexpr const char *kErrorExpectedProgramName =
    "Expected a program name = but found a shader name.";
constexpr const char *kErrorExpectedShaderName =
    "Expected a shader name = but found a program name.";
constexpr const char *kErrorExtensionNotEnabled = "Extension is not enabled.";
constexpr const char *kErrorFeedbackLoop =
    "Feedback loop formed between Framebuffer and active Texture.";
constexpr const char *kErrorFragDataBindingIndexOutOfRange =
    "Fragment output color index must be zero or one.";
constexpr const char *kErrorFramebufferIncomplete = "Framebuffer is incomplete.";
constexpr const char *kErrorFramebufferIncompleteAttachment =
    "Attachment type must be compatible with attachment object.";
constexpr const char *kErrorFramebufferTextureInvalidLayer =
    "Layer invalid for framebuffer texture attachment.";
constexpr const char *kErrorFramebufferTextureInvalidMipLevel =
    "Mip level invalid for framebuffer texture attachment.";
constexpr const char *kErrorFramebufferTextureLayerIncorrectTextureType =
    "Texture is not a three-dimensional or two-dimensionsal array texture.";
constexpr const char *kErrorGenerateMipmapNotAllowed =
    "Texture format does not support mipmap generation.";
constexpr const char *kErrorGeometryShaderExtensionNotEnabled =
    "GL_EXT_geometry_shader extension not enabled.";
constexpr const char *kErrorGLES1Only = "GLES1-only function.";
constexpr const char *kErrorImmutableTextureBound =
    "The value of TEXTURE_IMMUTABLE_FORMAT for the texture currently bound to target on the "
    "active texture unit is true.";
constexpr const char *kErrorIncompatibleDrawModeAgainstGeometryShader =
    "Primitive mode is incompatible with the input primitive type of the geometry shader.";
constexpr const char *kErrorIndexExceedsMaxActiveUniform =
    "Index must be less than program active uniform count.";
constexpr const char *kErrorIndexExceedsMaxActiveUniformBlock =
    "Index must be less than program active uniform block count.";
constexpr const char *kErrorIndexExceedsMaxAtomicCounterBufferBindings =
    "Index must be less than MAX_ATOMIC_COUNTER_BUFFER_BINDINGS.";
constexpr const char *kErrorIndexExceedsMaxDrawBuffer = "Index must be less than MAX_DRAW_BUFFERS.";
constexpr const char *kErrorIndexExceedsMaxTransformFeedbackAttribs =
    "Index must be less than MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS.";
constexpr const char *kErrorIndexExceedsMaxUniformBufferBindings =
    "Index must be less than MAX_UNIFORM_BUFFER_BINDINGS.";
constexpr const char *kErrorIndexExceedsMaxVertexAttribute =
    "Index must be less than MAX_VERTEX_ATTRIBS.";
constexpr const char *kErrorIndexExceedsMaxWorkgroupDimensions =
    "Index must be less than the number of workgroup dimensions (3).";
constexpr const char *kErrorIndexExceedsSamples = "Index must be less than the value of SAMPLES.";
constexpr const char *kErrorInsufficientBufferSize = "Insufficient buffer size.";
constexpr const char *kErrorInsufficientVertexBufferSize =
    "Vertex buffer is not big enough for the draw call";
constexpr const char *kErrorIntegerOverflow = "Integer overflow.";
constexpr const char *kErrorInternalFormatRequiresTexture2DArray =
    "internalformat is an ETC2/EAC format and target is not GL_TEXTURE_2D_ARRAY.";
constexpr const char *kErrorInvalidAttachment    = "Invalid Attachment Type.";
constexpr const char *kErrorInvalidBlendEquation = "Invalid blend equation.";
constexpr const char *kErrorInvalidBlendFunction = "Invalid blend function.";
constexpr const char *kErrorInvalidBooleanValue =
    "Invalid boolean value. Must be GL_FALSE or GL_TRUE.";
constexpr const char *kErrorInvalidBorder      = "Border must be 0.";
constexpr const char *kErrorInvalidBufferTypes = "Invalid buffer target.";
constexpr const char *kErrorInvalidBufferUsage = "Invalid buffer usage enum.";
constexpr const char *kErrorInvalidClearMask   = "Invalid mask bits.";
constexpr const char *kErrorInvalidClientState = "Invalid client vertex array type.";
constexpr const char *kErrorInvalidClipPlane   = "Invalid clip plane.";
constexpr const char *kErrorInvalidCombinedImageUnit =
    "Specified unit must be in [GL_TEXTURE0 = GL_TEXTURE0 + GL_MAX_COMBINED_IMAGE_UNITS)";
constexpr const char *kErrorInvalidCompressedFormat    = "Not a valid compressed texture format.";
constexpr const char *kErrorInvalidCompressedImageSize = "Invalid compressed image size.";
constexpr const char *kErrorInvalidConstantColor =
    "CONSTANT_COLOR (or ONE_MINUS_CONSTANT_COLOR) and CONSTANT_ALPHA (or "
    "ONE_MINUS_CONSTANT_ALPHA) cannot be used together as source and destination factors in the "
    "blend function.";
constexpr const char *kErrorInvalidCopyCombination = "Invalid copy texture format combination.";
constexpr const char *kErrorInvalidCoverMode       = "Invalid cover mode.";
constexpr const char *kErrorInvalidCullMode        = "Cull mode not recognized.";
constexpr const char *kErrorInvalidDebugSeverity   = "Invalid debug severity.";
constexpr const char *kErrorInvalidDebugSource     = "Invalid debug source.";
constexpr const char *kErrorInvalidDebugType       = "Invalid debug type.";
constexpr const char *kErrorInvalidDepthRange      = "Near value cannot be greater than far.";
constexpr const char *kErrorInvalidDepthStencilDrawBuffer =
    "Draw buffer must be zero when using depth or stencil.";
constexpr const char *kErrorInvalidDestinationTexture =
    "Destination texture is not a valid texture object.";
constexpr const char *kErrorInvalidDestinationTextureType = "Invalid destination texture type.";
constexpr const char *kErrorInvalidDrawMode               = "Invalid draw mode.";
constexpr const char *kErrorInvalidDrawModeTransformFeedback =
    "Draw mode must match current transform feedback object's draw mode.";
constexpr const char *kErrorInvalidFence          = "Invalid fence object.";
constexpr const char *kErrorInvalidFenceCondition = "Invalid value for condition.";
constexpr const char *kErrorInvalidFenceState     = "Fence must be set.";
constexpr const char *kErrorInvalidFillMode       = "Invalid fill mode.";
constexpr const char *kErrorInvalidFilterTexture =
    "Texture only supports NEAREST and LINEAR filtering.";
constexpr const char *kErrorInvalidFlags        = "Invalid value for flags.";
constexpr const char *kErrorInvalidFogDensity   = "Invalid fog density (must be nonnegative).";
constexpr const char *kErrorInvalidFogMode      = "Invalid fog mode.";
constexpr const char *kErrorInvalidFogParameter = "Invalid fog parameter.";
constexpr const char *kErrorInvalidFormat       = "Invalid format.";
constexpr const char *kErrorInvalidFormatCombination =
    "Invalid combination of format = type and internalFormat.";
constexpr const char *kErrorInvalidFramebufferTarget = "Invalid framebuffer target.";
constexpr const char *kErrorInvalidFramebufferTextureLevel =
    "Mipmap level must be 0 when attaching a texture.";
constexpr const char *kErrorInvalidFramebufferAttachmentParameter =
    "Invalid parameter name for framebuffer attachment.";
constexpr const char *kErrorInvalidFramebufferLayer =
    "Framebuffer layer cannot be less than 0 or greater than GL_MAX_FRAMEBUFFER_LAYERS_EXT.";
constexpr const char *kErrorInvalidIndirectOffset =
    "indirect must be a multiple of the size of uint in basic machine units.";
constexpr const char *kErrorInvalidImageUnit =
    "Image unit cannot be greater than or equal to the value of MAX_IMAGE_UNITS.";
constexpr const char *kErrorInvalidInternalFormat      = "Invalid internal format.";
constexpr const char *kErrorInvalidLight               = "Invalid light.";
constexpr const char *kErrorInvalidLightModelParameter = "Invalid light model parameter.";
constexpr const char *kErrorInvalidLightParameter      = "Invalid light parameter.";
constexpr const char *kErrorInvalidLogicOp             = "Invalid logical operation.";
constexpr const char *kErrorInvalidMaterialFace        = "Invalid material face.";
constexpr const char *kErrorInvalidMaterialParameter   = "Invalid material parameter.";
constexpr const char *kErrorInvalidMatrixMode          = "Invalid matrix mode.";
constexpr const char *kErrorInvalidMemoryBarrierBit    = "Invalid memory barrier bit.";
constexpr const char *kErrorInvalidMipLevel            = "Level of detail outside of range.";
constexpr const char *kErrorInvalidMipLevels           = "Invalid level count.";
constexpr const char *kErrorInvalidMultitextureUnit =
    "Specified unit must be in [GL_TEXTURE0 = GL_TEXTURE0 + GL_MAX_TEXTURE_UNITS)";
constexpr const char *kErrorInvalidMultisampledFramebufferOperation =
    "Invalid operation on multisampled framebuffer";
constexpr const char *kErrorInvalidName           = "Invalid name.";
constexpr const char *kErrorInvalidNameCharacters = "Name contains invalid characters.";
constexpr const char *kErrorInvalidPname          = "Invalid pname.";
constexpr const char *kErrorInvalidPointerQuery   = "Invalid pointer query.";
constexpr const char *kErrorInvalidPointParameter = "Invalid point parameter.";
constexpr const char *kErrorInvalidPointParameterValue =
    "Invalid point parameter value (must be non-negative).";
constexpr const char *kErrorInvalidPointSizeValue = "Invalid point size (must be positive).";
constexpr const char *kErrorInvalidPrecision      = "Invalid or unsupported precision type.";
constexpr const char *kErrorInvalidProgramName    = "Program object expected.";
constexpr const char *kErrorInvalidProjectionMatrix =
    "Invalid projection matrix. Left/right = top/bottom = near/far intervals cannot be zero = and "
    "near/far cannot be less than zero.";
constexpr const char *kErrorInvalidQueryId     = "Invalid query Id.";
constexpr const char *kErrorInvalidQueryTarget = "Invalid query target.";
constexpr const char *kErrorInvalidQueryType   = "Invalid query type.";
constexpr const char *kErrorInvalidRange       = "Invalid range.";
constexpr const char *kErrorInvalidRenderbufferInternalFormat =
    "Invalid renderbuffer internalformat.";
constexpr const char *kErrorInvalidRenderbufferTarget = "Invalid renderbuffer target.";
constexpr const char *kErrorInvalidRenderbufferTextureParameter =
    "Invalid parameter name for renderbuffer attachment.";
constexpr const char *kErrorInvalidRenderbufferWidthHeight =
    "Renderbuffer width and height cannot be negative and cannot exceed maximum texture size.";
constexpr const char *kErrorInvalidSampleMaskNumber =
    "MaskNumber cannot be greater than or equal to the value of MAX_SAMPLE_MASK_WORDS.";
constexpr const char *kErrorInvalidSampler       = "Sampler is not valid";
constexpr const char *kErrorInvalidShaderName    = "Shader object expected.";
constexpr const char *kErrorInvalidShaderType    = "Invalid shader type.";
constexpr const char *kErrorInvalidShadingModel  = "Invalid shading model.";
constexpr const char *kErrorInvalidSourceTexture = "Source texture is not a valid texture object.";
constexpr const char *kErrorInvalidSourceTextureLevel  = "Invalid source texture level.";
constexpr const char *kErrorInvalidSourceTextureSize   = "Invalid source texture height or width.";
constexpr const char *kErrorInvalidStencil             = "Invalid stencil.";
constexpr const char *kErrorInvalidStencilBitMask      = "Invalid stencil bit mask.";
constexpr const char *kErrorInvalidTarget              = "Invalid target.";
constexpr const char *kErrorInvalidTextureCombine      = "Invalid texture combine mode.";
constexpr const char *kErrorInvalidTextureCombineSrc   = "Invalid texture combine source.";
constexpr const char *kErrorInvalidTextureCombineOp    = "Invalid texture combine operand.";
constexpr const char *kErrorInvalidTextureEnvMode      = "Invalid texture environment mode.";
constexpr const char *kErrorInvalidTextureEnvParameter = "Invalid texture environment parameter.";
constexpr const char *kErrorInvalidTextureEnvScale     = "Invalid texture environment scale.";
constexpr const char *kErrorInvalidTextureEnvTarget    = "Invalid texture environment target.";
constexpr const char *kErrorInvalidTextureFilterParam  = "Texture filter not recognized.";
constexpr const char *kErrorInvalidTextureName         = "Not a valid texture object name.";
constexpr const char *kErrorInvalidTextureRange =
    "Cannot be less than 0 or greater than maximum number of textures.";
constexpr const char *kErrorInvalidTextureTarget = "Invalid or unsupported texture target.";
constexpr const char *kErrorInvalidTextureWrap   = "Texture wrap mode not recognized.";
constexpr const char *kErrorInvalidTimeout       = "Invalid value for timeout.";
constexpr const char *kErrorInvalidTransformFeedbackAttribsCount =
    "Count exeeds MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS.";
constexpr const char *kErrorInvalidType            = "Invalid type.";
constexpr const char *kErrorInvalidTypePureInt     = "Invalid type = should be integer";
constexpr const char *kErrorInvalidUniformCount    = "Only array uniforms may have count > 1.";
constexpr const char *kErrorInvalidUniformLocation = "Invalid uniform location";
constexpr const char *kErrorInvalidUnpackAlignment = "Unpack alignment must be 1 = 2 = 4 = or 8.";
constexpr const char *kErrorInvalidVertexArray     = "Vertex array does not exist.";
constexpr const char *kErrorInvalidVertexAttrSize =
    "Vertex attribute size must be 1 = 2 = 3 = or 4.";
constexpr const char *kErrorInvalidVertexPointerSize =
    "Size for built-in vertex attribute is outside allowed range.";
constexpr const char *kErrorInvalidVertexPointerStride =
    "Invalid stride for built-in vertex attribute.";
constexpr const char *kErrorInvalidVertexPointerType =
    "Invalid type for built-in vertex attribute.";
constexpr const char *kErrorInvalidWidth                = "Invalid width.";
constexpr const char *kErrorInvalidWrapModeTexture      = "Invalid wrap mode for texture type.";
constexpr const char *kErrorLevelNotZero                = "Texture level must be zero.";
constexpr const char *kErrorLightParameterOutOfRange    = "Light parameter out of range.";
constexpr const char *kErrorMaterialParameterOutOfRange = "Material parameter out of range.";
constexpr const char *kErrorMatrixStackOverflow         = "Current matrix stack is full.";
constexpr const char *kErrorMatrixStackUnderflow = "Current matrix stack has only a single matrix.";
constexpr const char *kErrorMismatchedByteCountType = "Buffer size does not align with data type.";
constexpr const char *kErrorMismatchedFormat        = "Format must match internal format.";
constexpr const char *kErrorMismatchedTargetAndFormat =
    "Invalid texture target and format combination.";
constexpr const char *kErrorMismatchedTypeAndFormat = "Invalid format and type combination.";
constexpr const char *kErrorMismatchedVariableProgram =
    "Variable is not part of the current program.";
constexpr const char *kErrorMissingReadAttachment = "Missing read attachment.";
constexpr const char *kErrorMissingTexture        = "No Texture is bound to the specified target.";
constexpr const char *kErrorMustHaveElementArrayBinding = "Must have element array buffer bound.";
constexpr const char *kErrorMultiviewActive =
    "The number of views in the active draw framebuffer is greater than 1.";
constexpr const char *kErrorMultiviewViewsTooLarge =
    "numViews cannot be greater than GL_MAX_VIEWS_ANGLE.";
constexpr const char *kErrorMultiviewViewsTooSmall = "numViews cannot be less than 1.";
constexpr const char *kErrorMultiviewNotAvailable  = "ANGLE_multiview is not available.";
constexpr const char *kErrorMultiviewMismatch =
    "The number of views in the active program and draw "
    "framebuffer does not match.";
constexpr const char *kErrorMultiviewTransformFeedback =
    "There is an active transform feedback object when "
    "the number of views in the active draw framebuffer "
    "is greater than 1.";
constexpr const char *kErrorMultiviewTimerQuery =
    "There is an active query for target "
    "GL_TIME_ELAPSED_EXT when the number of views in the "
    "active draw framebuffer is greater than 1.";
constexpr const char *kErrorMultisampleArrayExtensionRequired =
    "GL_ANGLE_texture_multisample_array not enabled.";
constexpr const char *kErrorMultisampleTextureExtensionOrES31Required =
    "GL_ANGLE_texture_multisample or GLES 3.1 required.";
constexpr const char *kErrorNameBeginsWithGL = "Attributes that begin with 'gl_' are not allowed.";
constexpr const char *kErrorNegativeAttachments = "Negative number of attachments.";
constexpr const char *kErrorNegativeBufferSize  = "Negative buffer size.";
constexpr const char *kErrorNegativeCount       = "Negative count.";
constexpr const char *kErrorNegativeHeightWidthDepth =
    "Cannot have negative height = width = or depth.";
constexpr const char *kErrorNegativeLayer     = "Negative layer.";
constexpr const char *kErrorNegativeLength    = "Negative length.";
constexpr const char *kErrorNegativeMaxCount  = "Negative maxcount.";
constexpr const char *kErrorNegativeOffset    = "Negative offset.";
constexpr const char *kErrorNegativePrimcount = "Primcount must be greater than or equal to zero.";
constexpr const char *kErrorNegativeSize      = "Cannot have negative height or width.";
constexpr const char *kErrorNegativeStart     = "Cannot have negative start.";
constexpr const char *kErrorNegativeStride    = "Cannot have negative stride.";
constexpr const char *kErrorNegativeXYZ       = "x = y = or z cannot be negative.";
constexpr const char *kErrorNoActiveComputeShaderStage =
    "No active compute shader stage in this program.";
constexpr const char *kErrorNoActiveGeometryShaderStage =
    "No active geometry shader stage in this program.";
constexpr const char *kErrorNoActiveGraphicsShaderStage =
    "It is a undefined behaviour to render without vertex shader stage or fragment shader stage.";
constexpr const char *kErrorNoActiveProgramWithComputeShader =
    "No active program for the compute shader stage.";
constexpr const char *kErrorNonPositiveDrawTextureDimension =
    "Both width and height argument of drawn texture must be positive.";
constexpr const char *kErrorNoSuchPath = "No such path object.";
constexpr const char *kErrorNoTransformFeedbackOutputVariables =
    "The active program has specified no output variables to record.";
constexpr const char *kErrorNoZeroDivisor =
    "At least one enabled attribute must have a divisor of zero.";
constexpr const char *kErrorNVFenceNotSupported = "GL_NV_fence is not supported";
constexpr const char *kErrorObjectNotGenerated =
    "Object cannot be used because it has not been generated.";
constexpr const char *kErrorOffsetMustBeMultipleOfType =
    "Offset must be a multiple of the passed in datatype.";
constexpr const char *kErrorOffsetMustBeMultipleOfUint =
    "Offset must be a multiple of the size = in basic machine units = of uint";
constexpr const char *kErrorOffsetOverflow  = "Offset overflows texture dimensions.";
constexpr const char *kErrorOutsideOfBounds = "Parameter outside of bounds.";
constexpr const char *kErrorParamOverflow =
    "The provided parameters overflow with the provided buffer.";
constexpr const char *kErrorPixelDataNotNull = "Pixel data must be null.";
constexpr const char *kErrorPixelDataNull    = "Pixel data cannot be null.";
constexpr const char *kErrorPixelPackBufferBoundForTransformFeedback =
    "It is undefined behavior to use a pixel pack buffer that is bound for transform feedback.";
constexpr const char *kErrorPixelUnpackBufferBoundForTransformFeedback =
    "It is undefined behavior to use a pixel unpack buffer that is bound for transform feedback.";
constexpr const char *kErrorPointSizeArrayExtensionNotEnabled =
    "GL_OES_point_size_array not enabled.";
constexpr const char *kErrorProgramDoesNotExist = "Program doesn't exist.";
constexpr const char *kErrorProgramInterfaceMustBeProgramOutput =
    "programInterface must be set to GL_PROGRAM_OUTPUT.";
constexpr const char *kErrorProgramNotBound          = "A program must be bound.";
constexpr const char *kErrorProgramNotLinked         = "Program not linked.";
constexpr const char *kErrorQueryActive              = "Query is active.";
constexpr const char *kErrorQueryExtensionNotEnabled = "Query extension not enabled.";
constexpr const char *kErrorReadBufferNone           = "Read buffer is GL_NONE.";
constexpr const char *kErrorReadBufferNotAttached    = "Read buffer has no attachment.";
constexpr const char *kErrorRectangleTextureCompressed =
    "Rectangle texture cannot have a compressed format.";
constexpr const char *kErrorRelativeOffsetTooLarge =
    "relativeOffset cannot be greater than MAX_VERTEX_ATTRIB_RELATIVE_OFFSET.";
constexpr const char *kErrorRenderableInternalFormat =
    "SizedInternalformat must be color-renderable = depth-renderable = or stencil-renderable.";
constexpr const char *kErrorRenderbufferNotBound = "A renderbuffer must be bound.";
constexpr const char *kErrorResourceMaxRenderbufferSize =
    "Desired resource size is greater than max renderbuffer size.";
constexpr const char *kErrorResourceMaxTextureSize =
    "Desired resource size is greater than max texture size.";
constexpr const char *kErrorSamplesZero = "Samples may not be zero.";
constexpr const char *kErrorSamplesOutOfRange =
    "Samples must not be greater than maximum supported value for the format.";
constexpr const char *kErrorShaderAttachmentHasShader = "Shader attachment already has a shader.";
constexpr const char *kErrorShaderSourceInvalidCharacters =
    "Shader source contains invalid characters.";
constexpr const char *kErrorShaderToDetachMustBeAttached =
    "Shader to be detached must be currently attached to the program.";
constexpr const char *kErrorSourceLevelNotDefined =
    "The source level of the source texture must be defined.";
constexpr const char *kErrorSourceTextureTooSmall =
    "The specified dimensions are outside of the bounds of the texture.";
constexpr const char *kErrorStencilReferenceMaskOrMismatch =
    "Stencil reference and mask values must be the same for front facing and back facing "
    "triangles.";
constexpr const char *kErrorStrideMustBeMultipleOfType =
    "Stride must be a multiple of the passed in datatype.";
constexpr const char *kErrorSyncMissing = "Sync object does not exist.";
constexpr const char *kErrorTargetMustBeTexture2DMultisampleArrayOES =
    "Target must be TEXTURE_2D_MULTISAMPLE_ARRAY_OES.";
constexpr const char *kErrorTextureIsImmutable = "Texture is immutable.";
constexpr const char *kErrorTextureNotBound    = "A texture must be bound.";
constexpr const char *kErrorTextureNotPow2     = "The texture is a non-power-of-two texture.";
constexpr const char *kErrorTextureSizeTooSmall =
    "Texture dimensions must all be greater than zero.";
constexpr const char *kErrorTextureTargetRequiresES31 =
    "Texture target requires at least OpenGL ES 3.1.";
constexpr const char *kErrorTextureTypeConflict =
    "Two textures of different types use the same sampler location.";
constexpr const char *kErrorTextureWidthOrHeightOutOfRange =
    "Width and height must be less than or equal to GL_MAX_TEXTURE_SIZE.";
constexpr const char *kErrorTransformFeedbackBufferDoubleBound =
    "A transform feedback buffer that would be written to is also bound to a "
    "non-transform-feedback target = which would cause undefined behavior.";
constexpr const char *kErrorTransformFeedbackBufferTooSmall =
    "Not enough space in bound transform feedback buffers.";
constexpr const char *kErrorTransformFeedbackDoesNotExist =
    "Transform feedback object that does not exist.";
constexpr const char *kErrorTransformFeedbackNotActive = "No Transform Feedback object is active.";
constexpr const char *kErrorTransformFeedbackNotPaused =
    "The active Transform Feedback object is not paused.";
constexpr const char *kErrorTransformFeedbackPaused =
    "The active Transform Feedback object is paused.";
constexpr const char *kErrorTransformFeedbackVaryingIndexOutOfRange =
    "Index must be less than the transform feedback varying count in the program.";
constexpr const char *kErrorTypeMismatch =
    "Passed in texture target and format must match the one originally used to define the "
    "texture.";
constexpr const char *kErrorTypeNotUnsignedShortByte =
    "Only UNSIGNED_SHORT and UNSIGNED_BYTE types are supported.";
constexpr const char *kErrorUniformBufferBoundForTransformFeedback =
    "It is undefined behavior to use an uniform buffer that is bound for transform feedback.";
constexpr const char *kErrorUniformBufferTooSmall =
    "It is undefined behaviour to use a uniform buffer that is too small.";
constexpr const char *kErrorUniformBufferUnbound =
    "It is undefined behaviour to have a used but unbound uniform buffer.";
constexpr const char *kErrorUniformSizeMismatch = "Uniform size does not match uniform method.";
constexpr const char *kErrorUnknownParameter    = "Unknown parameter value.";
constexpr const char *kErrorUnsizedInternalFormatUnsupported =
    "Internalformat is one of the unsupported unsized base internalformats.";
constexpr const char *kErrorUnsupportedDrawModeForTransformFeedback =
    "The draw command is unsupported when transform feedback is active and not paused.";
constexpr const char *kErrorVertexArrayNoBuffer = "An enabled vertex array has no buffer.";
constexpr const char *kErrorVertexArrayNoBufferPointer =
    "An enabled vertex array has no buffer and no pointer.";
constexpr const char *kErrorVertexBufferBoundForTransformFeedback =
    "It is undefined behavior to use a vertex buffer that is bound for transform feedback.";
constexpr const char *kErrorVertexShaderTypeMismatch =
    "Vertex shader input type does not match the type of the bound vertex attribute.";
constexpr const char *kErrorViewportNegativeSize = "Viewport size cannot be negative.";
constexpr const char *kErrorWebgl2NameLengthLimitExceeded =
    "Location lengths must not be greater than 1024 characters.";
constexpr const char *kErrorWebglBindAttribLocationReservedPrefix =
    "Attributes that begin with 'webgl_' = or '_webgl_' are not allowed.";
constexpr const char *kErrorWebglNameLengthLimitExceeded =
    "Location name lengths must not be greater than 256 characters.";
constexpr const char *kErrorZeroBoundToTarget = "Zero is bound to target.";
}  // namespace gl
#endif  // LIBANGLE_ERRORSTRINGS_H_
