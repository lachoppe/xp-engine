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

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice chosenGPU;
	VkDevice device;
	VkSurfaceKHR surface;

	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	void Init();

	void Cleanup();

	void Draw();

	void Run();

private:

	void InitVulkan();
	void InitSwapchain();
};