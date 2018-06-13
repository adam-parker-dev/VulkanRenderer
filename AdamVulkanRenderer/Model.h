#pragma once

#include <vector>

class Model
{
public: 
    std::vector<float> fileVertices;
    std::vector<float> fileNormals;
    std::vector<unsigned int> fileIndices;
};