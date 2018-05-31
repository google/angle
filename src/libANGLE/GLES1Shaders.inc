//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// GLES1Shaders.inc: Defines GLES1 emulation shader.

constexpr char kGLES1DrawVShader[] = R"(#version 300 es
precision highp float;

#define kMaxTexUnits                         4

in vec4 pos;
in vec3 normal;
in vec4 color;
in float pointsize;
in vec4 texcoord0;
in vec4 texcoord1;
in vec4 texcoord2;
in vec4 texcoord3;

uniform mat4 projection;
uniform mat4 modelview;
uniform mat4 modelview_invtr;
uniform mat4 texture_matrix[4];

uniform bool enable_rescale_normal;
uniform bool enable_normalize;

out vec4 pos_varying;
out vec3 normal_varying;
out vec4 color_varying;
flat out vec4 color_varying_flat;
out float pointsize_varying;
out vec4 texcoord0_varying;
out vec4 texcoord1_varying;
out vec4 texcoord2_varying;
out vec4 texcoord3_varying;

void main()
{
    pos_varying = modelview * pos;
    mat3 mvInvTr3 = mat3(modelview_invtr);
    normal_varying = mvInvTr3 * normal;

    if (enable_rescale_normal) {
        float rescale = 1.0;
        vec3 rescaleVec = vec3(mvInvTr3[2]);
        float len = length(rescaleVec);
        if (len > 0.0) {
            rescale = 1.0 / len;
        }
        normal_varying *= rescale;
    }

    if (enable_normalize) {
        normal_varying = normalize(normal_varying);
    }

    color_varying = color;
    color_varying_flat = color;
    pointsize_varying = pointsize;
    texcoord0_varying = texture_matrix[0] * texcoord0;
    texcoord1_varying = texture_matrix[1] * texcoord1;
    texcoord2_varying = texture_matrix[2] * texcoord2;
    texcoord3_varying = texture_matrix[3] * texcoord3;

    vec4 vertexPos = projection * modelview * pos;
    gl_Position = vertexPos;
}
)";

// version, flat,
constexpr char kGLES1DrawFShaderHeader[] = R"(#version 300 es
precision highp float;

// Defines for GL constants
#define kMaxLights                           8
#define kMaxTexUnits                         4

#define kModulate                       0x2100
#define kDecal                          0x2101
#define kCombine                        0x8570
#define kReplace                        0x1E01
#define kBlend                          0x0BE2
#define kAdd                            0x0104

#define kAddSigned                      0x8574
#define kInterpolate                    0x8575
#define kSubtract                       0x84E7
#define kDot3Rgb                        0x86AE
#define kDot3Rgba                       0x86AF

#define kAlpha                          0x1906
#define kRGB                            0x1907
#define kRGBA                           0x1908
#define kLuminance                      0x1909
#define kLuminanceAlpha                 0x190A

#define kTexture                        0x1702
#define kConstant                       0x8576
#define kPrimaryColor                   0x8577
#define kPrevious                       0x8578

#define kSrcColor                       0x0300
#define kOneMinusSrcColor               0x0301
#define kSrcAlpha                       0x0302
#define kOneMinusSrcAlpha               0x0303

#define kLinear                         0x2601
#define kExp                            0x0800
#define kExp2                           0x0801

#define kNever                          0x0200
#define kLess                           0x0201
#define kEqual                          0x0202
#define kLequal                         0x0203
#define kGreater                        0x0204
#define kNotequal                       0x0205
#define kGequal                         0x0206
#define kAlways                         0x0207
#define kZero                              0x0
#define kOne                               0x1

#define kClear                          0x1500
#define kAnd                            0x1501
#define kAnd_reverse                    0x1502
#define kCopy                           0x1503
#define kAnd_inverted                   0x1504
#define kNoop                           0x1505
#define kXor                            0x1506
#define kOr                             0x1507
#define kNor                            0x1508
#define kEquiv                          0x1509
#define kInvert                         0x150A
#define kOr_reverse                     0x150B
#define kCopy_inverted                  0x150C
#define kOr_inverted                    0x150D
#define kNand                           0x150E
#define kSet                            0x150F)";

constexpr char kGLES1DrawFShaderUniformDefs[] = R"(

// Texture units ///////////////////////////////////////////////////////////////

uniform bool enable_texture_2d[kMaxTexUnits];
uniform bool enable_texture_cube_map[kMaxTexUnits];

// These are not arrays because hw support for arrays
// of samplers is rather lacking.

uniform sampler2D tex_sampler0;
uniform samplerCube tex_cube_sampler0;

uniform sampler2D tex_sampler1;
uniform samplerCube tex_cube_sampler1;

uniform sampler2D tex_sampler2;
uniform samplerCube tex_cube_sampler2;

uniform sampler2D tex_sampler3;
uniform samplerCube tex_cube_sampler3;

// Vertex attributes////////////////////////////////////////////////////////////

in vec4 pos_varying;
in vec3 normal_varying;
in vec4 color_varying;
flat in vec4 color_varying_flat;
in float pointsize_varying;
in vec4 texcoord0_varying;
in vec4 texcoord1_varying;
in vec4 texcoord2_varying;
in vec4 texcoord3_varying;

// Alpha test///////////////////////////////////////////////////////////////////

uniform bool enable_alpha_test;
uniform int alpha_func;
uniform float alpha_test_ref;

// Shading: flat shading, lighting, and materials///////////////////////////////

uniform bool shade_model_flat;
uniform bool enable_lighting;
uniform bool enable_color_material;

uniform vec4 material_ambient;
uniform vec4 material_diffuse;
uniform vec4 material_specular;
uniform vec4 material_emissive;
uniform float material_specular_exponent;

uniform vec4 light_model_scene_ambient;
uniform bool light_model_two_sided;

uniform bool light_enables[kMaxLights];
uniform vec4 light_ambients[kMaxLights];
uniform vec4 light_diffuses[kMaxLights];
uniform vec4 light_speculars[kMaxLights];
uniform vec4 light_positions[kMaxLights];
uniform vec3 light_directions[kMaxLights];
uniform float light_spotlight_exponents[kMaxLights];
uniform float light_spotlight_cutoff_angles[kMaxLights];
uniform float light_attenuation_consts[kMaxLights];
uniform float light_attenuation_linears[kMaxLights];
uniform float light_attenuation_quadratics[kMaxLights];

// Outgoing fragment////////////////////////////////////////////////////////////

out vec4 frag_color;
)";

constexpr char kGLES1DrawFShaderFunctions[] = R"(

bool isTextureUnitEnabled(int unit)
{
    return enable_texture_2d[unit] || enable_texture_cube_map[unit];
}

vec4 getTextureColor(int unit)
{
    vec4 res;

    switch (unit)
    {
    case 0:
        if (enable_texture_2d[0])
        {
            res = texture(tex_sampler0, texcoord0_varying.xy);
        }
        else if (enable_texture_cube_map[0])
        {
            res = texture(tex_cube_sampler0, texcoord0_varying.xyz);
        }
        break;
    case 1:
        if (enable_texture_2d[1])
        {
            res = texture(tex_sampler1, texcoord1_varying.xy);
        }
        else if (enable_texture_cube_map[1])
        {
            res = texture(tex_cube_sampler1, texcoord1_varying.xyz);
        }
        break;
    case 2:
        if (enable_texture_2d[2])
        {
            res = texture(tex_sampler2, texcoord2_varying.xy);
        }
        else if (enable_texture_cube_map[2])
        {
            res = texture(tex_cube_sampler2, texcoord2_varying.xyz);
        }
        break;
    case 3:
        if (enable_texture_2d[3])
        {
            res = texture(tex_sampler3, texcoord3_varying.xy);
        }
        else if (enable_texture_cube_map[3])
        {
            // TODO: Weird stuff happens
            // res = texture(tex_cube_sampler3, texcoord3_varying.xyz);
        }
        break;
    default:
        break;
    }

    return res;
}

vec4 getColor(int unit, vec4 vertexColor)
{
    if (!isTextureUnitEnabled(unit))
    {
        return vertexColor;
    }

    return getTextureColor(unit);
}

float posDot(vec3 a, vec3 b) {
    return max(dot(a, b), 0.0);
}

vec4 doLighting(vec4 currentFragment) {
    vec4 materialAmbientActual = material_ambient;
    vec4 materialDiffuseActual = material_diffuse;

    if (enable_color_material || enable_texture_2d[0] || enable_texture_cube_map[0]) {
        materialAmbientActual = currentFragment;
        materialDiffuseActual = currentFragment;
    }

    vec4 lightingResult = material_emissive +
               materialAmbientActual * light_model_scene_ambient;

    for (int i = 0; i < kMaxLights; i++) {

        if (!light_enables[i]) continue;

        vec4 lightAmbient = light_ambients[i];
        vec4 lightDiffuse = light_diffuses[i];
        vec4 lightSpecular = light_speculars[i];
        vec4 lightPos = light_positions[i];
        vec3 lightDir = light_directions[i];
        float attConst = light_attenuation_consts[i];
        float attLinear = light_attenuation_linears[i];
        float attQuadratic = light_attenuation_quadratics[i];
        float spotAngle = light_spotlight_cutoff_angles[i];
        float spotExponent = light_spotlight_exponents[i];

        vec3 toLight;
        if (lightPos.w == 0.0) {
            toLight = lightPos.xyz;
        } else {
            toLight = (lightPos.xyz / lightPos.w - pos_varying.xyz);
        }

        float lightDist = length(toLight);
        vec3 h = normalize(toLight) + vec3(0.0, 0.0, 1.0);
        float ndotL = posDot(normal_varying, normalize(toLight));
        float ndoth = posDot(normal_varying, normalize(h));

        float specAtt;

        if (ndotL != 0.0) {
            specAtt = 1.0;
        } else {
            specAtt = 0.0;
        }

        float att;

        if (lightPos.w != 0.0) {
            float attDenom = (attConst + attLinear * lightDist +
                              attQuadratic * lightDist * lightDist);
            att = 1.0 / attDenom;
        } else {
            att = 1.0;
        }

        float spot;

        float spotAngleCos = cos(radians(spotAngle));
        vec3 toSurfaceDir = -normalize(toLight);
        float spotDot = posDot(toSurfaceDir, normalize(lightDir));

        if (spotAngle == 180.0 || lightPos.w == 0.0) {
            spot = 1.0;
        } else {
            if (spotDot < spotAngleCos) {
                spot = 0.0;
            } else {
                spot = pow(spotDot, spotExponent);
            }
        }

        vec4 contrib = materialAmbientActual * lightAmbient;
        contrib += ndotL * materialDiffuseActual * lightDiffuse;
        if (ndoth > 0.0 && material_specular_exponent > 0.0) {
            contrib += specAtt * pow(ndoth, material_specular_exponent) *
                                 material_specular * lightSpecular;
        } else {
            if (ndoth > 0.0) {
                contrib += specAtt * material_specular * lightSpecular;
            }
        }
        contrib *= att * spot;
        lightingResult += contrib;
    }

    return lightingResult;
}

bool doAlphaTest(vec4 currentFragment)
{
    bool shouldPassAlpha = false;
    float incAlpha = currentFragment.a;

    switch (alpha_func)
    {
        case kNever:
            shouldPassAlpha = false;
            break;
        case kLess:
            shouldPassAlpha = incAlpha < alpha_test_ref;
            break;
        case kLequal:
            shouldPassAlpha = incAlpha <= alpha_test_ref;
            break;
        case kEqual:
            shouldPassAlpha = incAlpha == alpha_test_ref;
            break;
        case kGequal:
            shouldPassAlpha = incAlpha >= alpha_test_ref;
            break;
        case kGreater:
            shouldPassAlpha = incAlpha > alpha_test_ref;
            break;
        case kNotequal:
            shouldPassAlpha = incAlpha != alpha_test_ref;
            break;
        case kAlways:
        default:
            shouldPassAlpha = true;
            break;
    }

    return shouldPassAlpha;
}

)";

constexpr char kGLES1DrawFShaderMain[] = R"(
void main()
{
    vec4 currentFragment;
    vec4 vertex_color;

    if (shade_model_flat)
    {
        vertex_color = color_varying_flat;
    }
    else
    {
        vertex_color = color_varying;
    }

    // Unit 0 only for now
    currentFragment = getColor(0, vertex_color);

    if (enable_lighting) {
        currentFragment = doLighting(currentFragment);
    }

    if (enable_alpha_test && !doAlphaTest(currentFragment))
    {
        discard;
    }

    frag_color = currentFragment;
}
)";
