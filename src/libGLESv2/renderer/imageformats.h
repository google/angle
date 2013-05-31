//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// imageformats.h: Defines image format types with functions for mip generation
// and copying.

#ifndef LIBGLESV2_RENDERER_IMAGEFORMATS_H_
#define LIBGLESV2_RENDERER_IMAGEFORMATS_H_

namespace rx
{

struct L8
{
    unsigned char L;

    static void average(L8 *dst, const L8 *src1, const L8 *src2)
    {
        dst->L = ((src1->L ^ src2->L) >> 1) + (src1->L & src2->L);
    }
};

struct R8
{
    unsigned char R;

    static void average(R8 *dst, const R8 *src1, const R8 *src2)
    {
        dst->R = ((src1->R ^ src2->R) >> 1) + (src1->R & src2->R);
    }
};

struct A8
{
    unsigned char A;

    static void average(A8 *dst, const A8 *src1, const A8 *src2)
    {
        dst->A = ((src1->A ^ src2->A) >> 1) + (src1->A & src2->A);
    }
};

struct A8L8
{
    unsigned char L;
    unsigned char A;

    static void average(A8L8 *dst, const A8L8 *src1, const A8L8 *src2)
    {
        *(unsigned short*)dst = (((*(unsigned short*)src1 ^ *(unsigned short*)src2) & 0xFEFE) >> 1) + (*(unsigned short*)src1 & *(unsigned short*)src2);
    }
};

struct R8G8
{
    unsigned char R;
    unsigned char G;

    static void average(R8G8 *dst, const R8G8 *src1, const R8G8 *src2)
    {
        *(unsigned short*)dst = (((*(unsigned short*)src1 ^ *(unsigned short*)src2) & 0xFEFE) >> 1) + (*(unsigned short*)src1 & *(unsigned short*)src2);
    }
};

struct A8R8G8B8
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned char A;

    static void average(A8R8G8B8 *dst, const A8R8G8B8 *src1, const A8R8G8B8 *src2)
    {
        *(unsigned int*)dst = (((*(unsigned int*)src1 ^ *(unsigned int*)src2) & 0xFEFEFEFE) >> 1) + (*(unsigned int*)src1 & *(unsigned int*)src2);
    }
};

struct R8G8B8A8
{
    unsigned char R;
    unsigned char G;
    unsigned char B;
    unsigned char A;

    static void average(R8G8B8A8 *dst, const R8G8B8A8 *src1, const R8G8B8A8 *src2)
    {
        *(unsigned int*)dst = (((*(unsigned int*)src1 ^ *(unsigned int*)src2) & 0xFEFEFEFE) >> 1) + (*(unsigned int*)src1 & *(unsigned int*)src2);
    }
};

struct B8G8R8A8
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned char A;

    static void average(B8G8R8A8 *dst, const B8G8R8A8 *src1, const B8G8R8A8 *src2)
    {
        *(unsigned int*)dst = (((*(unsigned int*)src1 ^ *(unsigned int*)src2) & 0xFEFEFEFE) >> 1) + (*(unsigned int*)src1 & *(unsigned int*)src2);
    }
};

struct R16
{
    unsigned short R;

    static void average(R16 *dst, const R16 *src1, const R16 *src2)
    {
        dst->R = ((src1->R ^ src2->R) >> 1) + (src1->R & src2->R);
    }
};

struct R16G16
{
    unsigned short R;
    unsigned short G;

    static void average(R16G16 *dst, const R16G16 *src1, const R16G16 *src2)
    {
        dst->R = ((src1->R ^ src2->R) >> 1) + (src1->R & src2->R);
        dst->G = ((src1->G ^ src2->G) >> 1) + (src1->G & src2->G);
    }
};

struct R16G16B16A16
{
    unsigned short R;
    unsigned short G;
    unsigned short B;
    unsigned short A;

    static void average(R16G16B16A16 *dst, const R16G16B16A16 *src1, const R16G16B16A16 *src2)
    {
        dst->R = ((src1->R ^ src2->R) >> 1) + (src1->R & src2->R);
        dst->G = ((src1->G ^ src2->G) >> 1) + (src1->G & src2->G);
        dst->B = ((src1->B ^ src2->B) >> 1) + (src1->B & src2->B);
        dst->A = ((src1->A ^ src2->A) >> 1) + (src1->A & src2->A);
    }
};

struct R32
{
    unsigned int R;

    static void average(R32 *dst, const R32 *src1, const R32 *src2)
    {
        dst->R = ((src1->R ^ src2->R) >> 1) + (src1->R & src2->R);
    }
};

struct R32G32
{
    unsigned int R;
    unsigned int G;

    static void average(R32G32 *dst, const R32G32 *src1, const R32G32 *src2)
    {
        dst->R = ((src1->R ^ src2->R) >> 1) + (src1->R & src2->R);
        dst->G = ((src1->G ^ src2->G) >> 1) + (src1->G & src2->G);
    }
};

struct R32G32B32
{
    unsigned int R;
    unsigned int G;
    unsigned int B;

    static void average(R32G32B32 *dst, const R32G32B32 *src1, const R32G32B32 *src2)
    {
        dst->R = ((src1->R ^ src2->R) >> 1) + (src1->R & src2->R);
        dst->G = ((src1->G ^ src2->G) >> 1) + (src1->G & src2->G);
        dst->B = ((src1->B ^ src2->B) >> 1) + (src1->B & src2->B);
    }
};

struct R32G32B32A32
{
    unsigned int R;
    unsigned int G;
    unsigned int B;
    unsigned int A;

    static void average(R32G32B32A32 *dst, const R32G32B32A32 *src1, const R32G32B32A32 *src2)
    {
        dst->R = ((src1->R ^ src2->R) >> 1) + (src1->R & src2->R);
        dst->G = ((src1->G ^ src2->G) >> 1) + (src1->G & src2->G);
        dst->B = ((src1->B ^ src2->B) >> 1) + (src1->B & src2->B);
        dst->A = ((src1->A ^ src2->A) >> 1) + (src1->A & src2->A);
    }
};

struct R8S
{
    char R;

    static void average(R8S *dst, const R8S *src1, const R8S *src2)
    {
        dst->R = ((short)src1->R + (short)src2->R) / 2;
    }
};

struct R8G8S
{
    char R;
    char G;

    static void average(R8G8S *dst, const R8G8S *src1, const R8G8S *src2)
    {
        dst->R = ((short)src1->R + (short)src2->R) / 2;
        dst->G = ((short)src1->G + (short)src2->G) / 2;
    }
};

struct R8G8B8A8S
{
    char R;
    char G;
    char B;
    char A;

    static void average(R8G8B8A8S *dst, const R8G8B8A8S *src1, const R8G8B8A8S *src2)
    {
        dst->R = ((short)src1->R + (short)src2->R) / 2;
        dst->G = ((short)src1->G + (short)src2->G) / 2;
        dst->B = ((short)src1->B + (short)src2->B) / 2;
        dst->A = ((short)src1->A + (short)src2->A) / 2;
    }
};

struct R16S
{
    unsigned short R;

    static void average(R16S *dst, const R16S *src1, const R16S *src2)
    {
        dst->R = ((int)src1->R + (int)src2->R) / 2;
    }
};

struct R16G16S
{
    unsigned short R;
    unsigned short G;

    static void average(R16G16S *dst, const R16G16S *src1, const R16G16S *src2)
    {
        dst->R = ((int)src1->R + (int)src2->R) / 2;
        dst->G = ((int)src1->G + (int)src2->G) / 2;
    }
};

struct R16G16B16A16S
{
    unsigned short R;
    unsigned short G;
    unsigned short B;
    unsigned short A;

    static void average(R16G16B16A16S *dst, const R16G16B16A16S *src1, const R16G16B16A16S *src2)
    {
        dst->R = ((int)src1->R + (int)src2->R) / 2;
        dst->G = ((int)src1->G + (int)src2->G) / 2;
        dst->B = ((int)src1->B + (int)src2->B) / 2;
        dst->A = ((int)src1->A + (int)src2->A) / 2;
    }
};

struct R32S
{
    unsigned int R;

    static void average(R32S *dst, const R32S *src1, const R32S *src2)
    {
        dst->R = ((long long)src1->R + (long long)src2->R) / 2;
    }
};

struct R32G32S
{
    unsigned int R;
    unsigned int G;

    static void average(R32G32S *dst, const R32G32S *src1, const R32G32S *src2)
    {
        dst->R = ((long long)src1->R + (long long)src2->R) / 2;
        dst->G = ((long long)src1->G + (long long)src2->G) / 2;
    }
};

struct R32G32B32S
{
    unsigned int R;
    unsigned int G;
    unsigned int B;

    static void average(R32G32B32S *dst, const R32G32B32S *src1, const R32G32B32S *src2)
    {
        dst->R = ((long long)src1->R + (long long)src2->R) / 2;
        dst->G = ((long long)src1->G + (long long)src2->G) / 2;
        dst->B = ((long long)src1->B + (long long)src2->B) / 2;
    }
};

struct R32G32B32A32S
{
    unsigned int R;
    unsigned int G;
    unsigned int B;
    unsigned int A;

    static void average(R32G32B32A32S *dst, const R32G32B32A32S *src1, const R32G32B32A32S *src2)
    {
        dst->R = ((long long)src1->R + (long long)src2->R) / 2;
        dst->G = ((long long)src1->G + (long long)src2->G) / 2;
        dst->B = ((long long)src1->B + (long long)src2->B) / 2;
        dst->A = ((long long)src1->A + (long long)src2->A) / 2;
    }
};

struct A16B16G16R16F
{
    unsigned short R;
    unsigned short G;
    unsigned short B;
    unsigned short A;

    static void average(A16B16G16R16F *dst, const A16B16G16R16F *src1, const A16B16G16R16F *src2)
    {
        dst->R = gl::float32ToFloat16((gl::float16ToFloat32(src1->R) + gl::float16ToFloat32(src2->R)) * 0.5f);
        dst->G = gl::float32ToFloat16((gl::float16ToFloat32(src1->G) + gl::float16ToFloat32(src2->G)) * 0.5f);
        dst->B = gl::float32ToFloat16((gl::float16ToFloat32(src1->B) + gl::float16ToFloat32(src2->B)) * 0.5f);
        dst->A = gl::float32ToFloat16((gl::float16ToFloat32(src1->A) + gl::float16ToFloat32(src2->A)) * 0.5f);
    }
};

struct R16G16B16A16F
{
    unsigned short R;
    unsigned short G;
    unsigned short B;
    unsigned short A;

    static void average(R16G16B16A16F *dst, const R16G16B16A16F *src1, const R16G16B16A16F *src2)
    {
        dst->R = gl::float32ToFloat16((gl::float16ToFloat32(src1->R) + gl::float16ToFloat32(src2->R)) * 0.5f);
        dst->G = gl::float32ToFloat16((gl::float16ToFloat32(src1->G) + gl::float16ToFloat32(src2->G)) * 0.5f);
        dst->B = gl::float32ToFloat16((gl::float16ToFloat32(src1->B) + gl::float16ToFloat32(src2->B)) * 0.5f);
        dst->A = gl::float32ToFloat16((gl::float16ToFloat32(src1->A) + gl::float16ToFloat32(src2->A)) * 0.5f);
    }
};

struct R16F
{
    unsigned short R;

    static void average(R16F *dst, const R16F *src1, const R16F *src2)
    {
        dst->R = gl::float32ToFloat16((gl::float16ToFloat32(src1->R) + gl::float16ToFloat32(src2->R)) * 0.5f);
    }
};

struct R16G16F
{
    unsigned short R;
    unsigned short G;

    static void average(R16G16F *dst, const R16G16F *src1, const R16G16F *src2)
    {
        dst->R = gl::float32ToFloat16((gl::float16ToFloat32(src1->R) + gl::float16ToFloat32(src2->R)) * 0.5f);
        dst->G = gl::float32ToFloat16((gl::float16ToFloat32(src1->G) + gl::float16ToFloat32(src2->G)) * 0.5f);
    }
};

struct A32B32G32R32F
{
    float R;
    float G;
    float B;
    float A;

    static void average(A32B32G32R32F *dst, const A32B32G32R32F *src1, const A32B32G32R32F *src2)
    {
        dst->R = (src1->R + src2->R) * 0.5f;
        dst->G = (src1->G + src2->G) * 0.5f;
        dst->B = (src1->B + src2->B) * 0.5f;
        dst->A = (src1->A + src2->A) * 0.5f;
    }
};

struct R32G32B32A32F
{
    float R;
    float G;
    float B;
    float A;

    static void average(R32G32B32A32F *dst, const R32G32B32A32F *src1, const R32G32B32A32F *src2)
    {
        dst->R = (src1->R + src2->R) * 0.5f;
        dst->G = (src1->G + src2->G) * 0.5f;
        dst->B = (src1->B + src2->B) * 0.5f;
        dst->A = (src1->A + src2->A) * 0.5f;
    }
};

struct R32F
{
    float R;

    static void average(R32F *dst, const R32F *src1, const R32F *src2)
    {
        dst->R = (src1->R + src2->R) * 0.5f;
    }
};

struct R32G32F
{
    float R;
    float G;

    static void average(R32G32F *dst, const R32G32F *src1, const R32G32F *src2)
    {
        dst->R = (src1->R + src2->R) * 0.5f;
        dst->G = (src1->G + src2->G) * 0.5f;
    }
};

struct R32G32B32F
{
    float R;
    float G;
    float B;

    static void average(R32G32B32F *dst, const R32G32B32F *src1, const R32G32B32F *src2)
    {
        dst->R = (src1->R + src2->R) * 0.5f;
        dst->G = (src1->G + src2->G) * 0.5f;
        dst->B = (src1->B + src2->B) * 0.5f;
    }
};

struct R10G10B10A2
{
    unsigned int R : 10;
    unsigned int G : 10;
    unsigned int B : 10;
    unsigned int A : 2;

    static void average(R10G10B10A2 *dst, const R10G10B10A2 *src1, const R10G10B10A2 *src2)
    {
        dst->R = (src1->R + src2->R) >> 1;
        dst->G = (src1->G + src2->G) >> 1;
        dst->B = (src1->B + src2->B) >> 1;
        dst->A = (src1->A + src2->A) >> 1;
    }
};

struct R9G9B9E5
{
    unsigned int R : 9;
    unsigned int G : 9;
    unsigned int B : 9;
    unsigned int E : 5;

    static void average(R9G9B9E5 *dst, const R9G9B9E5 *src1, const R9G9B9E5 *src2)
    {
        float r1, g1, b1;
        gl::convert999E5toRGBFloats(*reinterpret_cast<const unsigned int*>(src1), &r1, &g1, &b1);

        float r2, g2, b2;
        gl::convert999E5toRGBFloats(*reinterpret_cast<const unsigned int*>(src2), &r2, &g2, &b2);

        *reinterpret_cast<unsigned int*>(dst) = gl::convertRGBFloatsTo999E5((r1 + r2) * 0.5f, (g1 + g2) * 0.5f, (b1 + b2) * 0.5f);
    }
};

struct R11G11B10F
{
    unsigned int R : 11;
    unsigned int G : 11;
    unsigned int B : 10;

    static void average(R11G11B10F *dst, const R11G11B10F *src1, const R11G11B10F *src2)
    {
        dst->R = gl::float32ToFloat11((gl::float11ToFloat32(src1->R) + gl::float11ToFloat32(src2->R)) * 0.5f);
        dst->G = gl::float32ToFloat11((gl::float11ToFloat32(src1->G) + gl::float11ToFloat32(src2->G)) * 0.5f);
        dst->B = gl::float32ToFloat10((gl::float10ToFloat32(src1->B) + gl::float10ToFloat32(src2->B)) * 0.5f);
    }
};

}

#endif // LIBGLESV2_RENDERER_IMAGEFORMATS_H_
