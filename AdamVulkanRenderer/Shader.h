#ifndef _SHADER_H_
#define _SHADER_H_

#include <string>

class Shader
{
public:
    // Program from shader compile
    unsigned int gProgram;

    // Handles for resource updates
    // TODO: These are specific to one shader...maybe need to make child classes?
	unsigned int gvPositionHandle;
	unsigned int gvNormalHandle;
	unsigned int deltaTimeHandle;
	unsigned int matrixHandle;
	unsigned int viewHandle;
	unsigned int projectionHandle;

	void LoadFile(std::string fileName, std::string& fileContents);

	//unsigned int CreateProgram(const char* pVertexSource, const char* pFragmentSource);
	//unsigned int LoadShader(GLenum shaderType, const char* pSource);
	//unsigned int LoadShaderFile(GLenum shaderType, const char* pSource);
	//unsigned int CreateProgramFromFiles(const char* vertex, const char* fragment);
};

#endif