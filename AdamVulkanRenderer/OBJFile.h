#pragma once

#include <string>
#include <vector>
#include "Model.h"

// Load .obj files
class OBJFile
{
public:
    static void LoadFile(std::string fileName, std::vector<Model> &models);
};