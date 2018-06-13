#pragma once

struct Mat4
{
public:
    // Row major
    //  m00 = row 0, column 0
    //  m10 = row 1, column 0
    // m01 = row 0, column 1
    float m00, m01, m02, m03,
          m10, m11, m12, m13,
          m20, m21, m22, m23,
          m30, m31, m32, m33;
};