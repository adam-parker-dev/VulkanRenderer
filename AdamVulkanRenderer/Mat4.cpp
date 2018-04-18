#include "stdafx.h"
#include "Mat4.h"

// Initialize everything to 0.0
Mat4::Mat4() :
m00(0.0f), m10(0.0f), m20(0.0f), m30(0.0f),
m01(0.0f), m11(0.0f), m21(0.0f), m31(0.0f),
m02(0.0f), m12(0.0f), m22(0.0f), m32(0.0f),
m03(0.0f), m13(0.0f), m23(0.0f), m33(0.0f) {}

// Multiplication operator
// TODO: Optimize, use references, and const correctness
Mat4 Mat4::operator*(Mat4 right)
{
    Mat4 result;
    result.m00 = m00 * right.m00 + m10 * right.m01 + m20 * right.m02 + m30 * right.m03;
    result.m10 = m00 * right.m10 + m10 * right.m11 + m20 * right.m12 + m30 * right.m13;
    result.m20 = m00 * right.m20 + m10 * right.m21 + m20 * right.m22 + m30 * right.m23;
    result.m30 = m00 * right.m30 + m10 * right.m31 + m20 * right.m32 + m30 * right.m33;
    result.m01 = m01 * right.m00 + m11 * right.m01 + m21 * right.m02 + m31 * right.m03;
    result.m11 = m01 * right.m10 + m11 * right.m11 + m21 * right.m12 + m31 * right.m13;
    result.m21 = m01 * right.m20 + m11 * right.m21 + m21 * right.m22 + m31 * right.m23;
    result.m31 = m01 * right.m30 + m11 * right.m31 + m21 * right.m32 + m31 * right.m33;
    result.m02 = m02 * right.m00 + m12 * right.m01 + m22 * right.m02 + m32 * right.m03;
    result.m12 = m02 * right.m10 + m12 * right.m11 + m22 * right.m12 + m32 * right.m13;
    result.m22 = m02 * right.m20 + m12 * right.m21 + m22 * right.m22 + m32 * right.m23;
    result.m32 = m02 * right.m30 + m12 * right.m31 + m22 * right.m32 + m32 * right.m33;
    result.m03 = m03 * right.m00 + m13 * right.m01 + m23 * right.m02 + m33 * right.m03;
    result.m13 = m03 * right.m10 + m13 * right.m11 + m23 * right.m12 + m33 * right.m13;
    result.m23 = m03 * right.m20 + m13 * right.m21 + m23 * right.m22 + m33 * right.m23;
    result.m33 = m03 * right.m30 + m13 * right.m31 + m23 * right.m32 + m33 * right.m33;
    return result;
}

// Equals operator
// TODO: Optimize. use references, and const correctness
void Mat4::operator=(Mat4 right)
{
    m00 = right.m00;
    m10 = right.m10;
    m20 = right.m20;
    m30 = right.m30;
    m01 = right.m01;
    m11 = right.m11;
    m21 = right.m21;
    m31 = right.m31;
    m02 = right.m02;
    m12 = right.m12;
    m22 = right.m22;
    m32 = right.m32;
    m03 = right.m03;
    m13 = right.m13;
    m23 = right.m23;
    m33 = right.m33;
}


// Return an array GLSL can use
// TODO: Optimize or abstract entire class for GLSL
float* Mat4::ReturnArray()
{
	float* result = new float[16];
    result[0] = m00;
    result[1] = m10;
    result[2] = m20;
    result[3] = m30;
    result[4] = m01;
    result[5] = m11;
    result[6] = m21;
    result[7] = m31;
    result[8] = m02;
    result[9] = m12;
    result[10] = m22;
    result[11] = m32;
    result[12] = m03;
    result[13] = m13;
    result[14] = m23;
    result[15] = m33;
    return result;
}


Mat4 Mat4::CreateTranslationMatrix(float x, float y, float z)
{
    Mat4 result = ZeroMatrix();
    result.m00 = 1.0f;
    result.m11 = 1.0f;
    result.m22 = 1.0f;
    result.m33 = 1.0f;
    result.m30 = x;
    result.m31 = y;
    result.m32 = z;
    return result;
}

Mat4 Mat4::CreateScalingMatrix(float x, float y, float z)
{
    Mat4 result = ZeroMatrix();
    result.m00 = x;
    result.m11 = y;
    result.m22 = z;
    result.m33 = 1.0f;
    return result;
}

Mat4 Mat4::CreatePerspectiveMatrix(float left, float right, float bottom, float top, float n, float f)
{
    Mat4 result;
    result.m00 = (2.0f * n) / (right - left);
    result.m10 = 0.0f;
    result.m20 = ((right + left) / (right - left)); 
    result.m30 = 0.0f;
    result.m01 = 0.0f;
    result.m11 = (2.0f * n) / (top - bottom);
    result.m21 = ((top + bottom) / (top - bottom));
    result.m31 = 0.0f;
    result.m02 = 0.0f;
    result.m12 = 0.0f;
    result.m22 = -((f + n) / (f - n));
    result.m32 = -((2.0f * f * n) / (f - n));
    result.m03 = 0.0f;
    result.m13 = 0.0f;
    result.m23 = -1.0f;
    result.m33 = 0.0f;
    return result;
}

Mat4 Mat4::ZeroMatrix()
{
    Mat4 result;
    result.m00 = 0.0f;
    result.m10 = 0.0f;
    result.m20 = 0.0f;
    result.m30 = 0.0f;
    result.m01 = 0.0f;
    result.m11 = 0.0f;
    result.m21 = 0.0f;
    result.m31 = 0.0f;
    result.m02 = 0.0f;
    result.m12 = 0.0f;
    result.m22 = 0.0f;
    result.m32 = 0.0f;
    result.m03 = 0.0f;
    result.m13 = 0.0f;
    result.m23 = 0.0f;
    result.m33 = 0.0f;
    return result;
}

Mat4 Mat4::Transpose()
{
    Mat4 result;
    
    result.m00 = m00;
    result.m01 = m10;
    result.m02 = m20;
    result.m03 = m30;

    result.m11 = m11;
    result.m10 = m01;
    result.m12 = m21;
    result.m13 = m31;

    result.m22 = m22;
    result.m20 = m02;
    result.m21 = m12;
    result.m23 = m32;
    
    result.m33 = m33;
    result.m30 = m03;
    result.m31 = m13;
    result.m32 = m23;

    return result;
}