#pragma once

// Important, need this before including vulkan.h
#ifdef _WIN32
//#pragma comment(linker, "/subsystem:console")
#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX // Don't let Windows define min() or max()
#else
#define VK_USE_PLATFORM_XCB_KHR
#include <unistd.h>
#endif

// Force GLM to configure for Vulkan
#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "vulkan.h"
#include "SPIRV/GlslangToSpv.h"
#include <vector>
#include <mutex>
#include "Model.h"
#include "Texture.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Camera.h"

/* Number of descriptor sets needs to be the same at alloc, */
/* pipeline layout creation, and descriptor set layout creation */
#define NUM_DESCRIPTOR_SETS 1

// Fence timeout constant for Vulkan fences
#define FENCE_TIMEOUT 100000000

// Forward declaration for multi-threading callback data structure
struct CallbackData;

// Vulkan renderer
class VulkanInstance
{
public:
    // Initialize Vulkan
    void Initialize(HWND hwnd, HINSTANCE inst, int width, int height, bool multithreaded);

    // Draw a cube single-threaded
    void DrawCube(float dt);                                            // Vulkan tutorial step 15

    // Multi-threaded submit for drawing multiple cubes
    void DrawCubesMultithreaded(float dt);

    // Code to run per thread for building secondary command buffers
    void PerThreadCode(int threadId, float dt);

    // Release memory
    void Destroy();

	// Model and line drawing for .obj files and debug drawing
	void AddModel(Model &model);
	void AddLineBuffer(const std::vector<Vec4> &points);

private:
    // Init and creation functions
    void InitInstance();                                                // Vulkan tutorial step 1
    void EnumerateDevices();                                            // Vulkan tutorial step 2
    void CreateDevice();                                                // Vulkan tutorial step 3
    void InitCommandBuffer();                                           // Vulkan tutorial step 4
    void InitSurface(HWND hwnd, HINSTANCE inst);                        // Vulkan tutorial step 5
    void InitSwapChain();                                               // Vulkan tutorial step 5 (continued)
    void CreateDepthBuffer();                                           // Vulkan tutorial step 6
    void CreateUniformBuffer(int threadNum);                            // Vulkan tutorial step 7
    void CreateDescriptorLayouts();                                     // Vulkan tutorial step 8
    void AllocateDescriptorSets();                                      // Vulkan tutorial step 9
    void InitRenderPass();                                              // Vulkan tutorial step 10
    void InitShaders();                                                 // Vulkan tutorial step 11
    void InitFrameBuffers();                                            // Vulkan tutorial step 12
    void InitVertexBuffer();                                            // Vulkan tutorial step 13
    void InitPipeline();                                                // Vulkan tutorial step 14

    // Uniform buffer update inside draw
    void UpdateUniformBuffer(int threadNum, float dt);

	void UpdateUniformBufferForDebugCamera(int threadNum, float dt);

    // Init buffers for multithreaded
    void InitMultithreaded();

	// Get memory types for Vulkan
	bool GetMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlagBits memoryPropretyBit, uint32_t &memoryTypeIndex);

	// Texture and camera data
	Texture texture;
	Camera camera[2];

    // Persistent members required for rendering
    VkInstance m_vulkanInstance;
    VkSurfaceKHR m_vulkanSurface;
    VkDevice m_vulkanDevice;
    std::vector<VkPhysicalDevice> m_vulkanDeviceVector;
    VkSwapchainKHR m_vulkanSwapChain;
    VkQueue m_vulkanQueue;
    VkCommandPool m_vulkanCommandPool;
    VkCommandBuffer m_vulkanCommandBuffer;
    VkRenderPass m_vulkanRenderPass;
    VkDescriptorImageInfo m_vulkanImageInfo;
    VkPipeline m_vulkanPipeline[2];
    VkPipelineCache m_vulkanPipelineCache;
    VkPipelineLayout m_vulkanPipelineLayout;
    VkPipelineShaderStageCreateInfo m_vulkanPipelineShaderStageInfo[2];

    // Property checks		
    VkPhysicalDeviceMemoryProperties m_vulkanDeviceMemoryProperties;
    std::vector<VkQueueFamilyProperties> m_vulkanQueueFamilyPropertiesVector;

    // Index into graphics queue and buffer in swap chain
    uint32_t m_graphicsQueueFamilyIndex;
    uint32_t m_currentBuffer;

    // Queue count and swap chain image count
    uint32_t m_queueCount;
    uint32_t m_swapChainImageCount;

    std::vector<VkDescriptorSetLayout> m_vulkanDescriptorSetLayoutVector;

    // Format for swap chain and back back buffer
    VkFormat m_vulkanFormat;
    VkFramebuffer *m_vulkanFrameBuffers;

    // Window dimensions
    int m_windowWidth;
    int m_windowHeight;

    // Structure for swap chain buffer
    struct VulkanSwapChainBuffer
    {
        VkImage image;
        VkImageView view;
    };
    std::vector<VulkanSwapChainBuffer> m_swapChainBuffers;

    // Structure for depth buffer
    struct
    {
        VkFormat format;
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
    } m_depthBuffer;

    // Structure for uniform buffer
    struct VulkanUniformBuffer
    {
        VkBuffer buffer;
        VkDeviceMemory memory;
        VkDescriptorBufferInfo bufferInfo;
    } m_uniformBuffers[3];

    // Structure for vertex buffer
    struct VertexBuffer
    {
        VkBuffer buffer;
		VkBuffer indices;
        VkDeviceMemory memory;
		VkDeviceMemory indexMemory;
        VkDescriptorBufferInfo bufferInfo;
		VkDescriptorBufferInfo indexInfo;
		int numVertices;
		int numIndices;
    };

    // Structure for layer properties
    struct
    {
        VkLayerProperties properties;
        std::vector<VkExtensionProperties> extensions;
    } m_layerProperties;

    // Threading support
    // Need to allocate buffers per thread or there
    // is a race for access  to common memory
    // Causes flashing as a result of race condition
	// Could use locks, but this is lockless! :D
    VkCommandBuffer m_commandBuffers[3];
    VkCommandPool m_commandPools[3];
    HANDLE m_threadHandles[3];
    VertexBuffer m_vertexBuffers[3];
    glm::mat4 m_modelMatrices[3];
    VkDescriptorPool m_descriptorPools[3];
    std::vector<VkDescriptorSet> m_descriptorSets[3];

    // Multi-threaded 
    PTP_POOL m_renderThreadPool;
    PTP_WAIT m_threadWaits[3];
    PTP_CLEANUP_GROUP m_cleanupGroup;
    TP_CALLBACK_ENVIRON m_callbackEnv;
    PTP_WORK m_works[3];
    CallbackData *m_callbackData[3];

	// Loading models
	std::vector<VertexBuffer> models;
	std::vector<VertexBuffer> lines;
};

// Struct for callback data used in multi-threading
// TODO: Make sure this is allocated and cleaned up properly
struct CallbackData
{
    // Instance and thread
    // Use instance to call function
    // Use thread to pass parameter
    VulkanInstance *m_instance;
    int m_thread;
	float m_dt;

    // Constructor used to allocate memory
    CallbackData(VulkanInstance *instance, int thread, float dt)
    {
        m_instance = instance;
        m_thread = thread;
		m_dt = dt;
    }
};