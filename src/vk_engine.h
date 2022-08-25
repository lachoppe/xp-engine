#pragma once

#include <vector>
#include <deque>
#include <functional>

#include "vk_types.h"


void OutputMessage(const char* format, ...);
void OutputMessage(const wchar_t* format, ...);


struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void PushFunction(std::function<void()>&& function)
	{
		deletors.push_back(function);
	}

	void Flush()
	{
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
		{
			(*it)();
		}

		deletors.clear();
	}
};

class VulkanEngine
{
public:

	bool isInitialized{ false };
	int frameNumber{ 0 };
	int selectedShader{ 0 };

	VkExtent2D windowExtent{ 1700, 900 };

	DeletionQueue mainDeletionQueue;

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

	VkPipeline trianglePipeline;
	VkPipeline redTrianglePipeline;

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


class PipelineBuilder
{
public:

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	VkPipelineVertexInputStateCreateInfo vertexInput;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineLayout pipelineLayout;

	VkPipeline BuildPipeline(VkDevice device, VkRenderPass pass);
};