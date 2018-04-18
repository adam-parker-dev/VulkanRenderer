#include "stdafx.h"
#include "Shader.h"
#include <string>
#include <fstream>

void Shader::LoadFile(std::string fileName, std::string& fileContents)
{
	std::ifstream objFile(fileName.c_str());

	if (objFile.is_open())
	{
		objFile.seekg(0, std::ios::end);
		fileContents.reserve(objFile.tellg());
		objFile.seekg(0, std::ios::beg);

		fileContents.assign((std::istreambuf_iterator<char>(objFile)),
			std::istreambuf_iterator<char>());
		
		// TODO! Remove all \n from string before passing back.
	}
}
