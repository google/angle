//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/OutputHLSL.h"

#include "common/debug.h"

#include "compiler/InfoSink.h"
#include "compiler/UnfoldSelect.h"

namespace sh
{
// Integer to TString conversion
TString str(int i)
{
    char buffer[20];
    sprintf(buffer, "%d", i);
    return buffer;
}

OutputHLSL::OutputHLSL(TParseContext &context) : TIntermTraverser(true, true, true), mContext(context)
{
    mUnfoldSelect = new UnfoldSelect(context, this);
    mInsideFunction = false;

    mUsesTexture2D = false;
    mUsesTexture2D_bias = false;
    mUsesTexture2DProj = false;
    mUsesTexture2DProj_bias = false;
    mUsesTextureCube = false;
    mUsesTextureCube_bias = false;
    mUsesFaceforward1 = false;
    mUsesFaceforward2 = false;
    mUsesFaceforward3 = false;
    mUsesFaceforward4 = false;
    mUsesEqualMat2 = false;
    mUsesEqualMat3 = false;
    mUsesEqualMat4 = false;
    mUsesEqualVec2 = false;
    mUsesEqualVec3 = false;
    mUsesEqualVec4 = false;
    mUsesEqualIVec2 = false;
    mUsesEqualIVec3 = false;
    mUsesEqualIVec4 = false;
    mUsesEqualBVec2 = false;
    mUsesEqualBVec3 = false;
    mUsesEqualBVec4 = false;

    mArgumentIndex = 0;
}

OutputHLSL::~OutputHLSL()
{
    delete mUnfoldSelect;
}

void OutputHLSL::output()
{
    mContext.treeRoot->traverse(this);   // Output the body first to determine what has to go in the header and footer
    header();
    footer();

    mContext.infoSink.obj << mHeader.c_str();
    mContext.infoSink.obj << mBody.c_str();
    mContext.infoSink.obj << mFooter.c_str();
}

TInfoSinkBase &OutputHLSL::getBodyStream()
{
    return mBody;
}

int OutputHLSL::vectorSize(const TType &type) const
{
    int elementSize = type.isMatrix() ? type.getNominalSize() : 1;
    int arraySize = type.isArray() ? type.getArraySize() : 1;

    return elementSize * arraySize;
}

void OutputHLSL::header()
{
    EShLanguage language = mContext.language;
    TInfoSinkBase &out = mHeader;

    if (language == EShLangFragment)
    {
        TString uniforms;
        TString varyingInput;
        TString varyingGlobals;

        TSymbolTableLevel *symbols = mContext.symbolTable.getGlobalLevel();
        int semanticIndex = 0;

        for (TSymbolTableLevel::const_iterator namedSymbol = symbols->begin(); namedSymbol != symbols->end(); namedSymbol++)
        {
            const TSymbol *symbol = (*namedSymbol).second;
            const TString &name = symbol->getName();

            if (symbol->isVariable())
            {
                const TVariable *variable = static_cast<const TVariable*>(symbol);
                const TType &type = variable->getType();
                TQualifier qualifier = type.getQualifier();

                if (qualifier == EvqUniform)
                {
                    if (mReferencedUniforms.find(name.c_str()) != mReferencedUniforms.end())
                    {
                        uniforms += "uniform " + typeString(type) + " " + decorate(name) + arrayString(type) + ";\n";
                    }
                }
                else if (qualifier == EvqVaryingIn || qualifier == EvqInvariantVaryingIn)
                {
                    if (mReferencedVaryings.find(name.c_str()) != mReferencedVaryings.end())
                    {
                        // Program linking depends on this exact format
                        varyingInput += "    " + typeString(type) + " " + decorate(name) + arrayString(type) + " : TEXCOORD" + str(semanticIndex) + ";\n";
                        varyingGlobals += "static " + typeString(type) + " " + decorate(name) + arrayString(type) + " = " + initializer(type) + ";\n";

                        semanticIndex += type.isArray() ? type.getArraySize() : 1;
                    }
                }
                else if (qualifier == EvqGlobal || qualifier == EvqTemporary)
                {
                    // Globals are declared and intialized as an aggregate node
                }
                else if (qualifier == EvqConst)
                {
                    // Constants are repeated as literals where used
                }
                else UNREACHABLE();
            }
        }

        out << "uniform float4 dx_Window;\n"
               "uniform float2 dx_Depth;\n"
               "uniform bool dx_PointsOrLines;\n"
               "uniform bool dx_FrontCCW;\n"
               "\n";
        out <<  uniforms;
        out << "\n"
               "struct PS_INPUT\n"
               "{\n";
        out <<      varyingInput;
        out << "    float4 gl_FragCoord : TEXCOORD" << semanticIndex << ";\n";
        out << "    float vFace : VFACE;\n"
               "};\n"
               "\n";
        out <<    varyingGlobals;
        out << "\n"
               "struct PS_OUTPUT\n"
               "{\n"
               "    float4 gl_Color[1] : COLOR;\n"
               "};\n"
               "\n"
               "static float4 gl_Color[1] = {float4(0, 0, 0, 0)};\n"
               "static float4 gl_FragCoord = float4(0, 0, 0, 0);\n"
               "static float2 gl_PointCoord = float2(0.5, 0.5);\n"
               "static bool gl_FrontFacing = false;\n"
               "\n";

        if (mUsesTexture2D)
        {
            out << "float4 gl_texture2D(sampler2D s, float2 t)\n"
                   "{\n"
                   "    return tex2D(s, t);\n"
                   "}\n"
                   "\n";
        }

        if (mUsesTexture2D_bias)
        {
            out << "float4 gl_texture2D(sampler2D s, float2 t, float bias)\n"
                   "{\n"
                   "    return tex2Dbias(s, float4(t.x, t.y, 0, bias));\n"
                   "}\n"
                   "\n";
        }

        if (mUsesTexture2DProj)
        {
            out << "float4 gl_texture2DProj(sampler2D s, float3 t)\n"
                   "{\n"
                   "    return tex2Dproj(s, float4(t.x, t.y, 0, t.z));\n"
                   "}\n"
                   "\n"
                   "float4 gl_texture2DProj(sampler2D s, float4 t)\n"
                   "{\n"
                   "    return tex2Dproj(s, t);\n"
                   "}\n"
                   "\n";
        }

        if (mUsesTexture2DProj_bias)
        {
            out << "float4 gl_texture2DProj(sampler2D s, float3 t, float bias)\n"
                   "{\n"
                   "    return tex2Dbias(s, float4(t.x / t.z, t.y / t.z, 0, bias));\n"
                   "}\n"
                   "\n"
                   "float4 gl_texture2DProj(sampler2D s, float4 t, float bias)\n"
                   "{\n"
                   "    return tex2Dbias(s, float4(t.x / t.w, t.y / t.w, 0, bias));\n"
                   "}\n"
                   "\n";
        }

        if (mUsesTextureCube)
        {
            out << "float4 gl_textureCube(samplerCUBE s, float3 t)\n"
                   "{\n"
                   "    return texCUBE(s, t);\n"
                   "}\n"
                   "\n";
        }

        if (mUsesTextureCube_bias)
        {
            out << "float4 gl_textureCube(samplerCUBE s, float3 t, float bias)\n"
                   "{\n"
                   "    return texCUBEbias(s, float4(t.x, t.y, t.z, bias));\n"
                   "}\n"
                   "\n";
        }
    }
    else   // Vertex shader
    {
        TString uniforms;
        TString attributeInput;
        TString attributeGlobals;
        TString varyingOutput;
        TString varyingGlobals;

        TSymbolTableLevel *symbols = mContext.symbolTable.getGlobalLevel();
        int semanticIndex = 0;

        for (TSymbolTableLevel::const_iterator namedSymbol = symbols->begin(); namedSymbol != symbols->end(); namedSymbol++)
        {
            const TSymbol *symbol = (*namedSymbol).second;
            const TString &name = symbol->getName();

            if (symbol->isVariable())
            {
                const TVariable *variable = static_cast<const TVariable*>(symbol);
                const TType &type = variable->getType();
                TQualifier qualifier = type.getQualifier();

                if (qualifier == EvqUniform)
                {
                    if (mReferencedUniforms.find(name.c_str()) != mReferencedUniforms.end())
                    {
                        uniforms += "uniform " + typeString(type) + " " + decorate(name) + arrayString(type) + ";\n";
                    }
                }
                else if (qualifier == EvqAttribute)
                {
                    if (mReferencedAttributes.find(name.c_str()) != mReferencedAttributes.end())
                    {
                        attributeInput += "    " + typeString(type) + " " + decorate(name) + arrayString(type) + " : TEXCOORD" + str(semanticIndex) + ";\n";
                        attributeGlobals += "static " + typeString(type) + " " + decorate(name) + arrayString(type) + " = " + initializer(type) + ";\n";

                        semanticIndex += vectorSize(type);
                    }
                }
                else if (qualifier == EvqVaryingOut || qualifier == EvqInvariantVaryingOut)
                {
                    if (mReferencedVaryings.find(name.c_str()) != mReferencedVaryings.end())
                    {
                        // Program linking depends on this exact format
                        varyingOutput += "    " + typeString(type) + " " + decorate(name) + arrayString(type) + " : TEXCOORD0;\n";   // Actual semantic index assigned during link
                        varyingGlobals += "static " + typeString(type) + " " + decorate(name) + arrayString(type) + " = " + initializer(type) + ";\n";
                    }
                }
                else if (qualifier == EvqGlobal || qualifier == EvqTemporary)
                {
                    // Globals are declared and intialized as an aggregate node
                }
                else if (qualifier == EvqConst)
                {
                    // Constants are repeated as literals where used
                }
                else UNREACHABLE();
            }
        }

        out << "uniform float2 dx_HalfPixelSize;\n"
               "\n";
        out <<  uniforms;
        out << "\n"
               "struct VS_INPUT\n"
               "{\n";
        out <<        attributeInput;
        out << "};\n"
               "\n";
        out <<  attributeGlobals;
        out << "\n"
               "struct VS_OUTPUT\n"
               "{\n"
               "    float4 gl_Position : POSITION;\n"
               "    float gl_PointSize : PSIZE;\n"
               "    float4 gl_FragCoord : TEXCOORD0;\n";   // Actual semantic index assigned during link
        out <<      varyingOutput;
        out << "};\n"
               "\n"
               "static float4 gl_Position = float4(0, 0, 0, 0);\n"
               "static float gl_PointSize = float(1);\n";
        out <<  varyingGlobals;
        out << "\n";
    }

    out << "struct gl_DepthRangeParameters\n"
           "{\n"
           "    float near;\n"
           "    float far;\n"
           "    float diff;\n"
           "};\n"
           "\n"
           "uniform gl_DepthRangeParameters gl_DepthRange;\n"
           "\n"
           "bool xor(bool p, bool q)\n"
           "{\n"
           "    return (p || q) && !(p && q);\n"
           "}\n"
           "\n"
           "float mod(float x, float y)\n"
           "{\n"
           "    return x - y * floor(x / y);\n"
           "}\n"
           "\n"
           "float2 mod(float2 x, float y)\n"
           "{\n"
           "    return x - y * floor(x / y);\n"
           "}\n"
           "\n"
           "float3 mod(float3 x, float y)\n"
           "{\n"
           "    return x - y * floor(x / y);\n"
           "}\n"
           "\n"
           "float4 mod(float4 x, float y)\n"
           "{\n"
           "    return x - y * floor(x / y);\n"
           "}\n"
           "\n";

    if (mUsesFaceforward1)
    {
        out << "float faceforward(float N, float I, float Nref)\n"
               "{\n"
               "    if(dot(Nref, I) < 0)\n"
               "    {\n"
               "        return N;\n"
               "    }\n"
               "    else\n"
               "    {\n"
               "        return -N;\n"
               "    }\n"
               "}\n"
               "\n";
    }

    if (mUsesFaceforward2)
    {
        out << "float2 faceforward(float2 N, float2 I, float2 Nref)\n"
               "{\n"
               "    if(dot(Nref, I) < 0)\n"
               "    {\n"
               "        return N;\n"
               "    }\n"
               "    else\n"
               "    {\n"
               "        return -N;\n"
               "    }\n"
               "}\n"
               "\n";
    }

    if (mUsesFaceforward3)
    {
        out << "float3 faceforward(float3 N, float3 I, float3 Nref)\n"
               "{\n"
               "    if(dot(Nref, I) < 0)\n"
               "    {\n"
               "        return N;\n"
               "    }\n"
               "    else\n"
               "    {\n"
               "        return -N;\n"
               "    }\n"
               "}\n"
               "\n";
    }

    if (mUsesFaceforward4)
    {
        out << "float4 faceforward(float4 N, float4 I, float4 Nref)\n"
               "{\n"
               "    if(dot(Nref, I) < 0)\n"
               "    {\n"
               "        return N;\n"
               "    }\n"
               "    else\n"
               "    {\n"
               "        return -N;\n"
               "    }\n"
               "}\n"
               "\n";
    }

    if (mUsesEqualMat2)
    {
        out << "bool equal(float2x2 m, float2x2 n)\n"
               "{\n"
               "    return m[0][0] == n[0][0] && m[0][1] == n[0][1] &&\n"
               "           m[1][0] == n[1][0] && m[1][1] == n[1][1];\n"
               "}\n";
    }

    if (mUsesEqualMat3)
    {
        out << "bool equal(float3x3 m, float3x3 n)\n"
               "{\n"
               "    return m[0][0] == n[0][0] && m[0][1] == n[0][1] && m[0][2] == n[0][2] &&\n"
               "           m[1][0] == n[1][0] && m[1][1] == n[1][1] && m[1][2] == n[1][2] &&\n"
               "           m[2][0] == n[2][0] && m[2][1] == n[2][1] && m[2][2] == n[2][2];\n"
               "}\n";
    }

    if (mUsesEqualMat4)
    {
        out << "bool equal(float4x4 m, float4x4 n)\n"
               "{\n"
               "    return m[0][0] == n[0][0] && m[0][1] == n[0][1] && m[0][2] == n[0][2] && m[0][3] == n[0][3] &&\n"
               "           m[1][0] == n[1][0] && m[1][1] == n[1][1] && m[1][2] == n[1][2] && m[1][3] == n[1][3] &&\n"
               "           m[2][0] == n[2][0] && m[2][1] == n[2][1] && m[2][2] == n[2][2] && m[2][3] == n[2][3] &&\n"
               "           m[3][0] == n[3][0] && m[3][1] == n[3][1] && m[3][2] == n[3][2] && m[3][3] == n[3][3];\n"
               "}\n";
    }

    if (mUsesEqualVec2)
    {
        out << "bool equal(float2 v, float2 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y;\n"
               "}\n";
    }

    if (mUsesEqualVec3)
    {
        out << "bool equal(float3 v, float3 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z;\n"
               "}\n";
    }

    if (mUsesEqualVec4)
    {
        out << "bool equal(float4 v, float4 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z && v.w == u.w;\n"
               "}\n";
    }

    if (mUsesEqualIVec2)
    {
        out << "bool equal(int2 v, int2 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y;\n"
               "}\n";
    }

    if (mUsesEqualIVec3)
    {
        out << "bool equal(int3 v, int3 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z;\n"
               "}\n";
    }

    if (mUsesEqualIVec4)
    {
        out << "bool equal(int4 v, int4 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z && v.w == u.w;\n"
               "}\n";
    }

    if (mUsesEqualBVec2)
    {
        out << "bool equal(bool2 v, bool2 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y;\n"
               "}\n";
    }

    if (mUsesEqualBVec3)
    {
        out << "bool equal(bool3 v, bool3 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z;\n"
               "}\n";
    }

    if (mUsesEqualBVec4)
    {
        out << "bool equal(bool4 v, bool4 u)\n"
               "{\n"
               "    return v.x == u.x && v.y == u.y && v.z == u.z && v.w == u.w;\n"
               "}\n";
    }

    for (ConstructorSet::iterator constructor = mConstructors.begin(); constructor != mConstructors.end(); constructor++)
    {
        out << typeString(constructor->type) + " " + constructor->name + "(";

        for (unsigned int parameter = 0; parameter < constructor->parameters.size(); parameter++)
        {
            const TType &type = constructor->parameters[parameter];

            out << typeString(type) + " x" + str(parameter);

            if (parameter < constructor->parameters.size() - 1)
            {
                out << ", ";
            }
        }

        out << ")\n"
               "{\n";

        out << "    return " + typeString(constructor->type) + "(";

        if (constructor->type.isMatrix() && constructor->parameters.size() == 1)
        {
            int dim = constructor->type.getNominalSize();
            const TType &parameter = constructor->parameters[0];

            if (parameter.isScalar())
            {
                for (int row = 0; row < dim; row++)
                {
                    for (int col = 0; col < dim; col++)
                    {
                        out << TString((row == col) ? "x0" : "0.0");
                        
                        if (row < dim - 1 || col < dim - 1)
                        {
                            out << ", ";
                        }
                    }
                }
            }
            else if (parameter.isMatrix())
            {
                for (int row = 0; row < dim; row++)
                {
                    for (int col = 0; col < dim; col++)
                    {
                        if (row < parameter.getNominalSize() && col < parameter.getNominalSize())
                        {
                            out << TString("x0") + "[" + str(row) + "]" + "[" + str(col) + "]";
                        }
                        else
                        {
                            out << TString((row == col) ? "1.0" : "0.0");
                        }

                        if (row < dim - 1 || col < dim - 1)
                        {
                            out << ", ";
                        }
                    }
                }
            }
            else UNREACHABLE();
        }
        else
        {
            int remainingComponents = constructor->type.getObjectSize();
            int parameterIndex = 0;

            while (remainingComponents > 0)
            {
                const TType &parameter = constructor->parameters[parameterIndex];
                bool moreParameters = parameterIndex < (int)constructor->parameters.size() - 1;

                out << "x" + str(parameterIndex);

                if (parameter.isScalar())
                {
                    remainingComponents -= 1;
                }
                else if (parameter.isVector())
                {
                    if (remainingComponents == parameter.getInstanceSize() || moreParameters)
                    {
                        remainingComponents -= parameter.getInstanceSize();
                    }
                    else if (remainingComponents < parameter.getNominalSize())
                    {
                        switch (remainingComponents)
                        {
                        case 1: out << ".x";    break;
                        case 2: out << ".xy";   break;
                        case 3: out << ".xyz";  break;
                        case 4: out << ".xyzw"; break;
                        default: UNREACHABLE();
                        }

                        remainingComponents = 0;
                    }
                    else UNREACHABLE();
                }
                else if (parameter.isMatrix() || parameter.getStruct())
                {
                    ASSERT(remainingComponents == parameter.getInstanceSize() || moreParameters);
                    
                    remainingComponents -= parameter.getInstanceSize();
                }
                else UNREACHABLE();

                if (moreParameters)
                {
                    parameterIndex++;
                }

                if (remainingComponents)
                {
                    out << ", ";
                }
            }
        }

        out << ");\n"
               "}\n";
    }
}

void OutputHLSL::footer()
{
    EShLanguage language = mContext.language;
    TInfoSinkBase &out = mFooter;
    TSymbolTableLevel *symbols = mContext.symbolTable.getGlobalLevel();

    if (language == EShLangFragment)
    {
        out << "PS_OUTPUT main(PS_INPUT input)\n"
               "{\n"
               "    float rhw = 1.0 / input.gl_FragCoord.w;\n"
               "    gl_FragCoord.x = (input.gl_FragCoord.x * rhw) * dx_Window.x + dx_Window.z;\n"
               "    gl_FragCoord.y = (input.gl_FragCoord.y * rhw) * dx_Window.y + dx_Window.w;\n"
               "    gl_FragCoord.z = (input.gl_FragCoord.z * rhw) * dx_Depth.x + dx_Depth.y;\n"
               "    gl_FragCoord.w = rhw;\n"
               "    gl_FrontFacing = dx_PointsOrLines || (dx_FrontCCW ? (input.vFace >= 0.0) : (input.vFace <= 0.0));\n";

        for (TSymbolTableLevel::const_iterator namedSymbol = symbols->begin(); namedSymbol != symbols->end(); namedSymbol++)
        {
            const TSymbol *symbol = (*namedSymbol).second;
            const TString &name = symbol->getName();

            if (symbol->isVariable())
            {
                const TVariable *variable = static_cast<const TVariable*>(symbol);
                const TType &type = variable->getType();
                TQualifier qualifier = type.getQualifier();

                if (qualifier == EvqVaryingIn || qualifier == EvqInvariantVaryingIn)
                {
                    if (mReferencedVaryings.find(name.c_str()) != mReferencedVaryings.end())
                    {
                        out << "    " + decorate(name) + " = input." + decorate(name) + ";\n";
                    }
                }
            }
        }

        out << "\n"
               "    gl_main();\n"
               "\n"
               "    PS_OUTPUT output;\n"                 
               "    output.gl_Color[0] = gl_Color[0];\n";
    }
    else   // Vertex shader
    {
        out << "VS_OUTPUT main(VS_INPUT input)\n"
               "{\n";

        for (TSymbolTableLevel::const_iterator namedSymbol = symbols->begin(); namedSymbol != symbols->end(); namedSymbol++)
        {
            const TSymbol *symbol = (*namedSymbol).second;
            const TString &name = symbol->getName();

            if (symbol->isVariable())
            {
                const TVariable *variable = static_cast<const TVariable*>(symbol);
                const TType &type = variable->getType();
                TQualifier qualifier = type.getQualifier();

                if (qualifier == EvqAttribute)
                {
                    if (mReferencedAttributes.find(name.c_str()) != mReferencedAttributes.end())
                    {
                        const char *transpose = type.isMatrix() ? "transpose" : "";

                        out << "    " + decorate(name) + " = " + transpose + "(input." + decorate(name) + ");\n";
                    }
                }
            }
        }

        out << "\n"
               "    gl_main();\n"
               "\n"
               "    VS_OUTPUT output;\n"
               "    output.gl_Position.x = gl_Position.x - dx_HalfPixelSize.x * gl_Position.w;\n"
               "    output.gl_Position.y = -(gl_Position.y - dx_HalfPixelSize.y * gl_Position.w);\n"
               "    output.gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;\n"
               "    output.gl_Position.w = gl_Position.w;\n"
               "    output.gl_PointSize = gl_PointSize;\n"
               "    output.gl_FragCoord = gl_Position;\n";

        TSymbolTableLevel *symbols = mContext.symbolTable.getGlobalLevel();

        for (TSymbolTableLevel::const_iterator namedSymbol = symbols->begin(); namedSymbol != symbols->end(); namedSymbol++)
        {
            const TSymbol *symbol = (*namedSymbol).second;
            const TString &name = symbol->getName();

            if (symbol->isVariable())
            {
                const TVariable *variable = static_cast<const TVariable*>(symbol);
                TQualifier qualifier = variable->getType().getQualifier();

                if (qualifier == EvqVaryingOut || qualifier == EvqInvariantVaryingOut)
                {
                    if (mReferencedVaryings.find(name.c_str()) != mReferencedVaryings.end())
                    {
                        // Program linking depends on this exact format
                        out << "    output." + decorate(name) + " = " + decorate(name) + ";\n";
                    }
                }
            }
        }
    }

    out << "    return output;\n"
           "}\n";
}

void OutputHLSL::visitSymbol(TIntermSymbol *node)
{
    TInfoSinkBase &out = mBody;

    TString name = node->getSymbol();

    if (name == "gl_FragColor")
    {
        out << "gl_Color[0]";
    }
    else if (name == "gl_FragData")
    {
        out << "gl_Color";
    }
    else
    {
        TQualifier qualifier = node->getQualifier();

        if (qualifier == EvqUniform)
        {
            mReferencedUniforms.insert(name.c_str());
        }
        else if (qualifier == EvqAttribute)
        {
            mReferencedAttributes.insert(name.c_str());
        }
        else if (qualifier == EvqVaryingOut || qualifier == EvqInvariantVaryingOut || qualifier == EvqVaryingIn || qualifier == EvqInvariantVaryingIn)
        {
            mReferencedVaryings.insert(name.c_str());
        }

        out << decorate(name);
    }
}

bool OutputHLSL::visitBinary(Visit visit, TIntermBinary *node)
{
    TInfoSinkBase &out = mBody;

    switch (node->getOp())
    {
      case EOpAssign:                  outputTriplet(visit, "(", " = ", ")");           break;
      case EOpInitialize:              outputTriplet(visit, "", " = ", "");             break;
      case EOpAddAssign:               outputTriplet(visit, "(", " += ", ")");          break;
      case EOpSubAssign:               outputTriplet(visit, "(", " -= ", ")");          break;
      case EOpMulAssign:               outputTriplet(visit, "(", " *= ", ")");          break;
      case EOpVectorTimesScalarAssign: outputTriplet(visit, "(", " *= ", ")");          break;
      case EOpMatrixTimesScalarAssign: outputTriplet(visit, "(", " *= ", ")");          break;
      case EOpVectorTimesMatrixAssign:
        if (visit == PreVisit)
        {
            out << "(";
        }
        else if (visit == InVisit)
        {
            out << " = mul(";
            node->getLeft()->traverse(this);
            out << ", transpose(";   
        }
        else
        {
            out << "))";
        }
        break;
      case EOpMatrixTimesMatrixAssign:
        if (visit == PreVisit)
        {
            out << "(";
        }
        else if (visit == InVisit)
        {
            out << " = mul(";
            node->getLeft()->traverse(this);
            out << ", ";   
        }
        else
        {
            out << ")";
        }
        break;
      case EOpDivAssign:               outputTriplet(visit, "(", " /= ", ")");          break;
      case EOpIndexDirect:             outputTriplet(visit, "", "[", "]");              break;
      case EOpIndexIndirect:           outputTriplet(visit, "", "[", "]");              break;
      case EOpIndexDirectStruct:       outputTriplet(visit, "", ".", "");               break;
      case EOpVectorSwizzle:
        if (visit == InVisit)
        {
            out << ".";

            TIntermAggregate *swizzle = node->getRight()->getAsAggregate();

            if (swizzle)
            {
                TIntermSequence &sequence = swizzle->getSequence();

                for (TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); sit++)
                {
                    TIntermConstantUnion *element = (*sit)->getAsConstantUnion();

                    if (element)
                    {
                        int i = element->getUnionArrayPointer()[0].getIConst();

                        switch (i)
                        {
                        case 0: out << "x"; break;
                        case 1: out << "y"; break;
                        case 2: out << "z"; break;
                        case 3: out << "w"; break;
                        default: UNREACHABLE();
                        }
                    }
                    else UNREACHABLE();
                }
            }
            else UNREACHABLE();

            return false;   // Fully processed
        }
        break;
      case EOpAdd:               outputTriplet(visit, "(", " + ", ")"); break;
      case EOpSub:               outputTriplet(visit, "(", " - ", ")"); break;
      case EOpMul:               outputTriplet(visit, "(", " * ", ")"); break;
      case EOpDiv:               outputTriplet(visit, "(", " / ", ")"); break;
      case EOpEqual:
      case EOpNotEqual:
        if (node->getLeft()->isScalar())
        {
            if (node->getOp() == EOpEqual)
            {
                outputTriplet(visit, "(", " == ", ")");
            }
            else
            {
                outputTriplet(visit, "(", " != ", ")");
            }
        }
        else if (node->getLeft()->getBasicType() == EbtStruct)
        {
            if (node->getOp() == EOpEqual)
            {
                out << "(";
            }
            else
            {
                out << "!(";
            }

            const TTypeList *fields = node->getLeft()->getType().getStruct();

            for (size_t i = 0; i < fields->size(); i++)
            {
                const TType *fieldType = (*fields)[i].type;

                node->getLeft()->traverse(this);
                out << "." + fieldType->getFieldName() + " == ";
                node->getRight()->traverse(this);
                out << "." + fieldType->getFieldName();

                if (i < fields->size() - 1)
                {
                    out << " && ";
                }
            }

            out << ")";

            return false;
        }
        else
        {
            if (node->getLeft()->isMatrix())
            {
                switch (node->getLeft()->getSize())
                {
                  case 2 * 2: mUsesEqualMat2 = true; break;
                  case 3 * 3: mUsesEqualMat3 = true; break;
                  case 4 * 4: mUsesEqualMat4 = true; break;
                  default: UNREACHABLE();
                }
            }
            else if (node->getLeft()->isVector())
            {
                switch (node->getLeft()->getBasicType())
                {
                  case EbtFloat:
                    switch (node->getLeft()->getSize())
                    {
                      case 2: mUsesEqualVec2 = true; break;
                      case 3: mUsesEqualVec3 = true; break;
                      case 4: mUsesEqualVec4 = true; break;
                      default: UNREACHABLE();
                    }
                    break;
                  case EbtInt:
                    switch (node->getLeft()->getSize())
                    {
                      case 2: mUsesEqualIVec2 = true; break;
                      case 3: mUsesEqualIVec3 = true; break;
                      case 4: mUsesEqualIVec4 = true; break;
                      default: UNREACHABLE();
                    }
                    break;
                  case EbtBool:
                    switch (node->getLeft()->getSize())
                    {
                      case 2: mUsesEqualBVec2 = true; break;
                      case 3: mUsesEqualBVec3 = true; break;
                      case 4: mUsesEqualBVec4 = true; break;
                      default: UNREACHABLE();
                    }
                    break;
                  default: UNREACHABLE();
                }
            }
            else UNREACHABLE();

            if (node->getOp() == EOpEqual)
            {
                outputTriplet(visit, "equal(", ", ", ")");
            }
            else
            {
                outputTriplet(visit, "!equal(", ", ", ")");
            }
        }
        break;
      case EOpLessThan:          outputTriplet(visit, "(", " < ", ")");   break;
      case EOpGreaterThan:       outputTriplet(visit, "(", " > ", ")");   break;
      case EOpLessThanEqual:     outputTriplet(visit, "(", " <= ", ")");  break;
      case EOpGreaterThanEqual:  outputTriplet(visit, "(", " >= ", ")");  break;
      case EOpVectorTimesScalar: outputTriplet(visit, "(", " * ", ")");   break;
      case EOpMatrixTimesScalar: outputTriplet(visit, "(", " * ", ")");   break;
      case EOpVectorTimesMatrix: outputTriplet(visit, "mul(", ", transpose(", "))"); break;
      case EOpMatrixTimesVector: outputTriplet(visit, "mul(transpose(", "), ", ")"); break;
      case EOpMatrixTimesMatrix: outputTriplet(visit, "transpose(mul(transpose(", "), transpose(", ")))"); break;
      case EOpLogicalOr:         outputTriplet(visit, "(", " || ", ")");  break;
      case EOpLogicalXor:        outputTriplet(visit, "xor(", ", ", ")"); break;
      case EOpLogicalAnd:        outputTriplet(visit, "(", " && ", ")");  break;
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::visitUnary(Visit visit, TIntermUnary *node)
{
    TInfoSinkBase &out = mBody;

    switch (node->getOp())
    {
      case EOpNegative:         outputTriplet(visit, "(-", "", ")");  break;
      case EOpVectorLogicalNot: outputTriplet(visit, "(!", "", ")");  break;
      case EOpLogicalNot:       outputTriplet(visit, "(!", "", ")");  break;
      case EOpPostIncrement:    outputTriplet(visit, "(", "", "++)"); break;
      case EOpPostDecrement:    outputTriplet(visit, "(", "", "--)"); break;
      case EOpPreIncrement:     outputTriplet(visit, "(++", "", ")"); break;
      case EOpPreDecrement:     outputTriplet(visit, "(--", "", ")"); break;
      case EOpConvIntToBool:
      case EOpConvFloatToBool:
        switch (node->getOperand()->getType().getNominalSize())
        {
          case 1:    outputTriplet(visit, "bool(", "", ")");  break;
          case 2:    outputTriplet(visit, "bool2(", "", ")"); break;
          case 3:    outputTriplet(visit, "bool3(", "", ")"); break;
          case 4:    outputTriplet(visit, "bool4(", "", ")"); break;
          default: UNREACHABLE();
        }
        break;
      case EOpConvBoolToFloat:
      case EOpConvIntToFloat:
        switch (node->getOperand()->getType().getNominalSize())
        {
          case 1:    outputTriplet(visit, "float(", "", ")");  break;
          case 2:    outputTriplet(visit, "float2(", "", ")"); break;
          case 3:    outputTriplet(visit, "float3(", "", ")"); break;
          case 4:    outputTriplet(visit, "float4(", "", ")"); break;
          default: UNREACHABLE();
        }
        break;
      case EOpConvFloatToInt:
      case EOpConvBoolToInt:
        switch (node->getOperand()->getType().getNominalSize())
        {
          case 1:    outputTriplet(visit, "int(", "", ")");  break;
          case 2:    outputTriplet(visit, "int2(", "", ")"); break;
          case 3:    outputTriplet(visit, "int3(", "", ")"); break;
          case 4:    outputTriplet(visit, "int4(", "", ")"); break;
          default: UNREACHABLE();
        }
        break;
      case EOpRadians:          outputTriplet(visit, "radians(", "", ")");   break;
      case EOpDegrees:          outputTriplet(visit, "degrees(", "", ")");   break;
      case EOpSin:              outputTriplet(visit, "sin(", "", ")");       break;
      case EOpCos:              outputTriplet(visit, "cos(", "", ")");       break;
      case EOpTan:              outputTriplet(visit, "tan(", "", ")");       break;
      case EOpAsin:             outputTriplet(visit, "asin(", "", ")");      break;
      case EOpAcos:             outputTriplet(visit, "acos(", "", ")");      break;
      case EOpAtan:             outputTriplet(visit, "atan(", "", ")");      break;
      case EOpExp:              outputTriplet(visit, "exp(", "", ")");       break;
      case EOpLog:              outputTriplet(visit, "log(", "", ")");       break;
      case EOpExp2:             outputTriplet(visit, "exp2(", "", ")");      break;
      case EOpLog2:             outputTriplet(visit, "log2(", "", ")");      break;
      case EOpSqrt:             outputTriplet(visit, "sqrt(", "", ")");      break;
      case EOpInverseSqrt:      outputTriplet(visit, "rsqrt(", "", ")");     break;
      case EOpAbs:              outputTriplet(visit, "abs(", "", ")");       break;
      case EOpSign:             outputTriplet(visit, "sign(", "", ")");      break;
      case EOpFloor:            outputTriplet(visit, "floor(", "", ")");     break;
      case EOpCeil:             outputTriplet(visit, "ceil(", "", ")");      break;
      case EOpFract:            outputTriplet(visit, "frac(", "", ")");      break;
      case EOpLength:           outputTriplet(visit, "length(", "", ")");    break;
      case EOpNormalize:        outputTriplet(visit, "normalize(", "", ")"); break;
//    case EOpDPdx:             outputTriplet(visit, "ddx(", "", ")");       break;
//    case EOpDPdy:             outputTriplet(visit, "ddy(", "", ")");       break;
//    case EOpFwidth:           outputTriplet(visit, "fwidth(", "", ")");    break;        
      case EOpAny:              outputTriplet(visit, "any(", "", ")");       break;
      case EOpAll:              outputTriplet(visit, "all(", "", ")");       break;
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::visitAggregate(Visit visit, TIntermAggregate *node)
{
    EShLanguage language = mContext.language;
    TInfoSinkBase &out = mBody;

    switch (node->getOp())
    {
      case EOpSequence:
        {
            if (mInsideFunction)
            {
                out << "{\n";
            }

            for (TIntermSequence::iterator sit = node->getSequence().begin(); sit != node->getSequence().end(); sit++)
            {
                if (isSingleStatement(*sit))
                {
                    mUnfoldSelect->traverse(*sit);
                }

                (*sit)->traverse(this);

                out << ";\n";
            }

            if (mInsideFunction)
            {
                out << "}\n";
            }

            return false;
        }
      case EOpDeclaration:
        if (visit == PreVisit)
        {
            TIntermSequence &sequence = node->getSequence();
            TIntermTyped *variable = sequence[0]->getAsTyped();
            bool visit = true;

            if (variable && (variable->getQualifier() == EvqTemporary || variable->getQualifier() == EvqGlobal))
            {
                if (!variable->getAsSymbolNode() || variable->getAsSymbolNode()->getSymbol() != "")   // Variable declaration
                {
                    if (!mInsideFunction)
                    {
                        out << "static ";
                    }

                    out << typeString(variable->getType()) + " ";

                    for (TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); sit++)
                    {
                        TIntermSymbol *symbol = (*sit)->getAsSymbolNode();

                        if (symbol)
                        {
                            symbol->traverse(this);
                            out << arrayString(symbol->getType());
                            out << " = " + initializer(variable->getType());
                        }
                        else
                        {
                            (*sit)->traverse(this);
                        }

                        if (visit && this->inVisit)
                        {
                            if (*sit != sequence.back())
                            {
                                visit = this->visitAggregate(InVisit, node);
                            }
                        }
                    }

                    if (visit && this->postVisit)
                    {
                        this->visitAggregate(PostVisit, node);
                    }
                }
                else if (variable->getAsSymbolNode() && variable->getAsSymbolNode()->getSymbol() == "")   // Type (struct) declaration
                {
                    const TType &type = variable->getType();
                    const TTypeList &fields = *type.getStruct();

                    out << "struct " + decorate(type.getTypeName()) + "\n"
                           "{\n";

                    for (unsigned int i = 0; i < fields.size(); i++)
                    {
                        const TType &field = *fields[i].type;

                        out << "    " + typeString(field) + " " + field.getFieldName() + ";\n";
                    }

                    out << "};\n";
                }
                else UNREACHABLE();
            }
            
            return false;
        }
        else if (visit == InVisit)
        {
            out << ", ";
        }
        break;
      case EOpPrototype:
        if (visit == PreVisit)
        {
            out << typeString(node->getType()) << " " << decorate(node->getName()) << "(";

            TIntermSequence &arguments = node->getSequence();

            for (unsigned int i = 0; i < arguments.size(); i++)
            {
                TIntermSymbol *symbol = arguments[i]->getAsSymbolNode();

                if (symbol)
                {
                    out << argumentString(symbol);

                    if (i < arguments.size() - 1)
                    {
                        out << ", ";
                    }
                }
                else UNREACHABLE();
            }

            out << ");\n";

            return false;
        }
        break;
      case EOpComma:            outputTriplet(visit, "", ", ", "");                break;
      case EOpFunction:
        {
            TString name = TFunction::unmangleName(node->getName());

            if (visit == PreVisit)
            {
                out << typeString(node->getType()) << " ";

                if (name == "main")
                {
                    out << "gl_main(";
                }
                else
                {
                    out << decorate(name) << "(";
                }

                TIntermSequence &sequence = node->getSequence();
                TIntermSequence &arguments = sequence[0]->getAsAggregate()->getSequence();

                for (unsigned int i = 0; i < arguments.size(); i++)
                {
                    TIntermSymbol *symbol = arguments[i]->getAsSymbolNode();

                    if (symbol)
                    {
                        out << argumentString(symbol);

                        if (i < arguments.size() - 1)
                        {
                            out << ", ";
                        }
                    }
                    else UNREACHABLE();
                }

                sequence.erase(sequence.begin());

                out << ")\n"
                       "{\n";

                mInsideFunction = true;
            }
            else if (visit == PostVisit)
            {
                out << "}\n";

                mInsideFunction = false;
            }
        }
        break;
      case EOpFunctionCall:
        {
            if (visit == PreVisit)
            {
                TString name = TFunction::unmangleName(node->getName());

                if (node->isUserDefined())
                {
                    out << decorate(name) << "(";
                }
                else
                {
                    if (name == "texture2D")
                    {
                        if (node->getSequence().size() == 2)
                        {
                            mUsesTexture2D = true;
                        }
                        else if (node->getSequence().size() == 3)
                        {
                            mUsesTexture2D_bias = true;
                        }
                        else UNREACHABLE();

                        out << "gl_texture2D(";
                    }
                    else if (name == "texture2DProj")
                    {
                        if (node->getSequence().size() == 2)
                        {
                            mUsesTexture2DProj = true;
                        }
                        else if (node->getSequence().size() == 3)
                        {
                            mUsesTexture2DProj_bias = true;
                        }
                        else UNREACHABLE();

                        out << "gl_texture2DProj(";
                    }
                    else if (name == "textureCube")
                    {
                        if (node->getSequence().size() == 2)
                        {
                            mUsesTextureCube = true;
                        }
                        else if (node->getSequence().size() == 3)
                        {
                            mUsesTextureCube_bias = true;
                        }
                        else UNREACHABLE();

                        out << "gl_textureCube(";
                    }
                    else if (name == "texture2DLod")
                    {
                        UNIMPLEMENTED();   // Requires the vertex shader texture sampling extension
                    }
                    else if (name == "texture2DProjLod")
                    {
                        UNIMPLEMENTED();   // Requires the vertex shader texture sampling extension
                    }
                    else if (name == "textureCubeLod")
                    {
                        UNIMPLEMENTED();   // Requires the vertex shader texture sampling extension
                    }
                    else UNREACHABLE();
                }
            }
            else if (visit == InVisit)
            {
                out << ", ";
            }
            else
            {
                out << ")";
            }
        }
        break;
      case EOpParameters:       outputTriplet(visit, "(", ", ", ")\n{\n");             break;
      case EOpConstructFloat:
        addConstructor(node->getType(), "vec1", &node->getSequence());
        outputTriplet(visit, "vec1(", "", ")");
        break;
      case EOpConstructVec2:
        addConstructor(node->getType(), "vec2", &node->getSequence());
        outputTriplet(visit, "vec2(", ", ", ")");
        break;
      case EOpConstructVec3:
        addConstructor(node->getType(), "vec3", &node->getSequence());
        outputTriplet(visit, "vec3(", ", ", ")");
        break;
      case EOpConstructVec4:
        addConstructor(node->getType(), "vec4", &node->getSequence());
        outputTriplet(visit, "vec4(", ", ", ")");
        break;
      case EOpConstructBool:
        addConstructor(node->getType(), "bvec1", &node->getSequence());
        outputTriplet(visit, "bvec1(", "", ")");
        break;
      case EOpConstructBVec2:
        addConstructor(node->getType(), "bvec2", &node->getSequence());
        outputTriplet(visit, "bvec2(", ", ", ")");
        break;
      case EOpConstructBVec3:
        addConstructor(node->getType(), "bvec3", &node->getSequence());
        outputTriplet(visit, "bvec3(", ", ", ")");
        break;
      case EOpConstructBVec4:
        addConstructor(node->getType(), "bvec4", &node->getSequence());
        outputTriplet(visit, "bvec4(", ", ", ")");
        break;
      case EOpConstructInt:
        addConstructor(node->getType(), "ivec1", &node->getSequence());
        outputTriplet(visit, "ivec1(", "", ")");
        break;
      case EOpConstructIVec2:
        addConstructor(node->getType(), "ivec2", &node->getSequence());
        outputTriplet(visit, "ivec2(", ", ", ")");
        break;
      case EOpConstructIVec3:
        addConstructor(node->getType(), "ivec3", &node->getSequence());
        outputTriplet(visit, "ivec3(", ", ", ")");
        break;
      case EOpConstructIVec4:
        addConstructor(node->getType(), "ivec4", &node->getSequence());
        outputTriplet(visit, "ivec4(", ", ", ")");
        break;
      case EOpConstructMat2:
        addConstructor(node->getType(), "mat2", &node->getSequence());
        outputTriplet(visit, "mat2(", ", ", ")");
        break;
      case EOpConstructMat3:
        addConstructor(node->getType(), "mat3", &node->getSequence());
        outputTriplet(visit, "mat3(", ", ", ")");
        break;
      case EOpConstructMat4: 
        addConstructor(node->getType(), "mat4", &node->getSequence());
        outputTriplet(visit, "mat4(", ", ", ")");
        break;
      case EOpConstructStruct:  outputTriplet(visit, "{", ", ", "}");                  break;
      case EOpLessThan:         outputTriplet(visit, "(", " < ", ")");                 break;
      case EOpGreaterThan:      outputTriplet(visit, "(", " > ", ")");                 break;
      case EOpLessThanEqual:    outputTriplet(visit, "(", " <= ", ")");                break;
      case EOpGreaterThanEqual: outputTriplet(visit, "(", " >= ", ")");                break;
      case EOpVectorEqual:      outputTriplet(visit, "(", " == ", ")");                break;
      case EOpVectorNotEqual:   outputTriplet(visit, "(", " != ", ")");                break;
      case EOpMod:              outputTriplet(visit, "mod(", ", ", ")");               break;
      case EOpPow:              outputTriplet(visit, "pow(", ", ", ")");               break;
      case EOpAtan:
        if (node->getSequence().size() == 1)
        {
            outputTriplet(visit, "atan(", ", ", ")");
        }
        else if (node->getSequence().size() == 2)
        {
            outputTriplet(visit, "atan2(", ", ", ")");
        }
        else UNREACHABLE();
        break;
      case EOpMin:           outputTriplet(visit, "min(", ", ", ")");           break;
      case EOpMax:           outputTriplet(visit, "max(", ", ", ")");           break;
      case EOpClamp:         outputTriplet(visit, "clamp(", ", ", ")");         break;
      case EOpMix:           outputTriplet(visit, "lerp(", ", ", ")");          break;
      case EOpStep:          outputTriplet(visit, "step(", ", ", ")");          break;
      case EOpSmoothStep:    outputTriplet(visit, "smoothstep(", ", ", ")");    break;
      case EOpDistance:      outputTriplet(visit, "distance(", ", ", ")");      break;
      case EOpDot:           outputTriplet(visit, "dot(", ", ", ")");           break;
      case EOpCross:         outputTriplet(visit, "cross(", ", ", ")");         break;
      case EOpFaceForward:
        {
            switch (node->getSequence()[0]->getAsTyped()->getSize())   // Number of components in the first argument
            {
            case 1: mUsesFaceforward1 = true; break;
            case 2: mUsesFaceforward2 = true; break;
            case 3: mUsesFaceforward3 = true; break;
            case 4: mUsesFaceforward4 = true; break;
            default: UNREACHABLE();
            }
            
            outputTriplet(visit, "faceforward(", ", ", ")");
        }
        break;
      case EOpReflect:       outputTriplet(visit, "reflect(", ", ", ")");       break;
      case EOpRefract:       outputTriplet(visit, "refract(", ", ", ")");       break;
      case EOpMul:           outputTriplet(visit, "(", " * ", ")");             break;
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::visitSelection(Visit visit, TIntermSelection *node)
{
    TInfoSinkBase &out = mBody;

    if (node->usesTernaryOperator())
    {
        out << "t" << mUnfoldSelect->getTemporaryIndex();
    }
    else  // if/else statement
    {
        mUnfoldSelect->traverse(node->getCondition());

        out << "if(";

        node->getCondition()->traverse(this);

        out << ")\n"
               "{\n";

        if (node->getTrueBlock())
        {
            node->getTrueBlock()->traverse(this);
        }

        out << ";}\n";

        if (node->getFalseBlock())
        {
            out << "else\n"
                   "{\n";

            node->getFalseBlock()->traverse(this);

            out << ";}\n";
        }
    }

    return false;
}

void OutputHLSL::visitConstantUnion(TIntermConstantUnion *node)
{
    TInfoSinkBase &out = mBody;
    
    const TType &type = node->getType();

    if (type.isField())
    {
        out << type.getFieldName();
    }
    else
    {
        int size = type.getObjectSize();

        if (type.getBasicType() == EbtStruct)
        {
            out << "{";
        }
        else
        {
            bool matrix = type.isMatrix();
            TBasicType elementType = node->getUnionArrayPointer()[0].getType();

            switch (elementType)
            {
              case EbtBool:
                if (!matrix)
                {
                    switch (size)
                    {
                      case 1: out << "bool(";  break;
                      case 2: out << "bool2("; break;
                      case 3: out << "bool3("; break;
                      case 4: out << "bool4("; break;
                      default: UNREACHABLE();
                    }
                }
                else UNREACHABLE();
                break;
              case EbtFloat:
                if (!matrix)
                {
                    switch (size)
                    {
                      case 1: out << "float(";  break;
                      case 2: out << "float2("; break;
                      case 3: out << "float3("; break;
                      case 4: out << "float4("; break;
                      default: UNREACHABLE();
                    }
                }
                else
                {
                    switch (size)
                    {
                      case 4:  out << "float2x2("; break;
                      case 9:  out << "float3x3("; break;
                      case 16: out << "float4x4("; break;
                      default: UNREACHABLE();
                    }
                }
                break;
              case EbtInt:
                if (!matrix)
                {
                    switch (size)
                    {
                      case 1: out << "int(";  break;
                      case 2: out << "int2("; break;
                      case 3: out << "int3("; break;
                      case 4: out << "int4("; break;
                      default: UNREACHABLE();
                    }
                }
                else UNREACHABLE();
                break;
              default:
                UNIMPLEMENTED();   // FIXME
            }
        }

        for (int i = 0; i < size; i++)
        {
            switch (node->getUnionArrayPointer()[i].getType())
            {
              case EbtBool:
                if (node->getUnionArrayPointer()[i].getBConst())
                {
                    out << "true";
                }
                else
                {
                    out << "false";
                }
                break;
              case EbtFloat:
                out << node->getUnionArrayPointer()[i].getFConst();           
                break;
              case EbtInt:
                out << node->getUnionArrayPointer()[i].getIConst();
                break;
              default: 
                UNIMPLEMENTED();   // FIXME
            }

            if (i != size - 1)
            {
                out << ", ";
            }
        }

        if (type.getBasicType() == EbtStruct)
        {
            out << "}";
        }
        else
        {
            out << ")";
        }
    }
}

bool OutputHLSL::visitLoop(Visit visit, TIntermLoop *node)
{
    if (handleExcessiveLoop(node))
    {
        return false;
    }

    TInfoSinkBase &out = mBody;

    if (!node->testFirst())
    {
        out << "do\n"
               "{\n";
    }
    else
    {
        if (node->getInit())
        {
            mUnfoldSelect->traverse(node->getInit());
        }
        
        if (node->getTest())
        {
            mUnfoldSelect->traverse(node->getTest());
        }
        
        if (node->getTerminal())
        {
            mUnfoldSelect->traverse(node->getTerminal());
        }

        out << "for(";
        
        if (node->getInit())
        {
            node->getInit()->traverse(this);
        }

        out << "; ";

        if (node->getTest())
        {
            node->getTest()->traverse(this);
        }

        out << "; ";

        if (node->getTerminal())
        {
            node->getTerminal()->traverse(this);
        }

        out << ")\n"
               "{\n";
    }

    if (node->getBody())
    {
        node->getBody()->traverse(this);
    }

    out << "}\n";

    if (!node->testFirst())
    {
        out << "while(\n";

        node->getTest()->traverse(this);

        out << ")";
    }

    out << ";\n";

    return false;
}

bool OutputHLSL::visitBranch(Visit visit, TIntermBranch *node)
{
    TInfoSinkBase &out = mBody;

    switch (node->getFlowOp())
    {
      case EOpKill:     outputTriplet(visit, "discard", "", "");  break;
      case EOpBreak:    outputTriplet(visit, "break", "", "");    break;
      case EOpContinue: outputTriplet(visit, "continue", "", ""); break;
      case EOpReturn:
        if (visit == PreVisit)
        {
            if (node->getExpression())
            {
                out << "return ";
            }
            else
            {
                out << "return;\n";
            }
        }
        else if (visit == PostVisit)
        {
            out << ";\n";
        }
        break;
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::isSingleStatement(TIntermNode *node)
{
    TIntermAggregate *aggregate = node->getAsAggregate();

    if (aggregate)
    {
        if (aggregate->getOp() == EOpSequence)
        {
            return false;
        }
        else
        {
            for (TIntermSequence::iterator sit = aggregate->getSequence().begin(); sit != aggregate->getSequence().end(); sit++)
            {
                if (!isSingleStatement(*sit))
                {
                    return false;
                }
            }

            return true;
        }
    }

    return true;
}

// Handle loops with more than 255 iterations (unsupported by D3D9) by splitting them
bool OutputHLSL::handleExcessiveLoop(TIntermLoop *node)
{
    TInfoSinkBase &out = mBody;

    // Parse loops of the form:
    // for(int index = initial; index [comparator] limit; index += increment)
    TIntermSymbol *index = NULL;
    TOperator comparator = EOpNull;
    int initial = 0;
    int limit = 0;
    int increment = 0;

    // Parse index name and intial value
    if (node->getInit())
    {
        TIntermAggregate *init = node->getInit()->getAsAggregate();

        if (init)
        {
            TIntermSequence &sequence = init->getSequence();
            TIntermTyped *variable = sequence[0]->getAsTyped();

            if (variable && variable->getQualifier() == EvqTemporary)
            {
                TIntermBinary *assign = variable->getAsBinaryNode();

                if (assign->getOp() == EOpInitialize)
                {
                    TIntermSymbol *symbol = assign->getLeft()->getAsSymbolNode();
                    TIntermConstantUnion *constant = assign->getRight()->getAsConstantUnion();

                    if (symbol && constant)
                    {
                        if (constant->getBasicType() == EbtInt && constant->getSize() == 1)
                        {
                            index = symbol;
                            initial = constant->getUnionArrayPointer()[0].getIConst();
                        }
                    }
                }
            }
        }
    }

    // Parse comparator and limit value
    if (index != NULL && node->getTest())
    {
        TIntermBinary *test = node->getTest()->getAsBinaryNode();
        
        if (test && test->getLeft()->getAsSymbolNode()->getId() == index->getId())
        {
            TIntermConstantUnion *constant = test->getRight()->getAsConstantUnion();

            if (constant)
            {
                if (constant->getBasicType() == EbtInt && constant->getSize() == 1)
                {
                    comparator = test->getOp();
                    limit = constant->getUnionArrayPointer()[0].getIConst();
                }
            }
        }
    }

    // Parse increment
    if (index != NULL && comparator != EOpNull && node->getTerminal())
    {
        TIntermBinary *binaryTerminal = node->getTerminal()->getAsBinaryNode();
        TIntermUnary *unaryTerminal = node->getTerminal()->getAsUnaryNode();
        
        if (binaryTerminal)
        {
            TOperator op = binaryTerminal->getOp();
            TIntermConstantUnion *constant = binaryTerminal->getRight()->getAsConstantUnion();

            if (constant)
            {
                if (constant->getBasicType() == EbtInt && constant->getSize() == 1)
                {
                    int value = constant->getUnionArrayPointer()[0].getIConst();

                    switch (op)
                    {
                      case EOpAddAssign: increment = value;  break;
                      case EOpSubAssign: increment = -value; break;
                      default: UNIMPLEMENTED();
                    }
                }
            }
        }
        else if (unaryTerminal)
        {
            TOperator op = unaryTerminal->getOp();

            switch (op)
            {
              case EOpPostIncrement: increment = 1;  break;
              case EOpPostDecrement: increment = -1; break;
              case EOpPreIncrement:  increment = 1;  break;
              case EOpPreDecrement:  increment = -1; break;
              default: UNIMPLEMENTED();
            }
        }
    }

    if (index != NULL && comparator != EOpNull && increment != 0)
    {
        if (comparator == EOpLessThanEqual)
        {
            comparator = EOpLessThan;
            limit += 1;
        }

        if (comparator == EOpLessThan)
        {
            int iterations = (limit - initial + 1) / increment;

            if (iterations <= 255)
            {
                return false;   // Not an excessive loop
            }

            while (iterations > 0)
            {
                int remainder = (limit - initial + 1) % increment;
                int clampedLimit = initial + increment * std::min(255, iterations) - 1 - remainder;

                // for(int index = initial; index < clampedLimit; index += increment)

                out << "for(int ";
                index->traverse(this);
                out << " = ";
                out << initial;

                out << "; ";
                index->traverse(this);
                out << " < ";
                out << clampedLimit;

                out << "; ";
                index->traverse(this);
                out << " += ";
                out << increment;
                out << ")\n"
                       "{\n";

                if (node->getBody())
                {
                    node->getBody()->traverse(this);
                }

                out << "}\n";

                initial += 255 * increment;
                iterations -= 255;
            }

            return true;
        }
        else UNIMPLEMENTED();
    }

    return false;   // Not handled as an excessive loop
}

void OutputHLSL::outputTriplet(Visit visit, const TString &preString, const TString &inString, const TString &postString)
{
    TInfoSinkBase &out = mBody;

    if (visit == PreVisit)
    {
        out << preString;
    }
    else if (visit == InVisit)
    {
        out << inString;
    }
    else if (visit == PostVisit)
    {
        out << postString;
    }
}

TString OutputHLSL::argumentString(const TIntermSymbol *symbol)
{
    TQualifier qualifier = symbol->getQualifier();
    const TType &type = symbol->getType();
    TString name = symbol->getSymbol();

    if (name.empty())   // HLSL demands named arguments, also for prototypes
    {
        name = "x" + str(mArgumentIndex++);
    }
    else
    {
        name = decorate(name);
    }

    return qualifierString(qualifier) + " " + typeString(type) + " " + name + arrayString(type);
}

TString OutputHLSL::qualifierString(TQualifier qualifier)
{
    switch(qualifier)
    {
      case EvqIn:            return "in";
      case EvqOut:           return "out";
      case EvqInOut:         return "inout";
      case EvqConstReadOnly: return "const";
      default: UNREACHABLE();
    }

    return "";
}

TString OutputHLSL::typeString(const TType &type)
{
    if (type.getBasicType() == EbtStruct)
    {
        return decorate(type.getTypeName());
    }
    else if (type.isMatrix())
    {
        switch (type.getNominalSize())
        {
          case 2: return "float2x2";
          case 3: return "float3x3";
          case 4: return "float4x4";
        }
    }
    else
    {
        switch (type.getBasicType())
        {
          case EbtFloat:
            switch (type.getNominalSize())
            {
              case 1: return "float";
              case 2: return "float2";
              case 3: return "float3";
              case 4: return "float4";
            }
          case EbtInt:
            switch (type.getNominalSize())
            {
              case 1: return "int";
              case 2: return "int2";
              case 3: return "int3";
              case 4: return "int4";
            }
          case EbtBool:
            switch (type.getNominalSize())
            {
              case 1: return "bool";
              case 2: return "bool2";
              case 3: return "bool3";
              case 4: return "bool4";
            }
          case EbtVoid:
            return "void";
          case EbtSampler2D:
            return "sampler2D";
          case EbtSamplerCube:
            return "samplerCUBE";
        }
    }

    UNIMPLEMENTED();   // FIXME
    return "<unknown type>";
}

TString OutputHLSL::arrayString(const TType &type)
{
    if (!type.isArray())
    {
        return "";
    }

    return "[" + str(type.getArraySize()) + "]";
}

TString OutputHLSL::initializer(const TType &type)
{
    TString string;

    int arraySize = type.isArray() ? type.getArraySize() : 1;

    if (type.isArray())
    {
        string += "{";
    }

    for (int element = 0; element < arraySize; element++)
    {
        string += typeString(type) + "(";

        for (int component = 0; component < type.getInstanceSize(); component++)
        {
            string += "0";

            if (component < type.getInstanceSize() - 1)
            {
                string += ", ";
            }
        }

        string += ")";

        if (element < arraySize - 1)
        {
            string += ", ";
        }
    }

    if (type.isArray())
    {
        string += "}";
    }

    return string;
}

bool OutputHLSL::CompareConstructor::operator()(const Constructor &x, const Constructor &y) const
{
    if (x.name != y.name)
    {
        return x.name < y.name;
    }

    if (x.type != y.type)
    {
        return memcmp(&x.type, &y.type, sizeof(TType)) < 0;
    }

    if (x.parameters.size() != y.parameters.size())
    {
        return x.parameters.size() < y.parameters.size();
    }

    for (unsigned int i = 0; i < x.parameters.size(); i++)
    {
        if (x.parameters[i] != y.parameters[i])
        {
            return memcmp(&x.parameters[i], &y.parameters[i], sizeof(TType)) < 0;
        }
    }

    return false;
}

void OutputHLSL::addConstructor(const TType &type, const TString &name, const TIntermSequence *parameters)
{
    Constructor constructor;

    constructor.type = type;
    constructor.name = name;

    if (parameters)
    {
        for (TIntermSequence::const_iterator parameter = parameters->begin(); parameter != parameters->end(); parameter++)
        {
            constructor.parameters.push_back((*parameter)->getAsTyped()->getType());
        }
    }

    mConstructors.insert(constructor);
}

TString OutputHLSL::decorate(const TString &string)
{
    if (string.substr(0, 3) != "gl_" && string.substr(0, 3) != "dx_")
    {
        return "_" + string;
    }
    else
    {
        return string;
    }
}
}
