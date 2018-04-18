#ifndef _VEC_4_H_
#define _VEC_4_H_

class Vec4
{
public:
    float x;
    float y;
    float z;
    float w;

    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    Vec4(const Vec4 &_vec) : x(_vec.x), y(_vec.y), z(_vec.z), w(_vec.w) {}
};

#endif