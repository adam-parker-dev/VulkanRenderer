#pragma once

struct Vec3
{
public:
    float x;
    float y;
    float z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    Vec3(const Vec3 &_vec) : x(_vec.x), y(_vec.y), z(_vec.z) {}

    float& operator[](int index) 
    { 
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        }
    }

    float operator[](int index) const 
    { 
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        }
    }
};