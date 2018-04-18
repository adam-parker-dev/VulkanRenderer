#ifndef _MAT4_H_
#define _MAT4_H_

class Mat4
{
public:
    // Column order
    //  m00 = column 0, row 0
    //  m10 = column 1, row 0
    // m01 = column 0, row 1
    float m00, m10, m20, m30,
          m01, m11, m21, m31,
          m02, m12, m22, m32,
          m03, m13, m23, m33;

    // Constructors
    Mat4();

    // Operators
    Mat4 operator*(Mat4 right);
    void operator=(Mat4 right);

    // View matrix
    Mat4 CreateViewMatrix();
    Mat4 CreatePerspectiveMatrix(float left, float right, float bottom, float top, float near, float far);
    Mat4 CreateTranslationMatrix(float x, float y, float z);
    Mat4 CreateScalingMatrix(float x, float y, float z);
    Mat4 CreateRotationMatrix(float x, float y, float z, float degrees);
    Mat4 ZeroMatrix();
    Mat4 Transpose();

    // Return array format for GLSL handle
    float* ReturnArray();
};

#endif
