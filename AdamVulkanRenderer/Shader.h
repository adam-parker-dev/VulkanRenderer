#ifndef _SHADER_H_
#define _SHADER_H_

#include <vulkan.h>
#include <string>
#include <vector>
#include "SPIRV/GlslangToSpv.h"

class Shader
{
public:
	void LoadFile(std::string fileName, std::string& fileContents);
	bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spirv);
private:
	void InitShaderResources(TBuiltInResource &Resources);
	EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type);
};

#endif