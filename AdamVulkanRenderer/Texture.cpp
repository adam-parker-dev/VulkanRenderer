#include "stdafx.h"
#include "Texture.h"
#include <iostream>
#include <vulkan.h>
#include <assert.h>
#include "VulkanCommon.h"

bool Texture::ReadPPM(std::string filename, int &width, int &height, VkDeviceSize rowPitch, unsigned char *data)
{
	// PPM format expected from http://netpbm.sourceforge.net/doc/ppm.html
	//  1. magic number
	//  2. whitespace
	//  3. width
	//  4. whitespace
	//  5. height
	//  6. whitespace
	//  7. max color value
	//  8. whitespace
	//  7. data

	// Comments are not supported, but are detected
	// Only 8 bits per channel is supported

	char magicStr[3] = {};
	char heightStr[6] = {};
	char widthStr[6] = {};
	char formatStr[6] = {};

	FILE *fPtr;
	errno_t errorNum = fopen_s(&fPtr, filename.c_str(), "rb");

	if (!fPtr)
	{
		printf("Error fopen %i\n", errorNum);
		printf("Bad filename in read_ppm: %s\nb", filename.c_str());
	}

	// Sizes from array size

	// TODO: Add error checking here
	fscanf_s(fPtr, "%s %s %s %s ", magicStr, 3, widthStr, 6, heightStr, 6, formatStr, 6);

	if (magicStr[0] == '#' ||
		widthStr[0] == '#' ||
		heightStr[0] == '#' ||
		formatStr[0] == '#')
	{
		printf("Unhandled comment in PPM file\n");
		return false;
	}

	if (strncmp(magicStr, "P6", sizeof(magicStr)))
	{
		printf("Unhandled PPM magic number: %s\n", magicStr);
		return false;
	}

	width = atoi(widthStr);
	height = atoi(heightStr);

	static const int saneDimension = 99999;
	if (width <= 0 || width > saneDimension)
	{
		printf("Width outside acceptable bounds. Update PPM file.");
		return false;
	}
	if (height <= 0 || height > saneDimension)
	{
		printf("Height outside acceptable bounds. Update PPM file.");
		return false;
	}

	if (data == nullptr)
	{
		// Caller only wanted dimensions, no data read
		return true;
	}

	for (int y = 0; y < height; y++)
	{
		unsigned char *rowPtr = data;
		for (int x = 0; x < width; x++)
		{
			fread(rowPtr, 3, 1, fPtr);
			rowPtr[3] = 255; /* Alpha of 1. TODO: Support alpha */
			rowPtr += 4;
		}
		data += rowPitch; // why would we change pitch? Examples never do...
	}

	fclose(fPtr);

	return true;
}

void Texture::InitTextureFromFile(const VkDevice &device, 
	const VkPhysicalDevice &physical, 
	const VkCommandBuffer &cmdBuf,
	const VkQueue &queue,
	const std::string &filename)
{	
	VkResult result;

	int width = 0;
	int height = 0;
	if (!ReadPPM(filename.c_str(), width, height, 0, NULL))
	{
		std::cout << "Could not read texture file";
		exit(-1);
	}

	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(physical, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);

	VkFormatFeatureFlags allFeatures = (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
	bool needStaging = ((formatProps.linearTilingFeatures & allFeatures) != allFeatures) ? true : false;

	if (needStaging)
	{
		assert((formatProps.optimalTilingFeatures & allFeatures) == allFeatures);
	}

	// begin init of image
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = NUM_SAMPLES;
	imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageCreateInfo.usage = needStaging ? (VK_IMAGE_USAGE_TRANSFER_SRC_BIT) : (VK_IMAGE_USAGE_SAMPLED_BIT);
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.flags = 0;

	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImage mappableImage;
	VkDeviceMemory mappableMemory;

	VkMemoryRequirements mem_reqs;

	result = vkCreateImage(device, &imageCreateInfo, NULL, &mappableImage);
	assert(result == VK_SUCCESS);

	vkGetImageMemoryRequirements(device, mappableImage, &mem_reqs);
	assert(result == VK_SUCCESS);

	mem_alloc.allocationSize = mem_reqs.size;

	assert(VulkanCommon::GetMemoryType(mem_reqs.memoryTypeBits, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), mem_alloc.memoryTypeIndex));

	result = vkAllocateMemory(device, &mem_alloc, NULL, &(mappableMemory));
	assert(result == VK_SUCCESS);

	result = vkBindImageMemory(device, mappableImage, mappableMemory, 0);
	assert(result == VK_SUCCESS);

	// Begin set image layout
	VkImageMemoryBarrier imageMemoryBarrier;
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = NULL;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = 0;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = mappableImage;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;

	imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;

	VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(cmdBuf, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
	// end set image layout

	result = vkEndCommandBuffer(cmdBuf);
	assert(result == VK_SUCCESS);
	const VkCommandBuffer cmdBufs[] = { cmdBuf };
	VkFenceCreateInfo fenceInfo;
	VkFence cmdFence;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = NULL;
	fenceInfo.flags = 0;
	vkCreateFence(device, &fenceInfo, NULL, &cmdFence);

	VkSubmitInfo submitInfo[1] = {};
	submitInfo[0].pNext = NULL;
	submitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo[0].waitSemaphoreCount = 0;
	submitInfo[0].pWaitSemaphores = NULL;
	submitInfo[0].pWaitDstStageMask = NULL;
	submitInfo[0].commandBufferCount = 1;
	submitInfo[0].pCommandBuffers = cmdBufs;
	submitInfo[0].signalSemaphoreCount = 0;
	submitInfo[0].pSignalSemaphores = NULL;

	result = vkQueueSubmit(queue, 1, submitInfo, cmdFence);
	assert(result == VK_SUCCESS);

	VkImageSubresource subres = {};
	subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subres.mipLevel = 0;
	subres.arrayLayer = 0;

	VkSubresourceLayout layout;
	void *data;

	/* Get the subresource layout so we know what the row pitch is */
	vkGetImageSubresourceLayout(device, mappableImage, &subres, &layout);

	do
	{
		result = vkWaitForFences(device, 1, &cmdFence, VK_TRUE, FENCE_TIMEOUT);
	} while (result == VK_TIMEOUT);
	assert(result == VK_SUCCESS);

	vkDestroyFence(device, cmdFence, NULL);

	result = vkMapMemory(device, mappableMemory, 0, mem_reqs.size, 0, &data);
	assert(result == VK_SUCCESS);

	if (!ReadPPM(filename.c_str(), width, height, layout.rowPitch, (unsigned char*)data))
	{
		std::cout << "Could not load texture file " << filename.c_str();
		exit(-1);
	}

	vkUnmapMemory(device, mappableMemory);

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = NULL;
	cmdBufInfo.flags = 0;
	cmdBufInfo.pInheritanceInfo = NULL;

	result = vkResetCommandBuffer(cmdBuf, 0);
	result = vkBeginCommandBuffer(cmdBuf, &cmdBufInfo);
	assert(result == VK_SUCCESS);

	if (!needStaging)
	{
		this->image = mappableImage;
		this->memory = mappableMemory;
		this->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// set image layout

		imageMemoryBarrier;
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = NULL;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = 0;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.newLayout = this->imageLayout;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = mappableImage;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		// Depends on new lyout
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;


		src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		vkCmdPipelineBarrier(cmdBuf, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
		//end set image layout
	}
	else
	{
		// Mappable image cannot be texture, so create optimally tiled image and blit to it */
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		result = vkCreateImage(device, &imageCreateInfo, NULL, &this->image);
		assert(result == VK_SUCCESS);

		vkGetImageMemoryRequirements(device, this->image, &mem_reqs);

		mem_alloc.allocationSize = mem_reqs.size;

		assert(VulkanCommon::GetMemoryType(mem_reqs.memoryTypeBits, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), mem_alloc.memoryTypeIndex));

		result = vkAllocateMemory(device, &mem_alloc, NULL, &this->memory);
		assert(result == VK_SUCCESS);

		result = vkBindImageMemory(device, this->image, this->memory, 0);
		assert(result == VK_SUCCESS);

		/* Blit from mappable image and set layout to source optimal */
		imageMemoryBarrier;
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = NULL;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = 0;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = mappableImage;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		vkCmdPipelineBarrier(cmdBuf, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
		/* end set layout */

		/* Blit to texture, set destination optimal layout*/
		imageMemoryBarrier;
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = NULL;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = 0;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = mappableImage;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		vkCmdPipelineBarrier(cmdBuf, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
		/* end set layout */

		VkImageCopy copyRegion;
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.srcOffset.x = 0;
		copyRegion.srcOffset.y = 0;
		copyRegion.srcOffset.z = 0;
		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.baseArrayLayer = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.dstOffset.x = 0;
		copyRegion.dstOffset.y = 0;
		copyRegion.dstOffset.z = 0;
		copyRegion.extent.width = width;
		copyRegion.extent.height = height;
		copyRegion.extent.depth = 1;

		// Put copy command into commabd buffer
		vkCmdCopyImage(cmdBuf, mappableImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, this->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		// Set layout for texture image form destination optimal to shader read only
		this->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier;
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = NULL;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = 0;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = mappableImage;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		vkCmdPipelineBarrier(cmdBuf, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
		// end set image layout
	}

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext = NULL;
	viewInfo.image = VK_NULL_HANDLE;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	viewInfo.image = this->image;
	result = vkCreateImageView(device, &viewInfo, NULL, &this->view);
	assert(result == VK_SUCCESS);
	// END INIT IMAGE

	// BEGIN CREATE SAMPLER
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias = 0.0;
	samplerCreateInfo.anisotropyEnable = VK_FALSE,
		samplerCreateInfo.maxAnisotropy = 1;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0;
	samplerCreateInfo.maxLod = 0.0;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	/* create sampler */
	result = vkCreateSampler(device, &samplerCreateInfo, NULL, &this->sampler);
	assert(result == VK_SUCCESS);
	// ENd create sampler	
}

void Texture::InitTexture(const VkDevice &device, const VkPhysicalDevice &physical, const VkCommandBuffer &cmdBuf, const VkQueue &queue, VkImageType type, VkFormat format, bool writeable, int width, int height, int depth)
{
	VkResult result;

	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(physical, format, &formatProps);

	VkFormatFeatureFlags allFeatures = (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
	bool needStaging = ((formatProps.linearTilingFeatures & allFeatures) != allFeatures) ? true : false;

	if (needStaging)
	{
		assert((formatProps.optimalTilingFeatures & allFeatures) == allFeatures);
	}

	// TODO: Test 2d texture limits
	// If imageType is VK_IMAGE_TYPE_3D, extent.width, extent.height and extent.depth must be less than or equal to VkPhysicalDeviceLimits::maxImageDimension3D, 
	// or VkImageFormatProperties::maxExtent.width / height / depth(as returned by vkGetPhysicalDeviceImageFormatProperties with format, imageType, 
	// tiling, usage, and flags equal to those in this structure) - whichever is higher

	// begin init of image
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	imageCreateInfo.imageType = type;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = depth;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = NUM_SAMPLES;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageCreateInfo.usage = needStaging ? (VK_IMAGE_USAGE_TRANSFER_SRC_BIT) : (VK_IMAGE_USAGE_SAMPLED_BIT);
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (type == VkImageType::VK_IMAGE_TYPE_3D)
	{
		imageCreateInfo.flags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
	}
	else
	{
		imageCreateInfo.flags = 0;
	}
	
	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImage mappableImage;
	VkDeviceMemory mappableMemory;
	VkMemoryRequirements mem_reqs;

	result = vkCreateImage(device, &imageCreateInfo, NULL, &mappableImage);
	assert(result == VK_SUCCESS);

	vkGetImageMemoryRequirements(device, mappableImage, &mem_reqs);
	assert(result == VK_SUCCESS);

	mem_alloc.allocationSize = mem_reqs.size;

	if (type == VkImageType::VK_IMAGE_TYPE_3D)
	{
		assert(VulkanCommon::GetMemoryType(mem_reqs.memoryTypeBits, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), mem_alloc.memoryTypeIndex));
	}
	else
	{
		assert(VulkanCommon::GetMemoryType(mem_reqs.memoryTypeBits, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), mem_alloc.memoryTypeIndex));
	}

	result = vkAllocateMemory(device, &mem_alloc, NULL, &(mappableMemory));
	assert(result == VK_SUCCESS);

	result = vkBindImageMemory(device, mappableImage, mappableMemory, 0);
	assert(result == VK_SUCCESS);

	// Begin set image layout
	VkImageMemoryBarrier imageMemoryBarrier;
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = NULL;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = 0;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = mappableImage;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;

	imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;

	VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(cmdBuf, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
	// end set image layout

	result = vkEndCommandBuffer(cmdBuf);
	assert(result == VK_SUCCESS);
	const VkCommandBuffer cmdBufs[] = { cmdBuf };
	VkFenceCreateInfo fenceInfo;
	VkFence cmdFence;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = NULL;
	fenceInfo.flags = 0;
	vkCreateFence(device, &fenceInfo, NULL, &cmdFence);

	VkSubmitInfo submitInfo[1] = {};
	submitInfo[0].pNext = NULL;
	submitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo[0].waitSemaphoreCount = 0;
	submitInfo[0].pWaitSemaphores = NULL;
	submitInfo[0].pWaitDstStageMask = NULL;
	submitInfo[0].commandBufferCount = 1;
	submitInfo[0].pCommandBuffers = cmdBufs;
	submitInfo[0].signalSemaphoreCount = 0;
	submitInfo[0].pSignalSemaphores = NULL;

	result = vkQueueSubmit(queue, 1, submitInfo, cmdFence);
	assert(result == VK_SUCCESS);

	VkImageSubresource subres = {};
	subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subres.mipLevel = 0;
	subres.arrayLayer = 0;

	VkSubresourceLayout layout;
	void *data;

	/* Get the subresource layout so we know what the row pitch is */
	vkGetImageSubresourceLayout(device, mappableImage, &subres, &layout);

	do
	{
		result = vkWaitForFences(device, 1, &cmdFence, VK_TRUE, FENCE_TIMEOUT);
	} while (result == VK_TIMEOUT);
	assert(result == VK_SUCCESS);

	vkDestroyFence(device, cmdFence, NULL);

	this->image = mappableImage;
	this->memory = mappableMemory;
	this->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// BEGIN CREATE IMAGE VIEW
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext = NULL;
	viewInfo.image = VK_NULL_HANDLE;
	if (type == VK_IMAGE_TYPE_3D)
	{
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
	}
	else
	{
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	}
	viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	viewInfo.image = this->image;
	result = vkCreateImageView(device, &viewInfo, NULL, &this->view);
	assert(result == VK_SUCCESS);
	// END CREATE IMAGE VIEW

	// BEGIN CREATE SAMPLER
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias = 0.0;
	samplerCreateInfo.anisotropyEnable = VK_FALSE,
		samplerCreateInfo.maxAnisotropy = 1;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.minLod = 0.0;
	samplerCreateInfo.maxLod = 0.0;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	/* create sampler */
	result = vkCreateSampler(device, &samplerCreateInfo, NULL, &this->sampler);
	assert(result == VK_SUCCESS);
	// ENd create sampler	
}

void Texture::CopyBufferToImage(const VkDevice &device, const VkPhysicalDevice &physical, const VkCommandBuffer &cmdBuf)
{
	uint32_t *data = new uint32_t[5 * 5 * 5]; // change to input width, heigh, depth

	
	VkBuffer srcBuffer;
	VkImage dstImage;
	VkImageLayout dstImageLayout;

	VkBufferImageCopy copyRegion;
	//copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//copyRegion.imageSubresource.mipLevel = 0;
	//copyRegion.imageSubresource.baseArrayLayer = 0;
	//copyRegion.imageSubresource.layerCount = 1;
	//copyRegion.imageExtent.width = texture.width;
	//copyRegion.imageExtent.height = texture.height;
	//copyRegion.imageExtent.depth = texture.depth;

//	vkCmdCopyBufferToImage(cmdBuf, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
}