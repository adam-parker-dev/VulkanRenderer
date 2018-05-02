#ifndef _MODEL_H_
#define _MODEL_H_

#include "Triangle.h"
#include "Shader.h"
#include "Mat4.h"
#include <vector>

// TODO: Abstract triangle to model type
class Model
{
public: 
    Triangle triangle;

    std::vector<float> fileVertices;
    std::vector<float> fileNormals;
    std::vector<unsigned int> fileIndices;

    Shader shader;
    Mat4 matrix;
};

#endif