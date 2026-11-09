//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// renderer_utils:
//   Helper methods pertaining to most or all back-ends.
//

#ifdef UNSAFE_BUFFERS_BUILD
#    pragma allow_unsafe_buffers
#endif

#include "libANGLE/renderer/renderer_utils.h"

#include "common/base/anglebase/numerics/checked_math.h"
#include "common/string_utils.h"
#include "common/system_utils.h"
#include "common/utilities.h"
#include "image_util/copyimage.h"
#include "image_util/imageformats.h"
#include "libANGLE/AttributeMap.h"
#include "libANGLE/Context.h"
#include "libANGLE/Context.inl.h"
#include "libANGLE/Display.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/ContextImpl.h"
#include "libANGLE/renderer/Format.h"
#include "platform/Feature.h"

#include <cctype>
#include <cstring>
#include <type_traits>

namespace angle
{
namespace
{
// For the sake of feature name matching, underscore is ignored, and the names are matched
// case-insensitive.  This allows feature names to be overriden both in snake_case (previously used
// by ANGLE) and camelCase.  The second string (user-provided name) can end in `*` for wildcard
// matching.
bool FeatureNameMatch(const std::string &a, const std::string &b)
{
    size_t ai = 0;
    size_t bi = 0;

    while (ai < a.size() && bi < b.size())
    {
        if (a[ai] == '_')
        {
            ++ai;
        }
        if (b[bi] == '_')
        {
            ++bi;
        }
        if (b[bi] == '*' && bi + 1 == b.size())
        {
            // If selected feature name ends in wildcard, match it.
            return true;
        }
        if (std::tolower(a[ai++]) != std::tolower(b[bi++]))
        {
            return false;
        }
    }

    return ai == a.size() && bi == b.size();
}
}  // anonymous namespace

void FeatureInfo::applyOverride(bool state)
{
    enabled     = state;
    hasOverride = true;
}

// FeatureSetBase implementation
void FeatureSetBase::reset()
{
    for (const auto &iter : members)
    {
        FeatureInfo *feature = iter.second;
        feature->enabled     = false;
        feature->hasOverride = false;
    }
}

std::string FeatureSetBase::overrideFeatures(const std::vector<std::string> &featureNames,
                                             bool enabled)
{
    std::stringstream featureStream;

    for (const std::string &name : featureNames)
    {
        const bool hasWildcard = name.back() == '*';
        for (const auto &iter : members)
        {
            const std::string &featureName = iter.first;
            FeatureInfo *feature           = iter.second;

            if (!FeatureNameMatch(featureName, name))
            {
                continue;
            }

            feature->applyOverride(enabled);

            if (featureStream.str().empty())
            {
                featureStream << "Feature overrides: ";
            }
            featureStream << featureName << (enabled ? " enabled, " : " disabled, ");

            // If name has a wildcard, try to match it with all features.  Otherwise, bail on first
            // match, as names are unique.
            if (!hasWildcard)
            {
                break;
            }
        }
    }

    if (!featureStream.str().empty())
    {
        featureStream << std::endl;
    }

    return featureStream.str();
}

void FeatureSetBase::populateFeatureList(FeatureList *features) const
{
    for (const auto &member : members)
    {
        features->push_back(member.second);
    }
}
}  // namespace angle

namespace rx
{

namespace
{
// Both D3D and Vulkan support the same set of standard sample positions for 1, 2, 4, 8, and 16
// samples.  See:
//
// - https://msdn.microsoft.com/en-us/library/windows/desktop/ff476218.aspx
//
// -
// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#primsrast-multisampling
using SamplePositionsArray                                     = std::array<float, 32>;
constexpr std::array<SamplePositionsArray, 5> kSamplePositions = {
    {{{0.5f, 0.5f}},
     {{0.75f, 0.75f, 0.25f, 0.25f}},
     {{0.375f, 0.125f, 0.875f, 0.375f, 0.125f, 0.625f, 0.625f, 0.875f}},
     {{0.5625f, 0.3125f, 0.4375f, 0.6875f, 0.8125f, 0.5625f, 0.3125f, 0.1875f, 0.1875f, 0.8125f,
       0.0625f, 0.4375f, 0.6875f, 0.9375f, 0.9375f, 0.0625f}},
     {{0.5625f, 0.5625f, 0.4375f, 0.3125f, 0.3125f, 0.625f,  0.75f,   0.4375f,
       0.1875f, 0.375f,  0.625f,  0.8125f, 0.8125f, 0.6875f, 0.6875f, 0.1875f,
       0.375f,  0.875f,  0.5f,    0.0625f, 0.25f,   0.125f,  0.125f,  0.75f,
       0.0f,    0.5f,    0.9375f, 0.25f,   0.875f,  0.9375f, 0.0625f, 0.0f}}}};

struct IncompleteTextureParameters
{
    GLenum sizedInternalFormat;
    GLenum format;
    GLenum type;
    GLubyte clearColor[4];
};

// Note that for gl::SamplerFormat::Shadow, the clearColor datatype needs to be GLushort and as such
// we will reinterpret GLubyte[4] as GLushort[2].
constexpr angle::PackedEnumMap<gl::SamplerFormat, IncompleteTextureParameters>
    kIncompleteTextureParameters = {
        {gl::SamplerFormat::Float, {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, {0, 0, 0, 255}}},
        {gl::SamplerFormat::Unsigned,
         {GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, {0, 0, 0, 255}}},
        {gl::SamplerFormat::Signed, {GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE, {0, 0, 0, 127}}},
        {gl::SamplerFormat::Shadow,
         {GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, {0, 0, 0, 0}}}};

void CopyColor(gl::ColorF *color)
{
    // No-op
}

void PremultiplyAlpha(gl::ColorF *color)
{
    color->red *= color->alpha;
    color->green *= color->alpha;
    color->blue *= color->alpha;
}

void UnmultiplyAlpha(gl::ColorF *color)
{
    if (color->alpha != 0.0f)
    {
        float invAlpha = 1.0f / color->alpha;
        color->red *= invAlpha;
        color->green *= invAlpha;
        color->blue *= invAlpha;
    }
}

void ClipChannelsR(gl::ColorF *color)
{
    color->green = 0.0f;
    color->blue  = 0.0f;
    color->alpha = 1.0f;
}

void ClipChannelsRG(gl::ColorF *color)
{
    color->blue  = 0.0f;
    color->alpha = 1.0f;
}

void ClipChannelsRGB(gl::ColorF *color)
{
    color->alpha = 1.0f;
}

void ClipChannelsLuminance(gl::ColorF *color)
{
    color->alpha = 1.0f;
}

void ClipChannelsAlpha(gl::ColorF *color)
{
    color->red   = 0.0f;
    color->green = 0.0f;
    color->blue  = 0.0f;
}

void ClipChannelsNoOp(gl::ColorF *color) {}

void WriteUintColor(const gl::ColorF &color,
                    PixelWriteFunction colorWriteFunction,
                    uint8_t *destPixelData)
{
    gl::ColorUI destColor(
        static_cast<unsigned int>(color.red * 255), static_cast<unsigned int>(color.green * 255),
        static_cast<unsigned int>(color.blue * 255), static_cast<unsigned int>(color.alpha * 255));
    colorWriteFunction(reinterpret_cast<const uint8_t *>(&destColor), destPixelData);
}

void WriteFloatColor(const gl::ColorF &color,
                     PixelWriteFunction colorWriteFunction,
                     uint8_t *destPixelData)
{
    colorWriteFunction(reinterpret_cast<const uint8_t *>(&color), destPixelData);
}

template <int cols, int rows, bool IsColumnMajor>
constexpr inline int GetFlattenedIndex(int col, int row)
{
    if (IsColumnMajor)
    {
        return col * rows + row;
    }
    else
    {
        return row * cols + col;
    }
}

// The stride in bytes of a padded matrix column (std140, Vulkan's packed SPIR-V layout, and
// matCx3/matCx4 in std430).  A column of 16-bit floats uses the same 16-byte stride as a column of
// 32-bit floats, with padding after the last row.
constexpr int kPaddedMatrixColumnSize = 4 * sizeof(GLfloat);

// Copies one matrix of 32-bit floats from `value` to `target`.  Each component is stored as a
// `DstComponent`: `GLfloat`, or `GLhalf` to convert it to a 16-bit float.  The matrix is transposed
// if the source and destination orders differ.  `colsDst` x `rowsDst` is the size of the
// destination matrix in components, which can be larger than `colsSrc` x `rowsSrc`; that padding is
// zeroed, except that nothing is written after the last component of the matrix.
template <typename DstComponent,
          bool IsSrcColumnMajor,
          int colsSrc,
          int rowsSrc,
          bool IsDstColumnMajor,
          int colsDst,
          int rowsDst>
void CopyMatrix(uint8_t *target, const GLfloat *value)
{
    static_assert(std::is_same_v<DstComponent, GLfloat> || std::is_same_v<DstComponent, GLhalf>,
                  "Unsupported component type");
    static_assert(colsSrc <= colsDst && rowsSrc <= rowsDst, "Can only expand!");

    // Zero-initializing `staging` sets any unused padding at the end of every column (or row, when
    // `!IsDstColumnMajor`) other than the last to 0, so that shaders do not access uninitialized
    // memory.  Clamping `kDstFlatSize` to the last written component leaves the bytes after the
    // last column (or row) untouched, because other data may be packed tightly right after it.  See
    // http://anglebug.com/42266878.
    //
    // For example, for a column-major 3*3 matrix of 16-bit floats in 16-byte columns (`colsDst` =
    // 3, `rowsDst` = 8), `target` ends up with:
    //
    //   Other columns: |2-byte half float|2-byte half float|2-byte half float|10-byte of 0|
    //   Last column:   |2-byte half float|2-byte half float|2-byte half float|10-byte untouched|
    //
    // The same applies to rows when `!IsDstColumnMajor`.
    constexpr int kDstFlatSize =
        GetFlattenedIndex<colsDst, rowsDst, IsDstColumnMajor>(colsSrc - 1, rowsSrc - 1) + 1;
    DstComponent staging[kDstFlatSize] = {0};

    for (int r = 0; r < rowsSrc; r++)
    {
        for (int c = 0; c < colsSrc; c++)
        {
            int srcIndex = GetFlattenedIndex<colsSrc, rowsSrc, IsSrcColumnMajor>(c, r);
            int dstIndex = GetFlattenedIndex<colsDst, rowsDst, IsDstColumnMajor>(c, r);

            if constexpr (std::is_same_v<DstComponent, GLhalf>)
            {
                staging[dstIndex] = gl::float32ToFloat16(value[srcIndex]);
            }
            else
            {
                staging[dstIndex] = value[srcIndex];
            }
        }
    }

    memcpy(target, staging, kDstFlatSize * sizeof(DstComponent));
}

// Copies `count` consecutive matrices from `value` to `target`.  See `CopyMatrix`.
template <typename DstComponent,
          bool IsSrcColumnMajor,
          int colsSrc,
          int rowsSrc,
          bool IsDstColumnMajor,
          int colsDst,
          int rowsDst>
void CopyMatrices(unsigned int count, uint8_t *target, const GLfloat *value)
{
    constexpr size_t kDstMatrixSize = sizeof(DstComponent) * colsDst * rowsDst;
    constexpr bool kIsSameLayout    = std::is_same_v<DstComponent, GLfloat> &&
                                      IsSrcColumnMajor == IsDstColumnMajor && colsSrc == colsDst &&
                                      rowsSrc == rowsDst;

    if constexpr (kIsSameLayout)
    {
        // A single memcpy copies all the matrices.
        memcpy(target, value, kDstMatrixSize * count);
    }
    else
    {
        for (unsigned int i = 0; i < count; i++)
        {
            CopyMatrix<DstComponent, IsSrcColumnMajor, colsSrc, rowsSrc, IsDstColumnMajor, colsDst,
                       rowsDst>(target, value);

            target += kDstMatrixSize;
            value += colsSrc * rowsSrc;
        }
    }
}

// Sets up to `countIn` elements of an array of `elementCount` matrices, starting at element
// `arrayElementOffset`.  `value` holds `colsSrc` x `rowsSrc` matrices of 32-bit floats, in
// row-major order if `transpose` and column-major order otherwise.  Each element of `targetData` is
// a `colsDst` x `rowsDst` matrix of `DstComponent`.  See `CopyMatrix`.
template <typename DstComponent,
          int colsSrc,
          int rowsSrc,
          bool IsDstColumnMajor,
          int colsDst,
          int rowsDst>
void SetFloatUniformMatrix(unsigned int arrayElementOffset,
                           unsigned int elementCount,
                           GLsizei countIn,
                           GLboolean transpose,
                           const GLfloat *value,
                           uint8_t *targetData)
{
    const unsigned int count =
        std::min(elementCount - arrayElementOffset, static_cast<unsigned int>(countIn));
    if (count == 0)
    {
        // `value` can be null when there is nothing to copy.
        return;
    }

    uint8_t *target = targetData + arrayElementOffset * sizeof(DstComponent) * colsDst * rowsDst;

    if (transpose)
    {
        CopyMatrices<DstComponent, false, colsSrc, rowsSrc, IsDstColumnMajor, colsDst, rowsDst>(
            count, target, value);
    }
    else
    {
        CopyMatrices<DstComponent, true, colsSrc, rowsSrc, IsDstColumnMajor, colsDst, rowsDst>(
            count, target, value);
    }
}

// Reads a `columns` x `rows` matrix from `source` into `dataOut`, as 32-bit floats in column-major
// order.  Each column of `source` (each row, if `transpose`) is padded to
// `kPaddedMatrixColumnSize` bytes, and each component is stored as a `SrcComponent`: `GLfloat`, or
// `GLhalf` for a 16-bit float.
template <typename SrcComponent>
void ReadPaddedMatrix(int columns, int rows, const void *source, bool transpose, GLfloat *dataOut)
{
    static_assert(std::is_same_v<SrcComponent, GLfloat> || std::is_same_v<SrcComponent, GLhalf>,
                  "Unsupported component type");

    const uint8_t *sourceBytes = static_cast<const uint8_t *>(source);
    for (int col = 0; col < columns; ++col)
    {
        for (int row = 0; row < rows; ++row)
        {
            const size_t byteOffset =
                transpose ? (row * kPaddedMatrixColumnSize + col * sizeof(SrcComponent))
                          : (col * kPaddedMatrixColumnSize + row * sizeof(SrcComponent));
            SrcComponent component;
            memcpy(&component, sourceBytes + byteOffset, sizeof(SrcComponent));

            if constexpr (std::is_same_v<SrcComponent, GLhalf>)
            {
                dataOut[col * rows + row] = gl::float16ToFloat32(component);
            }
            else
            {
                dataOut[col * rows + row] = component;
            }
        }
    }
}
}  // anonymous namespace

bool IsRotatedAspectRatio(SurfaceRotation rotation)
{
    switch (rotation)
    {
        case SurfaceRotation::Rotated90Degrees:
        case SurfaceRotation::Rotated270Degrees:
        case SurfaceRotation::FlippedRotated90Degrees:
        case SurfaceRotation::FlippedRotated270Degrees:
            return true;
        default:
            return false;
    }
}

void RotateRectangle(const SurfaceRotation rotation,
                     const bool flipY,
                     const int framebufferWidth,
                     const int framebufferHeight,
                     const gl::Rectangle &incoming,
                     gl::Rectangle *outgoing)
{
    // GLES's y-axis points up; Vulkan's points down.
    switch (rotation)
    {
        case SurfaceRotation::Identity:
            // Do not rotate gl_Position (surface matches the device's orientation):
            outgoing->x     = incoming.x;
            outgoing->y     = flipY ? framebufferHeight - incoming.y - incoming.height : incoming.y;
            outgoing->width = incoming.width;
            outgoing->height = incoming.height;
            break;
        case SurfaceRotation::Rotated90Degrees:
            // Rotate gl_Position 90 degrees:
            outgoing->x      = incoming.y;
            outgoing->y      = flipY ? incoming.x : framebufferWidth - incoming.x - incoming.width;
            outgoing->width  = incoming.height;
            outgoing->height = incoming.width;
            break;
        case SurfaceRotation::Rotated180Degrees:
            // Rotate gl_Position 180 degrees:
            outgoing->x     = framebufferWidth - incoming.x - incoming.width;
            outgoing->y     = flipY ? incoming.y : framebufferHeight - incoming.y - incoming.height;
            outgoing->width = incoming.width;
            outgoing->height = incoming.height;
            break;
        case SurfaceRotation::Rotated270Degrees:
            // Rotate gl_Position 270 degrees:
            outgoing->x      = framebufferHeight - incoming.y - incoming.height;
            outgoing->y      = flipY ? framebufferWidth - incoming.x - incoming.width : incoming.x;
            outgoing->width  = incoming.height;
            outgoing->height = incoming.width;
            break;
        default:
            UNREACHABLE();
            break;
    }
}

PackPixelsParams::PackPixelsParams()
    : destFormat(nullptr),
      outputPitch(0),
      packBuffer(nullptr),
      reverseRowOrder(false),
      offset(0),
      rotation(SurfaceRotation::Identity)
{}

PackPixelsParams::PackPixelsParams(const gl::Rectangle &areaIn,
                                   const angle::Format &destFormat,
                                   GLuint outputPitchIn,
                                   bool reverseRowOrderIn,
                                   gl::Buffer *packBufferIn,
                                   ptrdiff_t offsetIn)
    : area(areaIn),
      destFormat(&destFormat),
      outputPitch(outputPitchIn),
      packBuffer(packBufferIn),
      reverseRowOrder(reverseRowOrderIn),
      offset(offsetIn),
      rotation(SurfaceRotation::Identity)
{}

void PackPixels(const PackPixelsParams &params,
                const angle::Format &sourceFormat,
                int inputPitchIn,
                const uint8_t *sourceIn,
                uint8_t *destWithoutOffset)
{
    uint8_t *destWithOffset = destWithoutOffset + params.offset;

    const uint8_t *source = sourceIn;
    int inputPitch        = inputPitchIn;
    int destWidth         = params.area.width;
    int destHeight        = params.area.height;
    int xAxisPitch        = 0;
    int yAxisPitch        = 0;
    switch (params.rotation)
    {
        case SurfaceRotation::Identity:
            // The source image is not rotated (i.e. matches the device's orientation), and may or
            // may not be y-flipped.  The image is row-major.  Each source row (one step along the
            // y-axis for each step in the dest y-axis) is inputPitch past the previous row.  Along
            // a row, each source pixel (one step along the x-axis for each step in the dest
            // x-axis) is sourceFormat.pixelBytes past the previous pixel.
            xAxisPitch = sourceFormat.pixelBytes;
            if (params.reverseRowOrder)
            {
                // The source image is y-flipped, which means we start at the last row, and each
                // source row is BEFORE the previous row.
                source += inputPitchIn * (params.area.height - 1);
                inputPitch = -inputPitch;
                yAxisPitch = -inputPitchIn;
            }
            else
            {
                yAxisPitch = inputPitchIn;
            }
            break;
        case SurfaceRotation::Rotated90Degrees:
            // The source image is rotated 90 degrees counter-clockwise.  Y-flip is always applied
            // to rotated images.  The image is column-major.  Each source column (one step along
            // the source x-axis for each step in the dest y-axis) is inputPitch past the previous
            // column.  Along a column, each source pixel (one step along the y-axis for each step
            // in the dest x-axis) is sourceFormat.pixelBytes past the previous pixel.
            xAxisPitch = inputPitchIn;
            yAxisPitch = sourceFormat.pixelBytes;
            destWidth  = params.area.height;
            destHeight = params.area.width;
            break;
        case SurfaceRotation::Rotated180Degrees:
            // The source image is rotated 180 degrees.  Y-flip is always applied to rotated
            // images.  The image is row-major, but upside down.  Each source row (one step along
            // the y-axis for each step in the dest y-axis) is inputPitch after the previous row.
            // Along a row, each source pixel (one step along the x-axis for each step in the dest
            // x-axis) is sourceFormat.pixelBytes BEFORE the previous pixel.
            xAxisPitch = -static_cast<int>(sourceFormat.pixelBytes);
            yAxisPitch = inputPitchIn;
            source += sourceFormat.pixelBytes * (params.area.width - 1);
            break;
        case SurfaceRotation::Rotated270Degrees:
            // The source image is rotated 270 degrees counter-clockwise (or 90 degrees clockwise).
            // Y-flip is always applied to rotated images.  The image is column-major, where each
            // column (one step in the source x-axis for one step in the dest y-axis) is inputPitch
            // BEFORE the previous column.  Along a column, each source pixel (one step along the
            // y-axis for each step in the dest x-axis) is sourceFormat.pixelBytes BEFORE the
            // previous pixel.  The first pixel is at the end of the source.
            xAxisPitch = -inputPitchIn;
            yAxisPitch = -static_cast<int>(sourceFormat.pixelBytes);
            destWidth  = params.area.height;
            destHeight = params.area.width;
            source += inputPitch * (params.area.height - 1) +
                      sourceFormat.pixelBytes * (params.area.width - 1);
            break;
        default:
            UNREACHABLE();
            break;
    }

    if (params.rotation == SurfaceRotation::Identity && sourceFormat == *params.destFormat)
    {
        // Direct copy possible
        for (int y = 0; y < params.area.height; ++y)
        {
            memcpy(destWithOffset + y * params.outputPitch, source + y * inputPitch,
                   params.area.width * sourceFormat.pixelBytes);
        }
        return;
    }

    FastCopyFunction fastCopyFunc = sourceFormat.fastCopyFunctions.get(params.destFormat->id);

    if (fastCopyFunc)
    {
        // Fast copy is possible through some special function
        fastCopyFunc(source, xAxisPitch, yAxisPitch, destWithOffset, params.destFormat->pixelBytes,
                     params.outputPitch, destWidth, destHeight);
        return;
    }

    PixelWriteFunction pixelWriteFunction = params.destFormat->pixelWriteFunction;
    ASSERT(pixelWriteFunction != nullptr);

    // Maximum size of any Color<T> type used.
    uint8_t temp[16];
    static_assert(sizeof(temp) >= sizeof(gl::ColorF) && sizeof(temp) >= sizeof(gl::ColorUI) &&
                      sizeof(temp) >= sizeof(gl::ColorI) &&
                      sizeof(temp) >= sizeof(angle::DepthStencil),
                  "Unexpected size of pixel struct.");

    PixelReadFunction pixelReadFunction = sourceFormat.pixelReadFunction;
    ASSERT(pixelReadFunction != nullptr);

    for (int y = 0; y < destHeight; ++y)
    {
        for (int x = 0; x < destWidth; ++x)
        {
            uint8_t *dest =
                destWithOffset + y * params.outputPitch + x * params.destFormat->pixelBytes;
            const uint8_t *src = source + y * yAxisPitch + x * xAxisPitch;

            // readFunc and writeFunc will be using the same type of color, CopyTexImage
            // will not allow the copy otherwise.
            pixelReadFunction(src, temp);
            pixelWriteFunction(temp, dest);
        }
    }
}

angle::Result GetPackPixelsParams(const gl::InternalFormat &sizedFormatInfo,
                                  GLuint outputPitch,
                                  const gl::PixelPackState &packState,
                                  gl::Buffer *packBuffer,
                                  const gl::Rectangle &area,
                                  const gl::Rectangle &clippedArea,
                                  rx::PackPixelsParams *paramsOut,
                                  GLuint *skipBytesOut)
{
    angle::CheckedNumeric<GLuint> checkedSkipBytes = *skipBytesOut;
    checkedSkipBytes += (clippedArea.x - area.x) * sizedFormatInfo.pixelBytes +
                        (clippedArea.y - area.y) * outputPitch;
    if (!checkedSkipBytes.AssignIfValid(skipBytesOut))
    {
        return angle::Result::Stop;
    }

    angle::FormatID angleFormatID =
        angle::Format::InternalFormatToID(sizedFormatInfo.sizedInternalFormat);
    const angle::Format &angleFormat = angle::Format::Get(angleFormatID);

    *paramsOut = rx::PackPixelsParams(clippedArea, angleFormat, outputPitch,
                                      packState.reverseRowOrder, packBuffer, 0);
    return angle::Result::Continue;
}

bool FastCopyFunctionMap::has(angle::FormatID formatID) const
{
    return (get(formatID) != nullptr);
}

namespace
{

const FastCopyFunctionMap::Entry *getEntry(const FastCopyFunctionMap::Entry *entry,
                                           size_t numEntries,
                                           angle::FormatID formatID)
{
    const FastCopyFunctionMap::Entry *end = entry + numEntries;
    while (entry != end)
    {
        if (entry->formatID == formatID)
        {
            return entry;
        }
        ++entry;
    }

    return nullptr;
}

}  // namespace

FastCopyFunction FastCopyFunctionMap::get(angle::FormatID formatID) const
{
    const FastCopyFunctionMap::Entry *entry = getEntry(mData, mSize, formatID);
    return entry ? entry->func : nullptr;
}

bool ShouldUseDebugLayers(const egl::AttributeMap &attribs)
{
    EGLAttrib debugSetting =
        attribs.get(EGL_PLATFORM_ANGLE_DEBUG_LAYERS_ENABLED_ANGLE, EGL_DONT_CARE);

    // Prefer to enable debug layers when available.
#if defined(ANGLE_ENABLE_ASSERTS)
    return (debugSetting != EGL_FALSE);
#else
    return (debugSetting == EGL_TRUE);
#endif  // defined(ANGLE_ENABLE_ASSERTS)
}

void CopyImageCHROMIUM(const uint8_t *sourceData,
                       size_t sourceRowPitch,
                       size_t sourcePixelBytes,
                       size_t sourceDepthPitch,
                       PixelReadFunction pixelReadFunction,
                       uint8_t *destData,
                       size_t destRowPitch,
                       size_t destPixelBytes,
                       size_t destDepthPitch,
                       PixelWriteFunction pixelWriteFunction,
                       GLenum destUnsizedFormat,
                       GLenum destComponentType,
                       size_t width,
                       size_t height,
                       size_t depth,
                       bool unpackFlipY,
                       bool unpackPremultiplyAlpha,
                       bool unpackUnmultiplyAlpha)
{
    using ConversionFunction              = void (*)(gl::ColorF *);
    ConversionFunction conversionFunction = CopyColor;
    if (unpackPremultiplyAlpha != unpackUnmultiplyAlpha)
    {
        if (unpackPremultiplyAlpha)
        {
            conversionFunction = PremultiplyAlpha;
        }
        else
        {
            conversionFunction = UnmultiplyAlpha;
        }
    }

    auto clipChannelsFunction = ClipChannelsNoOp;
    switch (destUnsizedFormat)
    {
        case GL_RED:
            clipChannelsFunction = ClipChannelsR;
            break;
        case GL_RG:
            clipChannelsFunction = ClipChannelsRG;
            break;
        case GL_RGB:
            clipChannelsFunction = ClipChannelsRGB;
            break;
        case GL_LUMINANCE:
            clipChannelsFunction = ClipChannelsLuminance;
            break;
        case GL_ALPHA:
            clipChannelsFunction = ClipChannelsAlpha;
            break;
    }

    auto writeFunction = (destComponentType == GL_UNSIGNED_INT) ? WriteUintColor : WriteFloatColor;

    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            for (size_t x = 0; x < width; x++)
            {
                const uint8_t *sourcePixelData =
                    sourceData + y * sourceRowPitch + x * sourcePixelBytes + z * sourceDepthPitch;

                gl::ColorF sourceColor;
                pixelReadFunction(sourcePixelData, reinterpret_cast<uint8_t *>(&sourceColor));

                conversionFunction(&sourceColor);
                clipChannelsFunction(&sourceColor);

                size_t destY = 0;
                if (unpackFlipY)
                {
                    destY += (height - 1);
                    destY -= y;
                }
                else
                {
                    destY += y;
                }

                uint8_t *destPixelData =
                    destData + destY * destRowPitch + x * destPixelBytes + z * destDepthPitch;
                writeFunction(sourceColor, pixelWriteFunction, destPixelData);
            }
        }
    }
}

// IncompleteTextureSet implementation.
void IncompleteTextureSet::onDestroy(const gl::Context *context)
{
    // Clear incomplete textures.
    for (auto &incompleteTextures : mIncompleteTextures)
    {
        for (auto &incompleteTexture : incompleteTextures)
        {
            if (incompleteTexture.get() != nullptr)
            {
                incompleteTexture->onDestroy(context);
                incompleteTexture.set(context, nullptr);
            }
        }
    }
}

angle::Result IncompleteTextureSet::getIncompleteTexture(
    const gl::Context *context,
    gl::TextureType type,
    gl::SamplerFormat format,
    MultisampleTextureInitializer *multisampleInitializer,
    gl::Texture **textureOut)
{
    *textureOut = mIncompleteTextures[format][type].get();
    if (*textureOut != nullptr)
    {
        return angle::Result::Continue;
    }

    ContextImpl *implFactory = context->getImplementation();

    gl::Extents colorSize(1, 1, 1);
    gl::PixelUnpackState unpack;
    unpack.alignment = 1;
    gl::Box area(0, 0, 0, 1, 1, 1);
    const IncompleteTextureParameters &incompleteTextureParam =
        kIncompleteTextureParameters[format];

    // Cube map arrays are expected to have layer counts that are multiples of 6
    constexpr int kCubeMapArraySize = 6;
    if (type == gl::TextureType::CubeMapArray)
    {
        // From the GLES 3.2 spec:
        //   8.18. IMMUTABLE-FORMAT TEXTURE IMAGES
        //   TexStorage3D Errors
        //   An INVALID_OPERATION error is generated if any of the following conditions hold:
        //     * target is TEXTURE_CUBE_MAP_ARRAY and depth is not a multiple of 6
        // Since ANGLE treats incomplete textures as immutable, respect that here.
        colorSize.depth = kCubeMapArraySize;
        area.depth      = kCubeMapArraySize;
    }

    // If a texture is external use a 2D texture for the incomplete texture
    gl::TextureType createType = (type == gl::TextureType::External) ? gl::TextureType::_2D : type;

    gl::Texture *tex =
        new gl::Texture(implFactory, {std::numeric_limits<GLuint>::max()}, createType);
    angle::UniqueObjectPointer<gl::Texture, gl::Context> t(tex, context);
    gl::Buffer *incompleteTextureBufferAttachment = nullptr;

    // This is a bit of a kludge but is necessary to consume the error.
    gl::Context *mutableContext = const_cast<gl::Context *>(context);

    if (createType == gl::TextureType::Buffer)
    {
        constexpr uint32_t kBufferInitData = 0;
        incompleteTextureBufferAttachment =
            new gl::Buffer(implFactory, {std::numeric_limits<GLuint>::max()});
        ANGLE_TRY(incompleteTextureBufferAttachment->bufferData(
            mutableContext, gl::BufferBinding::Texture, &kBufferInitData, sizeof(kBufferInitData),
            gl::BufferUsage::StaticDraw));
    }
    else if (createType == gl::TextureType::_2DMultisample ||
             createType == gl::TextureType::_2DMultisampleArray)
    {
        ANGLE_TRY(t->setStorageMultisample(mutableContext, createType, 1,
                                           incompleteTextureParam.sizedInternalFormat, colorSize,
                                           true));
    }
    else
    {
        ANGLE_TRY(t->setStorage(mutableContext, createType, 1,
                                incompleteTextureParam.sizedInternalFormat, colorSize));
    }
    t->markInternalIncompleteTexture();

    if (type == gl::TextureType::CubeMap)
    {
        for (gl::TextureTarget face : gl::AllCubeFaceTextureTargets())
        {
            ANGLE_TRY(t->setSubImage(mutableContext, unpack, nullptr, face, 0, area,
                                     incompleteTextureParam.format, incompleteTextureParam.type,
                                     incompleteTextureParam.clearColor));
        }
    }
    else if (type == gl::TextureType::CubeMapArray)
    {
        // We need to provide enough pixel data to fill the array of six faces
        GLubyte incompleteCubeArrayPixels[kCubeMapArraySize][4];
        for (GLubyte *incompleteCubeArrayPixel : incompleteCubeArrayPixels)
        {
            incompleteCubeArrayPixel[0] = incompleteTextureParam.clearColor[0];
            incompleteCubeArrayPixel[1] = incompleteTextureParam.clearColor[1];
            incompleteCubeArrayPixel[2] = incompleteTextureParam.clearColor[2];
            incompleteCubeArrayPixel[3] = incompleteTextureParam.clearColor[3];
        }

        ANGLE_TRY(t->setSubImage(mutableContext, unpack, nullptr,
                                 gl::NonCubeTextureTypeToTarget(createType), 0, area,
                                 incompleteTextureParam.format, incompleteTextureParam.type,
                                 *incompleteCubeArrayPixels));
    }
    else if (type == gl::TextureType::_2DMultisample ||
             type == gl::TextureType::_2DMultisampleArray)
    {
        // Call a specialized clear function to init a multisample texture.
        ANGLE_TRY(multisampleInitializer->initializeMultisampleTextureToBlack(context, t.get()));
        // The above initialization is invisible to the front-end
        t->setInitState(gl::InitState::Initialized);
    }
    else if (type == gl::TextureType::Buffer)
    {
        ASSERT(incompleteTextureBufferAttachment != nullptr);
        ANGLE_TRY(t->setBuffer(mutableContext, incompleteTextureBufferAttachment,
                               incompleteTextureParam.sizedInternalFormat));
    }
    else
    {
        ANGLE_TRY(t->setSubImage(mutableContext, unpack, nullptr,
                                 gl::NonCubeTextureTypeToTarget(createType), 0, area,
                                 incompleteTextureParam.format, incompleteTextureParam.type,
                                 incompleteTextureParam.clearColor));
    }

    if (format == gl::SamplerFormat::Shadow)
    {
        // To avoid the undefined spec behavior for shadow samplers with a depth texture, we set the
        // compare mode to GL_COMPARE_REF_TO_TEXTURE
        ASSERT(!t->hasObservers());
        t->setCompareMode(context, GL_COMPARE_REF_TO_TEXTURE);
    }

    ANGLE_TRY(t->syncState(context, gl::Command::Other));

    mIncompleteTextures[format][type].set(context, t.release());
    *textureOut = mIncompleteTextures[format][type].get();
    return angle::Result::Continue;
}

#define ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC(api, cols, rows) \
    template void SetFloatUniformMatrix##api<cols, rows>::Run(     \
        unsigned int, unsigned int, GLsizei, GLboolean, const GLfloat *, uint8_t *)

ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC(HLSL, 2, 2);
ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC(HLSL, 3, 3);
ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC(HLSL, 2, 3);
ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC(HLSL, 3, 2);
ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC(HLSL, 2, 4);
ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC(HLSL, 3, 4);
ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC(HLSL, 4, 2);
ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC(HLSL, 4, 3);
ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC(HLSL, 4, 4);

#undef ANGLE_INSTANTIATE_SET_UNIFORM_MATRIX_FUNC

template <int cols, int rows>
void SetFloatUniformMatrixHLSL<cols, rows>::Run(unsigned int arrayElementOffset,
                                                unsigned int elementCount,
                                                GLsizei countIn,
                                                GLboolean transpose,
                                                const GLfloat *value,
                                                uint8_t *targetData)
{
    // Internally store matrices as row-major to accomodate HLSL matrix indexing.  Each row is
    // padded to 4 columns.
    SetFloatUniformMatrix<GLfloat, cols, rows, false, 4, rows>(
        arrayElementOffset, elementCount, countIn, transpose, value, targetData);
}

template void GetMatrixUniform<GLint>(GLenum, GLint *, const GLint *, bool, bool);
template void GetMatrixUniform<GLuint>(GLenum, GLuint *, const GLuint *, bool, bool);

void GetMatrixUniform(GLenum type,
                      GLfloat *dataOut,
                      const GLfloat *source,
                      bool transpose,
                      bool isFloat16)
{
    const int columns = gl::VariableColumnCount(type);
    const int rows    = gl::VariableRowCount(type);

    if (isFloat16)
    {
        ASSERT(!transpose);
        ReadPaddedMatrix<GLhalf>(columns, rows, source, transpose, dataOut);
    }
    else
    {
        ReadPaddedMatrix<GLfloat>(columns, rows, source, transpose, dataOut);
    }
}

template <typename NonFloatT>
void GetMatrixUniform(GLenum type,
                      NonFloatT *dataOut,
                      const NonFloatT *source,
                      bool transpose,
                      bool isFloat16)
{
    UNREACHABLE();
}

BufferAndLayout::BufferAndLayout() = default;

BufferAndLayout::~BufferAndLayout() = default;

template <typename T>
ANGLE_NOINLINE void UpdateBufferWithLayoutStrided(GLsizei count,
                                                  uint32_t arrayIndex,
                                                  int componentCount,
                                                  const T *v,
                                                  const sh::BlockMemberInfo &layoutInfo,
                                                  angle::MemoryBuffer *uniformData)
{
    const int elementSize = sizeof(T) * componentCount;
    uint8_t *dst          = uniformData->data() + layoutInfo.offset;
    int maxIndex          = arrayIndex + count;
    for (int writeIndex = arrayIndex, readIndex = 0; writeIndex < maxIndex;
         writeIndex++, readIndex++)
    {
        const int arrayOffset = writeIndex * layoutInfo.arrayStride;
        uint8_t *writePtr     = dst + arrayOffset;
        const T *readPtr      = v + (readIndex * componentCount);
        ASSERT(writePtr + elementSize <= uniformData->data() + uniformData->size());
        memcpy(writePtr, readPtr, elementSize);
    }
}

template <typename T>
ANGLE_INLINE void UpdateBufferWithLayout(GLsizei count,
                                         uint32_t arrayIndex,
                                         int componentCount,
                                         const T *v,
                                         const sh::BlockMemberInfo &layoutInfo,
                                         angle::MemoryBuffer *uniformData)
{
    const int elementSize = sizeof(T) * componentCount;
    uint8_t *dst = uniformData->data() + layoutInfo.offset;
    if (ANGLE_LIKELY(layoutInfo.arrayStride == 0) ||
        ANGLE_LIKELY(layoutInfo.arrayStride == elementSize))
    {
        uint32_t arrayOffset = arrayIndex * layoutInfo.arrayStride;
        uint8_t *writePtr    = dst + arrayOffset;
        ASSERT(writePtr + (elementSize * count) <= uniformData->data() + uniformData->size());
        memcpy(writePtr, v, elementSize * count);
    }
    else
    {
        // Have to respect the arrayStride between each element of the array.
        UpdateBufferWithLayoutStrided(count, arrayIndex, componentCount, v, layoutInfo,
                                      uniformData);
    }
}

template <typename T>
void ReadFromBufferWithLayout(int componentCount,
                              uint32_t arrayIndex,
                              T *dst,
                              const sh::BlockMemberInfo &layoutInfo,
                              const angle::MemoryBuffer *uniformData,
                              bool isFloat16)
{
    ASSERT(layoutInfo.offset != -1);

    const int elementSize = sizeof(T) * componentCount;
    const uint8_t *source  = uniformData->data() + layoutInfo.offset;
    const uint8_t *readPtr = source + arrayIndex * layoutInfo.arrayStride;
    // Special case when the expected data read back is GLfloat, it is possible we need to
    // transform the data in memory from GLshort to GLfloat.
    if constexpr (std::is_same<T, GLfloat>::value)
    {
        if (isFloat16)
        {
            // check that dst is aligned to 4 bytes (size of GLfloat)
            ASSERT(reinterpret_cast<uintptr_t>(dst) % 4 == 0);
            // check that readPtr is aligned to 2 bytes (size of GLshort)
            ASSERT(reinterpret_cast<uintptr_t>(readPtr) % 2 == 0);
            const GLshort *transformedValues = reinterpret_cast<const GLshort *>(readPtr);
            for (size_t index = 0; index < static_cast<size_t>(componentCount); ++index)
            {
                dst[index] = gl::float16ToFloat32(transformedValues[index]);
            }
            // skip the generic case below
            return;
        }
    }
    // Generic case where no data transform is needed
    memcpy(dst, readPtr, elementSize);
}

template <typename T>
ANGLE_NOINLINE void SetUniformAsBool(const gl::ProgramExecutable *executable,
                                     GLint location,
                                     GLsizei count,
                                     const T *v,
                                     GLenum entryPointType,
                                     DefaultUniformBlockMap *defaultUniformBlocks,
                                     gl::ShaderBitSet *defaultUniformBlocksDirty)
{
    const gl::VariableLocation &locationInfo = executable->getUniformLocations()[location];
    const gl::LinkedUniform &linkedUniform   = executable->getUniforms()[locationInfo.index];

    for (const gl::ShaderType shaderType : executable->getLinkedShaderStages())
    {
        BufferAndLayout &uniformBlock         = *(*defaultUniformBlocks)[shaderType];
        const sh::BlockMemberInfo &layoutInfo = uniformBlock.uniformLayout[location];

        // Assume an offset of -1 means the block is unused.
        if (layoutInfo.offset == -1)
        {
            continue;
        }

        const GLint componentCount = linkedUniform.getElementComponents();

        ASSERT(linkedUniform.getType() == gl::VariableBoolVectorType(entryPointType));

        GLint initialArrayOffset =
            locationInfo.arrayIndex * layoutInfo.arrayStride + layoutInfo.offset;
        for (GLint i = 0; i < count; i++)
        {
            GLint elementOffset = i * layoutInfo.arrayStride + initialArrayOffset;
            GLint *dst = reinterpret_cast<GLint *>(uniformBlock.uniformData.data() + elementOffset);
            const T *source = v + i * componentCount;

            for (int c = 0; c < componentCount; c++)
            {
                dst[c] = (source[c] == static_cast<T>(0)) ? GL_FALSE : GL_TRUE;
            }
        }

        defaultUniformBlocksDirty->set(shaderType);
    }
}

template <typename T>
void SetUniform(const gl::ProgramExecutable *executable,
                GLint location,
                GLsizei count,
                const T *v,
                GLenum entryPointType,
                DefaultUniformBlockMap *defaultUniformBlocks,
                gl::ShaderBitSet *defaultUniformBlocksDirty)
{
    const gl::VariableLocation &locationInfo = executable->getUniformLocations()[location];
    const gl::LinkedUniform &linkedUniform   = executable->getUniforms()[locationInfo.index];

    ASSERT(!linkedUniform.isSampler());

    if (ANGLE_LIKELY(linkedUniform.getType() == entryPointType))
    {
        const GLint componentCount = linkedUniform.getElementComponents();
        if constexpr (std::is_same<T, GLfloat>::value)
        {
            if (linkedUniform.isFloat16())
            {
                // Special case where we need to transform the 32-bit float to 16-bit float before
                // storing.
                for (const gl::ShaderType shaderType : executable->getLinkedShaderStages())
                {
                    BufferAndLayout &uniformBlock         = *(*defaultUniformBlocks)[shaderType];
                    const sh::BlockMemberInfo &layoutInfo = uniformBlock.uniformLayout[location];

                    // Assume an offset of -1 means the block is unused.
                    if (layoutInfo.offset == -1)
                    {
                        continue;
                    }

                    const int elementSize = sizeof(GLshort) * componentCount;
                    uint8_t *dst          = uniformBlock.uniformData.data() + layoutInfo.offset;
                    int maxIndex          = locationInfo.arrayIndex + count;
                    for (int writeIndex = locationInfo.arrayIndex, readIndex = 0;
                         writeIndex < maxIndex; writeIndex++, readIndex++)
                    {
                        const int arrayOffset = writeIndex * layoutInfo.arrayStride;
                        uint8_t *writePtr     = dst + arrayOffset;
                        // check that writePtr is aligned to 2 bytes (size of GLshort)
                        ASSERT(reinterpret_cast<uintptr_t>(writePtr) % 2 == 0);
                        const GLfloat *readPtr = v + (readIndex * componentCount);
                        // check that readPtr is aligned to 4 bytes (size of GLfloat)
                        ASSERT(reinterpret_cast<uintptr_t>(readPtr) % 4 == 0);
                        // Ensure the uniformBlock.uniformData has enough space
                        ASSERT(writePtr + elementSize <=
                               uniformBlock.uniformData.data() + uniformBlock.uniformData.size());
                        // Transform each original GLfloat data to GLshort
                        GLshort *dstGLShortPtr = reinterpret_cast<GLshort *>(writePtr);
                        for (int componentIndex = 0; componentIndex < componentCount;
                             ++componentIndex)
                        {
                            dstGLShortPtr[componentIndex] =
                                gl::float32ToFloat16(readPtr[componentIndex]);
                        }
                        // Add paddings of 0 if the next item written to the destination memory is
                        // not tightly packed to the current item
                        if (writeIndex + 1 < maxIndex)
                        {
                            const int paddingSize = (writeIndex + 1) * layoutInfo.arrayStride -
                                                    arrayOffset - elementSize;
                            if (paddingSize > 0)
                            {
                                memset(writePtr + elementSize, 0, paddingSize);
                            }
                        }
                    }
                    defaultUniformBlocksDirty->set(shaderType);
                }
                // Skip the generic case below
                return;
            }
        }
        // Generic case where data transformation is not required
        for (const gl::ShaderType shaderType : executable->getLinkedShaderStages())
        {
            BufferAndLayout &uniformBlock         = *(*defaultUniformBlocks)[shaderType];
            const sh::BlockMemberInfo &layoutInfo = uniformBlock.uniformLayout[location];

            // Assume an offset of -1 means the block is unused.
            if (layoutInfo.offset == -1)
            {
                continue;
            }

            UpdateBufferWithLayout(count, locationInfo.arrayIndex, componentCount, v, layoutInfo,
                                   &uniformBlock.uniformData);
            defaultUniformBlocksDirty->set(shaderType);
        }
    }
    else
    {
        SetUniformAsBool(executable, location, count, v, entryPointType, defaultUniformBlocks,
                         defaultUniformBlocksDirty);
    }
}
template void SetUniform<GLint>(const gl::ProgramExecutable *executable,
                                GLint location,
                                GLsizei count,
                                const GLint *v,
                                GLenum entryPointType,
                                DefaultUniformBlockMap *defaultUniformBlocks,
                                gl::ShaderBitSet *defaultUniformBlocksDirty);
template void SetUniform<GLuint>(const gl::ProgramExecutable *executable,
                                 GLint location,
                                 GLsizei count,
                                 const GLuint *v,
                                 GLenum entryPointType,
                                 DefaultUniformBlockMap *defaultUniformBlocks,
                                 gl::ShaderBitSet *defaultUniformBlocksDirty);
template void SetUniform<GLfloat>(const gl::ProgramExecutable *executable,
                                  GLint location,
                                  GLsizei count,
                                  const GLfloat *v,
                                  GLenum entryPointType,
                                  DefaultUniformBlockMap *defaultUniformBlocks,
                                  gl::ShaderBitSet *defaultUniformBlocksDirty);

template <int cols, int rows>
void SetUniformMatrixfv(const gl::ProgramExecutable *executable,
                        GLint location,
                        GLsizei count,
                        GLboolean transpose,
                        const GLfloat *value,
                        DefaultUniformBlockMap *defaultUniformBlocks,
                        gl::ShaderBitSet *defaultUniformBlocksDirty)
{
    const gl::VariableLocation &locationInfo = executable->getUniformLocations()[location];
    const gl::LinkedUniform &linkedUniform   = executable->getUniforms()[locationInfo.index];

    for (const gl::ShaderType shaderType : executable->getLinkedShaderStages())
    {
        BufferAndLayout &uniformBlock         = *(*defaultUniformBlocks)[shaderType];
        const sh::BlockMemberInfo &layoutInfo = uniformBlock.uniformLayout[location];

        // Assume an offset of -1 means the block is unused.
        if (layoutInfo.offset == -1)
        {
            continue;
        }

        uint8_t *targetData = uniformBlock.uniformData.data() + layoutInfo.offset;
        if (linkedUniform.isFloat16())
        {
            // Each component is converted to a 16-bit float, but each column is still padded to
            // `kPaddedMatrixColumnSize` bytes.  So unlike 32-bit floats, matrices of 16-bit floats
            // can't be copied with a single memcpy even when `transpose` is false.
            //
            // For example, a column-major 4*4 matrix is laid out as follows, drawn one column per
            // line with addresses increasing left to right, then top to bottom.  The source is 64
            // bytes:
            //
            //   Column 0: |4-byte float|4-byte float|4-byte float|4-byte float|
            //   Column 1: |4-byte float|4-byte float|4-byte float|4-byte float|
            //   Column 2: |4-byte float|4-byte float|4-byte float|4-byte float|
            //   Column 3: |4-byte float|4-byte float|4-byte float|4-byte float|
            //
            // The destination is also 64 bytes, but each column is 8 bytes of 16-bit floats
            // followed by 8 bytes of padding.  The padding after the last column is left untouched
            // (see `CopyMatrix`):
            //
            //   Column 0: |2-byte half|2-byte half|2-byte half|2-byte half|8 bytes of 0     |
            //   Column 1: |2-byte half|2-byte half|2-byte half|2-byte half|8 bytes of 0     |
            //   Column 2: |2-byte half|2-byte half|2-byte half|2-byte half|8 bytes of 0     |
            //   Column 3: |2-byte half|2-byte half|2-byte half|2-byte half|8 bytes untouched|
            ASSERT(layoutInfo.matrixStride == kPaddedMatrixColumnSize);
            SetFloatUniformMatrix<GLhalf, cols, rows, true, cols,
                                  kPaddedMatrixColumnSize / sizeof(GLhalf)>(
                locationInfo.arrayIndex, linkedUniform.getBasicTypeElementCount(), count, transpose,
                value, targetData);
        }
        else if (rows == 2 && layoutInfo.matrixStride == 2 * sizeof(GLfloat))
        {
            // The columns of the matrix are tightly packed (std430).
            SetFloatUniformMatrix<GLfloat, cols, rows, true, cols, rows>(
                locationInfo.arrayIndex, linkedUniform.getBasicTypeElementCount(), count, transpose,
                value, targetData);
        }
        else
        {
            // Each column of the matrix is padded to 4 rows.
            ASSERT(layoutInfo.matrixStride == kPaddedMatrixColumnSize);
            SetFloatUniformMatrix<GLfloat, cols, rows, true, cols,
                                  kPaddedMatrixColumnSize / sizeof(GLfloat)>(
                locationInfo.arrayIndex, linkedUniform.getBasicTypeElementCount(), count, transpose,
                value, targetData);
        }

        defaultUniformBlocksDirty->set(shaderType);
    }
}

#define ANGLE_SET_UNIFORM_MATRIX_FV_SPECIALIZATION(cols, rows)                                   \
    template void SetUniformMatrixfv<cols, rows>(                                                \
        const gl::ProgramExecutable *executable, GLint location, GLsizei count,                  \
        GLboolean transpose, const GLfloat *value, DefaultUniformBlockMap *defaultUniformBlocks, \
        gl::ShaderBitSet *defaultUniformBlocksDirty)
ANGLE_SET_UNIFORM_MATRIX_FV_SPECIALIZATION(2, 2);
ANGLE_SET_UNIFORM_MATRIX_FV_SPECIALIZATION(2, 3);
ANGLE_SET_UNIFORM_MATRIX_FV_SPECIALIZATION(2, 4);
ANGLE_SET_UNIFORM_MATRIX_FV_SPECIALIZATION(3, 2);
ANGLE_SET_UNIFORM_MATRIX_FV_SPECIALIZATION(3, 3);
ANGLE_SET_UNIFORM_MATRIX_FV_SPECIALIZATION(3, 4);
ANGLE_SET_UNIFORM_MATRIX_FV_SPECIALIZATION(4, 2);
ANGLE_SET_UNIFORM_MATRIX_FV_SPECIALIZATION(4, 3);
ANGLE_SET_UNIFORM_MATRIX_FV_SPECIALIZATION(4, 4);

template <typename T>
void GetUniform(const gl::ProgramExecutable *executable,
                GLint location,
                T *v,
                GLenum entryPointType,
                const DefaultUniformBlockMap *defaultUniformBlocks)
{
    const gl::VariableLocation &locationInfo = executable->getUniformLocations()[location];
    const gl::LinkedUniform &linkedUniform   = executable->getUniforms()[locationInfo.index];

    ASSERT(!linkedUniform.isSampler() && !linkedUniform.isImage());

    const gl::ShaderType shaderType = linkedUniform.getFirstActiveShaderType();
    ASSERT(shaderType != gl::ShaderType::InvalidEnum);

    const BufferAndLayout &uniformBlock   = *(*defaultUniformBlocks)[shaderType];
    const sh::BlockMemberInfo &layoutInfo = uniformBlock.uniformLayout[location];

    ASSERT(linkedUniform.getUniformTypeInfo().componentType == entryPointType ||
           linkedUniform.getUniformTypeInfo().componentType ==
               gl::VariableBoolVectorType(entryPointType));

    // Padded matrices need GetMatrixUniform; tightly packed matCx2 (std430) is copied out directly
    // below.
    if (gl::IsMatrixType(linkedUniform.getType()) && layoutInfo.matrixStride != 2 * sizeof(GLfloat))
    {
        const uint8_t *ptrToElement = uniformBlock.uniformData.data() + layoutInfo.offset +
                                      (locationInfo.arrayIndex * layoutInfo.arrayStride);
        GetMatrixUniform(linkedUniform.getType(), v, reinterpret_cast<const T *>(ptrToElement),
                         false, linkedUniform.isFloat16());
    }
    else
    {
        // Matrices of 16-bit floats are always padded.
        ASSERT(!gl::IsMatrixType(linkedUniform.getType()) || !linkedUniform.isFloat16());
        ReadFromBufferWithLayout(linkedUniform.getElementComponents(), locationInfo.arrayIndex, v,
                                 layoutInfo, &uniformBlock.uniformData, linkedUniform.isFloat16());
    }
}

template void GetUniform<GLint>(const gl::ProgramExecutable *executable,
                                GLint location,
                                GLint *v,
                                GLenum entryPointType,
                                const DefaultUniformBlockMap *defaultUniformBlocks);
template void GetUniform<GLuint>(const gl::ProgramExecutable *executable,
                                 GLint location,
                                 GLuint *v,
                                 GLenum entryPointType,
                                 const DefaultUniformBlockMap *defaultUniformBlocks);
template void GetUniform<GLfloat>(const gl::ProgramExecutable *executable,
                                  GLint location,
                                  GLfloat *v,
                                  GLenum entryPointType,
                                  const DefaultUniformBlockMap *defaultUniformBlocks);

std::string RemoveArraySubscripts(const std::string &uniformName)
{
    std::string strippedName = uniformName;

    auto out = strippedName.begin();
    for (auto in = strippedName.begin(); in != strippedName.end(); in++)
    {
        if (*in == '[')
        {
            while (*in != ']')
            {
                in++;
                ASSERT(in != strippedName.end());
            }
        }
        else
        {
            *out++ = *in;
        }
    }

    strippedName.erase(out, strippedName.end());
    return strippedName;
}

std::string GetExtractedStructSamplerName(
    const std::string uniformNameWithoutIndices,
    angle::HashMap<std::string, size_t> *extractedSamplerIndices)
{
    ASSERT(uniformNameWithoutIndices.find('.') != std::string::npos);
    ASSERT(uniformNameWithoutIndices.find('[') == std::string::npos);

    // The first time this uniform is visited, the extracted sampler name is generated.  For
    // arrays, consecutive visits reuse the same extracted sampler name.
    size_t index = extractedSamplerIndices->size();

    auto extracted = extractedSamplerIndices->find(uniformNameWithoutIndices);
    if (extracted != extractedSamplerIndices->end())
    {
        index = extracted->second;
    }
    else
    {
        (*extractedSamplerIndices)[uniformNameWithoutIndices] = index;
    }

    std::ostringstream name;
    name << sh::kExtractedSamplerNamePrefix << '_' << index;
    return name.str();
}

const angle::Format &GetFormatFromFormatType(GLenum format, GLenum type)
{
    GLenum sizedInternalFormat    = gl::GetInternalFormatInfo(format, type).sizedInternalFormat;
    angle::FormatID angleFormatID = angle::Format::InternalFormatToID(sizedInternalFormat);
    return angle::Format::Get(angleFormatID);
}

angle::Result ComputeStartVertex(ContextImpl *contextImpl,
                                 const gl::IndexRange &indexRange,
                                 GLint baseVertex,
                                 GLint *firstVertexOut)
{
    // The base vertex is only used in DrawElementsIndirect. Given the assertion above and the
    // type of mBaseVertex (GLint), adding them both as 64-bit ints is safe.
    int64_t startVertexInt64 =
        static_cast<int64_t>(baseVertex) + static_cast<int64_t>(indexRange.start());

    // OpenGL ES 3.2 spec section 10.5: "Behavior of DrawElementsOneInstance is undefined if the
    // vertex ID is negative for any element"
    ANGLE_CHECK_GL_MATH(contextImpl, startVertexInt64 >= 0);

    // OpenGL ES 3.2 spec section 10.5: "If the vertex ID is larger than the maximum value
    // representable by type, it should behave as if the calculation were upconverted to 32-bit
    // unsigned integers(with wrapping on overflow conditions)." ANGLE does not fully handle
    // these rules, an overflow error is returned if the start vertex cannot be stored in a
    // 32-bit signed integer.
    ANGLE_CHECK_GL_MATH(contextImpl, startVertexInt64 <= std::numeric_limits<GLint>::max());

    *firstVertexOut = static_cast<GLint>(startVertexInt64);
    return angle::Result::Continue;
}

angle::Result GetVertexRangeInfo(const gl::Context *context,
                                 GLint firstVertex,
                                 GLsizei vertexOrIndexCount,
                                 gl::DrawElementsType indexTypeOrInvalid,
                                 const void *indices,
                                 GLint baseVertex,
                                 GLint *startVertexOut,
                                 size_t *vertexCountOut)
{
    if (indexTypeOrInvalid != gl::DrawElementsType::InvalidEnum)
    {
        gl::IndexRange indexRange;
        ANGLE_TRY(context->getState().getVertexArray()->getIndexRange(
            context, indexTypeOrInvalid, vertexOrIndexCount, indices,
            context->getState().isPrimitiveRestartEnabled(), &indexRange));
        ANGLE_TRY(ComputeStartVertex(context->getImplementation(), indexRange, baseVertex,
                                     startVertexOut));

        // Protect against requiring 64-bits to store a draw count. Most math is done in size_t and
        // not safe on 32-bit systems. This would require a UINT_MAX index when primitive restart is
        // disabled.
        uint64_t vertexCount = indexRange.vertexCount();
        ANGLE_CHECK_GL_MATH(context->getImplementation(),
                            vertexCount <= std::numeric_limits<GLuint>::max());

        *vertexCountOut = static_cast<size_t>(vertexCount);
    }
    else
    {
        *startVertexOut = firstVertex;
        *vertexCountOut = vertexOrIndexCount;
    }
    return angle::Result::Continue;
}

gl::Rectangle ClipRectToScissor(const gl::State &glState, const gl::Rectangle &rect, bool invertY)
{
    // If the scissor test isn't enabled, assume it has infinite size.  Its intersection with the
    // rect would be the rect itself.
    //
    // Note that on Vulkan, returning this (as opposed to a fixed max-int-sized rect) could lead to
    // unnecessary pipeline creations if two otherwise identical pipelines are used on framebuffers
    // with different sizes.  If such usage is observed in an application, we should investigate
    // possible optimizations.
    if (!glState.isScissorTestEnabled())
    {
        return rect;
    }

    gl::Rectangle clippedRect;
    if (!gl::ClipRectangle(glState.getScissor(), rect, &clippedRect))
    {
        return gl::Rectangle();
    }

    if (invertY)
    {
        clippedRect.y = rect.height - clippedRect.y - clippedRect.height;
    }

    return clippedRect;
}

void ApplyFeatureOverrides(angle::FeatureSetBase *features,
                           const angle::FeatureOverrides &overrides)
{
    std::stringstream featureStream;

    featureStream << features->overrideFeatures(overrides.enabled, true);
    featureStream << features->overrideFeatures(overrides.disabled, false);

    // Override with environment as well.
    constexpr char kAngleFeatureOverridesEnabledEnvName[]  = "ANGLE_FEATURE_OVERRIDES_ENABLED";
    constexpr char kAngleFeatureOverridesDisabledEnvName[] = "ANGLE_FEATURE_OVERRIDES_DISABLED";
    constexpr char kAngleFeatureOverridesEnabledPropertyName[] =
        "debug.angle.feature_overrides_enabled";
    constexpr char kAngleFeatureOverridesDisabledPropertyName[] =
        "debug.angle.feature_overrides_disabled";
    std::vector<std::string> overridesEnabled =
        angle::GetCachedStringsFromEnvironmentVarOrAndroidProperty(
            kAngleFeatureOverridesEnabledEnvName, kAngleFeatureOverridesEnabledPropertyName, ":");
    std::vector<std::string> overridesDisabled =
        angle::GetCachedStringsFromEnvironmentVarOrAndroidProperty(
            kAngleFeatureOverridesDisabledEnvName, kAngleFeatureOverridesDisabledPropertyName, ":");

    featureStream << features->overrideFeatures(overridesEnabled, true);
    featureStream << features->overrideFeatures(overridesDisabled, false);

    if (!featureStream.str().empty())
    {
        INFO() << featureStream.str();
    }
}

void StreamEmulatedLineLoopIndices(gl::DrawElementsType glIndexType,
                                   GLsizei indexCount,
                                   const uint8_t *srcPtr,
                                   uint8_t *outPtr,
                                   bool shouldConvertUint8)
{
    switch (glIndexType)
    {
        case gl::DrawElementsType::UnsignedByte:
            if (shouldConvertUint8)
            {
                CopyLineLoopIndicesWithRestart<uint8_t, uint16_t>(indexCount, srcPtr, outPtr);
            }
            else
            {
                CopyLineLoopIndicesWithRestart<uint8_t, uint8_t>(indexCount, srcPtr, outPtr);
            }
            break;
        case gl::DrawElementsType::UnsignedShort:
            CopyLineLoopIndicesWithRestart<uint16_t, uint16_t>(indexCount, srcPtr, outPtr);
            break;
        case gl::DrawElementsType::UnsignedInt:
            CopyLineLoopIndicesWithRestart<uint32_t, uint32_t>(indexCount, srcPtr, outPtr);
            break;
        default:
            UNREACHABLE();
    }
}

void GetSamplePosition(GLsizei sampleCount, size_t index, GLfloat *xy)
{
    ASSERT(gl::isPow2(sampleCount));
    if (sampleCount > 16)
    {
        // Vulkan (and D3D11) doesn't have standard sample positions for 32 and 64 samples (and no
        // drivers are known to support that many samples)
        xy[0] = 0.5f;
        xy[1] = 0.5f;
    }
    else
    {
        size_t indexKey = static_cast<size_t>(gl::log2(sampleCount));
        ASSERT(indexKey < kSamplePositions.size() &&
               (2 * index + 1) < kSamplePositions[indexKey].size());

        xy[0] = kSamplePositions[indexKey][2 * index];
        xy[1] = kSamplePositions[indexKey][2 * index + 1];
    }
}

// These macros are to avoid code too much duplication for variations of multi draw types
#define DRAW_ARRAYS__ contextImpl->drawArrays(context, mode, firsts[drawID], counts[drawID])
#define DRAW_ARRAYS_INSTANCED_                                                      \
    contextImpl->drawArraysInstanced(context, mode, firsts[drawID], counts[drawID], \
                                     instanceCounts[drawID])
#define DRAW_ELEMENTS__ \
    contextImpl->drawElements(context, mode, counts[drawID], type, indices[drawID])
#define DRAW_ELEMENTS_INSTANCED_                                                             \
    contextImpl->drawElementsInstanced(context, mode, counts[drawID], type, indices[drawID], \
                                       instanceCounts[drawID])
#define DRAW_ARRAYS_INSTANCED_BASE_INSTANCE                                                     \
    contextImpl->drawArraysInstancedBaseInstance(context, mode, firsts[drawID], counts[drawID], \
                                                 instanceCounts[drawID], baseInstances[drawID])
#define DRAW_ELEMENTS_INSTANCED_BASE_VERTEX_BASE_INSTANCE                             \
    contextImpl->drawElementsInstancedBaseVertexBaseInstance(                         \
        context, mode, counts[drawID], type, indices[drawID], instanceCounts[drawID], \
        baseVertices[drawID], baseInstances[drawID])
#define DRAW_CALL(drawType, instanced, bvbi) DRAW_##drawType##instanced##bvbi

#define MULTI_DRAW_BLOCK(drawType, instanced, bvbi, hasDrawID, hasBaseVertex, hasBaseInstance) \
    do                                                                                         \
    {                                                                                          \
        bool anyDraw = false;                                                                  \
        for (GLsizei drawID = 0; drawID < drawcount; ++drawID)                                 \
        {                                                                                      \
            if (ANGLE_NOOP_DRAW(instanced))                                                    \
            {                                                                                  \
                ANGLE_TRY(contextImpl->handleNoopDrawEvent());                                 \
                continue;                                                                      \
            }                                                                                  \
            ANGLE_SET_DRAW_ID_UNIFORM(hasDrawID)(drawID);                                      \
            ANGLE_SET_BASE_VERTEX_UNIFORM(hasBaseVertex)(baseVertices[drawID]);                \
            ANGLE_SET_BASE_INSTANCE_UNIFORM(hasBaseInstance)(baseInstances[drawID]);           \
            ANGLE_TRY(DRAW_CALL(drawType, instanced, bvbi));                                   \
            ANGLE_MARK_TRANSFORM_FEEDBACK_USAGE(instanced);                                    \
            gl::MarkShaderStorageUsage(context);                                               \
            anyDraw = true;                                                                    \
        }                                                                                      \
        /* reset the uniform to zero for non-multi-draw uses of the program */                 \
        ANGLE_SET_DRAW_ID_UNIFORM(hasDrawID)(0);                                               \
        if (!anyDraw)                                                                          \
        {                                                                                      \
            ANGLE_TRY(contextImpl->handleNoopMultiDrawEvent());                                \
        }                                                                                      \
    } while (0)

angle::Result MultiDrawArraysGeneral(ContextImpl *contextImpl,
                                     const gl::Context *context,
                                     gl::PrimitiveMode mode,
                                     const GLint *firsts,
                                     const GLsizei *counts,
                                     GLsizei drawcount)
{
    gl::ProgramExecutable *executable = context->getState().getLinkedProgramExecutable(context);
    const bool hasDrawID              = executable->hasDrawIDUniform();
    if (hasDrawID)
    {
        MULTI_DRAW_BLOCK(ARRAYS, _, _, 1, 0, 0);
    }
    else
    {
        MULTI_DRAW_BLOCK(ARRAYS, _, _, 0, 0, 0);
    }

    return angle::Result::Continue;
}

angle::Result MultiDrawArraysIndirectGeneral(ContextImpl *contextImpl,
                                             const gl::Context *context,
                                             gl::PrimitiveMode mode,
                                             const void *indirect,
                                             GLsizei drawcount,
                                             GLsizei stride)
{
    const GLubyte *indirectPtr = static_cast<const GLubyte *>(indirect);

    for (auto count = 0; count < drawcount; count++)
    {
        ANGLE_TRY(contextImpl->drawArraysIndirect(
            context, mode, reinterpret_cast<const gl::DrawArraysIndirectCommand *>(indirectPtr)));
        if (stride == 0)
        {
            indirectPtr += sizeof(gl::DrawArraysIndirectCommand);
        }
        else
        {
            indirectPtr += stride;
        }
    }

    return angle::Result::Continue;
}

angle::Result MultiDrawArraysInstancedGeneral(ContextImpl *contextImpl,
                                              const gl::Context *context,
                                              gl::PrimitiveMode mode,
                                              const GLint *firsts,
                                              const GLsizei *counts,
                                              const GLsizei *instanceCounts,
                                              GLsizei drawcount)
{
    gl::ProgramExecutable *executable = context->getState().getLinkedProgramExecutable(context);
    const bool hasDrawID              = executable->hasDrawIDUniform();
    if (hasDrawID)
    {
        MULTI_DRAW_BLOCK(ARRAYS, _INSTANCED, _, 1, 0, 0);
    }
    else
    {
        MULTI_DRAW_BLOCK(ARRAYS, _INSTANCED, _, 0, 0, 0);
    }

    return angle::Result::Continue;
}

angle::Result MultiDrawElementsGeneral(ContextImpl *contextImpl,
                                       const gl::Context *context,
                                       gl::PrimitiveMode mode,
                                       const GLsizei *counts,
                                       gl::DrawElementsType type,
                                       const GLvoid *const *indices,
                                       GLsizei drawcount)
{
    gl::ProgramExecutable *executable = context->getState().getLinkedProgramExecutable(context);
    const bool hasDrawID              = executable->hasDrawIDUniform();
    if (hasDrawID)
    {
        MULTI_DRAW_BLOCK(ELEMENTS, _, _, 1, 0, 0);
    }
    else
    {
        MULTI_DRAW_BLOCK(ELEMENTS, _, _, 0, 0, 0);
    }

    return angle::Result::Continue;
}

angle::Result MultiDrawElementsIndirectGeneral(ContextImpl *contextImpl,
                                               const gl::Context *context,
                                               gl::PrimitiveMode mode,
                                               gl::DrawElementsType type,
                                               const void *indirect,
                                               GLsizei drawcount,
                                               GLsizei stride)
{
    const GLubyte *indirectPtr = static_cast<const GLubyte *>(indirect);

    for (auto count = 0; count < drawcount; count++)
    {
        ANGLE_TRY(contextImpl->drawElementsIndirect(
            context, mode, type,
            reinterpret_cast<const gl::DrawElementsIndirectCommand *>(indirectPtr)));
        if (stride == 0)
        {
            indirectPtr += sizeof(gl::DrawElementsIndirectCommand);
        }
        else
        {
            indirectPtr += stride;
        }
    }

    return angle::Result::Continue;
}

angle::Result MultiDrawElementsInstancedGeneral(ContextImpl *contextImpl,
                                                const gl::Context *context,
                                                gl::PrimitiveMode mode,
                                                const GLsizei *counts,
                                                gl::DrawElementsType type,
                                                const GLvoid *const *indices,
                                                const GLsizei *instanceCounts,
                                                GLsizei drawcount)
{
    gl::ProgramExecutable *executable = context->getState().getLinkedProgramExecutable(context);
    const bool hasDrawID              = executable->hasDrawIDUniform();
    if (hasDrawID)
    {
        MULTI_DRAW_BLOCK(ELEMENTS, _INSTANCED, _, 1, 0, 0);
    }
    else
    {
        MULTI_DRAW_BLOCK(ELEMENTS, _INSTANCED, _, 0, 0, 0);
    }

    return angle::Result::Continue;
}

angle::Result MultiDrawArraysInstancedBaseInstanceGeneral(ContextImpl *contextImpl,
                                                          const gl::Context *context,
                                                          gl::PrimitiveMode mode,
                                                          const GLint *firsts,
                                                          const GLsizei *counts,
                                                          const GLsizei *instanceCounts,
                                                          const GLuint *baseInstances,
                                                          GLsizei drawcount)
{
    gl::ProgramExecutable *executable = context->getState().getLinkedProgramExecutable(context);
    const bool hasDrawID              = executable->hasDrawIDUniform();
    const bool hasBaseInstance        = executable->hasBaseInstanceUniform();
    ResetBaseVertexBaseInstance resetUniforms(executable, false, hasBaseInstance);

    if (hasDrawID && hasBaseInstance)
    {
        MULTI_DRAW_BLOCK(ARRAYS, _INSTANCED, _BASE_INSTANCE, 1, 0, 1);
    }
    else if (hasDrawID)
    {
        MULTI_DRAW_BLOCK(ARRAYS, _INSTANCED, _BASE_INSTANCE, 1, 0, 0);
    }
    else if (hasBaseInstance)
    {
        MULTI_DRAW_BLOCK(ARRAYS, _INSTANCED, _BASE_INSTANCE, 0, 0, 1);
    }
    else
    {
        MULTI_DRAW_BLOCK(ARRAYS, _INSTANCED, _BASE_INSTANCE, 0, 0, 0);
    }

    return angle::Result::Continue;
}

angle::Result MultiDrawElementsInstancedBaseVertexBaseInstanceGeneral(ContextImpl *contextImpl,
                                                                      const gl::Context *context,
                                                                      gl::PrimitiveMode mode,
                                                                      const GLsizei *counts,
                                                                      gl::DrawElementsType type,
                                                                      const GLvoid *const *indices,
                                                                      const GLsizei *instanceCounts,
                                                                      const GLint *baseVertices,
                                                                      const GLuint *baseInstances,
                                                                      GLsizei drawcount)
{
    gl::ProgramExecutable *executable = context->getState().getLinkedProgramExecutable(context);
    const bool hasDrawID              = executable->hasDrawIDUniform();
    const bool hasBaseVertex          = executable->hasBaseVertexUniform();
    const bool hasBaseInstance        = executable->hasBaseInstanceUniform();
    ResetBaseVertexBaseInstance resetUniforms(executable, hasBaseVertex, hasBaseInstance);

    if (hasDrawID)
    {
        if (hasBaseVertex)
        {
            if (hasBaseInstance)
            {
                MULTI_DRAW_BLOCK(ELEMENTS, _INSTANCED, _BASE_VERTEX_BASE_INSTANCE, 1, 1, 1);
            }
            else
            {
                MULTI_DRAW_BLOCK(ELEMENTS, _INSTANCED, _BASE_VERTEX_BASE_INSTANCE, 1, 1, 0);
            }
        }
        else
        {
            if (hasBaseInstance)
            {
                MULTI_DRAW_BLOCK(ELEMENTS, _INSTANCED, _BASE_VERTEX_BASE_INSTANCE, 1, 0, 1);
            }
            else
            {
                MULTI_DRAW_BLOCK(ELEMENTS, _INSTANCED, _BASE_VERTEX_BASE_INSTANCE, 1, 0, 0);
            }
        }
    }
    else
    {
        if (hasBaseVertex)
        {
            if (hasBaseInstance)
            {
                MULTI_DRAW_BLOCK(ELEMENTS, _INSTANCED, _BASE_VERTEX_BASE_INSTANCE, 0, 1, 1);
            }
            else
            {
                MULTI_DRAW_BLOCK(ELEMENTS, _INSTANCED, _BASE_VERTEX_BASE_INSTANCE, 0, 1, 0);
            }
        }
        else
        {
            if (hasBaseInstance)
            {
                MULTI_DRAW_BLOCK(ELEMENTS, _INSTANCED, _BASE_VERTEX_BASE_INSTANCE, 0, 0, 1);
            }
            else
            {
                MULTI_DRAW_BLOCK(ELEMENTS, _INSTANCED, _BASE_VERTEX_BASE_INSTANCE, 0, 0, 0);
            }
        }
    }

    return angle::Result::Continue;
}

ResetBaseVertexBaseInstance::ResetBaseVertexBaseInstance(gl::ProgramExecutable *executable,
                                                         bool resetBaseVertex,
                                                         bool resetBaseInstance)
    : mExecutable(executable),
      mResetBaseVertex(resetBaseVertex),
      mResetBaseInstance(resetBaseInstance)
{}

ResetBaseVertexBaseInstance::~ResetBaseVertexBaseInstance()
{
    if (mExecutable)
    {
        // Reset emulated uniforms to zero to avoid affecting other draw calls
        if (mResetBaseVertex)
        {
            mExecutable->setBaseVertexUniform(0);
        }

        if (mResetBaseInstance)
        {
            mExecutable->setBaseInstanceUniform(0);
        }
    }
}

angle::FormatID ConvertToSRGB(angle::FormatID formatID)
{
    switch (formatID)
    {
        case angle::FormatID::R8_UNORM:
            return angle::FormatID::R8_UNORM_SRGB;
        case angle::FormatID::R8G8_UNORM:
            return angle::FormatID::R8G8_UNORM_SRGB;
        case angle::FormatID::R8G8B8_UNORM:
            return angle::FormatID::R8G8B8_UNORM_SRGB;
        case angle::FormatID::R8G8B8X8_UNORM:
            return angle::FormatID::R8G8B8X8_UNORM_SRGB;
        case angle::FormatID::R8G8B8A8_UNORM:
            return angle::FormatID::R8G8B8A8_UNORM_SRGB;
        case angle::FormatID::B8G8R8A8_UNORM:
            return angle::FormatID::B8G8R8A8_UNORM_SRGB;
        case angle::FormatID::BC1_RGB_UNORM_BLOCK:
            return angle::FormatID::BC1_RGB_UNORM_SRGB_BLOCK;
        case angle::FormatID::BC1_RGBA_UNORM_BLOCK:
            return angle::FormatID::BC1_RGBA_UNORM_SRGB_BLOCK;
        case angle::FormatID::BC2_RGBA_UNORM_BLOCK:
            return angle::FormatID::BC2_RGBA_UNORM_SRGB_BLOCK;
        case angle::FormatID::BC3_RGBA_UNORM_BLOCK:
            return angle::FormatID::BC3_RGBA_UNORM_SRGB_BLOCK;
        case angle::FormatID::BC7_RGBA_UNORM_BLOCK:
            return angle::FormatID::BC7_RGBA_UNORM_SRGB_BLOCK;
        case angle::FormatID::ETC2_R8G8B8_UNORM_BLOCK:
            return angle::FormatID::ETC2_R8G8B8_SRGB_BLOCK;
        case angle::FormatID::ETC2_R8G8B8A1_UNORM_BLOCK:
            return angle::FormatID::ETC2_R8G8B8A1_SRGB_BLOCK;
        case angle::FormatID::ETC2_R8G8B8A8_UNORM_BLOCK:
            return angle::FormatID::ETC2_R8G8B8A8_SRGB_BLOCK;
        case angle::FormatID::ASTC_4x4_UNORM_BLOCK:
            return angle::FormatID::ASTC_4x4_SRGB_BLOCK;
        case angle::FormatID::ASTC_5x4_UNORM_BLOCK:
            return angle::FormatID::ASTC_5x4_SRGB_BLOCK;
        case angle::FormatID::ASTC_5x5_UNORM_BLOCK:
            return angle::FormatID::ASTC_5x5_SRGB_BLOCK;
        case angle::FormatID::ASTC_6x5_UNORM_BLOCK:
            return angle::FormatID::ASTC_6x5_SRGB_BLOCK;
        case angle::FormatID::ASTC_6x6_UNORM_BLOCK:
            return angle::FormatID::ASTC_6x6_SRGB_BLOCK;
        case angle::FormatID::ASTC_8x5_UNORM_BLOCK:
            return angle::FormatID::ASTC_8x5_SRGB_BLOCK;
        case angle::FormatID::ASTC_8x6_UNORM_BLOCK:
            return angle::FormatID::ASTC_8x6_SRGB_BLOCK;
        case angle::FormatID::ASTC_8x8_UNORM_BLOCK:
            return angle::FormatID::ASTC_8x8_SRGB_BLOCK;
        case angle::FormatID::ASTC_10x5_UNORM_BLOCK:
            return angle::FormatID::ASTC_10x5_SRGB_BLOCK;
        case angle::FormatID::ASTC_10x6_UNORM_BLOCK:
            return angle::FormatID::ASTC_10x6_SRGB_BLOCK;
        case angle::FormatID::ASTC_10x8_UNORM_BLOCK:
            return angle::FormatID::ASTC_10x8_SRGB_BLOCK;
        case angle::FormatID::ASTC_10x10_UNORM_BLOCK:
            return angle::FormatID::ASTC_10x10_SRGB_BLOCK;
        case angle::FormatID::ASTC_12x10_UNORM_BLOCK:
            return angle::FormatID::ASTC_12x10_SRGB_BLOCK;
        case angle::FormatID::ASTC_12x12_UNORM_BLOCK:
            return angle::FormatID::ASTC_12x12_SRGB_BLOCK;
        case angle::FormatID::ASTC_3x3x3_UNORM_BLOCK:
            return angle::FormatID::ASTC_3x3x3_UNORM_SRGB_BLOCK;
        case angle::FormatID::ASTC_4x3x3_UNORM_BLOCK:
            return angle::FormatID::ASTC_4x3x3_UNORM_SRGB_BLOCK;
        case angle::FormatID::ASTC_4x4x3_UNORM_BLOCK:
            return angle::FormatID::ASTC_4x4x3_UNORM_SRGB_BLOCK;
        case angle::FormatID::ASTC_4x4x4_UNORM_BLOCK:
            return angle::FormatID::ASTC_4x4x4_UNORM_SRGB_BLOCK;
        case angle::FormatID::ASTC_5x4x4_UNORM_BLOCK:
            return angle::FormatID::ASTC_5x4x4_UNORM_SRGB_BLOCK;
        case angle::FormatID::ASTC_5x5x4_UNORM_BLOCK:
            return angle::FormatID::ASTC_5x5x4_UNORM_SRGB_BLOCK;
        case angle::FormatID::ASTC_5x5x5_UNORM_BLOCK:
            return angle::FormatID::ASTC_5x5x5_UNORM_SRGB_BLOCK;
        case angle::FormatID::ASTC_6x5x5_UNORM_BLOCK:
            return angle::FormatID::ASTC_6x5x5_UNORM_SRGB_BLOCK;
        case angle::FormatID::ASTC_6x6x5_UNORM_BLOCK:
            return angle::FormatID::ASTC_6x6x5_UNORM_SRGB_BLOCK;
        case angle::FormatID::ASTC_6x6x6_UNORM_BLOCK:
            return angle::FormatID::ASTC_6x6x6_UNORM_SRGB_BLOCK;
        default:
            return angle::FormatID::NONE;
    }
}

angle::FormatID ConvertToLinear(angle::FormatID formatID)
{
    switch (formatID)
    {
        case angle::FormatID::R8_UNORM_SRGB:
            return angle::FormatID::R8_UNORM;
        case angle::FormatID::R8G8_UNORM_SRGB:
            return angle::FormatID::R8G8_UNORM;
        case angle::FormatID::R8G8B8_UNORM_SRGB:
            return angle::FormatID::R8G8B8_UNORM;
        case angle::FormatID::R8G8B8X8_UNORM_SRGB:
            return angle::FormatID::R8G8B8X8_UNORM;
        case angle::FormatID::R8G8B8A8_UNORM_SRGB:
            return angle::FormatID::R8G8B8A8_UNORM;
        case angle::FormatID::B8G8R8A8_UNORM_SRGB:
            return angle::FormatID::B8G8R8A8_UNORM;
        case angle::FormatID::BC1_RGB_UNORM_SRGB_BLOCK:
            return angle::FormatID::BC1_RGB_UNORM_BLOCK;
        case angle::FormatID::BC1_RGBA_UNORM_SRGB_BLOCK:
            return angle::FormatID::BC1_RGBA_UNORM_BLOCK;
        case angle::FormatID::BC2_RGBA_UNORM_SRGB_BLOCK:
            return angle::FormatID::BC2_RGBA_UNORM_BLOCK;
        case angle::FormatID::BC3_RGBA_UNORM_SRGB_BLOCK:
            return angle::FormatID::BC3_RGBA_UNORM_BLOCK;
        case angle::FormatID::BC7_RGBA_UNORM_SRGB_BLOCK:
            return angle::FormatID::BC7_RGBA_UNORM_BLOCK;
        case angle::FormatID::ETC2_R8G8B8_SRGB_BLOCK:
            return angle::FormatID::ETC2_R8G8B8_UNORM_BLOCK;
        case angle::FormatID::ETC2_R8G8B8A1_SRGB_BLOCK:
            return angle::FormatID::ETC2_R8G8B8A1_UNORM_BLOCK;
        case angle::FormatID::ETC2_R8G8B8A8_SRGB_BLOCK:
            return angle::FormatID::ETC2_R8G8B8A8_UNORM_BLOCK;
        case angle::FormatID::ASTC_4x4_SRGB_BLOCK:
            return angle::FormatID::ASTC_4x4_UNORM_BLOCK;
        case angle::FormatID::ASTC_5x4_SRGB_BLOCK:
            return angle::FormatID::ASTC_5x4_UNORM_BLOCK;
        case angle::FormatID::ASTC_5x5_SRGB_BLOCK:
            return angle::FormatID::ASTC_5x5_UNORM_BLOCK;
        case angle::FormatID::ASTC_6x5_SRGB_BLOCK:
            return angle::FormatID::ASTC_6x5_UNORM_BLOCK;
        case angle::FormatID::ASTC_6x6_SRGB_BLOCK:
            return angle::FormatID::ASTC_6x6_UNORM_BLOCK;
        case angle::FormatID::ASTC_8x5_SRGB_BLOCK:
            return angle::FormatID::ASTC_8x5_UNORM_BLOCK;
        case angle::FormatID::ASTC_8x6_SRGB_BLOCK:
            return angle::FormatID::ASTC_8x6_UNORM_BLOCK;
        case angle::FormatID::ASTC_8x8_SRGB_BLOCK:
            return angle::FormatID::ASTC_8x8_UNORM_BLOCK;
        case angle::FormatID::ASTC_10x5_SRGB_BLOCK:
            return angle::FormatID::ASTC_10x5_UNORM_BLOCK;
        case angle::FormatID::ASTC_10x6_SRGB_BLOCK:
            return angle::FormatID::ASTC_10x6_UNORM_BLOCK;
        case angle::FormatID::ASTC_10x8_SRGB_BLOCK:
            return angle::FormatID::ASTC_10x8_UNORM_BLOCK;
        case angle::FormatID::ASTC_10x10_SRGB_BLOCK:
            return angle::FormatID::ASTC_10x10_UNORM_BLOCK;
        case angle::FormatID::ASTC_12x10_SRGB_BLOCK:
            return angle::FormatID::ASTC_12x10_UNORM_BLOCK;
        case angle::FormatID::ASTC_12x12_SRGB_BLOCK:
            return angle::FormatID::ASTC_12x12_UNORM_BLOCK;
        case angle::FormatID::ASTC_3x3x3_UNORM_SRGB_BLOCK:
            return angle::FormatID::ASTC_3x3x3_UNORM_BLOCK;
        case angle::FormatID::ASTC_4x3x3_UNORM_SRGB_BLOCK:
            return angle::FormatID::ASTC_4x3x3_UNORM_BLOCK;
        case angle::FormatID::ASTC_4x4x3_UNORM_SRGB_BLOCK:
            return angle::FormatID::ASTC_4x4x3_UNORM_BLOCK;
        case angle::FormatID::ASTC_4x4x4_UNORM_SRGB_BLOCK:
            return angle::FormatID::ASTC_4x4x4_UNORM_BLOCK;
        case angle::FormatID::ASTC_5x4x4_UNORM_SRGB_BLOCK:
            return angle::FormatID::ASTC_5x4x4_UNORM_BLOCK;
        case angle::FormatID::ASTC_5x5x4_UNORM_SRGB_BLOCK:
            return angle::FormatID::ASTC_5x5x4_UNORM_BLOCK;
        case angle::FormatID::ASTC_5x5x5_UNORM_SRGB_BLOCK:
            return angle::FormatID::ASTC_5x5x5_UNORM_BLOCK;
        case angle::FormatID::ASTC_6x5x5_UNORM_SRGB_BLOCK:
            return angle::FormatID::ASTC_6x5x5_UNORM_BLOCK;
        case angle::FormatID::ASTC_6x6x5_UNORM_SRGB_BLOCK:
            return angle::FormatID::ASTC_6x6x5_UNORM_BLOCK;
        case angle::FormatID::ASTC_6x6x6_UNORM_SRGB_BLOCK:
            return angle::FormatID::ASTC_6x6x6_UNORM_BLOCK;
        default:
            return angle::FormatID::NONE;
    }
}

bool IsOverridableLinearOrSRGBFormat(angle::FormatID formatID)
{
    return ConvertToSRGB(formatID) != angle::FormatID::NONE ||
           ConvertToLinear(formatID) != angle::FormatID::NONE;
}

template <bool swizzledLuma>
const gl::ColorGeneric AdjustBorderColor(const angle::ColorGeneric &borderColorGeneric,
                                         const angle::Format &format,
                                         bool stencilMode)
{
    gl::ColorGeneric adjustedBorderColor = borderColorGeneric;

    // Handle depth formats
    if (format.hasDepthOrStencilBits())
    {
        if (stencilMode)
        {
            // Stencil component
            adjustedBorderColor.colorUI.red = gl::clampForBitCount<unsigned int>(
                adjustedBorderColor.colorUI.red, format.stencilBits);
            // Unused components need to be reset because some backends simulate integer samplers
            adjustedBorderColor.colorUI.green = 0u;
            adjustedBorderColor.colorUI.blue  = 0u;
            adjustedBorderColor.colorUI.alpha = 1u;
        }
        else
        {
            // Depth component
            if (format.isUnorm())
            {
                adjustedBorderColor.colorF.red = gl::clamp01(adjustedBorderColor.colorF.red);
            }
        }

        return adjustedBorderColor;
    }

    // Handle LUMA formats
    if (format.isLUMA())
    {
        if (format.isUnorm())
        {
            adjustedBorderColor.colorF.red   = gl::clamp01(adjustedBorderColor.colorF.red);
            adjustedBorderColor.colorF.alpha = gl::clamp01(adjustedBorderColor.colorF.alpha);
        }

        // Luma formats are either unpacked to RGBA or emulated with component swizzling
        if (swizzledLuma)
        {
            // L is R (no-op); A is R; LA is RG
            if (format.alphaBits > 0)
            {
                if (format.luminanceBits > 0)
                {
                    adjustedBorderColor.colorF.green = adjustedBorderColor.colorF.alpha;
                }
                else
                {
                    adjustedBorderColor.colorF.red = adjustedBorderColor.colorF.alpha;
                }
            }
        }
        else
        {
            // L is RGBX; A is A or RGBA; LA is RGBA
            if (format.alphaBits == 0)
            {
                adjustedBorderColor.colorF.alpha = 1.0f;
            }
            else if (format.luminanceBits == 0)
            {
                adjustedBorderColor.colorF.red = 0.0f;
            }
            adjustedBorderColor.colorF.green = adjustedBorderColor.colorF.red;
            adjustedBorderColor.colorF.blue  = adjustedBorderColor.colorF.red;
        }

        return adjustedBorderColor;
    }

    // Handle all other formats. Clamp border color to the ranges of color components.
    // On some platforms, RGB formats may be emulated with RGBA, enforce opaque border color there.
    if (format.isSint())
    {
        adjustedBorderColor.colorI.red =
            gl::clampForBitCount<int>(adjustedBorderColor.colorI.red, format.redBits);
        adjustedBorderColor.colorI.green =
            gl::clampForBitCount<int>(adjustedBorderColor.colorI.green, format.greenBits);
        adjustedBorderColor.colorI.blue =
            gl::clampForBitCount<int>(adjustedBorderColor.colorI.blue, format.blueBits);
        adjustedBorderColor.colorI.alpha =
            format.alphaBits > 0
                ? gl::clampForBitCount<int>(adjustedBorderColor.colorI.alpha, format.alphaBits)
                : 1;
    }
    else if (format.isUint())
    {
        adjustedBorderColor.colorUI.red =
            gl::clampForBitCount<unsigned int>(adjustedBorderColor.colorUI.red, format.redBits);
        adjustedBorderColor.colorUI.green =
            gl::clampForBitCount<unsigned int>(adjustedBorderColor.colorUI.green, format.greenBits);
        adjustedBorderColor.colorUI.blue =
            gl::clampForBitCount<unsigned int>(adjustedBorderColor.colorUI.blue, format.blueBits);
        adjustedBorderColor.colorUI.alpha =
            format.alphaBits > 0 ? gl::clampForBitCount<unsigned int>(
                                       adjustedBorderColor.colorUI.alpha, format.alphaBits)
                                 : 1;
    }
    else if (format.isSnorm())
    {
        // clamp between -1.0f and 1.0f
        adjustedBorderColor.colorF.red   = gl::clamp(adjustedBorderColor.colorF.red, -1.0f, 1.0f);
        adjustedBorderColor.colorF.green = gl::clamp(adjustedBorderColor.colorF.green, -1.0f, 1.0f);
        adjustedBorderColor.colorF.blue  = gl::clamp(adjustedBorderColor.colorF.blue, -1.0f, 1.0f);
        adjustedBorderColor.colorF.alpha =
            format.alphaBits > 0 ? gl::clamp(adjustedBorderColor.colorF.alpha, -1.0f, 1.0f) : 1.0f;
    }
    else if (format.isUnorm())
    {
        // clamp between 0.0f and 1.0f
        adjustedBorderColor.colorF.red   = gl::clamp01(adjustedBorderColor.colorF.red);
        adjustedBorderColor.colorF.green = gl::clamp01(adjustedBorderColor.colorF.green);
        adjustedBorderColor.colorF.blue  = gl::clamp01(adjustedBorderColor.colorF.blue);
        adjustedBorderColor.colorF.alpha =
            format.alphaBits > 0 ? gl::clamp01(adjustedBorderColor.colorF.alpha) : 1.0f;
    }
    else if (format.isFloat() && format.alphaBits == 0)
    {
        adjustedBorderColor.colorF.alpha = 1.0;
    }

    return adjustedBorderColor;
}

template const gl::ColorGeneric AdjustBorderColor<true>(
    const angle::ColorGeneric &borderColorGeneric,
    const angle::Format &format,
    bool stencilMode);
template const gl::ColorGeneric AdjustBorderColor<false>(
    const angle::ColorGeneric &borderColorGeneric,
    const angle::Format &format,
    bool stencilMode);

bool TextureHasAnyRedefinedLevels(const gl::CubeFaceArray<gl::TexLevelMask> &redefinedLevels)
{
    for (gl::TexLevelMask faceRedefinedLevels : redefinedLevels)
    {
        if (faceRedefinedLevels.any())
        {
            return true;
        }
    }

    return false;
}

bool IsTextureLevelRedefined(const gl::CubeFaceArray<gl::TexLevelMask> &redefinedLevels,
                             gl::TextureType textureType,
                             gl::OwnerLevel level)
{
    gl::TexLevelMask redefined = redefinedLevels[0];

    if (textureType == gl::TextureType::CubeMap)
    {
        for (size_t face = 1; face < gl::kCubeFaceCount; ++face)
        {
            redefined |= redefinedLevels[face];
        }
    }

    return redefined.test(level.get());
}

bool TextureRedefineLevel(const TextureLevelAllocation levelAllocation,
                          const TextureLevelDefinition levelDefinition,
                          bool immutableFormat,
                          uint32_t levelCount,
                          const gl::OwnerImageIndex &index,
                          gl::OwnerLevel imageFirstAllocatedLevel,
                          gl::CubeFaceArray<gl::TexLevelMask> *redefinedLevels)
{
    // If the level that's being redefined is outside the level range of the allocated
    // image, the application is free to use any size or format.  Any data uploaded to it
    // will live in staging area until the texture base/max level is adjusted to include
    // this level, at which point the image will be recreated.
    //
    // Otherwise, if the level that's being redefined has a different format or size,
    // only release the image if it's single-mip, and keep the uploaded data staged.
    // Otherwise the image is mip-incomplete anyway and will be eventually recreated when
    // needed.  Only exception to this latter is if all the levels of the texture are
    // redefined such that the image becomes mip-complete in the end.
    // redefinedLevels is used during syncState to support this use-case.
    //
    // Note that if the image has multiple mips, there could be a copy from one mip
    // happening to the other, which means the image cannot be released.
    //
    // In summary:
    //
    // - If the image has a single level, and that level is being redefined, release the
    //   image.
    // - Otherwise keep the image intact (another mip may be the source of a copy), and
    //   make sure any updates to this level are staged.
    gl::OwnerLevel levelIndexGL = index.getLevelIndex();
    const bool isCompatibleRedefinition =
        levelAllocation == TextureLevelAllocation::WithinAllocatedImage &&
        levelDefinition == TextureLevelDefinition::Compatible;
    const bool isCubeMap = index.getType() == gl::TextureType::CubeMap;

    // Mark the level as incompatibly redefined if that's the case.  Note that if the level
    // was previously incompatibly defined, then later redefined to be compatible, the
    // corresponding bit should clear.
    if (levelAllocation == TextureLevelAllocation::WithinAllocatedImage)
    {
        // Immutable texture should never have levels redefined.
        ASSERT(isCompatibleRedefinition || !immutableFormat);

        const uint32_t redefinedFace = isCubeMap ? index.getLayerIndex().get() : 0;
        (*redefinedLevels)[redefinedFace].set(levelIndexGL.get(), !isCompatibleRedefinition);
    }

    const bool isUpdateToSingleLevelImage =
        levelCount == 1 && imageFirstAllocatedLevel == levelIndexGL;

    // If incompatible, and redefining the single-level image, the caller will release the texture
    // so it can be recreated immediately.  This is needed so that the texture can be reallocated
    // with the correct format/size.
    //
    // For cubemaps, every face may be separately redefined, so only release the image if all faces
    // have been redefined.  Note that this is not possible for texture arrays in general.
    bool shouldReleaseImage = !isCompatibleRedefinition && isUpdateToSingleLevelImage;
    if (shouldReleaseImage && isCubeMap)
    {
        for (uint32_t face = 0; face < 6; ++face)
        {
            if (!(*redefinedLevels)[face][levelIndexGL.get()])
            {
                shouldReleaseImage = false;
                break;
            }
        }
    }
    return shouldReleaseImage;
}

void TextureRedefineGenerateMipmapLevels(gl::OwnerLevel baseLevel,
                                         gl::OwnerLevel maxLevel,
                                         gl::OwnerLevel firstGeneratedLevel,
                                         gl::CubeFaceArray<gl::TexLevelMask> *redefinedLevels)
{
    static_assert(gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS < 32,
                  "levels mask assumes 32-bits is enough");
    // Generate bitmask for (baseLevel, maxLevel]. `+1` because bitMask takes `the number of bits`
    // but levels start counting from 0
    gl::TexLevelMask levelsMask(angle::BitMask<uint32_t>(maxLevel.get() + 1));
    levelsMask &= static_cast<uint32_t>(~angle::BitMask<uint32_t>(firstGeneratedLevel.get()));
    // Remove (baseLevel, maxLevel] from redefinedLevels. These levels are no longer incompatibly
    // defined if they previously were.  The corresponding bits in redefinedLevels should be
    // cleared.
    for (size_t face = 0; face < gl::kCubeFaceCount; ++face)
    {
        (*redefinedLevels)[face] &= ~levelsMask;
    }
}
}  // namespace rx
