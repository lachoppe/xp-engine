#pragma once

#include <vector>

#include "vk_types.h"

class VulkanEngine
{
public:

	bool isInitialized{ false };
	int frameNumber{ 0 };

	VkExtent2D windowExtent{ 1700, 900 };

	struct SDL_Window* window{ nullptr };

	VkInstance instance{ nullptr };
	VkDebugUtilsMessengerEXT debugMessenger{ nullptr };
	VkPhysicalDevice chosenGPU{ nullptr };
	VkDevice device{ nullptr };
	VkSurfaceKHR surface{ nullptr };

	VkSwapchainKHR swapchain{ nullptr };
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;

	VkCommandPool commandPool{ nullptr };
	VkCommandBuffer mainCommandBuffer{ nullptr };

	VkRenderPass renderPass{ nullptr };
	std::vector<VkFramebuffer> frameBuffers;

	VkSemaphore presentSemaphore{ nullptr };
	VkSemaphore renderSemaphore{ nullptr };
	VkFence renderFence{ nullptr };

	void Init();

	void Cleanup();

	void Draw();

	void Run();

private:

	bool LoadShaderModule(const char* filePath, VkShaderModule* outShaderModule);

	void InitVulkan();
	void InitSwapchain();
	void InitCommands();
	void InitDefaultRenderPass();
	void InitFramebuffers();
	void InitSyncStructures();
	void InitPipelines();
};