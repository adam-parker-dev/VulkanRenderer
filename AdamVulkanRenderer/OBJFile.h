#ifndef _OBJ_FILE_H_
#define _OBJ_FILE_H_

#include <string>
#include <vector>
#include "Model.h"

// Load .obj files.
// FBX SDK does not work with android at the moment
// Maybe one day...
// Convert to FBX when that happens
using namespace std;

class OBJFile
{
public:    
    //std::vector<GLfloat> vertices;
    //std::vector<GLbyte> indices;
    static void LoadFile(std::string fileName, Model &model);
};


#endif