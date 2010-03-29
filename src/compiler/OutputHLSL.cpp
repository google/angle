//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "OutputHLSL.h"

#include "common/debug.h"
#include "InfoSink.h"

namespace sh
{
OutputHLSL::OutputHLSL(TParseContext &context) : TIntermTraverser(true, true, true), context(context)
{
}

void OutputHLSL::header()
{
    EShLanguage language = context.language;
    TInfoSinkBase &out = context.infoSink.obj;

    if (language == EShLangFragment)
    {
        TString uniforms;
        TString varyingInput;
        TString varyingGlobals;

        TSymbolTableLevel *symbols = context.symbolTable.getGlobalLevel();
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
                    uniforms += "uniform " + typeString(type) + " " + name + arrayString(type) + ";\n";
                }
                else if (qualifier == EvqVaryingIn || qualifier == EvqInvariantVaryingIn)
                {
                    char semantic[100];
                    sprintf(semantic, " : TEXCOORD%d", semanticIndex);
                    semanticIndex += type.isArray() ? type.getArraySize() : 1;

                    varyingInput += "    " + typeString(type) + " " + name + arrayString(type) + semantic + ";\n";
                    varyingGlobals += "static " + typeString(type) + " " + name + arrayString(type) + " = " + initializer(type) + ";\n";
                }
                else if (qualifier == EvqConst)
                {
                    // Constants are repeated as literals where used
                }
                else UNREACHABLE();
            }
        }

        out << "uniform float4 gl_Window;\n"
               "uniform float2 gl_Depth;\n"
               "uniform bool __frontCCW;\n"
               "\n";
        out <<  uniforms;
        out << "\n"
               "struct PS_INPUT\n"   // FIXME: Prevent name clashes
               "{\n";
        out <<      varyingInput;
        out << "    float4 gl_FragCoord : TEXCOORD" << HLSL_FRAG_COORD_SEMANTIC << ";\n";
        out << "    float __vFace : VFACE;\n"
               "};\n"
               "\n";
        out <<    varyingGlobals;
        out << "\n"
               "struct PS_OUTPUT\n"   // FIXME: Prevent name clashes
               "{\n"
               "    float4 gl_Color[1] : COLOR;\n"
               "};\n"
               "\n"
               "static float4 gl_Color[1] = {float4(0, 0, 0, 0)};\n"
               "static float4 gl_FragCoord = float4(0, 0, 0, 0);\n"
               "static float2 gl_PointCoord = float2(0.5, 0.5);\n"
               "static bool gl_FrontFacing = false;\n"
               "\n"
               "float4 gl_texture2D(sampler2D s, float2 t)\n"
               "{\n"
               "    return tex2D(s, t);\n"
               "}\n"
               "\n"
               "float4 gl_texture2DProj(sampler2D s, float3 t)\n"
               "{\n"
               "    return tex2Dproj(s, float4(t.x, t.y, 0, t.z));\n"
               "}\n"
               "float4 gl_texture2DBias(sampler2D s, float2 t, float bias)\n"
               "{\n"
               "    return tex2Dbias(s, float4(t.x, t.y, 0, bias));\n"
               "}\n"
               "\n"
               "float4 gl_textureCube(samplerCUBE s, float3 t)\n"
               "{\n"
               "    return texCUBE(s, t);\n"
               "}\n"
               "\n";
    }
    else
    {
        TString uniforms;
        TString attributeInput;
        TString attributeGlobals;
        TString varyingOutput;
        TString varyingGlobals;
        TString globals;

        TSymbolTableLevel *symbols = context.symbolTable.getGlobalLevel();
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
                    uniforms += "uniform " + typeString(type) + " " + name + arrayString(type) + ";\n";
                }
                else if (qualifier == EvqAttribute)
                {
                    char semantic[100];
                    sprintf(semantic, " : TEXCOORD%d", semanticIndex);
                    semanticIndex += type.isArray() ? type.getArraySize() : 1;

                    attributeInput += "    " + typeString(type) + " " + name + arrayString(type) + semantic + ";\n";
                    attributeGlobals += "static " + typeString(type) + " " + name + arrayString(type) + " = " + initializer(type) + ";\n";
                }
                else if (qualifier == EvqVaryingOut || qualifier == EvqInvariantVaryingOut)
                {
                    varyingOutput += "    " + typeString(type) + " " + name + arrayString(type) + " : TEXCOORD0;\n";   // Actual semantic index assigned during link
                    varyingGlobals += "static " + typeString(type) + " " + name + arrayString(type) + " = " + initializer(type) + ";\n";
                }
                else if (qualifier == EvqGlobal)
                {
                    globals += typeString(type) + " " + name + arrayString(type) + ";\n";
                }
                else if (qualifier == EvqConst)
                {
                    // Constants are repeated as literals where used
                }
                else UNREACHABLE();
            }
        }

        out << "uniform float2 gl_HalfPixelSize;\n"
               "\n";
        out <<  uniforms;
        out << "\n";
        out <<  globals;
        out << "\n"
               "struct VS_INPUT\n"   // FIXME: Prevent name clashes
               "{\n";
        out <<        attributeInput;
        out << "};\n"
               "\n";
        out <<  attributeGlobals;
        out << "\n"
               "struct VS_OUTPUT\n"   // FIXME: Prevent name clashes
               "{\n"
               "    float4 gl_Position : POSITION;\n"
               "    float gl_PointSize : PSIZE;\n"
               "    float4 gl_FragCoord : TEXCOORD" << HLSL_FRAG_COORD_SEMANTIC << ";\n";
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
           "float vec1(float x)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return x;\n"
           "}\n"
           "\n"
           "float vec1(float2 xy)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return xy[0];\n"
           "}\n"
           "\n"
           "float vec1(float3 xyz)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return xyz[0];\n"
           "}\n"
           "\n"
           "float vec1(float4 xyzw)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return xyzw[0];\n"
           "}\n"
           "\n"
           "float2 vec2(float x)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float2(x, x);\n"
           "}\n"
           "\n"
           "float2 vec2(float x, float y)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float2(x, y);\n"
           "}\n"
           "\n"
           "float2 vec2(float2 xy)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return xy;\n"
           "}\n"
           "\n"
           "float2 vec2(float3 xyz)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float2(xyz[0], xyz[1]);\n"
           "}\n"
           "\n"
           "float2 vec2(float4 xyzw)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float2(xyzw[0], xyzw[1]);\n"
           "}\n"
           "\n"
           "float3 vec3(float x)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float3(x, x, x);\n"
           "}\n"
           "\n"
           "float3 vec3(float x, float y, float z)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float3(x, y, z);\n"
           "}\n"
           "\n"
           "float3 vec3(float2 xy, float z)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float3(xy[0], xy[1], z);\n"
           "}\n"
           "\n"
           "float3 vec3(float x, float2 yz)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float3(x, yz[0], yz[1]);\n"
           "}\n"
           "\n"
           "float3 vec3(float3 xyz)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return xyz;\n"
           "}\n"
           "\n"
           "float3 vec3(float4 xyzw)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float3(xyzw[0], xyzw[1], xyzw[2]);\n"
           "}\n"
           "\n"
           "float4 vec4(float x)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float4(x, x, x, x);\n"
           "}\n"
           "\n"
           "float4 vec4(float x, float y, float z, float w)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float4(x, y, z, w);\n"
           "}\n"
           "\n"
           "float4 vec4(float2 xy, float z, float w)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float4(xy[0], xy[1], z, w);\n"
           "}\n"
           "\n"
           "float4 vec4(float x, float2 yz, float w)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float4(x, yz[0], yz[1], w);\n"
           "}\n"
           "\n"
           "float4 vec4(float x, float y, float2 zw)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float4(x, y, zw[0], zw[1]);\n"
           "}\n"
           "\n"
           "float4 vec4(float2 xy, float2 zw)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float4(xy[0], xy[1], zw[0], zw[1]);\n"
           "}\n"
           "\n"
           "float4 vec4(float3 xyz, float w)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float4(xyz[0], xyz[1], xyz[2], w);\n"
           "}\n"
           "\n"
           "float4 vec4(float x, float3 yzw)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return float4(x, yzw[0], yzw[1], yzw[2]);\n"
           "}\n"
           "\n"
           "float4 vec4(float4 xyzw)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return xyzw;\n"
           "}\n"
           "\n"
           "bool xor(bool p, bool q)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return (p || q) && !(p && q);\n"
           "}\n"
           "\n"
           "float mod(float x, float y)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return x - y * floor(x / y);\n"
           "}\n"
           "\n"
           "float2 mod(float2 x, float y)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return x - y * floor(x / y);\n"
           "}\n"
           "\n"
           "float3 mod(float3 x, float y)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return x - y * floor(x / y);\n"
           "}\n"
           "\n"
           "float4 mod(float4 x, float y)\n"   // FIXME: Prevent name clashes
           "{\n"
           "    return x - y * floor(x / y);\n"
           "}\n"
           "\n"
           "float faceforward(float N, float I, float Nref)\n"   // FIXME: Prevent name clashes
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
           "\n"
           "float2 faceforward(float2 N, float2 I, float2 Nref)\n"   // FIXME: Prevent name clashes
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
           "\n"
           "float3 faceforward(float3 N, float3 I, float3 Nref)\n"   // FIXME: Prevent name clashes
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
           "\n"
           "float4 faceforward(float4 N, float4 I, float4 Nref)\n"   // FIXME: Prevent name clashes
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
           "\n"
           "bool __equal(float2x2 m, float2x2 n)\n"
           "{\n"
           "    return m[0][0] == n[0][0] && m[0][1] == n[0][1] &&\n"
           "           m[1][0] == n[1][0] && m[1][1] == n[1][1];\n"
           "}\n"
           "\n"
           "bool __equal(float3x3 m, float3x3 n)\n"
           "{\n"
           "    return m[0][0] == n[0][0] && m[0][1] == n[0][1] && m[0][2] == n[0][2] &&\n"
           "           m[1][0] == n[1][0] && m[1][1] == n[1][1] && m[1][2] == n[1][2] &&\n"
           "           m[2][0] == n[2][0] && m[2][1] == n[2][1] && m[2][2] == n[2][2];\n"
           "}\n"
           "\n"
           "bool __equal(float4x4 m, float4x4 n)\n"
           "{\n"
           "    return m[0][0] == n[0][0] && m[0][1] == n[0][1] && m[0][2] == n[0][2] && m[0][3] == n[0][3] &&\n"
           "           m[1][0] == n[1][0] && m[1][1] == n[1][1] && m[1][2] == n[1][2] && m[1][3] == n[1][3] &&\n"
           "           m[2][0] == n[2][0] && m[2][1] == n[2][1] && m[2][2] == n[2][2] && m[2][3] == n[2][3] &&\n"
           "           m[3][0] == n[3][0] && m[3][1] == n[3][1] && m[3][2] == n[3][2] && m[3][3] == n[3][3];\n"
           "}\n"
           "\n";
}

void OutputHLSL::footer()
{
    EShLanguage language = context.language;
    TInfoSinkBase &out = context.infoSink.obj;
    TSymbolTableLevel *symbols = context.symbolTable.getGlobalLevel();

    if (language == EShLangFragment)
    {
        out << "PS_OUTPUT main(PS_INPUT input)\n"   // FIXME: Prevent name clashes
               "{\n"
               "    float rhw = 1.0 / input.gl_FragCoord.w;\n"
               "    gl_FragCoord.x = (input.gl_FragCoord.x * rhw) * gl_Window.x + gl_Window.z;\n"
               "    gl_FragCoord.y = (input.gl_FragCoord.y * rhw) * gl_Window.y + gl_Window.w;\n"
               "    gl_FragCoord.z = (input.gl_FragCoord.z * rhw) * gl_Depth.x + gl_Depth.y;\n"
               "    gl_FragCoord.w = rhw;\n"
               "    gl_FrontFacing = __frontCCW ? (input.__vFace >= 0.0) : (input.__vFace <= 0.0);\n";

        for (TSymbolTableLevel::const_iterator namedSymbol = symbols->begin(); namedSymbol != symbols->end(); namedSymbol++)
        {
            const TSymbol *symbol = (*namedSymbol).second;
            const TString &name = symbol->getName();

            if (symbol->isVariable())
            {
                const TVariable *variable = static_cast<const TVariable*>(symbol);
                const TType &type = variable->getType();
                TQualifier qualifier = type.getQualifier();

                if (qualifier == EvqVaryingIn)
                {
                    out << "    " + name + " = input." + name + ";\n";   // FIXME: Prevent name clashes
                }
            }
        }

        out << "\n"
               "    gl_main();\n"
               "\n"
               "    PS_OUTPUT output;\n"                    // FIXME: Prevent name clashes
               "    output.gl_Color[0] = gl_Color[0];\n";   // FIXME: Prevent name clashes

        TSymbolTableLevel *symbols = context.symbolTable.getGlobalLevel();

        for (TSymbolTableLevel::const_iterator namedSymbol = symbols->begin(); namedSymbol != symbols->end(); namedSymbol++)
        {
            const TSymbol *symbol = (*namedSymbol).second;
            const TString &name = symbol->getName();
        }
    }
    else
    {
        out << "VS_OUTPUT main(VS_INPUT input)\n"   // FIXME: Prevent name clashes
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
                    out << "    " + name + " = input." + name + ";\n";   // FIXME: Prevent name clashes
                }
            }
        }

        out << "\n"
               "    gl_main();\n"
               "\n"
               "    VS_OUTPUT output;\n"   // FIXME: Prevent name clashes
               "    output.gl_Position.x = gl_Position.x - gl_HalfPixelSize.x * gl_Position.w;\n"
               "    output.gl_Position.y = -(gl_Position.y - gl_HalfPixelSize.y * gl_Position.w);\n"
               "    output.gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;\n"
               "    output.gl_Position.w = gl_Position.w;\n"
               "    output.gl_PointSize = gl_PointSize;\n"
               "    output.gl_FragCoord = gl_Position;\n";

        TSymbolTableLevel *symbols = context.symbolTable.getGlobalLevel();

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
                    out << "    output." + name + " = " + name + ";\n";   // FIXME: Prevent name clashes
                }
            }
        }
    }

    out << "    return output;\n"   // FIXME: Prevent name clashes
           "}\n";
}

void OutputHLSL::visitSymbol(TIntermSymbol *node)
{
    TInfoSinkBase &out = context.infoSink.obj;

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
        out << name;
    }
}

bool OutputHLSL::visitBinary(Visit visit, TIntermBinary *node)
{
    TInfoSinkBase &out = context.infoSink.obj;

    switch (node->getOp())
    {
      case EOpAssign:                  outputTriplet(visit, "(", " = ", ")");           break;
      case EOpInitialize:              outputTriplet(visit, NULL, " = ", NULL);         break;
      case EOpAddAssign:               outputTriplet(visit, "(", " += ", ")");          break;
      case EOpSubAssign:               outputTriplet(visit, "(", " -= ", ")");          break;
      case EOpMulAssign:               outputTriplet(visit, "(", " *= ", ")");          break;
      case EOpVectorTimesScalarAssign: outputTriplet(visit, "(", " *= ", ")");          break;
      case EOpMatrixTimesScalarAssign: outputTriplet(visit, "(", " *= ", ")");          break;
      case EOpVectorTimesMatrixAssign:
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
            out << "))";
        }
        break;
      case EOpDivAssign:               outputTriplet(visit, "(", " /= ", ")");          break;
      case EOpIndexDirect:             outputTriplet(visit, NULL, "[", "]");            break;
      case EOpIndexIndirect:           outputTriplet(visit, NULL, "[", "]");            break;
      case EOpIndexDirectStruct:       outputTriplet(visit, NULL, ".", NULL);           break;
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
        if (!node->getLeft()->isMatrix())
        {
            outputTriplet(visit, "(", " == ", ")");
        }
        else
        {
            outputTriplet(visit, "__equal(", ", ", ")");
        }
        break;
      case EOpNotEqual:
        if (!node->getLeft()->isMatrix())
        {
            outputTriplet(visit, "(", " != ", ")");
        }
        else
        {
            outputTriplet(visit, "!__equal(", ", ", ")");
        }
        break;
      case EOpLessThan:          outputTriplet(visit, "(", " < ", ")");   break;
      case EOpGreaterThan:       outputTriplet(visit, "(", " > ", ")");   break;
      case EOpLessThanEqual:     outputTriplet(visit, "(", " <= ", ")");  break;
      case EOpGreaterThanEqual:  outputTriplet(visit, "(", " >= ", ")");  break;
      case EOpVectorTimesScalar: outputTriplet(visit, "(", " * ", ")");   break;
      case EOpMatrixTimesScalar: outputTriplet(visit, "(", " * ", ")");   break;
      case EOpVectorTimesMatrix: outputTriplet(visit, "mul(", ", ", ")"); break;
      case EOpMatrixTimesVector: outputTriplet(visit, "mul(", ", ", ")"); break;
      case EOpMatrixTimesMatrix: outputTriplet(visit, "mul(", ", ", ")"); break;
      case EOpLogicalOr:         outputTriplet(visit, "(", " || ", ")");  break;
      case EOpLogicalXor:        outputTriplet(visit, "xor(", ", ", ")"); break;   // FIXME: Prevent name clashes
      case EOpLogicalAnd:        outputTriplet(visit, "(", " && ", ")");  break;
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::visitUnary(Visit visit, TIntermUnary *node)
{
    TInfoSinkBase &out = context.infoSink.obj;

    switch (node->getOp())
    {
      case EOpNegative:         outputTriplet(visit, "(-", NULL, ")");  break;
      case EOpVectorLogicalNot: outputTriplet(visit, "(!", NULL, ")");  break;
      case EOpLogicalNot:       outputTriplet(visit, "(!", NULL, ")");  break;
      case EOpPostIncrement:    outputTriplet(visit, "(", NULL, "++)"); break;
      case EOpPostDecrement:    outputTriplet(visit, "(", NULL, "--)"); break;
      case EOpPreIncrement:     outputTriplet(visit, "(++", NULL, ")"); break;
      case EOpPreDecrement:     outputTriplet(visit, "(--", NULL, ")"); break;
      case EOpConvIntToBool:
      case EOpConvFloatToBool:
        switch (node->getOperand()->getType().getNominalSize())
        {
          case 1:    outputTriplet(visit, "bool(", NULL, ")");  break;
          case 2:    outputTriplet(visit, "bool2(", NULL, ")"); break;
          case 3:    outputTriplet(visit, "bool3(", NULL, ")"); break;
          case 4:    outputTriplet(visit, "bool4(", NULL, ")"); break;
          default: UNREACHABLE();
        }
        break;
      case EOpConvBoolToFloat:
      case EOpConvIntToFloat:
        switch (node->getOperand()->getType().getNominalSize())
        {
          case 1:    outputTriplet(visit, "float(", NULL, ")");  break;
          case 2:    outputTriplet(visit, "float2(", NULL, ")"); break;
          case 3:    outputTriplet(visit, "float3(", NULL, ")"); break;
          case 4:    outputTriplet(visit, "float4(", NULL, ")"); break;
          default: UNREACHABLE();
        }
        break;
      case EOpConvFloatToInt:
      case EOpConvBoolToInt:
        switch (node->getOperand()->getType().getNominalSize())
        {
          case 1:    outputTriplet(visit, "int(", NULL, ")");  break;
          case 2:    outputTriplet(visit, "int2(", NULL, ")"); break;
          case 3:    outputTriplet(visit, "int3(", NULL, ")"); break;
          case 4:    outputTriplet(visit, "int4(", NULL, ")"); break;
          default: UNREACHABLE();
        }
        break;
      case EOpRadians:          outputTriplet(visit, "radians(", NULL, ")");   break;
      case EOpDegrees:          outputTriplet(visit, "degrees(", NULL, ")");   break;
      case EOpSin:              outputTriplet(visit, "sin(", NULL, ")");       break;
      case EOpCos:              outputTriplet(visit, "cos(", NULL, ")");       break;
      case EOpTan:              outputTriplet(visit, "tan(", NULL, ")");       break;
      case EOpAsin:             outputTriplet(visit, "asin(", NULL, ")");      break;
      case EOpAcos:             outputTriplet(visit, "acos(", NULL, ")");      break;
      case EOpAtan:             outputTriplet(visit, "atan(", NULL, ")");      break;
      case EOpExp:              outputTriplet(visit, "exp(", NULL, ")");       break;
      case EOpLog:              outputTriplet(visit, "log(", NULL, ")");       break;
      case EOpExp2:             outputTriplet(visit, "exp2(", NULL, ")");      break;
      case EOpLog2:             outputTriplet(visit, "log2(", NULL, ")");      break;
      case EOpSqrt:             outputTriplet(visit, "sqrt(", NULL, ")");      break;
      case EOpInverseSqrt:      outputTriplet(visit, "rsqrt(", NULL, ")");     break;
      case EOpAbs:              outputTriplet(visit, "abs(", NULL, ")");       break;
      case EOpSign:             outputTriplet(visit, "sign(", NULL, ")");      break;
      case EOpFloor:            outputTriplet(visit, "floor(", NULL, ")");     break;
      case EOpCeil:             outputTriplet(visit, "ceil(", NULL, ")");      break;
      case EOpFract:            outputTriplet(visit, "frac(", NULL, ")");      break;
      case EOpLength:           outputTriplet(visit, "length(", NULL, ")");    break;
      case EOpNormalize:        outputTriplet(visit, "normalize(", NULL, ")"); break;
//    case EOpDPdx:             outputTriplet(visit, "ddx(", NULL, ")");       break;
//    case EOpDPdy:             outputTriplet(visit, "ddy(", NULL, ")");       break;
//    case EOpFwidth:           outputTriplet(visit, "fwidth(", NULL, ")");    break;        
      case EOpAny:              outputTriplet(visit, "any(", NULL, ")");       break;
      case EOpAll:              outputTriplet(visit, "all(", NULL, ")");       break;
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::visitAggregate(Visit visit, TIntermAggregate *node)
{
    EShLanguage language = context.language;
    TInfoSinkBase &out = context.infoSink.obj;

    if (node->getOp() == EOpNull)
    {
        out.message(EPrefixError, "node is still EOpNull!");
        return true;
    }

    switch (node->getOp())
    {
      case EOpSequence: outputTriplet(visit, NULL, ";\n", ";\n"); break;
      case EOpDeclaration:
        if (visit == PreVisit)
        {
            TIntermSequence &sequence = node->getSequence();
            TIntermTyped *variable = sequence[0]->getAsTyped();
            bool visit = true;

            if (variable && variable->getQualifier() == EvqTemporary)
            {
                out << typeString(variable->getType()) + " ";

                for (TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); sit++)
                {
                    TIntermSymbol *symbol = (*sit)->getAsSymbolNode();

                    if (symbol)
                    {
                        symbol->traverse(this);

                        out << arrayString(symbol->getType());
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
            
            return false;
        }
        else if (visit == InVisit)
        {
            out << ", ";
        }
        break;
      case EOpComma:         UNIMPLEMENTED(); /* FIXME */ out << "Comma\n"; return true;
      case EOpFunction:
        {
            const TString &mangledName = node->getName();
            TString name = TString(mangledName.c_str(), mangledName.find_first_of('('));

            if (visit == PreVisit)
            {
                if (name == "main")
                {
                    name = "gl_main";
                }

                out << typeString(node->getType()) << " " << name << "(";

                TIntermSequence &sequence = node->getSequence();
                TIntermSequence &arguments = sequence[0]->getAsAggregate()->getSequence();

                for (unsigned int i = 0; i < arguments.size(); i++)
                {
                    TIntermSymbol *symbol = arguments[i]->getAsSymbolNode();

                    if (symbol)
                    {
                        const TType &type = symbol->getType();
                        const TString &name = symbol->getSymbol();

                        out << typeString(type) + " " + name;

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
            }
            else if (visit == PostVisit)
            {
                out << "}\n";
            }
        }
        break;
      case EOpFunctionCall:
        {
            if (visit == PreVisit)
            {
                const TString &mangledName = node->getName();
                TString name = TString(mangledName.c_str(), mangledName.find_first_of('('));

                if (node->isUserDefined())
                {
                    out << name << "(";
                }
                else
                {
                    if (name == "texture2D")
                    {
                        if (node->getSequence().size() == 2)
                        {
                            out << "gl_texture2D(";
                        }
                        else if (node->getSequence().size() == 3)
                        {
                            out << "gl_texture2DBias(";
                        }
                        else UNREACHABLE();
                    }
                    else if (name == "texture2DProj")
                    {
                        out << "gl_texture2DProj(";
                    }
                    else if (name == "texture2DLod")
                    {
                        out << "gl_texture2DLod(";
                        UNIMPLEMENTED();   // FIXME: Move lod to last texture coordinate component
                    }
                    else if (name == "texture2DProjLod")
                    {
                        out << "gl_texture2DProjLod(";
                        UNIMPLEMENTED();   // FIXME: Move lod to last texture coordinate component
                    }
                    else if (name == "textureCube")
                    {
                        out << "gl_textureCube(";   // FIXME: Incorrect sampling location
                    }
                    else
                    {
                        UNIMPLEMENTED();   // FIXME
                    }
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
      case EOpParameters:       outputTriplet(visit, "(", ", ", ")\n{\n");  break;
      case EOpConstructFloat:   outputTriplet(visit, "vec1(", NULL, ")");   break;
      case EOpConstructVec2:    outputTriplet(visit, "vec2(", ", ", ")");   break;
      case EOpConstructVec3:    outputTriplet(visit, "vec3(", ", ", ")");   break;
      case EOpConstructVec4:    outputTriplet(visit, "vec4(", ", ", ")");   break;
      case EOpConstructBool:    UNIMPLEMENTED(); /* FIXME */ out << "Construct bool";  break;
      case EOpConstructBVec2:   UNIMPLEMENTED(); /* FIXME */ out << "Construct bvec2"; break;
      case EOpConstructBVec3:   UNIMPLEMENTED(); /* FIXME */ out << "Construct bvec3"; break;
      case EOpConstructBVec4:   UNIMPLEMENTED(); /* FIXME */ out << "Construct bvec4"; break;
      case EOpConstructInt:     UNIMPLEMENTED(); /* FIXME */ out << "Construct int";   break;
      case EOpConstructIVec2:   UNIMPLEMENTED(); /* FIXME */ out << "Construct ivec2"; break;
      case EOpConstructIVec3:   UNIMPLEMENTED(); /* FIXME */ out << "Construct ivec3"; break;
      case EOpConstructIVec4:   UNIMPLEMENTED(); /* FIXME */ out << "Construct ivec4"; break;
      case EOpConstructMat2:    outputTriplet(visit, "float2x2(", ", ", ")"); break;
      case EOpConstructMat3:    outputTriplet(visit, "float3x3(", ", ", ")"); break;
      case EOpConstructMat4:    outputTriplet(visit, "float4x4(", ", ", ")"); break;
      case EOpConstructStruct:  UNIMPLEMENTED(); /* FIXME */ out << "Construct structure";  break;
      case EOpLessThan:         outputTriplet(visit, "(", " < ", ")");             break;
      case EOpGreaterThan:      outputTriplet(visit, "(", " > ", ")");             break;
      case EOpLessThanEqual:    outputTriplet(visit, "(", " <= ", ")");            break;
      case EOpGreaterThanEqual: outputTriplet(visit, "(", " >= ", ")");            break;
      case EOpVectorEqual:      outputTriplet(visit, "(", " == ", ")");            break;
      case EOpVectorNotEqual:   outputTriplet(visit, "(", " != ", ")");            break;
      case EOpMod:              outputTriplet(visit, "mod(", ", ", ")");           break;   // FIXME: Prevent name clashes
      case EOpPow:              outputTriplet(visit, "pow(", ", ", ")");           break;
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
      case EOpFaceForward:   outputTriplet(visit, "faceforward(", ", ", ")");   break;
      case EOpReflect:       outputTriplet(visit, "reflect(", ", ", ")");       break;
      case EOpRefract:       outputTriplet(visit, "refract(", ", ", ")");       break;
      case EOpMul:           outputTriplet(visit, "(", " * ", ")");             break;
      default: UNREACHABLE();
    }

    return true;
}

bool OutputHLSL::visitSelection(Visit visit, TIntermSelection *node)
{
    TInfoSinkBase &out = context.infoSink.obj;

    if (node->usesTernaryOperator())
    {
        out << "(";
        node->getCondition()->traverse(this);
        out << ") ? (";
        node->getTrueBlock()->traverse(this);
        out << ") : (";
        node->getFalseBlock()->traverse(this);
        out << ")\n";
    }
    else  // if/else statement
    {
        out << "if(";

        node->getCondition()->traverse(this);

        out << ")\n"
               "{\n";

        node->getTrueBlock()->traverse(this);

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
    TInfoSinkBase &out = context.infoSink.obj;
    
    TType &type = node->getType();

    if (type.isField())
    {
        out << type.getFieldName();
    }
    else
    {
        int size = type.getObjectSize();
        bool matrix = type.isMatrix();
        TBasicType basicType = node->getUnionArrayPointer()[0].getType();

        switch (basicType)
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
            else
            {
                UNIMPLEMENTED();
            }
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
            else
            {
                UNIMPLEMENTED();
            }
            break;
          default:
            UNIMPLEMENTED();   // FIXME
        }

        for (int i = 0; i < size; i++)
        {
            switch (basicType)
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

        out << ")";
    }
}

bool OutputHLSL::visitLoop(Visit visit, TIntermLoop *node)
{
    TInfoSinkBase &out = context.infoSink.obj;

    if (!node->testFirst())
    {
        out << "do\n"
               "{\n";
    }
    else
    {
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
    TInfoSinkBase &out = context.infoSink.obj;

    switch (node->getFlowOp())
    {
      case EOpKill:     outputTriplet(visit, "discard", NULL, NULL);  break;
      case EOpBreak:    outputTriplet(visit, "break", NULL, NULL);    break;
      case EOpContinue: outputTriplet(visit, "continue", NULL, NULL); break;
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

void OutputHLSL::outputTriplet(Visit visit, const char *preString, const char *inString, const char *postString)
{
    TInfoSinkBase &out = context.infoSink.obj;

    if (visit == PreVisit && preString)
    {
        out << preString;
    }
    else if (visit == InVisit && inString)
    {
        out << inString;
    }
    else if (visit == PostVisit && postString)
    {
        out << postString;
    }
}

TString OutputHLSL::typeString(const TType &type)
{
    if (type.isMatrix())
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

    char buffer[100];
    sprintf(buffer, "[%d]", type.getArraySize());

    return buffer;
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

        for (int component = 0; component < type.getNominalSize(); component++)
        {
            string += "0";

            if (component < type.getNominalSize() - 1)
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
}
