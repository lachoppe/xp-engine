#pragma once

#include <vector>
#include <deque>
#include <functional>
#include <glm/glm.hpp>

#include "vk_types.h"
#include "vk_mesh.h"


void OutputMessage(const char* format, ...);
void OutputMessage(const wchar_t* format, ...);


struct MeshPushConstants
{
	glm::vec4 data;
	glm::mat4 renderMatrix;
};


struct Material
{
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};


struct RenderObject
{
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;

	bool operator < (const RenderObject& other) const
	{
		if (material->pipeline != other.material->pipeline)
			return (material->pipeline < other.material->pipeline);
		return mesh < other.mesh;
	}
};


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

	VmaAllocator allocator;
	DeletionQueue mainDeletionQueue;

	VkExtent2D windowExtent{ 1700, 900 };
	struct SDL_Window* window{ nullptr };
	float fieldOfView{ 70.0f };
	glm::vec3 camPos{ 0.0f, 0.0f, 0.0f };
	glm::vec3 camVel{ 0.0f, 0.0f, 0.0f };

	VkInstance instance{ nullptr };
	VkDebugUtilsMessengerEXT debugMessenger{ nullptr };
	VkPhysicalDevice chosenGPU{ nullptr };
	VkDevice device{ nullptr };
	VkSurfaceKHR surface{ nullptr };

	VkSwapchainKHR swapchain{ nullptr };
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkFormat depthFormat;
	AllocatedImage depthImage;
	VkImageView depthImageView;

	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;

	VkCommandPool commandPool{ nullptr };
	VkCommandBuffer mainCommandBuffer{ nullptr };

	VkRenderPass renderPass{ nullptr };
	std::vector<VkFramebuffer> frameBuffers;

	VkSemaphore presentSemaphore{ nullptr };
	VkSemaphore renderSemaphore{ nullptr };
	VkFence renderFence{ nullptr };

	bool useDvorak = true;
	const unsigned char* keyboardState{ nullptr };
	int keyboardStateLen;

	// scene
	std::vector<RenderObject> renderables;
	std::unordered_map<std::string, Material> materials;
	std::unordered_map<std::string, Mesh> meshes;

	Material* CreateMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
	Material* GetMaterial(const std::string& name);
	Mesh* GetMesh(const std::string& name);

	void UpdateCamera();
	void DrawObjects(VkCommandBuffer cmd, RenderObject* first, int count);

	void Init();
	void Run();
	void Draw();
	void Cleanup();

private:

	bool LoadShaderModule(const char* filePath, VkShaderModule* outShaderModule);
	void LoadMeshes();

	void InitVulkan();
	void InitSwapchain();
	void InitCommands();
	void InitDefaultRenderPass();
	void InitFramebuffers();
	void InitSyncStructures();
	void InitPipelines();
	void InitScene();

	void UploadMesh(Mesh& mesh);
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
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPipelineLayout pipelineLayout;

	VkPipeline BuildPipeline(VkDevice device, VkRenderPass pass);
};