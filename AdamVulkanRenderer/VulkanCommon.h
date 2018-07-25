#pragma once
#ifndef VULKAN_COMMON
#define VULKAN_COMMON

#include "vulkan.h"
#include <cassert>

class VulkanCommon
{
public:
	static VkPhysicalDeviceMemoryProperties m_vulkanDeviceMemoryProperties;

	// Heavily used function to get memory type for memory alloc
	static bool GetMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlagBits requirementsMask, uint32_t &memoryTypeIndex)
	{
		uint32_t typeBits = memoryTypeBits;
		bool pass = false;
		for (uint32_t i = 0; i < m_vulkanDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				// Type is available, does it match user properties?
				if ((m_vulkanDeviceMemoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask) 
				{
					memoryTypeIndex = i;
					pass = true;
					break;
				}
			}
			typeBits >>= 1;
		}
		return pass;
	}
};

#endif