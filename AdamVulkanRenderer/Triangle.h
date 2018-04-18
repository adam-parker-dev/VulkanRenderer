#ifndef _TRIANGLE_H_
#define _TRIANGLE_H_

class Triangle
{
    // Triangle vertices constants
public:
    float gTriangleVertices[6];
	float gTriangleNormals[9];
	float gTriangleIndices[3] = { 0, 1, 2 };

    Triangle() { gTriangleVertices[0] = 0.0f, 
                 gTriangleVertices[1] = 0.5f, 
                 gTriangleVertices[2] = 0.5f, 
                 gTriangleVertices[3] = -0.5f, 
                 gTriangleVertices[4] = -0.5f, 
                 gTriangleVertices[5] = -0.5f;
                 gTriangleNormals[0] = 0.0f; 
                 gTriangleNormals[1] = 0.0f;
                 gTriangleNormals[2] = 1.0f;
                 gTriangleNormals[3] = 0.0f;
                 gTriangleNormals[4] = 0.0f;
                 gTriangleNormals[5] = 1.0f;
                 gTriangleNormals[6] = 0.0f;
                 gTriangleNormals[7] = 0.0f;
                 gTriangleNormals[8] = 1.0f;
    }
    ~Triangle() {}
};

#endif