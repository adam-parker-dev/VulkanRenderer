#pragma once

#include "vulkan.h"
#include <string>

#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT
#define FENCE_TIMEOUT 100000000

class Texture
{
public:
	void InitTexture(const VkDevice &device,
		const VkPhysicalDevice &physical,
		const VkPhysicalDeviceMemoryProperties &props,
		const VkCommandBuffer &cmdBuf,
		const VkQueue &queue);
	VkImageView view;
	VkSampler sampler;
private:
	bool ReadPPM(std::string filename, int &width, int &height, VkDeviceSize rowPitch, unsigned char *data);

	
	VkImage image;
	VkImageLayout imageLayout;
	VkDeviceMemory memory;

	uint32_t texWidth;
	uint32_t texHeight;
};