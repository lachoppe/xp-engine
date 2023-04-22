#pragma once

#include <vector>
#include <deque>
#include <functional>
#include <glm/glm.hpp>

#include "vk_types.h"
#include "vk_mesh.h"
#include "vk_config.h"
#include "cvars.h"


static AutoCVar_Float cvar_lookSensitivity("i.lookSensitivity", "How sensitive the view rotation is to input", 5.0, 0.1, 10.0, CVarFlags::EditFloatDrag);
static AutoCVar_Int cvar_dvorak("i.dvorak", "Use Dvorak default keybindings (instead of WASD)", 0, 0, 1, static_cast<CVarFlags>(static_cast<uint32_t>(CVarFlags::EditCheckbox) | static_cast<uint32_t>(CVarFlags::Advanced)));
static AutoCVar_Int cvar_gpuDriven("r.gpuDriven", "Use GPU-driven rendering pipeline", 0, 0, 1, CVarFlags::EditCheckbox);

static AutoCVar_Int cvar_syncMode_0("r.syncMode_0", "No sync (IMMEDIATE)", VK_PRESENT_MODE_IMMEDIATE_KHR, CVarFlags::NoEdit);
static AutoCVar_Int cvar_syncMode_1("r.syncMode_1", "V-sync - no wait (MAILBOX)", VK_PRESENT_MODE_MAILBOX_KHR, CVarFlags::NoEdit);
static AutoCVar_Int cvar_syncMode_2("r.syncMode_2", "V-sync (FIFO)", VK_PRESENT_MODE_FIFO_KHR, CVarFlags::NoEdit);
static AutoCVar_Int cvar_syncMode("r.syncMode", "Which mode to use for syncing the frame render to display refresh", 1, 0, 2, CVarFlags::EditCombo);


struct MeshPushConstants
{
	glm::vec4 data;
	glm::mat4 renderMatrix;
};


struct Material
{
	VkDescriptorSet textureSet { VK_NULL_HANDLE };
	VkPipeline pipeline { VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout { VK_NULL_HANDLE };
};


struct Texture
{
	AllocatedImage image {0};
	VkImageView imageView { VK_NULL_HANDLE };
};


struct RenderObject
{
	Mesh* mesh { nullptr };
	Material* material { nullptr };
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


struct PaddedSizes
{
	std::vector<size_t> padded;
	size_t sum{ 0 };

	void Reset();
	size_t Add(const class VulkanEngine* engine, size_t bytes);
	size_t GetSum() const;
	size_t GetSizeOf(int i) const;
};


struct GPUCameraData
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewProj;
};


struct GPUSceneData
{
	glm::vec4 fogColor;			// w: exponent
	glm::vec4 fogDistances;		// x: min, y: max, zw: unused
	glm::vec4 ambientColor;
	glm::vec4 sunlightDir;		// w: intensity
	glm::vec4 sunlightColor;
};


struct GPUObjectData
{
	glm::mat4 model;
};


struct UploadContext
{
	VkFence uploadFence;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
};


struct FrameData
{
	VkSemaphore presentSemaphore { nullptr };
	VkSemaphore renderSemaphore { nullptr };
	VkFence renderFence { nullptr };

	AllocatedBuffer objectBuffer { nullptr, nullptr };
	VkDescriptorSet objectDescriptor;

	VkCommandPool commandPool { nullptr };
	VkCommandBuffer mainCommandBuffer { nullptr };
};

constexpr unsigned int FRAME_OVERLAP = 2;


struct Toast
{
	std::string message;
	uint64_t eraseTime{ 0 };
};

constexpr int DEFAULT_TOAST_DURATION_MS = 3000;


class VulkanEngine
{
public:

	bool isInitialized { false };
	VkPhysicalDeviceProperties gpuProperties;
	int frameNumber { 0 };
	uint64_t lastFrameTimeMS { 0 };
	int lastSecFrameNumber { 0 };
	float lastFPS { 0.0f };
	int selectedShader { 0 };

	VmaAllocator allocator;
	DeletionQueue mainDeletionQueue;

	VkExtent2D windowExtent { 1700, 900 };
	struct SDL_Window* window { nullptr };
	float fieldOfView { 70.0f };
	glm::vec3 camPos { 0.0f, 0.0f, 0.0f };
	glm::vec3 camVel { 0.0f, 0.0f, 0.0f };
	float camPitch { 0.0f };
	float camYaw { 0.0f };

	VkInstance instance { nullptr };
	VkDebugUtilsMessengerEXT debugMessenger { nullptr };
	VkPhysicalDevice chosenGPU { nullptr };
	VkDevice device { nullptr };
	VkSurfaceKHR surface { nullptr };
	VkSurfaceCapabilitiesKHR surfaceCaps;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;

	UploadContext uploadContext;

	VkSwapchainKHR swapchain { nullptr };
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	bool needSwapchainRecreate { false };

	VkFormat depthFormat;
	AllocatedImage depthImage;
	VkImageView depthImageView;

	std::unordered_map<std::string, Texture> loadedTextures;

	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;

	VkRenderPass renderPass { nullptr };
	std::vector<VkFramebuffer> frameBuffers;

	VkDescriptorSetLayout globalSetLayout;
	VkDescriptorSetLayout objectSetLayout;
	VkDescriptorSetLayout singleTextureSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet globalDescriptor;

	AllocatedBuffer cameraSceneDataBuffer;
	PaddedSizes cameraSceneFrameSize;

	FrameData frames[FRAME_OVERLAP];

	Config config;

	bool showOptions = false;
	bool showingOptions = false;

	const unsigned char* keyboardState { nullptr };
	int keyboardStateLen;

	std::vector<Toast> toasts;

	// scene
	std::vector<RenderObject> renderables;
	std::unordered_map<std::string, Material> materials;
	std::unordered_map<std::string, Mesh> meshes;

	void AddToast(const char* message, int durationMS = DEFAULT_TOAST_DURATION_MS);

	size_t PadUniformBufferSize(size_t originalSize) const;

	AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	Material* CreateMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);

	Material* GetMaterial(const std::string& name);
	Mesh* GetMesh(const std::string& name);
	FrameData& GetCurrentFrame();

	void UpdateCamera(int deltaX, int deltaY);
	void DrawObjects(VkCommandBuffer cmd, RenderObject* first, int count);

	void DrawGUI();

	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	void Init();
	void Run();
	void Draw();
	void Cleanup();

private:

	bool LoadShaderModule(const char* filePath, VkShaderModule* outShaderModule);
	void LoadMeshes();
	void LoadImages();

	void InitVulkan();
	void InitSwapchain();
	void InitCommands();
	void InitDefaultRenderPass();
	void InitFramebuffers();
	void InitSyncStructures();
	void InitDescriptors();
	void InitPipelines();
	void InitScene();
	void InitImGui();

	void CleanupFramebuffers();
	void CleanupSwapchain();
	void RecreateSwapchain();

	void UploadMesh(Mesh& mesh);
};


class PipelineBuilder
{
public:

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	VkPipelineVertexInputStateCreateInfo vertexInput {};
	VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
	VkViewport viewport {};
	VkRect2D scissor {};
	VkPipelineRasterizationStateCreateInfo rasterizer {};
	VkPipelineColorBlendAttachmentState colorBlendAttachment {};
	VkPipelineMultisampleStateCreateInfo multisampling {};
	VkPipelineDepthStencilStateCreateInfo depthStencil {};
	VkPipelineLayout pipelineLayout {};

	VkPipeline BuildPipeline(VkDevice device, VkRenderPass pass);
};