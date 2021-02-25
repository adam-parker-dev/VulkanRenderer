#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "glslang/SPIRV/GlslangToSpv.h"

class Shader
{
public:
	void LoadFile(std::string fileName, std::string& fileContents);
	bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spirv);

private:
	void InitShaderResources(TBuiltInResource &Resources);
	EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type);
};