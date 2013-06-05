#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// formatutils.cpp: Queries for GL image formats.

#include "libGLESv2/formatutils.h"
#include "libGLESv2/Context.h"
#include "common/mathutil.h"
#include "libGLESv2/renderer/Renderer.h"

namespace gl
{

// ES2 requires that format is equal to internal format at all glTex*Image2D entry points and the implementation
// can decide the true, sized, internal format. The ES2FormatMap determines the internal format for all valid
// format and type combinations.

typedef std::pair<GLenum, GLenum> FormatTypePair;
typedef std::pair<FormatTypePair, GLint> FormatPair;
typedef std::map<FormatTypePair, GLint> FormatMap;

// A helper function to insert data into the D3D11LoadFunctionMap with fewer characters.
static inline void InsertFormatMapping(FormatMap *map, GLenum format, GLenum type, GLint internalFormat)
{
    map->insert(FormatPair(FormatTypePair(format, type), internalFormat));
}

FormatMap BuildES2FormatMap()
{
    FormatMap map;

    //                       | Format                            | Type                             | Internal format                  |
    InsertFormatMapping(&map, GL_ALPHA,                           GL_UNSIGNED_BYTE,                  GL_ALPHA8_EXT                     );
    InsertFormatMapping(&map, GL_ALPHA,                           GL_FLOAT,                          GL_ALPHA32F_EXT                   );
    InsertFormatMapping(&map, GL_ALPHA,                           GL_HALF_FLOAT_OES,                 GL_ALPHA16F_EXT                   );

    InsertFormatMapping(&map, GL_LUMINANCE,                       GL_UNSIGNED_BYTE,                  GL_LUMINANCE8_EXT                 );
    InsertFormatMapping(&map, GL_LUMINANCE,                       GL_FLOAT,                          GL_LUMINANCE32F_EXT               );
    InsertFormatMapping(&map, GL_LUMINANCE,                       GL_HALF_FLOAT_OES,                 GL_LUMINANCE16F_EXT               );

    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,                 GL_UNSIGNED_BYTE,                  GL_LUMINANCE8_ALPHA8_EXT          );
    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,                 GL_FLOAT,                          GL_LUMINANCE_ALPHA32F_EXT         );
    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,                 GL_HALF_FLOAT_OES,                 GL_LUMINANCE_ALPHA16F_EXT         );

    InsertFormatMapping(&map, GL_RGB,                             GL_UNSIGNED_BYTE,                  GL_RGB8_OES                       );
    InsertFormatMapping(&map, GL_RGB,                             GL_UNSIGNED_SHORT_5_6_5,           GL_RGB565                         );
    InsertFormatMapping(&map, GL_RGB,                             GL_FLOAT,                          GL_RGB32F_EXT                     );
    InsertFormatMapping(&map, GL_RGB,                             GL_HALF_FLOAT_OES,                 GL_RGB16F_EXT                     );

    InsertFormatMapping(&map, GL_RGBA,                            GL_UNSIGNED_BYTE,                  GL_RGBA8_OES                      );
    InsertFormatMapping(&map, GL_RGBA,                            GL_UNSIGNED_SHORT_4_4_4_4,         GL_RGBA4                          );
    InsertFormatMapping(&map, GL_RGBA,                            GL_UNSIGNED_SHORT_5_5_5_1,         GL_RGB5_A1                        );
    InsertFormatMapping(&map, GL_RGBA,                            GL_FLOAT,                          GL_RGBA32F_EXT                    );
    InsertFormatMapping(&map, GL_RGBA,                            GL_HALF_FLOAT_OES,                 GL_RGBA16F_EXT                    );

    InsertFormatMapping(&map, GL_BGRA_EXT,                        GL_UNSIGNED_BYTE,                  GL_BGRA8_EXT                      );
    InsertFormatMapping(&map, GL_BGRA_EXT,                        GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, GL_BGRA4_ANGLEX                   );
    InsertFormatMapping(&map, GL_BGRA_EXT,                        GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, GL_BGR5_A1_ANGLEX                 );

    InsertFormatMapping(&map, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    GL_UNSIGNED_BYTE,                  GL_COMPRESSED_RGB_S3TC_DXT1_EXT   );
    InsertFormatMapping(&map, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   GL_UNSIGNED_BYTE,                  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  );
    InsertFormatMapping(&map, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, GL_UNSIGNED_BYTE,                  GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE);
    InsertFormatMapping(&map, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, GL_UNSIGNED_BYTE,                  GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE);

    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,                 GL_UNSIGNED_SHORT,                 GL_DEPTH_COMPONENT16              );
    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,                 GL_UNSIGNED_INT,                   GL_DEPTH_COMPONENT32_OES          );

    InsertFormatMapping(&map, GL_DEPTH_STENCIL_OES,               GL_UNSIGNED_INT_24_8_OES,          GL_DEPTH24_STENCIL8_OES           );

    return map;
}

static const FormatMap &GetES2FormatMap()
{
    static const FormatMap es2FormatMap = BuildES2FormatMap();
    return es2FormatMap;
}

FormatMap BuildES3FormatMap()
{
    FormatMap map;

    //                       | Format               | Type                             | Internal format         |
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_BYTE,                  GL_RGBA8                 );
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_SHORT_4_4_4_4,         GL_RGBA4                 );
    InsertFormatMapping(&map, GL_RGBA,               GL_UNSIGNED_SHORT_5_5_5_1,         GL_RGB5_A1               );
    InsertFormatMapping(&map, GL_RGBA,               GL_FLOAT,                          GL_RGBA32F               );
    InsertFormatMapping(&map, GL_RGBA,               GL_HALF_FLOAT,                     GL_RGBA16F               );

    InsertFormatMapping(&map, GL_RGB,                GL_UNSIGNED_BYTE,                  GL_RGB8                  );
    InsertFormatMapping(&map, GL_RGB,                GL_UNSIGNED_SHORT_5_6_5,           GL_RGB565                );
    InsertFormatMapping(&map, GL_RGB,                GL_FLOAT,                          GL_RGB32F                );
    InsertFormatMapping(&map, GL_RGB,                GL_HALF_FLOAT,                     GL_RGB16F                );

    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_UNSIGNED_BYTE,                  GL_LUMINANCE8_ALPHA8_EXT );
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_UNSIGNED_BYTE,                  GL_LUMINANCE8_EXT        );
    InsertFormatMapping(&map, GL_ALPHA,              GL_UNSIGNED_BYTE,                  GL_ALPHA8_EXT            );
    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_FLOAT,                          GL_LUMINANCE_ALPHA32F_EXT);
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_FLOAT,                          GL_LUMINANCE32F_EXT      );
    InsertFormatMapping(&map, GL_ALPHA,              GL_FLOAT,                          GL_ALPHA32F_EXT          );
    InsertFormatMapping(&map, GL_LUMINANCE_ALPHA,    GL_HALF_FLOAT,                     GL_LUMINANCE_ALPHA16F_EXT);
    InsertFormatMapping(&map, GL_LUMINANCE,          GL_HALF_FLOAT,                     GL_LUMINANCE16F_EXT      );
    InsertFormatMapping(&map, GL_ALPHA,              GL_HALF_FLOAT,                     GL_ALPHA16F_EXT          );

    InsertFormatMapping(&map, GL_BGRA_EXT,           GL_UNSIGNED_BYTE,                  GL_BGRA8_EXT             );
    InsertFormatMapping(&map, GL_BGRA_EXT,           GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, GL_BGRA4_ANGLEX          );
    InsertFormatMapping(&map, GL_BGRA_EXT,           GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, GL_BGR5_A1_ANGLEX        );

    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,    GL_UNSIGNED_SHORT,                 GL_DEPTH_COMPONENT16     );
    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,    GL_UNSIGNED_INT,                   GL_DEPTH_COMPONENT24     );
    InsertFormatMapping(&map, GL_DEPTH_COMPONENT,    GL_FLOAT,                          GL_DEPTH_COMPONENT32F    );

    InsertFormatMapping(&map, GL_DEPTH_STENCIL,      GL_UNSIGNED_INT_24_8,              GL_DEPTH24_STENCIL8      );
    InsertFormatMapping(&map, GL_DEPTH_STENCIL,      GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_DEPTH32F_STENCIL8     );

    return map;
}

struct FormatInfo
{
    GLint mInternalformat;
    GLenum mFormat;
    GLenum mType;

    FormatInfo(GLint internalformat, GLenum format, GLenum type)
        : mInternalformat(internalformat), mFormat(format), mType(type) { }

    bool operator<(const FormatInfo& other) const
    {
        return memcmp(this, &other, sizeof(FormatInfo)) < 0;
    }
};

// ES3 has a specific set of permutations of internal formats, formats and types which are acceptable.
typedef std::set<FormatInfo> ES3FormatSet;

ES3FormatSet BuildES3FormatSet()
{
    ES3FormatSet set;

    // Format combinations from ES 3.0.1 spec, table 3.2

    //                   | Internal format      | Format            | Type                            |
    //                   |                      |                   |                                 |
    set.insert(FormatInfo(GL_RGBA8,              GL_RGBA,            GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGB5_A1,            GL_RGBA,            GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGBA4,              GL_RGBA,            GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_SRGB8_ALPHA8,       GL_RGBA,            GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGBA8_SNORM,        GL_RGBA,            GL_BYTE                          ));
    set.insert(FormatInfo(GL_RGBA4,              GL_RGBA,            GL_UNSIGNED_SHORT_4_4_4_4        ));
    set.insert(FormatInfo(GL_RGB10_A2,           GL_RGBA,            GL_UNSIGNED_INT_2_10_10_10_REV   ));
    set.insert(FormatInfo(GL_RGB5_A1,            GL_RGBA,            GL_UNSIGNED_INT_2_10_10_10_REV   ));
    set.insert(FormatInfo(GL_RGB5_A1,            GL_RGBA,            GL_UNSIGNED_SHORT_5_5_5_1        ));
    set.insert(FormatInfo(GL_RGBA16F,            GL_RGBA,            GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_RGBA32F,            GL_RGBA,            GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RGBA16F,            GL_RGBA,            GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RGBA8UI,            GL_RGBA_INTEGER,    GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGBA8I,             GL_RGBA_INTEGER,    GL_BYTE                          ));
    set.insert(FormatInfo(GL_RGBA16UI,           GL_RGBA_INTEGER,    GL_UNSIGNED_SHORT                ));
    set.insert(FormatInfo(GL_RGBA16I,            GL_RGBA_INTEGER,    GL_SHORT                         ));
    set.insert(FormatInfo(GL_RGBA32UI,           GL_RGBA_INTEGER,    GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_RGBA32I,            GL_RGBA_INTEGER,    GL_INT                           ));
    set.insert(FormatInfo(GL_RGB10_A2UI,         GL_RGBA_INTEGER,    GL_UNSIGNED_INT_2_10_10_10_REV   ));
    set.insert(FormatInfo(GL_RGB8,               GL_RGB,             GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGB565,             GL_RGB,             GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_SRGB8,              GL_RGB,             GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGB8_SNORM,         GL_RGB,             GL_BYTE                          ));
    set.insert(FormatInfo(GL_RGB565,             GL_RGB,             GL_UNSIGNED_SHORT_5_6_5          ));
    set.insert(FormatInfo(GL_R11F_G11F_B10F,     GL_RGB,             GL_UNSIGNED_INT_10F_11F_11F_REV  ));
    set.insert(FormatInfo(GL_RGB9_E5,            GL_RGB,             GL_UNSIGNED_INT_5_9_9_9_REV      ));
    set.insert(FormatInfo(GL_RGB16F,             GL_RGB,             GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_R11F_G11F_B10F,     GL_RGB,             GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_RGB9_E5,            GL_RGB,             GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_RGB32F,             GL_RGB,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RGB16F,             GL_RGB,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_R11F_G11F_B10F,     GL_RGB,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RGB9_E5,            GL_RGB,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RGB8UI,             GL_RGB_INTEGER,     GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGB8I,              GL_RGB_INTEGER,     GL_BYTE                          ));
    set.insert(FormatInfo(GL_RGB16UI,            GL_RGB_INTEGER,     GL_UNSIGNED_SHORT                ));
    set.insert(FormatInfo(GL_RGB16I,             GL_RGB_INTEGER,     GL_SHORT                         ));
    set.insert(FormatInfo(GL_RGB32UI,            GL_RGB_INTEGER,     GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_RGB32I,             GL_RGB_INTEGER,     GL_INT                           ));
    set.insert(FormatInfo(GL_RG8,                GL_RG,              GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RG8_SNORM,          GL_RG,              GL_BYTE                          ));
    set.insert(FormatInfo(GL_RG16F,              GL_RG,              GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_RG32F,              GL_RG,              GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RG16F,              GL_RG,              GL_FLOAT                         ));
    set.insert(FormatInfo(GL_RG8UI,              GL_RG_INTEGER,      GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RG8I,               GL_RG_INTEGER,      GL_BYTE                          ));
    set.insert(FormatInfo(GL_RG16UI,             GL_RG_INTEGER,      GL_UNSIGNED_SHORT                ));
    set.insert(FormatInfo(GL_RG16I,              GL_RG_INTEGER,      GL_SHORT                         ));
    set.insert(FormatInfo(GL_RG32UI,             GL_RG_INTEGER,      GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_RG32I,              GL_RG_INTEGER,      GL_INT                           ));
    set.insert(FormatInfo(GL_R8,                 GL_RED,             GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_R8_SNORM,           GL_RED,             GL_BYTE                          ));
    set.insert(FormatInfo(GL_R16F,               GL_RED,             GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_R32F,               GL_RED,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_R16F,               GL_RED,             GL_FLOAT                         ));
    set.insert(FormatInfo(GL_R8UI,               GL_RED_INTEGER,     GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_R8I,                GL_RED_INTEGER,     GL_BYTE                          ));
    set.insert(FormatInfo(GL_R16UI,              GL_RED_INTEGER,     GL_UNSIGNED_SHORT                ));
    set.insert(FormatInfo(GL_R16I,               GL_RED_INTEGER,     GL_SHORT                         ));
    set.insert(FormatInfo(GL_R32UI,              GL_RED_INTEGER,     GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_R32I,               GL_RED_INTEGER,     GL_INT                           ));

    // Unsized formats
    set.insert(FormatInfo(GL_RGBA,               GL_RGBA,            GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGBA,               GL_RGBA,            GL_UNSIGNED_SHORT_4_4_4_4        ));
    set.insert(FormatInfo(GL_RGBA,               GL_RGBA,            GL_UNSIGNED_SHORT_5_5_5_1        ));
    set.insert(FormatInfo(GL_RGB,                GL_RGB,             GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_RGB,                GL_RGB,             GL_UNSIGNED_SHORT_5_6_5          ));
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_LUMINANCE,          GL_LUMINANCE,       GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_ALPHA,              GL_ALPHA,           GL_UNSIGNED_BYTE                 ));

    // Depth stencil formats
    set.insert(FormatInfo(GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT                ));
    set.insert(FormatInfo(GL_DEPTH_COMPONENT24,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_DEPTH_COMPONENT16,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT                  ));
    set.insert(FormatInfo(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT                         ));
    set.insert(FormatInfo(GL_DEPTH24_STENCIL8,   GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8             ));
    set.insert(FormatInfo(GL_DEPTH32F_STENCIL8,  GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV));

    // From GL_OES_texture_float
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_FLOAT                         ));
    set.insert(FormatInfo(GL_LUMINANCE,          GL_LUMINANCE,       GL_FLOAT                         ));
    set.insert(FormatInfo(GL_ALPHA,              GL_ALPHA,           GL_FLOAT                         ));

    // From GL_OES_texture_half_float
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA,    GL_LUMINANCE_ALPHA, GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_LUMINANCE,          GL_LUMINANCE,       GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_ALPHA,              GL_ALPHA,           GL_HALF_FLOAT                    ));

    // From GL_EXT_texture_storage
    //                   | Internal format          | Format            | Type                            |
    //                   |                          |                   |                                 |
    set.insert(FormatInfo(GL_ALPHA8_EXT,             GL_ALPHA,           GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_LUMINANCE8_EXT,         GL_LUMINANCE,       GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_LUMINANCE8_ALPHA8_EXT,  GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_ALPHA32F_EXT,           GL_ALPHA,           GL_FLOAT                         ));
    set.insert(FormatInfo(GL_LUMINANCE32F_EXT,       GL_LUMINANCE,       GL_FLOAT                         ));
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA32F_EXT, GL_LUMINANCE_ALPHA, GL_FLOAT                         ));
    set.insert(FormatInfo(GL_ALPHA16F_EXT,           GL_ALPHA,           GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_LUMINANCE16F_EXT,       GL_LUMINANCE,       GL_HALF_FLOAT                    ));
    set.insert(FormatInfo(GL_LUMINANCE_ALPHA16F_EXT, GL_LUMINANCE_ALPHA, GL_HALF_FLOAT                    ));

    set.insert(FormatInfo(GL_BGRA8_EXT,              GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_BGRA4_ANGLEX,           GL_BGRA_EXT,        GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT));
    set.insert(FormatInfo(GL_BGRA4_ANGLEX,           GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 ));
    set.insert(FormatInfo(GL_BGR5_A1_ANGLEX,         GL_BGRA_EXT,        GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT));
    set.insert(FormatInfo(GL_BGR5_A1_ANGLEX,         GL_BGRA_EXT,        GL_UNSIGNED_BYTE                 ));

    // From GL_ANGLE_depth_texture
    set.insert(FormatInfo(GL_DEPTH_COMPONENT32_OES,  GL_DEPTH_COMPONENT, GL_UNSIGNED_INT_24_8_OES         ));

    // Compressed formats
    // From ES 3.0.1 spec, table 3.16
    //                   | Internal format                             | Format                                      | Type           |
    //                   |                                             |                                             |                |
    set.insert(FormatInfo(GL_COMPRESSED_R11_EAC,                        GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_R11_EAC,                        GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_SIGNED_R11_EAC,                 GL_COMPRESSED_SIGNED_R11_EAC,                 GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_RG11_EAC,                       GL_COMPRESSED_RG11_EAC,                       GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_SIGNED_RG11_EAC,                GL_COMPRESSED_SIGNED_RG11_EAC,                GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_RGB8_ETC2,                      GL_COMPRESSED_RGB8_ETC2,                      GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_SRGB8_ETC2,                     GL_COMPRESSED_SRGB8_ETC2,                     GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_UNSIGNED_BYTE));


    // From GL_EXT_texture_compression_dxt1
    set.insert(FormatInfo(GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              GL_UNSIGNED_BYTE));
    set.insert(FormatInfo(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             GL_UNSIGNED_BYTE));

    // From GL_ANGLE_texture_compression_dxt3
    set.insert(FormatInfo(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,           GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,           GL_UNSIGNED_BYTE));

    // From GL_ANGLE_texture_compression_dxt5
    set.insert(FormatInfo(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,           GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,           GL_UNSIGNED_BYTE));

    return set;
}

static const ES3FormatSet &GetES3FormatSet()
{
    static const ES3FormatSet es3FormatSet = BuildES3FormatSet();
    return es3FormatSet;
}

// Map of sizes of input types
struct TypeInfo
{
    GLuint mTypeBytes;
    bool mSpecialInterpretation;

    TypeInfo()
        : mTypeBytes(0), mSpecialInterpretation(false) { }

    TypeInfo(GLuint typeBytes, bool specialInterpretation)
        : mTypeBytes(typeBytes), mSpecialInterpretation(specialInterpretation) { }

    bool operator<(const TypeInfo& other) const
    {
        return memcmp(this, &other, sizeof(TypeInfo)) < 0;
    }
};

typedef std::pair<GLenum, TypeInfo> TypeInfoPair;
typedef std::map<GLenum, TypeInfo> TypeInfoMap;

static TypeInfoMap BuildTypeInfoMap()
{
    TypeInfoMap map;

    map.insert(TypeInfoPair(GL_UNSIGNED_BYTE,                  TypeInfo( 1, false)));
    map.insert(TypeInfoPair(GL_BYTE,                           TypeInfo( 1, false)));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT,                 TypeInfo( 2, false)));
    map.insert(TypeInfoPair(GL_SHORT,                          TypeInfo( 2, false)));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT,                   TypeInfo( 4, false)));
    map.insert(TypeInfoPair(GL_INT,                            TypeInfo( 4, false)));
    map.insert(TypeInfoPair(GL_HALF_FLOAT,                     TypeInfo( 2, false)));
    map.insert(TypeInfoPair(GL_FLOAT,                          TypeInfo( 4, false)));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT_5_6_5,           TypeInfo( 2, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT_4_4_4_4,         TypeInfo( 2, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT_5_5_5_1,         TypeInfo( 2, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, TypeInfo( 2, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, TypeInfo( 2, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT_2_10_10_10_REV,    TypeInfo( 4, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT_24_8,              TypeInfo( 4, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT_10F_11F_11F_REV,   TypeInfo( 4, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT_5_9_9_9_REV,       TypeInfo( 4, true )));
    map.insert(TypeInfoPair(GL_FLOAT_32_UNSIGNED_INT_24_8_REV, TypeInfo( 4, true )));
    map.insert(TypeInfoPair(GL_UNSIGNED_INT_24_8_OES,          TypeInfo( 4, true )));

    return map;
}

static bool GetTypeInfo(GLenum type, TypeInfo *outTypeInfo)
{
    static const TypeInfoMap infoMap = BuildTypeInfoMap();
    TypeInfoMap::const_iterator iter = infoMap.find(type);
    if (iter != infoMap.end())
    {
        if (outTypeInfo)
        {
            *outTypeInfo = iter->second;
        }
        return true;
    }
    else
    {
        return false;
    }
}

// Information about internal formats
typedef bool ((Context::*ContextSupportCheckMemberFunction)(void) const);
typedef bool (*ContextSupportCheckFunction)(const Context *context);

typedef bool ((rx::Renderer::*RendererSupportCheckMemberFunction)(void) const);
typedef bool (*ContextRendererSupportCheckFunction)(const Context *context, const rx::Renderer *renderer);

template <ContextSupportCheckMemberFunction func>
bool CheckSupport(const Context *context)
{
    return (context->*func)();
}

template <ContextSupportCheckMemberFunction contextFunc, RendererSupportCheckMemberFunction rendererFunc>
bool CheckSupport(const Context *context, const rx::Renderer *renderer)
{
    if (context)
    {
        return (context->*contextFunc)();
    }
    else if (renderer)
    {
        return (renderer->*rendererFunc)();
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

template <typename objectType>
bool AlwaysSupported(const objectType*)
{
    return true;
}

template <typename objectTypeA, typename objectTypeB>
bool AlwaysSupported(const objectTypeA*, const objectTypeB*)
{
    return true;
}

template <typename objectType>
bool NeverSupported(const objectType*)
{
    return false;
}

template <typename objectTypeA, typename objectTypeB>
bool NeverSupported(const objectTypeA *, const objectTypeB *)
{
    return false;
}

template <typename objectType>
bool UnimplementedSupport(const objectType*)
{
    UNIMPLEMENTED();
    return false;
}

template <typename objectTypeA, typename objectTypeB>
bool UnimplementedSupport(const objectTypeA*, const objectTypeB*)
{
    UNIMPLEMENTED();
    return false;
}

enum InternalFormatStorageType
{
    Unknown,
    NormalizedFixedPoint,
    FloatingPoint,
    SignedInteger,
    UnsignedInteger,
    Compressed,
};

struct InternalFormatInfo
{
    GLuint mRedBits;
    GLuint mGreenBits;
    GLuint mBlueBits;

    GLuint mLuminanceBits;

    GLuint mAlphaBits;
    GLuint mSharedBits;

    GLuint mDepthBits;
    GLuint mStencilBits;

    GLuint mPixelBits;

    GLuint mComponentCount;

    GLuint mCompressedBlockWidth;
    GLuint mCompressedBlockHeight;

    GLenum mFormat;
    GLenum mType;

    InternalFormatStorageType mStorageType;

    bool mIsSRGB;

    ContextRendererSupportCheckFunction mIsColorRenderable;
    ContextRendererSupportCheckFunction mIsDepthRenderable;
    ContextRendererSupportCheckFunction mIsStencilRenderable;
    ContextRendererSupportCheckFunction mIsTextureFilterable;

    ContextSupportCheckFunction mSupportFunction;

    InternalFormatInfo() : mRedBits(0), mGreenBits(0), mBlueBits(0), mLuminanceBits(0), mAlphaBits(0), mSharedBits(0), mDepthBits(0), mStencilBits(0),
                           mPixelBits(0), mComponentCount(0), mCompressedBlockWidth(0), mCompressedBlockHeight(0), mFormat(GL_NONE), mType(GL_NONE),
                           mStorageType(Unknown), mIsSRGB(false), mIsColorRenderable(NeverSupported), mIsDepthRenderable(NeverSupported), mIsStencilRenderable(NeverSupported),
                           mIsTextureFilterable(NeverSupported), mSupportFunction(NeverSupported)
    {
    }

    static InternalFormatInfo UnsizedFormat(GLenum format, ContextSupportCheckFunction supportFunction)
    {
        InternalFormatInfo formatInfo;
        formatInfo.mFormat = format;
        formatInfo.mSupportFunction = supportFunction;
        return formatInfo;
    }

    static InternalFormatInfo RGBAFormat(GLuint red, GLuint green, GLuint blue, GLuint alpha, GLuint shared,
                                         GLenum format, GLenum type, InternalFormatStorageType storageType, bool srgb,
                                         ContextRendererSupportCheckFunction colorRenderable,
                                         ContextRendererSupportCheckFunction textureFilterable,
                                         ContextSupportCheckFunction supportFunction)
    {
        InternalFormatInfo formatInfo;
        formatInfo.mRedBits = red;
        formatInfo.mGreenBits = green;
        formatInfo.mBlueBits = blue;
        formatInfo.mAlphaBits = alpha;
        formatInfo.mSharedBits = shared;
        formatInfo.mPixelBits = red + green + blue + alpha + shared;
        formatInfo.mComponentCount = ((red > 0) ? 1 : 0) + ((green > 0) ? 1 : 0) + ((blue > 0) ? 1 : 0) + ((alpha > 0) ? 1 : 0);
        formatInfo.mFormat = format;
        formatInfo.mType = type;
        formatInfo.mStorageType = storageType;
        formatInfo.mIsSRGB = srgb;
        formatInfo.mIsColorRenderable = colorRenderable;
        formatInfo.mIsTextureFilterable = textureFilterable;
        formatInfo.mSupportFunction = supportFunction;
        return formatInfo;
    }

    static InternalFormatInfo LUMAFormat(GLuint luminance, GLuint alpha, GLenum format, GLenum type,
                                         InternalFormatStorageType storageType,
                                         ContextSupportCheckFunction supportFunction)
    {
        InternalFormatInfo formatInfo;
        formatInfo.mLuminanceBits = luminance;
        formatInfo.mAlphaBits = alpha;
        formatInfo.mPixelBits = luminance + alpha;
        formatInfo.mComponentCount = ((luminance > 0) ? 1 : 0) + ((alpha > 0) ? 1 : 0);
        formatInfo.mFormat = format;
        formatInfo.mType = type;
        formatInfo.mStorageType = storageType;
        formatInfo.mIsTextureFilterable = AlwaysSupported;
        formatInfo.mSupportFunction = supportFunction;
        return formatInfo;
    }

    static InternalFormatInfo DepthStencilFormat(GLuint depth, GLuint stencil, GLenum format, GLenum type,
                                                 InternalFormatStorageType storageType,
                                                 ContextRendererSupportCheckFunction depthRenderable,
                                                 ContextRendererSupportCheckFunction stencilRenderable,
                                                 ContextSupportCheckFunction supportFunction)
    {
        InternalFormatInfo formatInfo;
        formatInfo.mDepthBits = depth;
        formatInfo.mStencilBits = stencil;
        formatInfo.mPixelBits = depth + stencil;
        formatInfo.mComponentCount = ((depth > 0) ? 1 : 0) + ((stencil > 0) ? 1 : 0);
        formatInfo.mFormat = format;
        formatInfo.mType = type;
        formatInfo.mStorageType = storageType;
        formatInfo.mIsDepthRenderable = depthRenderable;
        formatInfo.mIsStencilRenderable = stencilRenderable;
        formatInfo.mSupportFunction = supportFunction;
        return formatInfo;
    }

    static InternalFormatInfo CompressedFormat(GLuint compressedBlockWidth, GLuint compressedBlockHeight,
                                               GLuint compressedBlockSize, GLuint componentCount, GLenum format, GLenum type,
                                               ContextSupportCheckFunction supportFunction)
    {
        InternalFormatInfo formatInfo;
        formatInfo.mCompressedBlockWidth = compressedBlockWidth;
        formatInfo.mCompressedBlockHeight = compressedBlockHeight;
        formatInfo.mPixelBits = compressedBlockSize;
        formatInfo.mComponentCount = componentCount;
        formatInfo.mFormat = format;
        formatInfo.mType = type;
        formatInfo.mStorageType = Compressed;
        formatInfo.mIsTextureFilterable = AlwaysSupported;
        formatInfo.mSupportFunction = supportFunction;
        return formatInfo;
    }
};

typedef std::pair<GLuint, InternalFormatInfo> InternalFormatInfoPair;
typedef std::map<GLuint, InternalFormatInfo> InternalFormatInfoMap;

static InternalFormatInfoMap BuildES3InternalFormatInfoMap()
{
    InternalFormatInfoMap map;

    // From ES 3.0.1 spec, table 3.12
    map.insert(InternalFormatInfoPair(GL_NONE,              InternalFormatInfo()));

    //                               | Internal format     |                              | R | G | B | A |S | Format         | Type                           | Internal format     | SRGB | Color          | Texture        | Supported          |
    //                               |                     |                              |   |   |   |   |  |                |                                | type                |      | renderable     | filterable     |                    |
    map.insert(InternalFormatInfoPair(GL_R8,                InternalFormatInfo::RGBAFormat( 8,  0,  0,  0, 0, GL_RED,          GL_UNSIGNED_BYTE,                NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_R8_SNORM,          InternalFormatInfo::RGBAFormat( 8,  0,  0,  0, 0, GL_RED,          GL_BYTE,                         NormalizedFixedPoint, false, NeverSupported,  AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RG8,               InternalFormatInfo::RGBAFormat( 8,  8,  0,  0, 0, GL_RG,           GL_UNSIGNED_BYTE,                NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RG8_SNORM,         InternalFormatInfo::RGBAFormat( 8,  8,  0,  0, 0, GL_RG,           GL_BYTE,                         NormalizedFixedPoint, false, NeverSupported,  AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB8,              InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB,          GL_UNSIGNED_BYTE,                NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB8_SNORM,        InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB,          GL_BYTE,                         NormalizedFixedPoint, false, NeverSupported,  AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB565,            InternalFormatInfo::RGBAFormat( 5,  6,  5,  0, 0, GL_RGB,          GL_UNSIGNED_SHORT_5_5_5_1,       NormalizedFixedPoint, false, NeverSupported,  AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA4,             InternalFormatInfo::RGBAFormat( 4,  4,  4,  4, 0, GL_RGBA,         GL_UNSIGNED_SHORT_4_4_4_4,       NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB5_A1,           InternalFormatInfo::RGBAFormat( 5,  5,  5,  1, 0, GL_RGBA,         GL_UNSIGNED_SHORT_5_5_5_1,       NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA8,             InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA,         GL_UNSIGNED_BYTE,                NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA8_SNORM,       InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA,         GL_BYTE,                         NormalizedFixedPoint, false, NeverSupported,  AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB10_A2,          InternalFormatInfo::RGBAFormat(10, 10, 10,  2, 0, GL_RGBA,         GL_UNSIGNED_INT_2_10_10_10_REV,  NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB10_A2UI,        InternalFormatInfo::RGBAFormat(10, 10, 10,  2, 0, GL_RGBA,         GL_UNSIGNED_INT_2_10_10_10_REV,  UnsignedInteger,      false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_SRGB8,             InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB,          GL_UNSIGNED_BYTE,                NormalizedFixedPoint, true,  NeverSupported,  AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_SRGB8_ALPHA8,      InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA,         GL_UNSIGNED_BYTE,                NormalizedFixedPoint, true,  AlwaysSupported, AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_R11F_G11F_B10F,    InternalFormatInfo::RGBAFormat(11, 11, 10,  0, 0, GL_RGB,          GL_UNSIGNED_INT_10F_11F_11F_REV, FloatingPoint,        false, NeverSupported,  AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB9_E5,           InternalFormatInfo::RGBAFormat( 9,  9,  9,  0, 5, GL_RGB,          GL_UNSIGNED_INT_5_9_9_9_REV,     FloatingPoint,        false, NeverSupported,  AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_R8I,               InternalFormatInfo::RGBAFormat( 8,  0,  0,  0, 0, GL_RED_INTEGER,  GL_BYTE,                         SignedInteger,        false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_R8UI,              InternalFormatInfo::RGBAFormat( 8,  0,  0,  0, 0, GL_RED_INTEGER,  GL_UNSIGNED_BYTE,                UnsignedInteger,      false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_R16I,              InternalFormatInfo::RGBAFormat(16,  0,  0,  0, 0, GL_RED_INTEGER,  GL_SHORT,                        SignedInteger,        false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_R16UI,             InternalFormatInfo::RGBAFormat(16,  0,  0,  0, 0, GL_RED_INTEGER,  GL_UNSIGNED_SHORT,               UnsignedInteger,      false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_R32I,              InternalFormatInfo::RGBAFormat(32,  0,  0,  0, 0, GL_RED_INTEGER,  GL_INT,                          SignedInteger,        false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_R32UI,             InternalFormatInfo::RGBAFormat(32,  0,  0,  0, 0, GL_RED_INTEGER,  GL_UNSIGNED_INT,                 UnsignedInteger,      false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RG8I,              InternalFormatInfo::RGBAFormat( 8,  8,  0,  0, 0, GL_RG_INTEGER,   GL_BYTE,                         SignedInteger,        false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RG8UI,             InternalFormatInfo::RGBAFormat( 8,  8,  0,  0, 0, GL_RG_INTEGER,   GL_UNSIGNED_BYTE,                UnsignedInteger,      false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RG16I,             InternalFormatInfo::RGBAFormat(16, 16,  0,  0, 0, GL_RG_INTEGER,   GL_SHORT,                        SignedInteger,        false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RG16UI,            InternalFormatInfo::RGBAFormat(16, 16,  0,  0, 0, GL_RG_INTEGER,   GL_UNSIGNED_SHORT,               UnsignedInteger,      false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RG32I,             InternalFormatInfo::RGBAFormat(32, 32,  0,  0, 0, GL_RG_INTEGER,   GL_INT,                          SignedInteger,        false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RG32UI,            InternalFormatInfo::RGBAFormat(32, 32,  0,  0, 0, GL_RG_INTEGER,   GL_UNSIGNED_INT,                 UnsignedInteger,      false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB8I,             InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB_INTEGER,  GL_BYTE,                         SignedInteger,        false, NeverSupported,  NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB8UI,            InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB_INTEGER,  GL_UNSIGNED_BYTE,                UnsignedInteger,      false, NeverSupported,  NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB16I,            InternalFormatInfo::RGBAFormat(16, 16, 16,  0, 0, GL_RGB_INTEGER,  GL_SHORT,                        SignedInteger,        false, NeverSupported,  NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB16UI,           InternalFormatInfo::RGBAFormat(16, 16, 16,  0, 0, GL_RGB_INTEGER,  GL_UNSIGNED_SHORT,               UnsignedInteger,      false, NeverSupported,  NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB32I,            InternalFormatInfo::RGBAFormat(32, 32, 32,  0, 0, GL_RGB_INTEGER,  GL_INT,                          SignedInteger,        false, NeverSupported,  NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB32UI,           InternalFormatInfo::RGBAFormat(32, 32, 32,  0, 0, GL_RGB_INTEGER,  GL_UNSIGNED_INT,                 UnsignedInteger,      false, NeverSupported,  NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA8I,            InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA_INTEGER, GL_BYTE,                         SignedInteger,        false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA8UI,           InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE,                UnsignedInteger,      false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA16I,           InternalFormatInfo::RGBAFormat(16, 16, 16, 16, 0, GL_RGBA_INTEGER, GL_SHORT,                        SignedInteger,        false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA16UI,          InternalFormatInfo::RGBAFormat(16, 16, 16, 16, 0, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT,               UnsignedInteger,      false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA32I,           InternalFormatInfo::RGBAFormat(32, 32, 32, 32, 0, GL_RGBA_INTEGER, GL_INT,                          SignedInteger,        false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA32UI,          InternalFormatInfo::RGBAFormat(32, 32, 32, 32, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT,                 UnsignedInteger,      false, AlwaysSupported, NeverSupported,  AlwaysSupported     )));

    map.insert(InternalFormatInfoPair(GL_BGRA8_EXT,         InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_BGRA_EXT,     GL_UNSIGNED_BYTE,                  NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_BGRA4_ANGLEX,      InternalFormatInfo::RGBAFormat( 4,  4,  4,  4, 0, GL_BGRA_EXT,     GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_BGR5_A1_ANGLEX,    InternalFormatInfo::RGBAFormat( 5,  5,  5,  1, 0, GL_BGRA_EXT,     GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported     )));

    // Floating point renderability and filtering is provided by OES_texture_float and OES_texture_half_float
    //                               | Internal format        |                                   | D |S | Format             | Type                           | Internal fmt | SRGB | Color                                                                                                     | Texture                                                                                             | Supported           |
    //                               |                        |                                   |   |  |                    |                                | type         |      | renderable                                                                                                          | filterable                                                                                          |                     |
    map.insert(InternalFormatInfoPair(GL_R16F,              InternalFormatInfo::RGBAFormat(16,  0,  0,  0, 0, GL_RED,          GL_HALF_FLOAT,                   FloatingPoint, false, CheckSupport<&Context::supportsFloat16RenderableTextures, &rx::Renderer::getFloat16TextureRenderingSupport>, CheckSupport<&Context::supportsFloat16LinearFilter, &rx::Renderer::getFloat16TextureFilteringSupport>, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RG16F,             InternalFormatInfo::RGBAFormat(16, 16,  0,  0, 0, GL_RG,           GL_HALF_FLOAT,                   FloatingPoint, false, CheckSupport<&Context::supportsFloat16RenderableTextures, &rx::Renderer::getFloat16TextureRenderingSupport>, CheckSupport<&Context::supportsFloat16LinearFilter, &rx::Renderer::getFloat16TextureFilteringSupport>, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB16F,            InternalFormatInfo::RGBAFormat(16, 16, 16,  0, 0, GL_RGB,          GL_HALF_FLOAT,                   FloatingPoint, false, CheckSupport<&Context::supportsFloat16RenderableTextures, &rx::Renderer::getFloat16TextureRenderingSupport>, CheckSupport<&Context::supportsFloat16LinearFilter, &rx::Renderer::getFloat16TextureFilteringSupport>, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA16F,           InternalFormatInfo::RGBAFormat(16, 16, 16, 16, 0, GL_RGBA,         GL_HALF_FLOAT,                   FloatingPoint, false, CheckSupport<&Context::supportsFloat16RenderableTextures, &rx::Renderer::getFloat16TextureRenderingSupport>, CheckSupport<&Context::supportsFloat16LinearFilter, &rx::Renderer::getFloat16TextureFilteringSupport>, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_R32F,              InternalFormatInfo::RGBAFormat(32,  0,  0,  0, 0, GL_RED,          GL_FLOAT,                        FloatingPoint, false, CheckSupport<&Context::supportsFloat32RenderableTextures, &rx::Renderer::getFloat32TextureRenderingSupport>, CheckSupport<&Context::supportsFloat32LinearFilter, &rx::Renderer::getFloat32TextureFilteringSupport>, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RG32F,             InternalFormatInfo::RGBAFormat(32, 32,  0,  0, 0, GL_RG,           GL_FLOAT,                        FloatingPoint, false, CheckSupport<&Context::supportsFloat32RenderableTextures, &rx::Renderer::getFloat32TextureRenderingSupport>, CheckSupport<&Context::supportsFloat32LinearFilter, &rx::Renderer::getFloat32TextureFilteringSupport>, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGB32F,            InternalFormatInfo::RGBAFormat(32, 32, 32,  0, 0, GL_RGB,          GL_FLOAT,                        FloatingPoint, false, CheckSupport<&Context::supportsFloat32RenderableTextures, &rx::Renderer::getFloat32TextureRenderingSupport>, CheckSupport<&Context::supportsFloat32LinearFilter, &rx::Renderer::getFloat32TextureFilteringSupport>, AlwaysSupported     )));
    map.insert(InternalFormatInfoPair(GL_RGBA32F,           InternalFormatInfo::RGBAFormat(32, 32, 32, 32, 0, GL_RGBA,         GL_FLOAT,                        FloatingPoint, false, CheckSupport<&Context::supportsFloat32RenderableTextures, &rx::Renderer::getFloat32TextureRenderingSupport>, CheckSupport<&Context::supportsFloat32LinearFilter, &rx::Renderer::getFloat32TextureFilteringSupport>, AlwaysSupported     )));

    // Depth stencil formats
    //                               | Internal format        |                                   | D |S | Format             | Type                             | Internal format     | Color          | Texture        | Supported     |
    //                               |                        |                                   |   |  |                    |                                  | type                | renderable     | filterable     |               |
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT16, InternalFormatInfo::DepthStencilFormat(16, 0, GL_DEPTH_COMPONENT,  GL_UNSIGNED_SHORT,                 NormalizedFixedPoint, AlwaysSupported, NeverSupported,  AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT24, InternalFormatInfo::DepthStencilFormat(24, 0, GL_DEPTH_COMPONENT,  GL_UNSIGNED_INT,                   NormalizedFixedPoint, AlwaysSupported, NeverSupported,  AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT32F,InternalFormatInfo::DepthStencilFormat(32, 0, GL_DEPTH_COMPONENT,  GL_FLOAT,                          FloatingPoint,        AlwaysSupported, NeverSupported,  AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_DEPTH24_STENCIL8,  InternalFormatInfo::DepthStencilFormat(24, 8, GL_DEPTH_STENCIL,    GL_UNSIGNED_INT_24_8,              NormalizedFixedPoint, AlwaysSupported, AlwaysSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_DEPTH32F_STENCIL8, InternalFormatInfo::DepthStencilFormat(32, 8, GL_DEPTH_STENCIL,    GL_FLOAT_32_UNSIGNED_INT_24_8_REV, FloatingPoint,        AlwaysSupported, AlwaysSupported, AlwaysSupported)));

    // Luminance alpha formats
    //                               | Internal format          |                              | L | A | Format            | Type            | Internal format     | Supported     |
    //                               |                          |                              |   |   |                   |                 | type                |               |
    map.insert(InternalFormatInfoPair(GL_ALPHA8_EXT,             InternalFormatInfo::LUMAFormat( 0,  8, GL_ALPHA,           GL_UNSIGNED_BYTE, NormalizedFixedPoint, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE8_EXT,         InternalFormatInfo::LUMAFormat( 8,  0, GL_LUMINANCE,       GL_UNSIGNED_BYTE, NormalizedFixedPoint, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_ALPHA32F_EXT,           InternalFormatInfo::LUMAFormat( 0, 32, GL_ALPHA,           GL_FLOAT,         FloatingPoint,        AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE32F_EXT,       InternalFormatInfo::LUMAFormat(32,  0, GL_LUMINANCE,       GL_FLOAT,         FloatingPoint,        AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_ALPHA16F_EXT,           InternalFormatInfo::LUMAFormat( 0, 16, GL_ALPHA,           GL_HALF_FLOAT,    FloatingPoint,        AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE16F_EXT,       InternalFormatInfo::LUMAFormat(16,  0, GL_LUMINANCE,       GL_HALF_FLOAT,    FloatingPoint,        AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE8_ALPHA8_EXT,  InternalFormatInfo::LUMAFormat( 8,  8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, NormalizedFixedPoint, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA32F_EXT, InternalFormatInfo::LUMAFormat(32, 32, GL_LUMINANCE_ALPHA, GL_FLOAT,         FloatingPoint,        AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA16F_EXT, InternalFormatInfo::LUMAFormat(16, 16, GL_LUMINANCE_ALPHA, GL_HALF_FLOAT,    FloatingPoint,        AlwaysSupported)));

    // Unsized formats
    //                               | Internal format   |                                 | Format            | Supported     |
    //                               |                   |                                 |                   |               |
    map.insert(InternalFormatInfoPair(GL_ALPHA,           InternalFormatInfo::UnsizedFormat(GL_ALPHA,           AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE,       InternalFormatInfo::UnsizedFormat(GL_LUMINANCE,       AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA, InternalFormatInfo::UnsizedFormat(GL_LUMINANCE_ALPHA, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB,             InternalFormatInfo::UnsizedFormat(GL_RGB,             AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA,            InternalFormatInfo::UnsizedFormat(GL_RGBA,            AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_BGRA_EXT,        InternalFormatInfo::UnsizedFormat(GL_BGRA_EXT,        AlwaysSupported)));

    // Compressed formats, From ES 3.0.1 spec, table 3.16
    //                               | Internal format                             |                                    |W |H | B  |C | Format                                      | Type            | Supported          |
    //                               |                                             |                                    |  |  | S  |C |                                             |                 |                    |
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_R11_EAC,                        InternalFormatInfo::CompressedFormat(4, 4,  64, 1, GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SIGNED_R11_EAC,                 InternalFormatInfo::CompressedFormat(4, 4,  64, 1, GL_COMPRESSED_SIGNED_R11_EAC,                 GL_UNSIGNED_BYTE, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RG11_EAC,                       InternalFormatInfo::CompressedFormat(4, 4, 128, 2, GL_COMPRESSED_RG11_EAC,                       GL_UNSIGNED_BYTE, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SIGNED_RG11_EAC,                InternalFormatInfo::CompressedFormat(4, 4, 128, 2, GL_COMPRESSED_SIGNED_RG11_EAC,                GL_UNSIGNED_BYTE, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGB8_ETC2,                      InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_RGB8_ETC2,                      GL_UNSIGNED_BYTE, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SRGB8_ETC2,                     InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_SRGB8_ETC2,                     GL_UNSIGNED_BYTE, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_UNSIGNED_BYTE, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_UNSIGNED_BYTE, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA8_ETC2_EAC,                 InternalFormatInfo::CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_UNSIGNED_BYTE, UnimplementedSupport)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          InternalFormatInfo::CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_UNSIGNED_BYTE, UnimplementedSupport)));

    // From GL_EXT_texture_compression_dxt1
    //                               | Internal format                   |                                    |W |H | B  |C | Format                            | Type            | Supported     |
    //                               |                                   |                                    |  |  | S  |C |                                   |                 |               |
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    GL_UNSIGNED_BYTE, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   InternalFormatInfo::CompressedFormat(4, 4,  64, 4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   GL_UNSIGNED_BYTE, AlwaysSupported)));

    // From GL_ANGLE_texture_compression_dxt3
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, InternalFormatInfo::CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, GL_UNSIGNED_BYTE, AlwaysSupported)));

    // From GL_ANGLE_texture_compression_dxt5
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, InternalFormatInfo::CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, GL_UNSIGNED_BYTE, AlwaysSupported)));

    return map;
}

static InternalFormatInfoMap BuildES2InternalFormatInfoMap()
{
    InternalFormatInfoMap map;

    // From ES 2.0.25 table 4.5
    map.insert(InternalFormatInfoPair(GL_NONE,                 InternalFormatInfo()));

    //                               | Internal format        |                              | R | G | B | A |S | Format          | Type                     | Internal format     | SRGB | Color         | Texture        | Supported      |
    //                               |                        |                              |   |   |   |   |  |                 |                          | type                |      | renderable    | filterable     |                |
    map.insert(InternalFormatInfoPair(GL_RGBA4,                InternalFormatInfo::RGBAFormat( 4,  4,  4,  4, 0, GL_RGBA,          GL_UNSIGNED_SHORT_4_4_4_4, NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB5_A1,              InternalFormatInfo::RGBAFormat( 5,  5,  5,  1, 0, GL_RGBA,          GL_UNSIGNED_SHORT_5_5_5_1, NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB565,               InternalFormatInfo::RGBAFormat( 5,  6,  5,  0, 0, GL_RGB,           GL_UNSIGNED_SHORT_5_6_5,   NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported)));

    // Extension formats
    map.insert(InternalFormatInfoPair(GL_RGB8_OES,             InternalFormatInfo::RGBAFormat( 8,  8,  8,  0, 0, GL_RGB,           GL_UNSIGNED_BYTE,          NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA8_OES,            InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_RGBA,          GL_UNSIGNED_BYTE,          NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_BGRA8_EXT,            InternalFormatInfo::RGBAFormat( 8,  8,  8,  8, 0, GL_BGRA_EXT,      GL_UNSIGNED_BYTE,          NormalizedFixedPoint, false, AlwaysSupported, AlwaysSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_BGRA4_ANGLEX,         InternalFormatInfo::RGBAFormat( 4,  4,  4,  4, 0, GL_BGRA_EXT,      GL_UNSIGNED_SHORT_4_4_4_4, NormalizedFixedPoint, false, NeverSupported,  AlwaysSupported, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_BGR5_A1_ANGLEX,       InternalFormatInfo::RGBAFormat( 5,  5,  5,  1, 0, GL_BGRA_EXT,      GL_UNSIGNED_SHORT_5_5_5_1, NormalizedFixedPoint, false, NeverSupported,  AlwaysSupported, AlwaysSupported)));

    // Floating point formats have to query the renderer for support
    //                               | Internal format        |                              | R | G | B | A |S | Format          | Type                     | Internal fmt | SRGB | Color                                                                                                      | Texture                                                                                              | Supported                                     |
    //                               |                        |                              |   |   |   |   |  |                 |                          | type         |      | renderable                                                                                                 | filterable                                                                                           |                                               |
    map.insert(InternalFormatInfoPair(GL_RGB16F_EXT,           InternalFormatInfo::RGBAFormat(16, 16, 16,  0, 0, GL_RGB,           GL_HALF_FLOAT_OES,         FloatingPoint, false, CheckSupport<&Context::supportsFloat16RenderableTextures, &rx::Renderer::getFloat16TextureRenderingSupport>, CheckSupport<&Context::supportsFloat16LinearFilter, &rx::Renderer::getFloat16TextureFilteringSupport>, CheckSupport<&Context::supportsFloat16Textures>)));
    map.insert(InternalFormatInfoPair(GL_RGB32F_EXT,           InternalFormatInfo::RGBAFormat(32, 32, 32,  0, 0, GL_RGB,           GL_FLOAT,                  FloatingPoint, false, CheckSupport<&Context::supportsFloat32RenderableTextures, &rx::Renderer::getFloat32TextureRenderingSupport>, CheckSupport<&Context::supportsFloat32LinearFilter, &rx::Renderer::getFloat32TextureFilteringSupport>, CheckSupport<&Context::supportsFloat32Textures>)));
    map.insert(InternalFormatInfoPair(GL_RGBA16F_EXT,          InternalFormatInfo::RGBAFormat(16, 16, 16, 16, 0, GL_RGBA,          GL_HALF_FLOAT_OES,         FloatingPoint, false, CheckSupport<&Context::supportsFloat16RenderableTextures, &rx::Renderer::getFloat16TextureRenderingSupport>, CheckSupport<&Context::supportsFloat16LinearFilter, &rx::Renderer::getFloat16TextureFilteringSupport>, CheckSupport<&Context::supportsFloat16Textures>)));
    map.insert(InternalFormatInfoPair(GL_RGBA32F_EXT,          InternalFormatInfo::RGBAFormat(32, 32, 32, 32, 0, GL_RGBA,          GL_FLOAT,                  FloatingPoint, false, CheckSupport<&Context::supportsFloat32RenderableTextures, &rx::Renderer::getFloat32TextureRenderingSupport>, CheckSupport<&Context::supportsFloat32LinearFilter, &rx::Renderer::getFloat32TextureFilteringSupport>, CheckSupport<&Context::supportsFloat32Textures>)));

    // Depth and stencil formats
    //                               | Internal format        |                                      | D |S | Format              | Type                     | Internal format     | Color          | Texture         | Supported                                  |
    //                               |                        |                                      |   |  |                     |                          | type                | renderable     | filterable      |                                            |
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT32_OES,InternalFormatInfo::DepthStencilFormat(32, 0, GL_DEPTH_COMPONENT,   GL_UNSIGNED_INT,           NormalizedFixedPoint, AlwaysSupported, NeverSupported,  CheckSupport<&Context::supportsDepthTextures>)));
    map.insert(InternalFormatInfoPair(GL_DEPTH24_STENCIL8_OES, InternalFormatInfo::DepthStencilFormat(24, 8, GL_DEPTH_STENCIL_OES, GL_UNSIGNED_INT_24_8_OES,  NormalizedFixedPoint, AlwaysSupported, AlwaysSupported, CheckSupport<&Context::supportsDepthTextures>)));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT16,    InternalFormatInfo::DepthStencilFormat(16, 0, GL_DEPTH_COMPONENT,   GL_UNSIGNED_SHORT,         NormalizedFixedPoint, AlwaysSupported, NeverSupported,  AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_STENCIL_INDEX8,       InternalFormatInfo::DepthStencilFormat( 0, 8, GL_DEPTH_STENCIL_OES, GL_UNSIGNED_BYTE,          NormalizedFixedPoint, NeverSupported,  AlwaysSupported, AlwaysSupported)));

    // Unsized formats
    //                               | Internal format        |                                 | Format              | Supported     |
    //                               |                        |                                 |                     |               |
    map.insert(InternalFormatInfoPair(GL_ALPHA,                InternalFormatInfo::UnsizedFormat(GL_ALPHA,             AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE,            InternalFormatInfo::UnsizedFormat(GL_LUMINANCE,         AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA,      InternalFormatInfo::UnsizedFormat(GL_LUMINANCE_ALPHA,   AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGB,                  InternalFormatInfo::UnsizedFormat(GL_RGB,               AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_RGBA,                 InternalFormatInfo::UnsizedFormat(GL_RGBA,              AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_BGRA_EXT,             InternalFormatInfo::UnsizedFormat(GL_BGRA_EXT,          AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_DEPTH_COMPONENT,      InternalFormatInfo::UnsizedFormat(GL_DEPTH_COMPONENT,   AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_DEPTH_STENCIL_OES,    InternalFormatInfo::UnsizedFormat(GL_DEPTH_STENCIL_OES, AlwaysSupported)));

    // Luminance alpha formats from GL_EXT_texture_storage
    //                               | Internal format        |                                | L | A | Format                   | Type                     | Internal format     | Supported     |
    //                               |                        |                                |   |   |                          |                          | type                |               |
    map.insert(InternalFormatInfoPair(GL_ALPHA8_EXT,             InternalFormatInfo::LUMAFormat( 0,  8, GL_ALPHA,                  GL_UNSIGNED_BYTE,          NormalizedFixedPoint, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE8_EXT,         InternalFormatInfo::LUMAFormat( 8,  0, GL_LUMINANCE,              GL_UNSIGNED_BYTE,          NormalizedFixedPoint, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_ALPHA32F_EXT,           InternalFormatInfo::LUMAFormat( 0, 32, GL_ALPHA,                  GL_FLOAT,                  FloatingPoint,        AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE32F_EXT,       InternalFormatInfo::LUMAFormat(32,  0, GL_LUMINANCE,              GL_FLOAT,                  FloatingPoint,        AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_ALPHA16F_EXT,           InternalFormatInfo::LUMAFormat( 0, 16, GL_ALPHA,                  GL_HALF_FLOAT_OES,         FloatingPoint,        AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE16F_EXT,       InternalFormatInfo::LUMAFormat(16,  0, GL_LUMINANCE,              GL_HALF_FLOAT_OES,         FloatingPoint,        AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE8_ALPHA8_EXT,  InternalFormatInfo::LUMAFormat( 8,  8, GL_LUMINANCE_ALPHA,        GL_UNSIGNED_BYTE,          NormalizedFixedPoint, AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA32F_EXT, InternalFormatInfo::LUMAFormat(32, 32, GL_LUMINANCE_ALPHA,        GL_FLOAT,                  FloatingPoint,        AlwaysSupported)));
    map.insert(InternalFormatInfoPair(GL_LUMINANCE_ALPHA16F_EXT, InternalFormatInfo::LUMAFormat(16, 16, GL_LUMINANCE_ALPHA,        GL_HALF_FLOAT_OES,         FloatingPoint,        AlwaysSupported)));

    // From GL_EXT_texture_compression_dxt1
    //                               | Internal format                   |                                    |W |H | B  |C |Format                            | Type            | Supported                                  |
    //                               |                                   |                                    |  |  | S  |C |                                  |                 |                                            |
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    InternalFormatInfo::CompressedFormat(4, 4,  64, 3, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    GL_UNSIGNED_BYTE, CheckSupport<&Context::supportsDXT1Textures>)));
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   InternalFormatInfo::CompressedFormat(4, 4,  64, 4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   GL_UNSIGNED_BYTE, CheckSupport<&Context::supportsDXT1Textures>)));

    // From GL_ANGLE_texture_compression_dxt3
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, InternalFormatInfo::CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, GL_UNSIGNED_BYTE, CheckSupport<&Context::supportsDXT3Textures>)));

    // From GL_ANGLE_texture_compression_dxt5
    map.insert(InternalFormatInfoPair(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, InternalFormatInfo::CompressedFormat(4, 4, 128, 4, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, GL_UNSIGNED_BYTE, CheckSupport<&Context::supportsDXT5Textures>)));

    return map;
}

static bool GetInternalFormatInfo(GLint internalFormat, GLuint clientVersion, InternalFormatInfo *outFormatInfo)
{
    const InternalFormatInfoMap* map = NULL;

    if (clientVersion == 2)
    {
        static const InternalFormatInfoMap formatMap = BuildES2InternalFormatInfoMap();
        map = &formatMap;
    }
    else if (clientVersion == 3)
    {
        static const InternalFormatInfoMap formatMap = BuildES3InternalFormatInfoMap();
        map = &formatMap;
    }
    else
    {
        UNREACHABLE();
    }

    InternalFormatInfoMap::const_iterator iter = map->find(internalFormat);
    if (iter != map->end())
    {
        if (outFormatInfo)
        {
            *outFormatInfo = iter->second;
        }
        return true;
    }
    else
    {
        return false;
    }
}

typedef std::set<GLenum> FormatSet;

static FormatSet BuildES2ValidFormatSet()
{
    static const FormatMap &formatMap = GetES2FormatMap();

    FormatSet set;

    for (FormatMap::const_iterator i = formatMap.begin(); i != formatMap.end(); i++)
    {
        const FormatTypePair& formatPair = i->first;
        set.insert(formatPair.first);
    }

    return set;
}

static FormatSet BuildES3ValidFormatSet()
{
    static const ES3FormatSet &formatSet = GetES3FormatSet();

    FormatSet set;

    for (ES3FormatSet::const_iterator i = formatSet.begin(); i != formatSet.end(); i++)
    {
        const FormatInfo& formatInfo = *i;
        set.insert(formatInfo.mFormat);
    }

    return set;
}

typedef std::set<GLenum> TypeSet;

static TypeSet BuildES2ValidTypeSet()
{
    static const FormatMap &formatMap = GetES2FormatMap();

    TypeSet set;

    for (FormatMap::const_iterator i = formatMap.begin(); i != formatMap.end(); i++)
    {
        const FormatTypePair& formatPair = i->first;
        set.insert(formatPair.second);
    }

    return set;
}

static TypeSet BuildES3ValidTypeSet()
{
    static const ES3FormatSet &formatSet = GetES3FormatSet();

    TypeSet set;

    for (ES3FormatSet::const_iterator i = formatSet.begin(); i != formatSet.end(); i++)
    {
        const FormatInfo& formatInfo = *i;
        set.insert(formatInfo.mType);
    }

    return set;
}

struct CopyConversion
{
    GLenum mTextureFormat;
    GLenum mFramebufferFormat;

    CopyConversion(GLenum textureFormat, GLenum framebufferFormat)
        : mTextureFormat(textureFormat), mFramebufferFormat(framebufferFormat) { }

    bool operator<(const CopyConversion& other) const
    {
        return memcmp(this, &other, sizeof(CopyConversion)) < 0;
    }
};

typedef std::set<CopyConversion> CopyConversionSet;

static CopyConversionSet BuildValidES3CopyTexImageCombinations()
{
    CopyConversionSet set;

    // From ES 3.0.1 spec, table 3.15
    set.insert(CopyConversion(GL_ALPHA,           GL_RGBA));
    set.insert(CopyConversion(GL_LUMINANCE,       GL_RED));
    set.insert(CopyConversion(GL_LUMINANCE,       GL_RG));
    set.insert(CopyConversion(GL_LUMINANCE,       GL_RGB));
    set.insert(CopyConversion(GL_LUMINANCE,       GL_RGBA));
    set.insert(CopyConversion(GL_LUMINANCE_ALPHA, GL_RGBA));
    set.insert(CopyConversion(GL_RED,             GL_RED));
    set.insert(CopyConversion(GL_RED,             GL_RG));
    set.insert(CopyConversion(GL_RED,             GL_RGB));
    set.insert(CopyConversion(GL_RED,             GL_RGBA));
    set.insert(CopyConversion(GL_RG,              GL_RG));
    set.insert(CopyConversion(GL_RG,              GL_RGB));
    set.insert(CopyConversion(GL_RG,              GL_RGBA));
    set.insert(CopyConversion(GL_RGB,             GL_RGB));
    set.insert(CopyConversion(GL_RGB,             GL_RGBA));
    set.insert(CopyConversion(GL_RGBA,            GL_RGBA));

    set.insert(CopyConversion(GL_RED_INTEGER,     GL_RED_INTEGER));
    set.insert(CopyConversion(GL_RED_INTEGER,     GL_RG_INTEGER));
    set.insert(CopyConversion(GL_RED_INTEGER,     GL_RGB_INTEGER));
    set.insert(CopyConversion(GL_RED_INTEGER,     GL_RGBA_INTEGER));
    set.insert(CopyConversion(GL_RG_INTEGER,      GL_RG_INTEGER));
    set.insert(CopyConversion(GL_RG_INTEGER,      GL_RGB_INTEGER));
    set.insert(CopyConversion(GL_RG_INTEGER,      GL_RGBA_INTEGER));
    set.insert(CopyConversion(GL_RGB_INTEGER,     GL_RGB_INTEGER));
    set.insert(CopyConversion(GL_RGB_INTEGER,     GL_RGBA_INTEGER));
    set.insert(CopyConversion(GL_RGBA_INTEGER,    GL_RGBA_INTEGER));

    return set;
}

bool IsValidInternalFormat(GLint internalFormat, const Context *context)
{
    if (!context)
    {
        return false;
    }

    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, context->getClientVersion(), &internalFormatInfo))
    {
        ASSERT(internalFormatInfo.mSupportFunction != NULL);
        return internalFormatInfo.mSupportFunction(context);
    }
    else
    {
        return false;
    }
}

bool IsValidFormat(GLenum format, GLuint clientVersion)
{
    if (clientVersion == 2)
    {
        static const FormatSet formatSet = BuildES2ValidFormatSet();
        return formatSet.find(format) != formatSet.end();
    }
    else if (clientVersion == 3)
    {
        static const FormatSet formatSet = BuildES3ValidFormatSet();
        return formatSet.find(format) != formatSet.end();
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsValidType(GLenum type, GLuint clientVersion)
{
    if (clientVersion == 2)
    {
        static const TypeSet typeSet = BuildES2ValidTypeSet();
        return typeSet.find(type) != typeSet.end();
    }
    else if (clientVersion == 3)
    {
        static const TypeSet typeSet = BuildES3ValidTypeSet();
        return typeSet.find(type) != typeSet.end();
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsValidFormatCombination(GLint internalFormat, GLenum format, GLenum type, GLuint clientVersion)
{
    if (clientVersion == 2)
    {
        static const FormatMap &formats = GetES2FormatMap();
        FormatMap::const_iterator iter = formats.find(FormatTypePair(format, type));

        return (iter != formats.end()) && ((internalFormat == (GLint)type) || (internalFormat == iter->second));
    }
    else if (clientVersion == 3)
    {
        static const ES3FormatSet &formats = GetES3FormatSet();
        return formats.find(FormatInfo(internalFormat, format, type)) != formats.end();
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsValidCopyTexImageCombination(GLenum textureInternalFormat, GLenum frameBufferInternalFormat, GLuint clientVersion)
{
    InternalFormatInfo textureInternalFormatInfo;
    InternalFormatInfo framebufferInternalFormatInfo;
    if (GetInternalFormatInfo(textureInternalFormat, clientVersion, &textureInternalFormatInfo) &&
        GetInternalFormatInfo(frameBufferInternalFormat, clientVersion, &framebufferInternalFormatInfo))
    {
        if (clientVersion == 2)
        {
            UNIMPLEMENTED();
            return false;
        }
        else if (clientVersion == 3)
        {
            static const CopyConversionSet conversionSet = BuildValidES3CopyTexImageCombinations();
            const CopyConversion conversion = CopyConversion(textureInternalFormatInfo.mFormat,
                                                             framebufferInternalFormatInfo.mFormat);
            if (conversionSet.find(conversion) != conversionSet.end())
            {
                // Section 3.8.5 of the GLES3 3.0.2 spec states that source and destination formats
                // must both be signed or unsigned or fixed/floating point and both source and destinations
                // must be either both SRGB or both not SRGB

                if (textureInternalFormatInfo.mIsSRGB != framebufferInternalFormatInfo.mIsSRGB)
                {
                    return false;
                }

                if ((textureInternalFormatInfo.mStorageType == SignedInteger   && framebufferInternalFormatInfo.mStorageType == SignedInteger  ) ||
                    (textureInternalFormatInfo.mStorageType == UnsignedInteger && framebufferInternalFormatInfo.mStorageType == UnsignedInteger))
                {
                    return true;
                }

                if ((textureInternalFormatInfo.mStorageType     == NormalizedFixedPoint || textureInternalFormatInfo.mStorageType     == FloatingPoint) &&
                    (framebufferInternalFormatInfo.mStorageType == NormalizedFixedPoint || framebufferInternalFormatInfo.mStorageType == FloatingPoint))
                {
                    return true;
                }
            }

            return false;
        }
        else
        {
            UNREACHABLE();
            return false;
        }
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsSizedInternalFormat(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mPixelBits > 0;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

GLint GetSizedInternalFormat(GLenum format, GLenum type, GLuint clientVersion)
{
    if (clientVersion == 2)
    {
        static const FormatMap &formats = GetES2FormatMap();
        FormatMap::const_iterator iter = formats.find(FormatTypePair(format, type));
        return (iter != formats.end()) ? iter->second : GL_NONE;
    }
    else if (clientVersion == 3)
    {
        static const FormatMap formats = BuildES3FormatMap();
        FormatMap::const_iterator iter = formats.find(FormatTypePair(format, type));
        return (iter != formats.end()) ? iter->second : GL_NONE;
    }
    else
    {
        UNREACHABLE();
        return GL_NONE;
    }
}

GLuint GetPixelBytes(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mPixelBits / 8;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetAlphaBits(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mAlphaBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetRedBits(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mRedBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetGreenBits(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mGreenBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetBlueBits(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mBlueBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetLuminanceBits(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mLuminanceBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetDepthBits(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mDepthBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetStencilBits(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mStencilBits;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLenum GetFormat(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mFormat;
    }
    else
    {
        UNREACHABLE();
        return GL_NONE;
    }
}

GLenum GetType(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mType;
    }
    else
    {
        UNREACHABLE();
        return GL_NONE;
    }
}

bool IsNormalizedFixedPointFormat(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mStorageType == NormalizedFixedPoint;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsIntegerFormat(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mStorageType == UnsignedInteger ||
               internalFormatInfo.mStorageType == SignedInteger;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsSignedIntegerFormat(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mStorageType == SignedInteger;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsUnsignedIntegerFormat(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mStorageType == UnsignedInteger;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsFloatingPointFormat(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mStorageType == FloatingPoint;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsSRGBFormat(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mIsSRGB;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsColorRenderingSupported(GLint internalFormat, const rx::Renderer *renderer)
{
    InternalFormatInfo internalFormatInfo;
    if (renderer && GetInternalFormatInfo(internalFormat, renderer->getCurrentClientVersion(), &internalFormatInfo))
    {
        return internalFormatInfo.mIsColorRenderable(NULL, renderer);
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsColorRenderingSupported(GLint internalFormat, const Context *context)
{
    InternalFormatInfo internalFormatInfo;
    if (context && GetInternalFormatInfo(internalFormat, context->getClientVersion(), &internalFormatInfo))
    {
        return internalFormatInfo.mIsColorRenderable(context, NULL);
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsTextureFilteringSupported(GLint internalFormat, const rx::Renderer *renderer)
{
    InternalFormatInfo internalFormatInfo;
    if (renderer && GetInternalFormatInfo(internalFormat, renderer->getCurrentClientVersion(), &internalFormatInfo))
    {
        return internalFormatInfo.mIsTextureFilterable(NULL, renderer);
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsTextureFilteringSupported(GLint internalFormat, const Context *context)
{
    InternalFormatInfo internalFormatInfo;
    if (context && GetInternalFormatInfo(internalFormat, context->getClientVersion(), &internalFormatInfo))
    {
        return internalFormatInfo.mIsTextureFilterable(context, NULL);
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsDepthRenderingSupported(GLint internalFormat, const rx::Renderer *renderer)
{
    InternalFormatInfo internalFormatInfo;
    if (renderer && GetInternalFormatInfo(internalFormat, renderer->getCurrentClientVersion(), &internalFormatInfo))
    {
        return internalFormatInfo.mIsDepthRenderable(NULL, renderer);
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsDepthRenderingSupported(GLint internalFormat, const Context *context)
{
    InternalFormatInfo internalFormatInfo;
    if (context && GetInternalFormatInfo(internalFormat, context->getClientVersion(), &internalFormatInfo))
    {
        return internalFormatInfo.mIsDepthRenderable(context, NULL);
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsStencilRenderingSupported(GLint internalFormat, const rx::Renderer *renderer)
{
    InternalFormatInfo internalFormatInfo;
    if (renderer && GetInternalFormatInfo(internalFormat, renderer->getCurrentClientVersion(), &internalFormatInfo))
    {
        return internalFormatInfo.mIsStencilRenderable(NULL, renderer);
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

bool IsStencilRenderingSupported(GLint internalFormat, const Context *context)
{
    InternalFormatInfo internalFormatInfo;
    if (context && GetInternalFormatInfo(internalFormat, context->getClientVersion(), &internalFormatInfo))
    {
        return internalFormatInfo.mIsStencilRenderable(context, NULL);
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

GLuint GetRowPitch(GLint internalFormat, GLenum type, GLuint clientVersion, GLsizei width, GLint alignment)
{
    ASSERT(alignment > 0 && isPow2(alignment));
    return (GetBlockSize(internalFormat, type, clientVersion, width, 1) + alignment - 1) & ~(alignment - 1);
}

GLuint GetDepthPitch(GLint internalFormat, GLenum type, GLuint clientVersion, GLsizei width, GLsizei height, GLint alignment)
{
    return (GetBlockSize(internalFormat, type, clientVersion, width, height) + alignment - 1) & ~(alignment - 1);
}

GLuint GetBlockSize(GLint internalFormat, GLenum type, GLuint clientVersion, GLsizei width, GLsizei height)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        if (internalFormatInfo.mStorageType == Compressed)
        {
            GLsizei numBlocksWide = (width + internalFormatInfo.mCompressedBlockWidth - 1) / internalFormatInfo.mCompressedBlockWidth;
            GLsizei numBlocksHight = (height + internalFormatInfo.mCompressedBlockHeight - 1) / internalFormatInfo.mCompressedBlockHeight;

            return (internalFormatInfo.mPixelBits * numBlocksWide * numBlocksHight) / 8;
        }
        else
        {
            TypeInfo typeInfo;
            if (GetTypeInfo(type, &typeInfo))
            {
                if (typeInfo.mSpecialInterpretation)
                {
                    return typeInfo.mTypeBytes * width * height;
                }
                else
                {
                    return internalFormatInfo.mComponentCount * typeInfo.mTypeBytes * width * height;
                }
            }
            else
            {
                UNREACHABLE();
                return 0;
            }
        }
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

bool IsFormatCompressed(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mStorageType == Compressed;
    }
    else
    {
        UNREACHABLE();
        return false;
    }
}

GLuint GetCompressedBlockWidth(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mCompressedBlockWidth;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetCompressedBlockHeight(GLint internalFormat, GLuint clientVersion)
{
    InternalFormatInfo internalFormatInfo;
    if (GetInternalFormatInfo(internalFormat, clientVersion, &internalFormatInfo))
    {
        return internalFormatInfo.mCompressedBlockHeight;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

}
