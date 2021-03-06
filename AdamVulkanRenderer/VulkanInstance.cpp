#include "stdafx.h"
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <string>
#include "VulkanInstance.h"
#include "Cube.h"
#include <winsock2.h>
#include <windows.h>
#include "Shader.h"

// Declare Vulkan Common statics for code reuse
#include "VulkanCommon.h"
VkPhysicalDeviceMemoryProperties VulkanCommon::m_vulkanDeviceMemoryProperties;

// Call all the initialize functions needed to make the Vulkan render pipeline work
void VulkanInstance::Initialize(HWND hwnd, HINSTANCE inst, int width, int height, bool multithreaded, bool clusteredRendering, bool importObjs)
{
    assert(width);
    assert(height);

	m_windowWidth = width;
	m_windowHeight = height;

	camera[0] = Camera();
	camera[0].fov = glm::radians(45.0f);
	camera[0].aspect = static_cast<float>(width) / static_cast<float>(height);
	camera[0].nearPlane = 0.1f;
	camera[0].farPlane = 10000.0f;
	camera[0].center = Vec3(0, 0, -100);
	camera[0].eye = Vec3(0, 2, 10);
	camera[0].up = Vec3(0, 1, 0);

	// For Unreal .obj imports 
	//View = glm::lookAt(glm::vec3(0, 500, 1000),
	//    glm::vec3(0, 0, 0),
	//    glm::vec3(0, 1, 0));

	// Debug camera for view frustum
	camera[1] = Camera();
	camera[1].fov = glm::radians(45.0f);
	camera[1].aspect = static_cast<float>(width) / static_cast<float>(height);
	camera[1].nearPlane = 0.1f;
	camera[1].farPlane = 10000.0f;
	camera[1].center = Vec3(0, 0, -50);
	camera[1].eye = Vec3(100, 50, 50);
	camera[1].up = Vec3(0, 1, 0);

	camera[2] = Camera();
	camera[2].fov = glm::radians(45.0f);
	camera[2].aspect = static_cast<float>(width) / static_cast<float>(height);
	camera[2].nearPlane = 0.1f;
	camera[2].farPlane = 10000.0f;
	camera[2].center = Vec3(0, 0, 0);
	camera[2].eye = Vec3(0, 500, 1000);
	camera[2].up = Vec3(0, 1, 0);

	// Vulkan setup
    InitInstance();
    EnumerateDevices();
    InitSurface(hwnd, inst);
    CreateDevice();
    InitCommandBuffer();
    InitSwapChain();
    CreateDepthBuffer();
    CreateDescriptorLayouts();
    AllocateDescriptorSets();
    InitRenderPass();
    InitShaders();
    InitFrameBuffers();
    InitPipeline();

    if (multithreaded)
    {
        // Have multiple uniform buffers, one per thread
        // Avoids synchronization issues that would defeat the 
        // advantages of multi-threading.
        InitMultithreaded();
    }
    else
    {
        // Only need one uniform buffer
        // TODO: Maybe def out so we don't have extra memory
        // sitting around unused for the other buffers
        // Could also allocate depending on number of threads
        CreateUniformBuffer(0);
        InitVertexBuffer();
    }

	// Create texture for cube
	albedoTexture.InitTextureFromFile(m_vulkanDevice, m_vulkanDeviceVector[0], m_vulkanCommandBuffer, m_vulkanQueue, "adam.ppm");
	m_vulkanImageInfo.imageView = albedoTexture.view;
	m_vulkanImageInfo.sampler = albedoTexture.sampler;
	m_vulkanImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	m_currentCamera = 0;
	if (importObjs)
	{
		m_currentCamera = 2;
	}

	if (clusteredRendering)
	{
		m_currentCamera = 1;
		// Debug clustered frustum
		uint32_t xSlices, ySlices, zSlices;
		xSlices = ySlices = zSlices = 5;
		std::vector<Vec4> clusteredFrustum;
		camera[0].SubdivideFrustum(clusteredFrustum, zSlices, 20, xSlices, ySlices);
		std::vector<Vec4> lineList;
		camera[0].ConstructDebugLineList(clusteredFrustum, lineList, 5, 5, 5);
		AddLineBuffer(lineList);

		// Create 3D texture for clustered light list
		frustum3dTexutre.InitTexture(m_vulkanDevice, m_vulkanDeviceVector[0], m_vulkanCommandBuffer, m_vulkanQueue, VK_IMAGE_TYPE_3D, VK_FORMAT_R32G32_UINT, true, xSlices, ySlices, zSlices);
		m_vulkanImageInfo.imageView = frustum3dTexutre.view;
		m_vulkanImageInfo.sampler = frustum3dTexutre.sampler;
	}
}

// Initialize a Vulkan instance
// Information found in step 1 of VulkanAPI samples
void VulkanInstance::InitInstance()
{
    VkResult result;

    // These extension names are needed to create a valid win32 surface
    std::vector<const char *> instanceExtensionNames;
    instanceExtensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    instanceExtensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

    // Application information
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
    appInfo.pApplicationName = "Adam's Renderer";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "Adam's Engine";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Instance create info
    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = NULL;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = 0;
    instanceInfo.ppEnabledLayerNames = NULL;
    instanceInfo.enabledExtensionCount = instanceExtensionNames.size();
    instanceInfo.ppEnabledExtensionNames = instanceExtensionNames.data();

    // Create a Vulkan instance
    result = vkCreateInstance(&instanceInfo, NULL, &m_vulkanInstance);

    // Error handling
    if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        std::cout << "YO! Your driver is not compatible with Vulkan ICD\n";
        exit(-1);
    }
    else if (result)
    {
        std::cout << "YO! Unknown error\n";
        exit(-1);
    }
}

// Enumerate Vulkan devices
// Information found in step 2 of VulkanAPI samples
void VulkanInstance::EnumerateDevices()
{    
    VkResult result;

    // Iterate over devices
    uint32_t gpuCount;
    result = vkEnumeratePhysicalDevices(m_vulkanInstance, &gpuCount, NULL);
    assert(gpuCount);

    m_vulkanDeviceVector.resize(gpuCount);
    result = vkEnumeratePhysicalDevices(m_vulkanInstance, &gpuCount, m_vulkanDeviceVector.data());
    assert(!result && gpuCount >= 1);

    // Grab device properties for first device
    vkGetPhysicalDeviceQueueFamilyProperties(m_vulkanDeviceVector[0], &m_queueCount, NULL);
    assert(m_queueCount >= 1);

    m_vulkanQueueFamilyPropertiesVector.resize(m_queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vulkanDeviceVector[0], &m_queueCount, m_vulkanQueueFamilyPropertiesVector.data());
    assert(m_queueCount >= 1);

    // Grab memory properties used when creating buffers
    vkGetPhysicalDeviceMemoryProperties(m_vulkanDeviceVector[0], &VulkanCommon::m_vulkanDeviceMemoryProperties);
}

void VulkanInstance::InitSurface(HWND hwnd, HINSTANCE inst)
{
    // TODO: Window allocation already happened for this project. Need different pipeline for Android
    VkResult result;

#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.hinstance = inst;
    createInfo.hwnd = hwnd;
    createInfo.flags = 0;
    result = vkCreateWin32SurfaceKHR(m_vulkanInstance, &createInfo, NULL, &m_vulkanSurface);
#else  // _WIN32
    VkXcbSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.connection = info.connection;
    createInfo.window = info.window;
    result = vkCreateXcbSurfaceKHR(info.inst, &createInfo, NULL, &info.surface);
#endif // _WIN32
    assert(result == VK_SUCCESS);

    // Make sure each queue supports presenting
    VkBool32 *supportsPresent = (VkBool32 *)malloc(m_queueCount * sizeof(VkBool32));
    for (uint32_t i = 0; i < m_queueCount; ++i)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(m_vulkanDeviceVector[0], i, m_vulkanSurface, &supportsPresent[i]);
    }

    // Search for graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < m_queueCount; i++)
    {
        if ((m_vulkanQueueFamilyPropertiesVector[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if (graphicsQueueNodeIndex == UINT32_MAX)
            {
                graphicsQueueNodeIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE)
            {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }

    // Grab the present queue if we don't have it yet
    if (presentQueueNodeIndex == UINT32_MAX)
    {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (uint32_t i = 0; i < m_queueCount; ++i) {

            if (supportsPresent[i] == VK_TRUE)
            {
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    free(supportsPresent);

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
    {
        OutputDebugString(L"Could not find a graphics and a present queue\nSwapchain Initialization Failure\n");
        exit(-1);
    }

    // TODO: Add support for separate queues, including presentation,
    //       synchronization, and appropriate tracking for QueueSubmit.
    // NOTE: While it is possible for an application to use a separate graphics
    //       and a present queues, this demo program assumes it is only using
    //       one:
    if (graphicsQueueNodeIndex != presentQueueNodeIndex)
    {
        OutputDebugString(L"Could not find a common graphics and a present queue\nSwapchain Initialization Failure\n");
        exit(-1);
    }

    // Store graphics queue index for future
    m_graphicsQueueFamilyIndex = graphicsQueueNodeIndex;

    //List of VKFormats supported
    uint32_t formatCount;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vulkanDeviceVector[0], m_vulkanSurface, &formatCount, NULL);
    assert(result == VK_SUCCESS);

    // Allocate and get formats for surface
    VkSurfaceFormatKHR *surfFormats = (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vulkanDeviceVector[0], m_vulkanSurface, &formatCount, surfFormats);
    assert(result == VK_SUCCESS);

    // If format list has one VK_FORMAT_UNDEFIEND entry, surface has no preferred format. Otherwise, at lease one
    // will be returned
    // Store format for future use
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        m_vulkanFormat = VK_FORMAT_B8G8R8_UNORM;
    }
    else
    {
        assert(formatCount >= 1);
        m_vulkanFormat = surfFormats[0].format;
    }

    free(surfFormats);
}

// Create a Vulkan device
// Information found in step 3 of VulkanAPI samples
void VulkanInstance::CreateDevice()
{
    // This is needed for the valid swap chain
    std::vector<const char *> deviceExtensionNames;
    deviceExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // Setup queue create info
    float queuePriorities[1] = { 0.0 };
    VkDeviceQueueCreateInfo queueInfo;
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext = NULL;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queuePriorities;
    queueInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;

    // Setup device create info
    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = NULL;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = deviceExtensionNames.size();
    deviceInfo.ppEnabledExtensionNames = deviceExtensionNames.data();
    deviceInfo.pEnabledFeatures = NULL;

    // Create the device and verify
    VkResult result = vkCreateDevice(m_vulkanDeviceVector[0], &deviceInfo, NULL, &m_vulkanDevice);
    assert(result == VK_SUCCESS);

    // Get the queue for this device
    vkGetDeviceQueue(m_vulkanDevice, m_graphicsQueueFamilyIndex, 0, &m_vulkanQueue);

    // TODO: Add later for device info
    //for (uint32_t i = 0; i < 1; ++i) {
    //    VkPhysicalDeviceProperties properties;	
    //    vkGetPhysicalDeviceProperties(devices[i], &properties);

    //    std::cout << "apiVersion: ";
    //    std::cout << ((properties.apiVersion >> 22) & 0xfff) << '.'; // Major.
    //    std::cout << ((properties.apiVersion >> 12) & 0x3ff) << '.'; // Minor.
    //    std::cout << (properties.apiVersion & 0xfff);                // Patch.
    //    std::cout << '\n';

    //    std::cout << "driverVersion: " << properties.driverVersion << '\n';

    //    std::cout << std::showbase << std::internal << '0' << std::hex;
    //    std::cout << "vendorId: " << ' ' << properties.vendorID
    //        << '\n';
    //    std::cout << "deviceId: " << ' ' << properties.deviceID
    //        << '\n';
    //    std::cout << std::noshowbase << std::right << ' ' << std::dec;

    //    std::cout << "deviceType: ";
    //    switch (properties.deviceType) {
    //    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
    //        std::cout << "VK_PHYSICAL_DEVICE_TYPE_OTHER";
    //        break;
    //    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
    //        std::cout << "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
    //        break;
    //    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    //        std::cout << "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
    //        break;
    //    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
    //        std::cout << "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
    //        break;
    //    case VK_PHYSICAL_DEVICE_TYPE_CPU:
    //        std::cout << "VK_PHYSICAL_DEVICE_TYPE_CPU";
    //        break;
    //    default:
    //        break;
    //    }
    //    std::cout << '\n';

    //    std::cout << "deviceName: " << properties.deviceName << '\n';

    //    std::cout << "pipelineCacheUUID: ";
    //    std::cout << ' ' << std::hex;
    //    std::cout << properties.pipelineCacheUUID;
    //    std::cout << ' ' << std::dec;
    //    std::cout << '\n';
    //    std::cout << '\n';
    //}
}

// Create a Vulkan command buffer
// Information found in step 4 of VulkanAPI samples
void VulkanInstance::InitCommandBuffer()
{
    VkResult result;

    // Command pool info
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.pNext = NULL;
    commandPoolInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
    commandPoolInfo.flags = 0;

    // Create command pool and verify
    result = vkCreateCommandPool(m_vulkanDevice, &commandPoolInfo, NULL, &m_vulkanCommandPool);
    assert(result == VK_SUCCESS);

    // Fill in command buffer create info
    VkCommandBufferAllocateInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.pNext = NULL;
    commandBufferInfo.commandPool = m_vulkanCommandPool;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferInfo.commandBufferCount = 1;

    // Create command buffer and verify
    result = vkAllocateCommandBuffers(m_vulkanDevice, &commandBufferInfo, &m_vulkanCommandBuffer);
    assert(result == VK_SUCCESS);
}

// Create a Vulkan swap chain
// Information found in step 5 of VulkanAPI samples
void VulkanInstance::InitSwapChain()
{
    VkResult result;

    // Get surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vulkanDeviceVector[0], m_vulkanSurface, &surfaceCapabilities);
    assert(result == VK_SUCCESS);

    // Get present modes
    uint32_t presentModeCount;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vulkanDeviceVector[0], m_vulkanSurface, &presentModeCount, NULL);
    assert(result == VK_SUCCESS);
    VkPresentModeKHR *presentModes = (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vulkanDeviceVector[0], m_vulkanSurface, &presentModeCount, presentModes);
    assert(result == VK_SUCCESS);

    // Set width and height for swap chain
    VkExtent2D swapChainExtent;
    // width and height are either both -1 or both not -1
    if (surfaceCapabilities.currentExtent.width == (uint32_t)-1)
    {
        swapChainExtent.width = m_windowWidth;
        swapChainExtent.height = m_windowHeight;
    }
    else
    {
        swapChainExtent = surfaceCapabilities.currentExtent;
    }

    //Use mailbox mode if available. Lowest-latency non-tearing mode. If not, try immediate.
    VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < presentModeCount; ++i)
    {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapChainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if ((swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
        {
            swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    // Determine the number of VkImages to use in swap chain. 
    uint32_t numSwapChainImages = surfaceCapabilities.minImageCount + 1;
    if ((surfaceCapabilities.maxImageCount > 0) && (numSwapChainImages > surfaceCapabilities.maxImageCount))
    {
        numSwapChainImages = surfaceCapabilities.maxImageCount;
    }

    // Get the pre-transform
    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        preTransform = surfaceCapabilities.currentTransform;
    }

    // Fill in create info
    VkSwapchainCreateInfoKHR swapChainInfo = {};
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.pNext = NULL;
    swapChainInfo.surface = m_vulkanSurface;
    swapChainInfo.minImageCount = numSwapChainImages;
    swapChainInfo.imageFormat = m_vulkanFormat;
    swapChainInfo.imageExtent.width = swapChainExtent.width;
    swapChainInfo.imageExtent.height = swapChainExtent.height;
    swapChainInfo.preTransform = preTransform;
    swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.presentMode = swapChainPresentMode;
    swapChainInfo.oldSwapchain = NULL;
    swapChainInfo.clipped = true;
    swapChainInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.queueFamilyIndexCount = 0;
    swapChainInfo.pQueueFamilyIndices = NULL;

    // Create swap chain and verify
    result = vkCreateSwapchainKHR(m_vulkanDevice, &swapChainInfo, NULL, &m_vulkanSwapChain);
    assert(result == VK_SUCCESS);

    // Get images and verify
    result = vkGetSwapchainImagesKHR(m_vulkanDevice, m_vulkanSwapChain, &m_swapChainImageCount, NULL);
    assert(result == VK_SUCCESS);

    // Allocate and get images
    VkImage *swapChainImages = (VkImage *)malloc(m_swapChainImageCount * sizeof(VkImage));
    assert(swapChainImages);
    result = vkGetSwapchainImagesKHR(m_vulkanDevice, m_vulkanSwapChain, &m_swapChainImageCount, swapChainImages);
    assert(result == VK_SUCCESS);

    // Create the swap chain buffer
    m_swapChainBuffers.resize(m_swapChainImageCount);
    for (uint32_t i = 0; i < m_swapChainImageCount; ++i)
    {
        // Fill out image create info
        VkImageViewCreateInfo colorImageViewInfo = {};
        colorImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorImageViewInfo.pNext = NULL;
        colorImageViewInfo.format = m_vulkanFormat;
        colorImageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        colorImageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        colorImageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        colorImageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        colorImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorImageViewInfo.subresourceRange.baseMipLevel = 0;
        colorImageViewInfo.subresourceRange.levelCount = 1;
        colorImageViewInfo.subresourceRange.baseArrayLayer = 0;
        colorImageViewInfo.subresourceRange.layerCount = 1;
        colorImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorImageViewInfo.flags = 0;

        m_swapChainBuffers[i].image = swapChainImages[i];
        colorImageViewInfo.image = m_swapChainBuffers[i].image;

        // Fill out memory barrier info
        VkImageMemoryBarrier imgMemBarrier = {};
        imgMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imgMemBarrier.pNext = NULL;
        imgMemBarrier.srcAccessMask = 0;
        imgMemBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imgMemBarrier.image = m_swapChainBuffers[i].image;
        imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgMemBarrier.subresourceRange.baseMipLevel = 0;
        imgMemBarrier.subresourceRange.levelCount = 1;
        imgMemBarrier.subresourceRange.layerCount = 1;

        // Source and destination staging bits
        VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        // Create command pipeline barrier
        vkCmdPipelineBarrier(m_vulkanCommandBuffer, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &imgMemBarrier);

        // Create image view and verify
        result = vkCreateImageView(m_vulkanDevice, &colorImageViewInfo, NULL, &m_swapChainBuffers[i].view);
        assert(result == VK_SUCCESS);
    }

    // Free memory
    free(swapChainImages);
    free(presentModes);

    // m_current buffer will be updated by AcquireImageKHR and cycle through 
    // the buffer images in the swap chain
    m_currentBuffer = 0;
}

// Create a Vulkan depth buffer
// Information found in step 6 of VulkanAPI samples
void VulkanInstance::CreateDepthBuffer()
{
	// Set format for depth buffer
    m_depthBuffer.format = VK_FORMAT_D16_UNORM;

    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(m_vulkanDeviceVector[0], m_depthBuffer.format, &props);

	VkImageCreateInfo imageInfo = {};
    if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
    }
    else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    }
    else
    {
        std::cout << "VK_FORMAT_D16_UNORM not supported.\n";
        exit(-1);
    }

    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = NULL;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = m_depthBuffer.format;
    imageInfo.extent.width = m_windowWidth;
    imageInfo.extent.height = m_windowHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = NUM_SAMPLES;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices = NULL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0;

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = NULL;
    memAlloc.allocationSize = 0;
    memAlloc.memoryTypeIndex = 0;

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = NULL;
    viewInfo.image = VK_NULL_HANDLE;
    viewInfo.format = m_depthBuffer.format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.flags = 0;

    VkMemoryRequirements memoryRequirements;

    VkResult result = vkCreateImage(m_vulkanDevice, &imageInfo, NULL, &m_depthBuffer.image);
    assert(result == VK_SUCCESS);

    vkGetImageMemoryRequirements(m_vulkanDevice, m_depthBuffer.image, &memoryRequirements);

    memAlloc.allocationSize = memoryRequirements.size;

	// Search memory types to find first index with those properties
	bool pass = VulkanCommon::GetMemoryType(memoryRequirements.memoryTypeBits, VkMemoryPropertyFlagBits(0), memAlloc.memoryTypeIndex);
	assert(pass);

    //Allocate memory
    result = vkAllocateMemory(m_vulkanDevice, &memAlloc, NULL, &m_depthBuffer.memory);
    assert(result == VK_SUCCESS);

    // Bind memory
    result = vkBindImageMemory(m_vulkanDevice, m_depthBuffer.image, m_depthBuffer.memory, 0);
    assert(result == VK_SUCCESS);

    // Set image layout to depth stencil optimal
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = NULL;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = 0;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    imageMemoryBarrier.image = m_depthBuffer.image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(m_vulkanCommandBuffer, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);

    // Create image view
    viewInfo.image = m_depthBuffer.image;
    result = vkCreateImageView(m_vulkanDevice, &viewInfo, NULL, &m_depthBuffer.view);
    assert(result == VK_SUCCESS);
}

// Create a Vulkan uniform buffer
// Information found in step 7 of VulkanAPI samples
void VulkanInstance::CreateUniformBuffer(int threadNum)
{
	// For size and initial memory copy (which is fine if it is just garbage)
    glm::mat4 MVP;

    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = NULL;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.size = sizeof(MVP);
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.pQueueFamilyIndices = NULL;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.flags = 0;
    VkResult result = vkCreateBuffer(m_vulkanDevice, &bufferInfo, NULL, &m_uniformBuffers[threadNum].buffer);
    assert(result == VK_SUCCESS);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(m_vulkanDevice, m_uniformBuffers[threadNum].buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = NULL;
    allocInfo.memoryTypeIndex = 0;
    allocInfo.allocationSize = memoryRequirements.size;

	// Search memory types to find first index with those properties
	bool pass = VulkanCommon::GetMemoryType(memoryRequirements.memoryTypeBits, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), allocInfo.memoryTypeIndex);
	assert(pass);

    result = vkAllocateMemory(m_vulkanDevice, &allocInfo, NULL, &m_uniformBuffers[threadNum].memory);
    assert(result == VK_SUCCESS);

    uint8_t *pData;
    result = vkMapMemory(m_vulkanDevice, m_uniformBuffers[threadNum].memory, 0, memoryRequirements.size, 0, (void **)&pData);
    assert(result == VK_SUCCESS);

    memcpy(pData, &MVP, sizeof(MVP));

    vkUnmapMemory(m_vulkanDevice, m_uniformBuffers[threadNum].memory);

    result = vkBindBufferMemory(m_vulkanDevice, m_uniformBuffers[threadNum].buffer, m_uniformBuffers[threadNum].memory, 0);
    assert(result == VK_SUCCESS);

    m_uniformBuffers[threadNum].bufferInfo.buffer = m_uniformBuffers[threadNum].buffer;
    m_uniformBuffers[threadNum].bufferInfo.offset = 0;
    m_uniformBuffers[threadNum].bufferInfo.range = sizeof(MVP);
}

void VulkanInstance::UpdateUniformBufferForDebugCamera(int threadNum, float dt)
{
	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 Clip;
	glm::mat4 MVP;

	Projection = glm::perspective(camera[1].fov, static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight), camera[1].nearPlane, camera[1].farPlane);

	View = glm::lookAt(glm::vec3(camera[1].eye.x, camera[1].eye.y, camera[1].eye.z),
		glm::vec3(camera[1].center.x, camera[1].center.y, camera[1].center.z),
		glm::vec3(camera[1].up.x, camera[1].up.y, camera[1].up.z));

	glm::mat4 CameraMatrix = glm::lookAt(glm::vec3(camera[0].eye.x, camera[0].eye.y, camera[0].eye.z),
		glm::vec3(camera[0].center.x, camera[0].center.y, camera[0].center.z),
		glm::vec3(camera[0].up.x, camera[0].up.y, camera[0].up.z));

	// Need to invert Y and clip Z axis due to the Vulkan layout.
	Clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);

	MVP = Clip * Projection * View * CameraMatrix;

	// Search memory types to find first index with those properties
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_vulkanDevice, m_uniformBuffers[threadNum].buffer, &memoryRequirements);

	uint8_t *pData;
	VkResult result = vkMapMemory(m_vulkanDevice, m_uniformBuffers[threadNum].memory, 0, memoryRequirements.size, 0, (void **)&pData);
	assert(result == VK_SUCCESS);

	memcpy(pData, (const void *)&MVP[0][0], sizeof(MVP));

	vkUnmapMemory(m_vulkanDevice, m_uniformBuffers[threadNum].memory);
}

void VulkanInstance::UpdateUniformBuffer(int threadNum, float dt, int cameraId)
{
    glm::mat4 Projection, View, Clip, MVP;

    Projection = glm::perspective(camera[cameraId].fov, static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight), camera[cameraId].nearPlane, camera[cameraId].farPlane);

	View = glm::lookAt(glm::vec3(camera[cameraId].eye.x, camera[cameraId].eye.y, camera[cameraId].eye.z),
	    glm::vec3(camera[cameraId].center.x, camera[cameraId].center.y, camera[cameraId].center.z),
	    glm::vec3(camera[cameraId].up.x, camera[cameraId].up.y, camera[cameraId].up.z));
   	
    m_modelMatrices[threadNum] = glm::rotate(m_modelMatrices[threadNum], 1.0f * dt, glm::vec3(0, 1, 0));
    m_modelMatrices[threadNum][3][0] = (float)(threadNum * 3);
	m_modelMatrices[threadNum][3][2] = -(float)(threadNum * 3);

    // Need to invert Y and clip Z axis due to the Vulkan layout.
    Clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f, 
        0.0f, 0.0f, 0.5f, 1.0f);

	MVP = Clip * Projection * View * m_modelMatrices[threadNum];

    // Search memory types to find first index with those properties
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(m_vulkanDevice, m_uniformBuffers[threadNum].buffer, &memoryRequirements);

    uint8_t *pData;
    VkResult result = vkMapMemory(m_vulkanDevice, m_uniformBuffers[threadNum].memory, 0, memoryRequirements.size, 0, (void **)&pData);
    assert(result == VK_SUCCESS);

    memcpy(pData, (const void *)&MVP[0][0], sizeof(MVP));

    vkUnmapMemory(m_vulkanDevice, m_uniformBuffers[threadNum].memory);
}

// Create a Vulkan descriptor layout
// Information found in step 8 of VulkanAPI samples
void VulkanInstance::CreateDescriptorLayouts()
{
    VkDescriptorSetLayoutBinding layoutBinding[2];
    layoutBinding[0].binding = 0;
    layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding[0].descriptorCount = 1;
    layoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBinding[0].pImmutableSamplers = NULL;

    layoutBinding[1].binding = 1;
    layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding[1].descriptorCount = 1;
    layoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBinding[1].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
    descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayout.pNext = NULL;
    descriptorLayout.bindingCount = 2;
    descriptorLayout.pBindings = layoutBinding;

    m_vulkanDescriptorSetLayoutVector.resize(NUM_DESCRIPTOR_SETS);
    VkResult result = vkCreateDescriptorSetLayout(m_vulkanDevice, &descriptorLayout, NULL, m_vulkanDescriptorSetLayoutVector.data());
    assert(result == VK_SUCCESS);

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext = NULL;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
    pPipelineLayoutCreateInfo.setLayoutCount = NUM_DESCRIPTOR_SETS;
    pPipelineLayoutCreateInfo.pSetLayouts = m_vulkanDescriptorSetLayoutVector.data();

    result = vkCreatePipelineLayout(m_vulkanDevice, &pPipelineLayoutCreateInfo, NULL, &m_vulkanPipelineLayout);
    assert(result == VK_SUCCESS);
}

// Create a Vulkan descriptor set
// Information found in step 9 of VulkanAPI samples
void VulkanInstance::AllocateDescriptorSets()
{
    VkResult result;

    VkDescriptorPoolSize typeCount[2];
    typeCount[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    typeCount[0].descriptorCount = 1;
    typeCount[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    typeCount[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPool = {};
    descriptorPool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPool.pNext = NULL;
    descriptorPool.maxSets = 1;
    descriptorPool.poolSizeCount = 2;
    descriptorPool.pPoolSizes = typeCount;

    for (int i = 0; i < 3; ++i)
    {
        result = vkCreateDescriptorPool(m_vulkanDevice, &descriptorPool, NULL, &m_descriptorPools[i]);
        assert(result == VK_SUCCESS);
    }

    VkDescriptorSetAllocateInfo allocInfo[1];
    allocInfo->sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo[0].pNext = NULL;
    allocInfo[0].descriptorPool = m_descriptorPools[0];
    allocInfo[0].descriptorSetCount = NUM_DESCRIPTOR_SETS;
    allocInfo[0].pSetLayouts = m_vulkanDescriptorSetLayoutVector.data();

    for (int i = 0; i < 3; ++i)
    {
        allocInfo[0].descriptorPool = m_descriptorPools[i];
        m_descriptorSets[i].resize(NUM_DESCRIPTOR_SETS);
        result = vkAllocateDescriptorSets(m_vulkanDevice, allocInfo, m_descriptorSets[i].data());
        assert(result == VK_SUCCESS);
    }
}

// Create a Vulkan render pass
// Information found in step 10 of VulkanAPI samples
void VulkanInstance::InitRenderPass()
{
    VkAttachmentReference colorReference;
    VkAttachmentReference depthReference;

    VkAttachmentDescription attachments[2];
    attachments[0].format = m_vulkanFormat;
    attachments[0].samples = NUM_SAMPLES;    
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].flags = 0;

    attachments[1].format = m_depthBuffer.format;
    attachments[1].samples = NUM_SAMPLES;    
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].flags = 0;

    colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subPass = {};
    subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPass.flags = 0;
    subPass.inputAttachmentCount = 0;
    subPass.pInputAttachments = NULL;
    subPass.colorAttachmentCount = 1;
    subPass.pColorAttachments = &colorReference;
    subPass.pResolveAttachments = NULL;
    subPass.pDepthStencilAttachment = &depthReference;
    subPass.preserveAttachmentCount = 0;
    subPass.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo rpInfo = {};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.pNext = NULL;
    rpInfo.attachmentCount = 2;
    rpInfo.pAttachments = attachments;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subPass;
    rpInfo.dependencyCount = 0;
    rpInfo.pDependencies = NULL;

    VkResult result = vkCreateRenderPass(m_vulkanDevice, &rpInfo, NULL, &m_vulkanRenderPass);
    assert(result == VK_SUCCESS);
}

// Create Vulkan shaders
// Information found in step 11 of VulkanAPI samples
void VulkanInstance::InitShaders()
{
	Shader processor;
	std::string vs;
	std::string fs;
	processor.LoadFile("vertex.vs", vs);
	processor.LoadFile("fragment.fs", fs);

    std::vector<unsigned int> vtxSpv;
    m_vulkanPipelineShaderStageInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_vulkanPipelineShaderStageInfo[0].pNext = NULL;
    m_vulkanPipelineShaderStageInfo[0].pSpecializationInfo = NULL;
    m_vulkanPipelineShaderStageInfo[0].flags = 0;
    m_vulkanPipelineShaderStageInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    m_vulkanPipelineShaderStageInfo[0].pName = "main";

    glslang::InitializeProcess();
	bool returnVal = processor.GLSLtoSPV(VK_SHADER_STAGE_VERTEX_BIT, vs.c_str(), vtxSpv);
    assert(returnVal);

    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = vtxSpv.size() * sizeof(unsigned int);
    moduleCreateInfo.pCode = vtxSpv.data();
    VkResult result = vkCreateShaderModule(m_vulkanDevice, &moduleCreateInfo, NULL, &m_vulkanPipelineShaderStageInfo[0].module);
    assert(result == VK_SUCCESS);

    std::vector<unsigned int> fragSpv;
    m_vulkanPipelineShaderStageInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_vulkanPipelineShaderStageInfo[1].pNext = NULL;
    m_vulkanPipelineShaderStageInfo[1].pSpecializationInfo = NULL;
    m_vulkanPipelineShaderStageInfo[1].flags = 0;
    m_vulkanPipelineShaderStageInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    m_vulkanPipelineShaderStageInfo[1].pName = "main";

	returnVal = processor.GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT_BIT, fs.c_str(), fragSpv);
    assert(returnVal);

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = fragSpv.size() * sizeof(unsigned int);
    moduleCreateInfo.pCode = fragSpv.data();
    result = vkCreateShaderModule(m_vulkanDevice, &moduleCreateInfo, NULL, &m_vulkanPipelineShaderStageInfo[1].module);
    assert(result == VK_SUCCESS);

    glslang::FinalizeProcess();
}

// Create a Vulkan frame buffer
// Information found in step 12 of VulkanAPI samples
void VulkanInstance::InitFrameBuffers()
{
    VkImageView attachments[2];
    attachments[1] = m_depthBuffer.view;

    VkFramebufferCreateInfo fbInfo = {};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.pNext = NULL;
    fbInfo.renderPass = m_vulkanRenderPass;
    fbInfo.attachmentCount = 2;
    fbInfo.pAttachments = attachments;
    fbInfo.width = m_windowWidth;
    fbInfo.height = m_windowHeight;
    fbInfo.layers = 1;

    uint32_t i;
    m_vulkanFrameBuffers = (VkFramebuffer *)malloc(m_swapChainImageCount * sizeof(VkFramebuffer));
    assert(m_vulkanFrameBuffers);

    for (i = 0; i < m_swapChainImageCount; ++i)
    {
        attachments[0] = m_swapChainBuffers[i].view;
        VkResult result = vkCreateFramebuffer(m_vulkanDevice, &fbInfo, NULL, &m_vulkanFrameBuffers[i]);
        assert(result == VK_SUCCESS);
    }

}

// Create a Vulkan vertex buffer
// Information found in step 13 of VulkanAPI samples
void VulkanInstance::InitVertexBuffer()
{
    VkBufferCreateInfo bufInfo = {};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.pNext = NULL;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufInfo.size = sizeof(g_vb_texture_Data);
    bufInfo.queueFamilyIndexCount = 0;
    bufInfo.pQueueFamilyIndices = NULL;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufInfo.flags = 0;
    VkResult result = vkCreateBuffer(m_vulkanDevice, &bufInfo, NULL, &m_vertexBuffers[0].buffer);
    assert(result == VK_SUCCESS);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(m_vulkanDevice, m_vertexBuffers[0].buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = NULL;
    allocInfo.memoryTypeIndex = 0;
    allocInfo.allocationSize = memoryRequirements.size;
	
	// Search memory types to find first index with those properties
	bool test = VulkanCommon::GetMemoryType(memoryRequirements.memoryTypeBits, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), allocInfo.memoryTypeIndex);
	assert(test);

    result = vkAllocateMemory(m_vulkanDevice, &allocInfo, NULL, &m_vertexBuffers[0].memory);
    assert(result == VK_SUCCESS);
    m_vertexBuffers[0].bufferInfo.range = memoryRequirements.size;
    m_vertexBuffers[0].bufferInfo.offset = 0;

    uint8_t *pData;
    result = vkMapMemory(m_vulkanDevice, m_vertexBuffers[0].memory, 0, memoryRequirements.size, 0, (void **)&pData);
    assert(result == VK_SUCCESS);

    memcpy(pData, g_vb_texture_Data, sizeof(g_vb_texture_Data));

    vkUnmapMemory(m_vulkanDevice, m_vertexBuffers[0].memory);

    result = vkBindBufferMemory(m_vulkanDevice, m_vertexBuffers[0].buffer, m_vertexBuffers[0].memory, 0);
    assert(result == VK_SUCCESS);
}


// Create a Vulkan pipeline
// Information found in step 14 of VulkanAPI samples
void VulkanInstance::InitPipeline()
{
    // Setup binding for vertex data
    VkVertexInputBindingDescription vulkanVertexInputBinding;
    VkVertexInputAttributeDescription vulkanVertextInputAttributes[2];

    vulkanVertexInputBinding.binding = 0;
    vulkanVertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vulkanVertexInputBinding.stride = sizeof(g_vb_texture_Data[0]);

    vulkanVertextInputAttributes[0].binding = 0;
    vulkanVertextInputAttributes[0].location = 0;
    vulkanVertextInputAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vulkanVertextInputAttributes[0].offset = 0;
    vulkanVertextInputAttributes[1].binding = 0;
    vulkanVertextInputAttributes[1].location = 1;
    vulkanVertextInputAttributes[1].format = VK_FORMAT_R32G32_SFLOAT; // for texture coordinates
    vulkanVertextInputAttributes[1].offset = 16;

    VkPipelineCacheCreateInfo pipelineCacheInfo;
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheInfo.pNext = NULL;
    pipelineCacheInfo.initialDataSize = 0;
    pipelineCacheInfo.pInitialData = NULL;
    pipelineCacheInfo.flags = 0;

    VkResult result = vkCreatePipelineCache(m_vulkanDevice, &pipelineCacheInfo, NULL, &m_vulkanPipelineCache);
    assert(result == VK_SUCCESS);

    VkDynamicState dynamicStateEnables[2]; // viewport and scissor?
    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.pNext = NULL;
    dynamicStateInfo.pDynamicStates = dynamicStateEnables;
    dynamicStateInfo.dynamicStateCount = 0;

    VkPipelineVertexInputStateCreateInfo viInfo;
    viInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    viInfo.pNext = NULL;
    viInfo.flags = 0;
    viInfo.vertexBindingDescriptionCount = 1;
    viInfo.pVertexBindingDescriptions = &vulkanVertexInputBinding;
    viInfo.vertexAttributeDescriptionCount = 2;
    viInfo.pVertexAttributeDescriptions = vulkanVertextInputAttributes;

    VkPipelineInputAssemblyStateCreateInfo iaInfo;
    iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaInfo.pNext = NULL;
    iaInfo.flags = 0;
    iaInfo.primitiveRestartEnable = VK_FALSE;
    iaInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo rsInfo;
    rsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rsInfo.pNext = NULL;
    rsInfo.flags = 0;
    rsInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rsInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rsInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rsInfo.depthClampEnable = VK_FALSE;
    rsInfo.rasterizerDiscardEnable = VK_FALSE;
    rsInfo.depthBiasEnable = VK_FALSE;
    rsInfo.depthBiasConstantFactor = 0;
    rsInfo.depthBiasClamp = 0;
    rsInfo.depthBiasSlopeFactor = 0;
    rsInfo.lineWidth = 0;

    VkPipelineColorBlendStateCreateInfo cbInfo;
    cbInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbInfo.pNext = NULL;
    cbInfo.flags = 0;

    VkPipelineColorBlendAttachmentState attachmentState[1];
    attachmentState[0].colorWriteMask = 0xf;
    attachmentState[0].blendEnable = VK_FALSE;
    attachmentState[0].alphaBlendOp = VK_BLEND_OP_ADD;
    attachmentState[0].colorBlendOp = VK_BLEND_OP_ADD;
    attachmentState[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachmentState[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachmentState[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachmentState[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    cbInfo.attachmentCount = 1;
    cbInfo.pAttachments = attachmentState;
    cbInfo.logicOpEnable = VK_FALSE;
    cbInfo.logicOp = VK_LOGIC_OP_NO_OP;
    cbInfo.blendConstants[0] = 1.0f;
    cbInfo.blendConstants[1] = 1.0f;
    cbInfo.blendConstants[2] = 1.0f;
    cbInfo.blendConstants[3] = 1.0f;

    VkPipelineViewportStateCreateInfo vpInfo = {};
    vpInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpInfo.pNext = NULL;
    vpInfo.flags = 0;
    vpInfo.viewportCount = 1;
    vpInfo.scissorCount = 1;
    dynamicStateEnables[dynamicStateInfo.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStateEnables[dynamicStateInfo.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    vpInfo.pScissors = NULL;
    vpInfo.pViewports = NULL;

    VkPipelineDepthStencilStateCreateInfo dsInfo;
    dsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dsInfo.pNext = NULL;
    dsInfo.flags = 0;
    dsInfo.depthTestEnable = VK_TRUE;
    dsInfo.depthWriteEnable = VK_TRUE;
    dsInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    dsInfo.depthBoundsTestEnable = VK_FALSE;
    dsInfo.minDepthBounds = 0;
    dsInfo.maxDepthBounds = 0;
    dsInfo.stencilTestEnable = VK_FALSE;
    dsInfo.back.failOp = VK_STENCIL_OP_KEEP;
    dsInfo.back.passOp = VK_STENCIL_OP_KEEP;
    dsInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
    dsInfo.back.compareMask = 0;
    dsInfo.back.reference = 0;
    dsInfo.back.depthFailOp = VK_STENCIL_OP_KEEP;
    dsInfo.back.writeMask = 0;
    dsInfo.front = dsInfo.back;

    VkPipelineMultisampleStateCreateInfo msInfo;
    msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msInfo.pNext = NULL;
    msInfo.flags = 0;
    msInfo.pSampleMask = NULL;
    msInfo.rasterizationSamples = NUM_SAMPLES;
    msInfo.sampleShadingEnable = VK_FALSE;
    msInfo.alphaToCoverageEnable = VK_FALSE;
    msInfo.alphaToOneEnable = VK_FALSE;
    msInfo.minSampleShading = 0.0;

    VkGraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = NULL;
    pipelineInfo.layout = m_vulkanPipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = 0;
    pipelineInfo.flags = 0;
    pipelineInfo.pVertexInputState = &viInfo;
    pipelineInfo.pInputAssemblyState = &iaInfo;
    pipelineInfo.pRasterizationState = &rsInfo;
    pipelineInfo.pColorBlendState = &cbInfo;
    pipelineInfo.pTessellationState = NULL;
    pipelineInfo.pMultisampleState = &msInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.pViewportState = &vpInfo;
    pipelineInfo.pDepthStencilState = &dsInfo;
    pipelineInfo.pStages = m_vulkanPipelineShaderStageInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.renderPass = m_vulkanRenderPass;
    pipelineInfo.subpass = 0;

    result = vkCreateGraphicsPipelines(m_vulkanDevice, m_vulkanPipelineCache, 1, &pipelineInfo, NULL, &m_vulkanPipeline[0]);
    assert(result == VK_SUCCESS);

	// Create a pipeline for drawing lines with same setup except different input assembler
	VkGraphicsPipelineCreateInfo pipelineInfoLines = pipelineInfo;

	VkVertexInputBindingDescription vulkanVertexInputBindingLines;
	VkVertexInputAttributeDescription vulkanVertextInputAttributesLines;

	vulkanVertexInputBindingLines.binding = 0;
	vulkanVertexInputBindingLines.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vulkanVertexInputBindingLines.stride = sizeof(Vec4);

	vulkanVertextInputAttributesLines.binding = 0;
	vulkanVertextInputAttributesLines.location = 0;
	vulkanVertextInputAttributesLines.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vulkanVertextInputAttributesLines.offset = 0;

	VkPipelineInputAssemblyStateCreateInfo iaInfoLines;
	iaInfoLines.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	iaInfoLines.pNext = NULL;
	iaInfoLines.flags = 0;
	iaInfoLines.primitiveRestartEnable = VK_FALSE;
	iaInfoLines.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

	VkPipelineVertexInputStateCreateInfo viInfoLines;
	viInfoLines.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	viInfoLines.pNext = NULL;
	viInfoLines.flags = 0;
	viInfoLines.vertexBindingDescriptionCount = 1;
	viInfoLines.pVertexBindingDescriptions = &vulkanVertexInputBindingLines;
	viInfoLines.vertexAttributeDescriptionCount = 1;
	viInfoLines.pVertexAttributeDescriptions = &vulkanVertextInputAttributesLines;

	VkPipelineRasterizationStateCreateInfo rsInfoLines;
	rsInfoLines.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rsInfoLines.pNext = NULL;
	rsInfoLines.flags = 0;
	rsInfoLines.polygonMode = VK_POLYGON_MODE_LINE;
	rsInfoLines.cullMode = VK_CULL_MODE_BACK_BIT;
	rsInfoLines.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rsInfoLines.depthClampEnable = VK_FALSE;
	rsInfoLines.rasterizerDiscardEnable = VK_FALSE;
	rsInfoLines.depthBiasEnable = VK_FALSE;
	rsInfoLines.depthBiasConstantFactor = 0;
	rsInfoLines.depthBiasClamp = 0;
	rsInfoLines.depthBiasSlopeFactor = 0;
	rsInfoLines.lineWidth = 1;

	pipelineInfoLines.pInputAssemblyState = &iaInfoLines;
	pipelineInfoLines.pVertexInputState = &viInfoLines;
	pipelineInfoLines.pRasterizationState = &rsInfoLines;

	result = vkCreateGraphicsPipelines(m_vulkanDevice, m_vulkanPipelineCache, 1, &pipelineInfoLines, NULL, &m_vulkanPipeline[1]);
	assert(result == VK_SUCCESS);
}

// Draw a cube with Vulkan
// Information found in step 15 of VulkanAPI samples
void VulkanInstance::DrawCube(float dt)
{
    // Update matrix position for cube	
	if (m_currentCamera == 2)
		dt = 0;

    UpdateUniformBuffer(0, dt, m_currentCamera);

    VkWriteDescriptorSet writes[2];

    writes[0] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pNext = NULL;
    writes[0].dstSet = m_descriptorSets[0][0];
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &m_uniformBuffers[0].bufferInfo;
    writes[0].dstArrayElement = 0;
    writes[0].dstBinding = 0;

    writes[1] = {};
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_descriptorSets[0][0];
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &m_vulkanImageInfo;
    writes[1].dstArrayElement = 0;
    writes[1].dstBinding = 1;

    vkUpdateDescriptorSets(m_vulkanDevice, 2, writes, 0, NULL);

    VkCommandBufferBeginInfo commandBufferBegin = {};
    commandBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBegin.pNext = NULL;
    commandBufferBegin.flags = 0;
    commandBufferBegin.pInheritanceInfo = NULL;

    VkResult result = vkBeginCommandBuffer(m_vulkanCommandBuffer, &commandBufferBegin);
    assert(result == VK_SUCCESS);

    VkClearValue clearValues[2];
    clearValues[0].color.float32[0] = 0.2f;
    clearValues[0].color.float32[1] = 0.2f;
    clearValues[0].color.float32[2] = 0.2f;
    clearValues[0].color.float32[3] = 0.2f;
    clearValues[1].depthStencil.depth = 1.0f;
    clearValues[1].depthStencil.stencil = 0;

    VkSemaphoreCreateInfo presentCompleteSemaphoreInfo;
    presentCompleteSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreInfo.pNext = NULL;
    presentCompleteSemaphoreInfo.flags = 0;

    VkSemaphore presentCompleteSemaphore;
    result = vkCreateSemaphore(m_vulkanDevice, &presentCompleteSemaphoreInfo, NULL, &presentCompleteSemaphore);
    assert(result == VK_SUCCESS);

    result = vkAcquireNextImageKHR(m_vulkanDevice, m_vulkanSwapChain, UINT64_MAX, presentCompleteSemaphore, NULL, &m_currentBuffer);
    assert(result == VK_SUCCESS);

    VkImageMemoryBarrier prePresentBarrier = {};
    prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    prePresentBarrier.pNext = NULL;
    prePresentBarrier.srcAccessMask = 0;
    prePresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    prePresentBarrier.subresourceRange.baseMipLevel = 0;
    prePresentBarrier.subresourceRange.levelCount = 1;
    prePresentBarrier.subresourceRange.baseArrayLayer = 0;
    prePresentBarrier.subresourceRange.layerCount = 1;
    prePresentBarrier.image = m_swapChainBuffers[m_currentBuffer].image;
    vkCmdPipelineBarrier(m_vulkanCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &prePresentBarrier);

    VkRenderPassBeginInfo rpBeginInfo;
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.pNext = NULL;
    rpBeginInfo.renderPass = m_vulkanRenderPass;
    rpBeginInfo.framebuffer = m_vulkanFrameBuffers[m_currentBuffer];
    rpBeginInfo.renderArea.offset.x = 0;
    rpBeginInfo.renderArea.offset.y = 0;
    rpBeginInfo.renderArea.extent.width = m_windowWidth;
    rpBeginInfo.renderArea.extent.height = m_windowHeight;
    rpBeginInfo.clearValueCount = 2;
    rpBeginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(m_vulkanCommandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipeline[0]);
    vkCmdBindDescriptorSets(m_vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipelineLayout, 0, NUM_DESCRIPTOR_SETS, m_descriptorSets[0].data(), 0, NULL);

    VkViewport viewport;
    viewport.width = (float)m_windowWidth;
    viewport.height = (float)m_windowHeight;    
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(m_vulkanCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.extent.width = m_windowWidth;
    scissor.extent.height = m_windowHeight;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(m_vulkanCommandBuffer, 0, 1, &scissor);

	// OBJ MODEL BEGIN
	for (unsigned int i = 0; i < models.size(); ++i)
	{
		const VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_vulkanCommandBuffer, 0, 1, &models[i].buffer, offsets);
		vkCmdBindIndexBuffer(m_vulkanCommandBuffer, models[i].indices, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(m_vulkanCommandBuffer, models[i].numIndices, 1, 0, 0, 0);
	}
	// OBJ MODEL END

	// Draw textured cube
	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(m_vulkanCommandBuffer, 0, 1, &m_vertexBuffers[0].buffer, offsets);
	vkCmdDraw(m_vulkanCommandBuffer, 12 * 3, 1, 0, 0);

	// Draw lines
	vkCmdBindPipeline(m_vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipeline[1]);

	if (m_currentCamera == 1)
	{
		UpdateUniformBufferForDebugCamera(0, dt);
	}
	else if(m_currentCamera == 2)
	{
		UpdateUniformBuffer(0, 0, m_currentCamera);
	}
	else
	{
		UpdateUniformBuffer(0, dt, m_currentCamera);
	}

	for (unsigned int i = 0; i < lines.size(); ++i)
	{
		vkCmdBindVertexBuffers(m_vulkanCommandBuffer, 0, 1, &lines[i].buffer, offsets);
		vkCmdDraw(m_vulkanCommandBuffer, lines[i].numVertices, 1, 0, 0);
	}

	// End render pass
	vkCmdEndRenderPass(m_vulkanCommandBuffer);

	result = vkEndCommandBuffer(m_vulkanCommandBuffer);
	assert(result == VK_SUCCESS);

    VkFenceCreateInfo fenceInfo;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;

    VkFence drawFence;
    result = vkCreateFence(m_vulkanDevice, &fenceInfo, NULL, &drawFence);
    assert(result == VK_SUCCESS);

    VkPipelineStageFlags pipeStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo[1] = {};
    submitInfo[0].pNext = NULL;
    submitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo[0].waitSemaphoreCount = 1;
    submitInfo[0].pWaitSemaphores = &presentCompleteSemaphore;
    submitInfo[0].pWaitDstStageMask = &pipeStageFlags;
    submitInfo[0].commandBufferCount = 1;
    submitInfo[0].pCommandBuffers = &m_vulkanCommandBuffer;
    submitInfo[0].signalSemaphoreCount = 0;
    submitInfo[0].pSignalSemaphores = NULL;

    result = vkQueueSubmit(m_vulkanQueue, 1, submitInfo, drawFence);
    assert(result == VK_SUCCESS);

    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_vulkanSwapChain;
    presentInfo.pImageIndices = &m_currentBuffer;
    presentInfo.pWaitSemaphores = NULL;
    presentInfo.waitSemaphoreCount = 0;
    presentInfo.pResults = NULL;

    do
    {
        result = vkWaitForFences(m_vulkanDevice, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    } while (result == VK_TIMEOUT);
    assert(result == VK_SUCCESS);


    result = vkQueuePresentKHR(m_vulkanQueue, &presentInfo);
    assert(result == VK_SUCCESS);

    // Need this destroy to avoid memory leak during draw cube
    vkDestroySemaphore(m_vulkanDevice, presentCompleteSemaphore, NULL);
    vkDestroyFence(m_vulkanDevice, drawFence, NULL);
}

void VulkanInstance::Destroy()
{
    //----------------------------------------------------------------------------
    // Destruction phase

    // Destroy pipeline
    vkDestroyPipeline(m_vulkanDevice, m_vulkanPipeline[0], NULL);
    vkDestroyPipelineCache(m_vulkanDevice, m_vulkanPipelineCache, NULL);

    for (int i = 0; i < 3; ++i)
    {
        // Destroy vertex buffer
        vkDestroyBuffer(m_vulkanDevice, m_vertexBuffers[i].buffer, NULL);
        vkFreeMemory(m_vulkanDevice, m_vertexBuffers[i].memory, NULL);

        // Destroy descriptor pool
        vkDestroyDescriptorPool(m_vulkanDevice, m_descriptorPools[i], NULL);
    }

    // Destroy frame buffer
    for (uint32_t i = 0; i < m_swapChainImageCount; ++i)
    {
        vkDestroyFramebuffer(m_vulkanDevice, m_vulkanFrameBuffers[i], NULL);
    }

    // Destroy shaders
    vkDestroyShaderModule(m_vulkanDevice, m_vulkanPipelineShaderStageInfo[0].module, NULL);
    vkDestroyShaderModule(m_vulkanDevice, m_vulkanPipelineShaderStageInfo[1].module, NULL);

    // Destroy render pass
    vkDestroyRenderPass(m_vulkanDevice, m_vulkanRenderPass, NULL);

    // Destroy descriptor set layouts
    for (int i = 0; i < NUM_DESCRIPTOR_SETS; i++)
        vkDestroyDescriptorSetLayout(m_vulkanDevice, m_vulkanDescriptorSetLayoutVector[i], NULL);
    vkDestroyPipelineLayout(m_vulkanDevice, m_vulkanPipelineLayout, NULL);

    // Destroy uniform buffer
    vkDestroyBuffer(m_vulkanDevice, m_uniformBuffers[0].buffer, NULL);
    vkDestroyBuffer(m_vulkanDevice, m_uniformBuffers[1].buffer, NULL);
    vkDestroyBuffer(m_vulkanDevice, m_uniformBuffers[2].buffer, NULL);
    vkFreeMemory(m_vulkanDevice, m_uniformBuffers[0].memory, NULL);
    vkFreeMemory(m_vulkanDevice, m_uniformBuffers[1].memory, NULL);
    vkFreeMemory(m_vulkanDevice, m_uniformBuffers[2].memory, NULL);

    // Destroy depth buffer
    vkDestroyImageView(m_vulkanDevice, m_depthBuffer.view, NULL);
    vkDestroyImage(m_vulkanDevice, m_depthBuffer.image, NULL);
    vkFreeMemory(m_vulkanDevice, m_depthBuffer.memory, NULL);

    // DESTROY IMAGE VIEW AND SWAP CHAIN
    for (uint32_t i = 0; i < m_swapChainImageCount; i++)
    {
        vkDestroyImageView(m_vulkanDevice, m_swapChainBuffers[i].view, NULL);
    }
    vkDestroySwapchainKHR(m_vulkanDevice, m_vulkanSwapChain, NULL);

    // FREE COMMAND BUFFERS
    VkCommandBuffer commandBuffers[1] = { m_vulkanCommandBuffer };
    vkFreeCommandBuffers(m_vulkanDevice, m_vulkanCommandPool, 1, commandBuffers);

    // FREE COMMAND POOL
    vkDestroyCommandPool(m_vulkanDevice, m_vulkanCommandPool, NULL);

    // DESTROY VULKAN DEVICE
    vkDestroyDevice(m_vulkanDevice, NULL);

    // DESTORY VULKAN INSTANCE
    vkDestroyInstance(m_vulkanInstance, NULL);
}

void VulkanInstance::PerThreadCode(int threadId, float dt)
{
    UpdateUniformBuffer(threadId, dt, m_currentCamera);

    VkWriteDescriptorSet writes[2];

    writes[0] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pNext = NULL;
    writes[0].dstSet = m_descriptorSets[threadId][0];
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &m_uniformBuffers[threadId].bufferInfo;
    writes[0].dstArrayElement = 0;
    writes[0].dstBinding = 0;

    writes[1] = {};
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = m_descriptorSets[threadId][0];
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &m_vulkanImageInfo;
    writes[1].dstArrayElement = 0;
    writes[1].dstBinding = 1;

    vkUpdateDescriptorSets(m_vulkanDevice, 2, writes, 0, NULL);

    VkCommandBufferInheritanceInfo inherit = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
    inherit.framebuffer = m_vulkanFrameBuffers[m_currentBuffer];
    inherit.renderPass = m_vulkanRenderPass;

    VkCommandBufferBeginInfo cmdBufBeginInfo = {};
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBeginInfo.pNext = NULL;
    cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT |
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;;
    cmdBufBeginInfo.pInheritanceInfo = &inherit;

    VkResult result = vkBeginCommandBuffer(m_commandBuffers[threadId], &cmdBufBeginInfo);
    assert(result == VK_SUCCESS);

    vkCmdBindPipeline(m_commandBuffers[threadId], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipeline[0]);
    vkCmdBindDescriptorSets(m_commandBuffers[threadId], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipelineLayout, 0, NUM_DESCRIPTOR_SETS, m_descriptorSets[threadId].data(), 0, NULL);

    const VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(m_commandBuffers[threadId], 0, 1, &m_vertexBuffers[threadId].buffer, offsets);

    VkViewport viewport;
    viewport.height = (float)m_windowHeight;
    viewport.width = (float)m_windowWidth;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(m_commandBuffers[threadId], 0, 1, &viewport);

    VkRect2D scissor;
    scissor.extent.width = m_windowWidth;
    scissor.extent.height = m_windowHeight;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(m_commandBuffers[threadId], 0, 1, &scissor);

    vkCmdDraw(m_commandBuffers[threadId], 12 * 3, 1, 0, 0);

    result = vkEndCommandBuffer(m_commandBuffers[threadId]);
    assert(result == VK_SUCCESS);
}

void CALLBACK WorkCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    CallbackData *cb = (CallbackData*)Parameter;
    cb->m_instance->PerThreadCode(cb->m_thread, cb->m_dt);
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);
    return;
}

void VulkanInstance::DrawCubesMultithreaded(float dt)
{
    VkResult result = {};
    VkSubmitInfo submitInfo[1] = {};
    VkFenceCreateInfo fenceInfo = {};
    VkClearValue clearValues[2] = {};
    VkCommandBufferBeginInfo cmdBufBeginInfo = {};
    VkSemaphoreCreateInfo presentCompleteSemaphoreInfo = {};
    VkSemaphore presentCompleteSemaphore = {};
    VkRenderPassBeginInfo renderPassBegin = {};
    VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo = {};
    VkSemaphore renderSemaphore = {};
    VkImageMemoryBarrier prePresentBarrier = {};
    VkFence drawFence = {};
    VkPresentInfoKHR present = {};

    // Set our clear values for both attachments
    clearValues[0].color.float32[0] = 0.0f;
    clearValues[0].color.float32[1] = 0.0f;
    clearValues[0].color.float32[2] = 0.0f;
    clearValues[0].color.float32[3] = 0.0f;
    clearValues[1].depthStencil.depth = 1.0f;
    clearValues[1].depthStencil.stencil = 0;

    // Have we finished present on the previous frame?
    presentCompleteSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreInfo.pNext = NULL;
    presentCompleteSemaphoreInfo.flags = 0;

    result = vkCreateSemaphore(m_vulkanDevice, &presentCompleteSemaphoreInfo, NULL, &presentCompleteSemaphore);
    assert(result == VK_SUCCESS);

    // This will advance current_buffer to the next frame buffer in the swap chain
    // In my current build, has 3 buffers and cycles 0, 1, 2, 0, 1, 2
    result = vkAcquireNextImageKHR(m_vulkanDevice, m_vulkanSwapChain, UINT64_MAX, presentCompleteSemaphore, NULL, &m_currentBuffer);
    assert(result == VK_SUCCESS);

    imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    imageAcquiredSemaphoreCreateInfo.pNext = NULL;
    imageAcquiredSemaphoreCreateInfo.flags = 0;

    result = vkCreateSemaphore(m_vulkanDevice, &imageAcquiredSemaphoreCreateInfo, VK_NULL_HANDLE, &renderSemaphore);
    assert(result == VK_SUCCESS);

    // Info to start recording render commands
    cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBeginInfo.pNext = NULL;
    cmdBufBeginInfo.flags = 0;
    cmdBufBeginInfo.pInheritanceInfo = NULL;

    result = vkBeginCommandBuffer(m_vulkanCommandBuffer, &cmdBufBeginInfo);
    assert(result == VK_SUCCESS);

    // Render pass begin structure that will tie a render pass to a frame buffer
    renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBegin.pNext = NULL;
    renderPassBegin.renderPass = m_vulkanRenderPass;
    renderPassBegin.framebuffer = m_vulkanFrameBuffers[m_currentBuffer];
    renderPassBegin.renderArea.offset.x = 0;
    renderPassBegin.renderArea.offset.y = 0;
    renderPassBegin.renderArea.extent.width = m_windowWidth;
    renderPassBegin.renderArea.extent.height = m_windowHeight;
    renderPassBegin.clearValueCount = 2;
    renderPassBegin.pClearValues = clearValues;

    vkCmdBeginRenderPass(m_vulkanCommandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	// Update dt
	m_callbackData[0]->m_dt = dt;
	m_callbackData[1]->m_dt = dt;
	m_callbackData[2]->m_dt = dt;

	// Submit work to thread pool using PerThreadCode callback
    SubmitThreadpoolWork(m_works[0]);
    SubmitThreadpoolWork(m_works[1]);
    SubmitThreadpoolWork(m_works[2]);

    // Wait for all threads before syncing commands to the main command buffer
    WaitForThreadpoolWorkCallbacks(m_works[0], false);
    WaitForThreadpoolWorkCallbacks(m_works[1], false);
    WaitForThreadpoolWorkCallbacks(m_works[2], false);

    // Execute secondary command buffers on main command buffer
    vkCmdExecuteCommands(m_vulkanCommandBuffer, 3, m_commandBuffers);

    // End the render pass
    vkCmdEndRenderPass(m_vulkanCommandBuffer);

    // Pre present barrier 
    // Need more documentation to understand this
    prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    prePresentBarrier.pNext = NULL;
    prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    prePresentBarrier.subresourceRange.baseMipLevel = 0;
    prePresentBarrier.subresourceRange.levelCount = 1;
    prePresentBarrier.subresourceRange.baseArrayLayer = 0;
    prePresentBarrier.subresourceRange.layerCount = 1;
    prePresentBarrier.image = m_swapChainBuffers[m_currentBuffer].image;
    vkCmdPipelineBarrier(m_vulkanCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &prePresentBarrier);

    // End command buffer recording
    result = vkEndCommandBuffer(m_vulkanCommandBuffer);
    assert(result == VK_SUCCESS);

    // Fill out submit info
    // Only submitting main command buffer
    // If we had four primary command buffers, we would up the count to 4 and point to an array of command buffers
    // We can split render passes this way, three command buffers, a render pass each
    // A fourth command buffer for the pre present barrier
    // The other three could be on separate threads
    // And this submit would sync them
    // However, that would split render passes and cause load op delays on some platforms
    uint32_t submitpipeStateFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    submitInfo[0].pNext = NULL;
    submitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo[0].waitSemaphoreCount = 1;
    submitInfo[0].pWaitSemaphores = &presentCompleteSemaphore;
    submitInfo[0].pWaitDstStageMask = &submitpipeStateFlags;
    submitInfo[0].commandBufferCount = 1;
    submitInfo[0].pCommandBuffers = &m_vulkanCommandBuffer;
    submitInfo[0].signalSemaphoreCount = 1;
    submitInfo[0].pSignalSemaphores = &renderSemaphore;

    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;
    vkCreateFence(m_vulkanDevice, &fenceInfo, NULL, &drawFence);

    result = vkQueueSubmit(m_vulkanQueue, 1, submitInfo, drawFence);
    assert(result == VK_SUCCESS);

    do
    {
        result = vkWaitForFences(m_vulkanDevice, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    } while (result == VK_TIMEOUT);
    assert(result == VK_SUCCESS);

    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &m_vulkanSwapChain;
    present.pImageIndices = &m_currentBuffer;
    present.pWaitSemaphores = &renderSemaphore;
    present.waitSemaphoreCount = 1;
    present.pResults = NULL;

    result = vkQueuePresentKHR(m_vulkanQueue, &present);
    assert(!result);

    vkDestroyFence(m_vulkanDevice, drawFence, NULL);
    vkDestroySemaphore(m_vulkanDevice, renderSemaphore, NULL);
    vkDestroySemaphore(m_vulkanDevice, presentCompleteSemaphore, NULL);
}

// Init objects per thread and set up work and callback data
void VulkanInstance::InitMultithreaded()
{
    // Thread pool create and settings
    m_renderThreadPool = CreateThreadpool(NULL);
    SetThreadpoolThreadMinimum(m_renderThreadPool, 3);
    SetThreadpoolThreadMaximum(m_renderThreadPool, 3);
    InitializeThreadpoolEnvironment(&m_callbackEnv);
    m_cleanupGroup = CreateThreadpoolCleanupGroup();
    SetThreadpoolCallbackCleanupGroup(&m_callbackEnv, m_cleanupGroup, NULL);

    // Declare Vulkan variables needed for per thread code	
    VkResult result = {};
    VkCommandPoolCreateInfo poolInfo = {};
    VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
    VkBufferCreateInfo bufInfo = {};
    VkMemoryRequirements memoryRequirements = {};
    VkMemoryAllocateInfo allocInfo = {};

    for (int thread = 0; thread < 3; ++thread)
    {
        // Uniform buffer per thread, so no shared access requiring locks
        CreateUniformBuffer(thread);

        // Setup command pool 
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.pNext = NULL;
        poolInfo.queueFamilyIndex = m_graphicsQueueFamilyIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        vkCreateCommandPool(m_vulkanDevice, &poolInfo, NULL, &m_commandPools[thread]);

        // Allocate command buffer per thread out of command pool per thread
        cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocInfo.pNext = NULL;
        cmdBufAllocInfo.commandBufferCount = 1;
        cmdBufAllocInfo.commandPool = m_commandPools[thread];
        cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        vkAllocateCommandBuffers(m_vulkanDevice, &cmdBufAllocInfo, &m_commandBuffers[thread]);

        // Set buffer info and create vertex buffer per thread
        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.pNext = NULL;
        bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufInfo.size = sizeof(g_vb_texture_Data);
        bufInfo.queueFamilyIndexCount = 0;
        bufInfo.pQueueFamilyIndices = NULL;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufInfo.flags = 0;
        result = vkCreateBuffer(m_vulkanDevice, &bufInfo, NULL, &m_vertexBuffers[thread].buffer);
        assert(result == VK_SUCCESS);

        // Get the memory requirements and fill out allocation info
        vkGetBufferMemoryRequirements(m_vulkanDevice, m_vertexBuffers[thread].buffer, &memoryRequirements);

        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = NULL;
        allocInfo.memoryTypeIndex = 0;
        allocInfo.allocationSize = memoryRequirements.size;

        // Search memory types to find first index with those properties
		bool pass = VulkanCommon::GetMemoryType(memoryRequirements.memoryTypeBits, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), allocInfo.memoryTypeIndex);
		assert(pass);

        // Allocate vertex buffer memory per thread
        result = vkAllocateMemory(m_vulkanDevice, &allocInfo, NULL, &m_vertexBuffers[thread].memory);
        assert(result == VK_SUCCESS);
        m_vertexBuffers[thread].bufferInfo.range = memoryRequirements.size;
        m_vertexBuffers[thread].bufferInfo.offset = 0;

        // Map memory and copy over vertex data
        uint8_t *pData;
        result = vkMapMemory(m_vulkanDevice, m_vertexBuffers[thread].memory, 0, memoryRequirements.size, 0, (void **)&pData);
        assert(result == VK_SUCCESS);

        memcpy(pData, g_vb_texture_Data, sizeof(g_vb_texture_Data));

        vkUnmapMemory(m_vulkanDevice, m_vertexBuffers[thread].memory);

        result = vkBindBufferMemory(m_vulkanDevice, m_vertexBuffers[thread].buffer, m_vertexBuffers[thread].memory, 0);
        assert(result == VK_SUCCESS);

        // Create thread work that we can start
        m_callbackData[thread] = new CallbackData(this, thread, 0);
        m_works[thread] = CreateThreadpoolWork(WorkCallback, m_callbackData[thread], &m_callbackEnv);
    }
}

void VulkanInstance::AddModel(Model &model)
{
	std::vector<float> modifiedVertices;
	for (unsigned int i = 0; i < model.fileVertices.size(); i+=3)
	{
		modifiedVertices.push_back(model.fileVertices[i]);
		modifiedVertices.push_back(model.fileVertices[i + 1]);
		modifiedVertices.push_back(model.fileVertices[i + 2]);
		modifiedVertices.push_back(1.0f);
		modifiedVertices.push_back(0);
		modifiedVertices.push_back(0);
	}
	model.fileVertices = modifiedVertices;

	VkBufferCreateInfo bufInfo = {};
	VkMemoryRequirements memoryRequirements = {};
	VkMemoryAllocateInfo allocInfo = {};
	VkResult result = {};
	VertexBuffer buffer;

	// VERTEX ---------------------------------------------------------
	// Set buffer info and create vertex buffer per thread
	bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.pNext = NULL;
	bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufInfo.size = sizeof(model.fileVertices[0]) * model.fileVertices.size();
	bufInfo.queueFamilyIndexCount = 0;
	bufInfo.pQueueFamilyIndices = NULL;
	bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufInfo.flags = 0;
	result = vkCreateBuffer(m_vulkanDevice, &bufInfo, NULL, &buffer.buffer);
	assert(result == VK_SUCCESS);

	// Get the memory requirements and fill out allocation info
	vkGetBufferMemoryRequirements(m_vulkanDevice, buffer.buffer, &memoryRequirements);

	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = NULL;
	allocInfo.memoryTypeIndex = 0;
	allocInfo.allocationSize = memoryRequirements.size;

	// Search memory types to find first index with those properties
	bool pass = VulkanCommon::GetMemoryType(memoryRequirements.memoryTypeBits, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), allocInfo.memoryTypeIndex);
	assert(pass);

	// Allocate vertex buffer memory per thread
	result = vkAllocateMemory(m_vulkanDevice, &allocInfo, NULL, &buffer.memory);
	assert(result == VK_SUCCESS);
	buffer.bufferInfo.range = memoryRequirements.size;
	buffer.bufferInfo.offset = 0;

	// Map memory and copy over vertex data
	uint8_t *pData;
	result = vkMapMemory(m_vulkanDevice, buffer.memory, 0, memoryRequirements.size, 0, (void **)&pData);
	assert(result == VK_SUCCESS);

	memcpy(pData, model.fileVertices.data(), bufInfo.size);

	vkUnmapMemory(m_vulkanDevice, buffer.memory);

	result = vkBindBufferMemory(m_vulkanDevice, buffer.buffer, buffer.memory, 0);
	assert(result == VK_SUCCESS);

	// INDEX--------------------------------------------------------
	bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.pNext = NULL;
	bufInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufInfo.size = sizeof(model.fileIndices[0]) * model.fileIndices.size();
	bufInfo.queueFamilyIndexCount = 0;
	bufInfo.pQueueFamilyIndices = NULL;
	bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufInfo.flags = 0;
	result = vkCreateBuffer(m_vulkanDevice, &bufInfo, NULL, &buffer.indices);
	assert(result == VK_SUCCESS);

	// Get the memory requirements and fill out allocation info
	vkGetBufferMemoryRequirements(m_vulkanDevice, buffer.indices, &memoryRequirements);

	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = NULL;
	allocInfo.memoryTypeIndex = 0;
	allocInfo.allocationSize = memoryRequirements.size;

	// Search memory types to find first index with those properties
	pass = VulkanCommon::GetMemoryType(memoryRequirements.memoryTypeBits, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), allocInfo.memoryTypeIndex);
	assert(pass);

	// Allocate vertex buffer memory per thread
	result = vkAllocateMemory(m_vulkanDevice, &allocInfo, NULL, &buffer.indexMemory);
	assert(result == VK_SUCCESS);
	buffer.indexInfo.range = memoryRequirements.size;
	buffer.indexInfo.offset = 0;

	// Map memory and copy over vertex data
	uint8_t *indexData;
	result = vkMapMemory(m_vulkanDevice, buffer.indexMemory, 0, memoryRequirements.size, 0, (void **)&indexData);
	assert(result == VK_SUCCESS);

	memcpy(indexData, model.fileIndices.data(), bufInfo.size);

	vkUnmapMemory(m_vulkanDevice, buffer.indexMemory);

	result = vkBindBufferMemory(m_vulkanDevice, buffer.indices, buffer.indexMemory, 0);
	assert(result == VK_SUCCESS);

	buffer.numIndices = model.fileIndices.size();
	buffer.numVertices = model.fileVertices.size();

	// Add to models list for rendering
	models.push_back(buffer);
}

void VulkanInstance::AddLineBuffer(const std::vector<Vec4> &points)
{
	// Buffer info to populate
	VertexBuffer buffer;

	// Result for VKResults
	VkResult result;

	// VERTEX ---------------------------------------------------------
	// Set buffer info and create vertex buffer per thread
	VkBufferCreateInfo bufInfo = {};
	bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.pNext = NULL;
	bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufInfo.size = sizeof(Vec4) * points.size();
	bufInfo.queueFamilyIndexCount = 0;
	bufInfo.pQueueFamilyIndices = NULL;
	bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufInfo.flags = 0;
	result = vkCreateBuffer(m_vulkanDevice, &bufInfo, NULL, &buffer.buffer);
	assert(result == VK_SUCCESS);

	// Get the memory requirements and fill out allocation info
	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(m_vulkanDevice, buffer.buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = NULL;
	allocInfo.memoryTypeIndex = 0;
	allocInfo.allocationSize = memoryRequirements.size;

	// Search memory types to find first index with those properties
	bool pass = VulkanCommon::GetMemoryType(memoryRequirements.memoryTypeBits, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), allocInfo.memoryTypeIndex);
	assert(pass);

	// Allocate vertex buffer memory per thread
	result = vkAllocateMemory(m_vulkanDevice, &allocInfo, NULL, &buffer.memory);
	assert(result == VK_SUCCESS);
	buffer.bufferInfo.range = memoryRequirements.size;
	buffer.bufferInfo.offset = 0;

	// Map memory and copy over vertex data
	uint8_t *pData;
	result = vkMapMemory(m_vulkanDevice, buffer.memory, 0, memoryRequirements.size, 0, (void **)&pData);
	assert(result == VK_SUCCESS);

	memcpy(pData, points.data(), bufInfo.size);

	vkUnmapMemory(m_vulkanDevice, buffer.memory);

	result = vkBindBufferMemory(m_vulkanDevice, buffer.buffer, buffer.memory, 0);
	assert(result == VK_SUCCESS);

	buffer.numVertices = points.size();

	// Add to lines list for rendering
	lines.push_back(buffer);
}