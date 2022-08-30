#include "vk_engine.h"

#include <windows.h>
#include <debugapi.h>
#include <stdio.h>
#include <fstream>

#include "vk_types.h"
#include "vk_initializers.h"

#include "VKBootstrap.h"

#include "SDL.h"
#include "SDL_vulkan.h"
#include "SDL_keyboard.h"

#include "glm/gtx/transform.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#ifdef UNICODE
#define VK_CHECK(x)																		\
	do																					\
	{																					\
		VkResult err = x;																\
		if (err)																		\
		{																				\
			const int msgLen = 64;														\
			wchar_t str[msgLen];														\
			swprintf(str, msgLen, L"[%d] Detected Vulkan error: %d\n", __LINE__, err);	\
			OutputDebugString(str);														\
			abort();																	\
		}																				\
	} while (0);
#else
#define VK_CHECK(x)																		\
	do																					\
	{																					\
		VkResult err = x;																\
		if (err)																		\
		{																				\
			const int msgLen = 64;														\
			char str[msgLen];															\
			sprintf_s(str, msgLen, "[%d] Detected Vulkan error: %d\n", __LINE__, err);	\
			OutputDebugString(str);														\
			abort();																	\
		}																				\
	} while (0);
#endif


void OutputMessage(const char* format, ...)
{
	const int bufLen{ 512 };
	char buffer[bufLen];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, bufLen, format, args);

	OutputDebugStringA(buffer);

	va_end(args);
}

void OutputMessage(const wchar_t* format, ...)
{
	const int bufLen{ 512 };
	wchar_t buffer[bufLen];
	va_list args;
	va_start(args, format);
	vswprintf(buffer, bufLen, format, args);

	OutputDebugStringW(buffer);

	va_end(args);
}


Material* VulkanEngine::CreateMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name)
{
	Material mat;
	mat.pipeline = pipeline;
	mat.pipelineLayout = layout;
	materials[name] = mat;
	return &materials[name];
}


Material* VulkanEngine::GetMaterial(const std::string& name)
{
	auto it = materials.find(name);
	if (it == materials.end())
		return nullptr;
	else
		return &(*it).second;
}


Mesh* VulkanEngine::GetMesh(const std::string& name)
{
	auto it = meshes.find(name);
	if (it == meshes.end())
		return nullptr;
	else
		return &(*it).second;
}


void VulkanEngine::DrawObjects(VkCommandBuffer cmd, RenderObject* first, int count)
{
	const glm::mat4 view = glm::translate(glm::mat4(1.0f), camPos);
	glm::mat4 projection = glm::perspective(glm::radians(fieldOfView), static_cast<float>(windowExtent.width) / static_cast<float>(windowExtent.height), 0.1f, 200.0f);
	projection[1][1] *= -1;

	Mesh* lastMesh = nullptr;
	Material* lastMaterial = nullptr;
	for (int i = 0; i < count; i++)
	{
		const RenderObject& object = first[i];

		if (object.material != lastMaterial)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
			lastMaterial = object.material;
		}

		if (object.material == nullptr)
			continue;

		const glm::mat4 model = object.transformMatrix;
		glm::mat4 meshMatrix = projection * view * model;

		MeshPushConstants constants;
		constants.renderMatrix = meshMatrix;

		vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

		if (object.mesh != lastMesh)
		{
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertexBuffer.buffer, &offset);
			lastMesh = object.mesh;
		}

		if (object.mesh == nullptr)
			continue;

		vkCmdDraw(cmd, static_cast<int>(object.mesh->vertices.size()), 1, 0, 0);
	}
}



void VulkanEngine::Init()
{
	SDL_Init(SDL_INIT_VIDEO);
	keyboardState = SDL_GetKeyboardState(&keyboardStateLen);

	SDL_WindowFlags window_flags = SDL_WindowFlags::SDL_WINDOW_VULKAN;
	window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		windowExtent.width,
		windowExtent.height,
		window_flags
	);

	InitVulkan();
	InitSwapchain();
	InitCommands();
	InitDefaultRenderPass();
	InitFramebuffers();
	InitSyncStructures();
	InitPipelines();

	LoadMeshes();

	InitScene();

	isInitialized = true;
}



const char* to_string_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT s) {
	switch (s) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		return "VERBOSE";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		return "ERROR";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		return "WARNING";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		return "INFO";
	default:
		return "UNKNOWN";
	}
}

const char* to_string_message_type(VkDebugUtilsMessageTypeFlagsEXT s) {
	if (s == 7) return "General | Validation | Performance";
	if (s == 6) return "Validation | Performance";
	if (s == 5) return "General | Performance";
	if (s == 4 /*VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT*/) return "Performance";
	if (s == 3) return "General | Validation";
	if (s == 2 /*VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT*/) return "Validation";
	if (s == 1 /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT*/) return "General";
	return "Unknown";
}

VKAPI_ATTR VkBool32 VKAPI_CALL custom_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*)
{
	auto ms = to_string_message_severity(messageSeverity);
	auto mt = to_string_message_type(messageType);

	OutputMessage("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

	return VK_FALSE; // Applications must return false here
}


bool VulkanEngine::LoadShaderModule(const char* filename, VkShaderModule* outShaderModule)
{
	const char* root = "../shaders/";
	const char* ext = ".spv";
	const int pathLen = 256;
	char filePath[pathLen];
	sprintf_s(filePath, pathLen, "%s%s%s", root, filename, ext);

	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		OutputMessage("Error when building shader module: %s\n", filename);
		return false;
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return false;
	}
	*outShaderModule = shaderModule;

	OutputMessage("Shader successfully loaded: %s\n", filename);
	return true;
}


void VulkanEngine::LoadMeshes()
{
	Mesh triangleMesh;
	triangleMesh.vertices.resize(3);
	triangleMesh.vertices[0].position = {  1.0f,  1.0f,  0.0f };
	triangleMesh.vertices[1].position = { -1.0f,  1.0f,  0.0f };
	triangleMesh.vertices[2].position = {  0.0f, -1.0f,  0.0f };
	triangleMesh.vertices[0].color = {  0.0f,  1.0f,  0.0f };
	triangleMesh.vertices[1].color = {  0.0f,  1.0f,  0.0f };
	triangleMesh.vertices[2].color = {  0.0f,  1.0f,  0.0f };
	UploadMesh(triangleMesh);
	meshes["triangle"] = triangleMesh;

	std::unordered_map<std::string, std::string> meshesToLoad;

	// List of meshes to load
	meshesToLoad["monkey"]			= "../assets/monkey_smooth.obj";
	meshesToLoad["apple"]			= "../assets/apple.obj";
// 	meshesToLoad["rabbit_low"]		= "../assets/rabbit_low.obj";
// 	meshesToLoad["rabbit_med"]		= "../assets/rabbit_med.obj";
	meshesToLoad["rabbit_high"]		= "../assets/rabbit_high.obj";

	for (auto it = meshesToLoad.begin(); it != meshesToLoad.end(); ++it)
	{
		Mesh mesh;
		if (mesh.LoadFromObj((*it).second.c_str()))
		{
			UploadMesh(mesh);
			meshes[(*it).first.c_str()] = mesh;
		}
	}
}


void VulkanEngine::UploadMesh(Mesh& mesh)
{
	const size_t bufferSizeBytes = mesh.vertices.size() * sizeof(Vertex);

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	bufferInfo.size = bufferSizeBytes;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &mesh.vertexBuffer.buffer, &mesh.vertexBuffer.allocation, nullptr));

	mainDeletionQueue.PushFunction([=]()
		{
			vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
		});

	void* data;
	vmaMapMemory(allocator, mesh.vertexBuffer.allocation, &data);
	memcpy(data, mesh.vertices.data(), bufferSizeBytes);
	vmaUnmapMemory(allocator, mesh.vertexBuffer.allocation);
}


void VulkanEngine::InitVulkan()
{
	vkb::InstanceBuilder builder;

	vkb::detail::Result<vkb::Instance> instRet = builder.set_app_name("Vulkan Test Application")
		.request_validation_layers(true)
		.require_api_version(1, 1, 0)
		.set_debug_callback(custom_debug_callback)
		.build();

	vkb::Instance vkbInst = instRet.value();

	instance = vkbInst.instance;
	debugMessenger = vkbInst.debug_messenger;

	SDL_Vulkan_CreateSurface(window, instance, &surface);

	vkb::PhysicalDeviceSelector selector{ vkbInst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 1)
		.set_surface(surface)
		.select()
		.value();

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	vkb::Device vkbDevice = deviceBuilder.build().value();

	device = vkbDevice.device;
	chosenGPU = physicalDevice.physical_device;

	graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = chosenGPU;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;
	vmaCreateAllocator(&allocatorInfo, &allocator);
}


void VulkanEngine::InitSwapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{ chosenGPU, device, surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(windowExtent.width, windowExtent.height)
		.build()
		.value();

	swapchain = vkbSwapchain.swapchain;
	swapchainImages = vkbSwapchain.get_images().value();
	swapchainImageViews = vkbSwapchain.get_image_views().value();
	swapchainImageFormat = vkbSwapchain.image_format;

	mainDeletionQueue.PushFunction([=]()
		{
			vkDestroySwapchainKHR(device, swapchain, nullptr);
		});

	VkExtent3D depthImageExtent = {
		windowExtent.width,
		windowExtent.height,
		1
	};
	depthFormat = VK_FORMAT_D32_SFLOAT;

	VkImageCreateInfo dImgInfo = vkinit::ImageCreateInfo(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
	VmaAllocationCreateInfo dImgAllocInfo = {};
	dImgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dImgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vmaCreateImage(allocator, &dImgInfo, &dImgAllocInfo, &depthImage.image, &depthImage.allocation, nullptr);

	VkImageViewCreateInfo dImgViewInfo = vkinit::ImageViewCreateInfo(depthFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);
	VK_CHECK(vkCreateImageView(device, &dImgViewInfo, nullptr, &depthImageView));

	mainDeletionQueue.PushFunction([=]()
		{
			vkDestroyImageView(device, depthImageView, nullptr);
			vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
		});
}


void VulkanEngine::InitCommands()
{
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::CommandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool));

	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::CommandBufferAllocateInfo(commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &mainCommandBuffer));

	mainDeletionQueue.PushFunction([=]()
		{
			vkDestroyCommandPool(device, commandPool, nullptr);
		});
}


void VulkanEngine::InitDefaultRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;			// optimal for presenting on-screen

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	// optimal for rendering into

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.flags = 0;
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency colorDependency = {};
	colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	colorDependency.dstSubpass = 0;
	colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	colorDependency.srcAccessMask = 0;
	colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depthDependency = {};
	depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthDependency.dstSubpass = 0;
	depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.srcAccessMask = 0;
	depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
	VkSubpassDependency dependencies[2] = { colorDependency, depthDependency };
	
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = &attachments[0];
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = &dependencies[0];
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

	mainDeletionQueue.PushFunction([=]()
		{
			vkDestroyRenderPass(device, renderPass, nullptr);
		});
}


void VulkanEngine::InitFramebuffers()
{
	VkFramebufferCreateInfo fbInfo = {};
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.renderPass = renderPass;
	fbInfo.attachmentCount = 1;
	fbInfo.width = windowExtent.width;
	fbInfo.height = windowExtent.height;
	fbInfo.layers = 1;

	const size_t swapchainImageCount = swapchainImages.size();
	frameBuffers = std::vector<VkFramebuffer>(swapchainImageCount);

	for (size_t i = 0; i < swapchainImageCount; ++i)
	{
		VkImageView attachments[2];
		attachments[0] = swapchainImageViews[i];
		attachments[1] = depthImageView;

		fbInfo.pAttachments = attachments;
		fbInfo.attachmentCount = 2;
		VK_CHECK(vkCreateFramebuffer(device, &fbInfo, nullptr, &frameBuffers[i]));

		mainDeletionQueue.PushFunction([=]()
			{
				vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
				vkDestroyImageView(device, swapchainImageViews[i], nullptr);
			});
	}
}


void VulkanEngine::InitSyncStructures()
{
	// Creating this fence with the signaled bit set, so we can wait on it before using it on a GPU
	VkFenceCreateInfo fenceCreateInfo = vkinit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

	VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFence));

	mainDeletionQueue.PushFunction([=]()
		{
			vkDestroyFence(device, renderFence, nullptr);
		});

	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::SemaphoreCreateInfo();

	VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentSemaphore));
	VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderSemaphore));

	mainDeletionQueue.PushFunction([=]()
		{
			vkDestroySemaphore(device, presentSemaphore, nullptr);
			vkDestroySemaphore(device, renderSemaphore, nullptr);
		});
}


void VulkanEngine::InitPipelines()
{
	// Shader load (match with cleanup)
	VkShaderModule coloredTriangleFragShader;
	LoadShaderModule("colored_triangle.frag", &coloredTriangleFragShader);

	VkShaderModule triangleVertShader;
	LoadShaderModule("colored_triangle.vert", &triangleVertShader);

	VkShaderModule redTriangleFragShader;
	LoadShaderModule("triangle.frag", &redTriangleFragShader);

	VkShaderModule redTriangleVertShader;
	LoadShaderModule("triangle.vert", &redTriangleVertShader);

	VkShaderModule meshVertShader;
	LoadShaderModule("tri_mesh.vert", &meshVertShader);

	VkShaderModule greyscaleTriangleFragShader;
	LoadShaderModule("greyscale_triangle.frag", &greyscaleTriangleFragShader);


	// Pipeline common
	PipelineBuilder pipelineBuilder;

	pipelineBuilder.vertexInput = vkinit::VertexInputStateCreateInfo();
	pipelineBuilder.inputAssembly = vkinit::InputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.viewport.x = 0.0f;
	pipelineBuilder.viewport.y = 0.0f;
	pipelineBuilder.viewport.width = static_cast<float>(windowExtent.width);
	pipelineBuilder.viewport.height = static_cast<float>(windowExtent.height);
	pipelineBuilder.viewport.minDepth = 0.0f;
	pipelineBuilder.viewport.maxDepth = 1.0f;
	pipelineBuilder.scissor.offset = { 0, 0 };
	pipelineBuilder.scissor.extent = windowExtent;
	pipelineBuilder.rasterizer = vkinit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
	pipelineBuilder.multisampling = vkinit::MultisampleStateCreateInfo();
	pipelineBuilder.colorBlendAttachment = vkinit::ColorBlendAttachmentState();
	pipelineBuilder.depthStencil = vkinit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);



	VkPipeline trianglePipeline;
	VkPipeline redTrianglePipeline;
	VkPipeline meshPipeline;
	VkPipeline greyMeshPipeline;
	VkPipelineLayout meshPipelineLayout;


	// Triangle layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::LayoutCreateInfo();
	VkPipelineLayout trianglePipelineLayout;
	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &trianglePipelineLayout));
	pipelineBuilder.pipelineLayout = trianglePipelineLayout;

	// Colored triangle pipeline
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, triangleVertShader));
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, coloredTriangleFragShader));
	trianglePipeline = pipelineBuilder.BuildPipeline(device, renderPass);

	// Red triangle pipeline
	pipelineBuilder.shaderStages.clear();
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, redTriangleVertShader));
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));
	redTrianglePipeline = pipelineBuilder.BuildPipeline(device, renderPass);


	// Reset builder shaders
	pipelineBuilder.shaderStages.clear();


	// Mesh pipeline layout
	VkPipelineLayoutCreateInfo meshPipelineLayoutInfo = vkinit::LayoutCreateInfo();

	VkPushConstantRange pushConstant;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(MeshPushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	meshPipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	meshPipelineLayoutInfo.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout));
	mainDeletionQueue.PushFunction([=]()
		{
			vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
		});

	pipelineBuilder.pipelineLayout = meshPipelineLayout;

	// Mesh pipelines
	VertexInputDescription vertexDescription = Vertex::GetVertexDescription();
	pipelineBuilder.vertexInput.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder.vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDescription.attributes.size());
	pipelineBuilder.vertexInput.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder.vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDescription.bindings.size());

	// Colored version
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, coloredTriangleFragShader));
	meshPipeline = pipelineBuilder.BuildPipeline(device, renderPass);
	CreateMaterial(meshPipeline, meshPipelineLayout, "defaultMesh");

	// Grey version
	pipelineBuilder.shaderStages.clear();
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, greyscaleTriangleFragShader));
	greyMeshPipeline = pipelineBuilder.BuildPipeline(device, renderPass);
	CreateMaterial(greyMeshPipeline, meshPipelineLayout, "greyMesh");



	// Shader load cleanup
	vkDestroyShaderModule(device, meshVertShader, nullptr);
	vkDestroyShaderModule(device, redTriangleVertShader, nullptr);
	vkDestroyShaderModule(device, redTriangleFragShader, nullptr);
	vkDestroyShaderModule(device, triangleVertShader, nullptr);
	vkDestroyShaderModule(device, coloredTriangleFragShader, nullptr);
	vkDestroyShaderModule(device, greyscaleTriangleFragShader, nullptr);

	mainDeletionQueue.PushFunction([=]()
		{
			vkDestroyPipeline(device, greyMeshPipeline, nullptr);
			vkDestroyPipeline(device, meshPipeline, nullptr);
			vkDestroyPipeline(device, redTrianglePipeline, nullptr);
			vkDestroyPipeline(device, trianglePipeline, nullptr);

			vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);
		});
}


void VulkanEngine::InitScene()
{
	camPos = { 0.0f, -6.0f, -10.0f };

	std::vector<std::string> meshNames;
	meshNames.push_back("monkey");
	meshNames.push_back("apple");
	meshNames.push_back("rabbit_high");

	std::vector<Mesh*> meshes;
	for (auto it = meshNames.begin(); it != meshNames.end(); ++it)
	{
		Mesh* mesh = GetMesh((*it).c_str());
		if (mesh == nullptr)
			continue;
		meshes.push_back(mesh);
	}

	if (!meshes.empty())
	{
		// Very slow framerate, but interactive.  No heavy CPU or GPU, likely bandwidth or driver time?
//		const int meshVolumeDim = 20;
//		const float meshVolumeWidth = 200.0f;
 		const int meshVolumeDim = 10;
 		const float meshVolumeWidth = 100.0f;

		const float step = meshVolumeWidth / meshVolumeDim;
		const glm::vec3 start{ -meshVolumeWidth * 0.5f };
		int meshIndex = 0;
		for (int x = 0; x < meshVolumeDim; ++x)
		{
			for (int y = 0; y < meshVolumeDim; ++y)
			{
				for (int z = 0; z < meshVolumeDim; ++z)
				{
					RenderObject ro;
					ro.mesh = meshes[meshIndex];
					ro.material = ((x + y + z) % 2) ? GetMaterial("greyMesh") : GetMaterial("defaultMesh");
					glm::vec3 pos{ start.x + x * step, start.y + y * step, start.z + z * step };
					ro.transformMatrix = glm::translate(glm::mat4{ 1.0f }, pos - ro.mesh->GetObjectCenter());
					renderables.push_back(ro);

					meshIndex = (meshIndex + 1) % static_cast<int>(meshes.size());
				}
			}
		}
	}

	for (int x = -20; x <= 20; ++x)
	{
		for (int y = -20; y <= 20; ++y)
		{
			RenderObject tri;
			tri.mesh = GetMesh("triangle");
			tri.material = GetMaterial("defaultMesh");
			glm::mat4 translation = glm::translate(glm::mat4{ 1.0f }, glm::vec3(x, 0.0f, y));
			glm::mat4 scale = glm::scale(glm::mat4{ 1.0f }, glm::vec3(0.2f, 0.2f, 0.2f));
			tri.transformMatrix = translation * scale;

			renderables.push_back(tri);
		}
	}
}


void VulkanEngine::Cleanup()
{
	if (isInitialized)
	{
		vkWaitForFences(device, 1, &renderFence, VK_TRUE, 1000000000);

		mainDeletionQueue.Flush();

		vmaDestroyAllocator(allocator);

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(device, nullptr);
		vkb::destroy_debug_utils_messenger(instance, debugMessenger);
 		vkDestroyInstance(instance, nullptr);
		SDL_DestroyWindow(window);

		isInitialized = false;
	}
}


void VulkanEngine::Draw()
{
	const uint64_t timeoutNS = 1000000000;		// one second

	VK_CHECK(vkWaitForFences(device, 1, &renderFence, true, timeoutNS));
	VK_CHECK(vkResetFences(device, 1, &renderFence));

	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, timeoutNS, presentSemaphore, nullptr, &swapchainImageIndex));

	VK_CHECK(vkResetCommandBuffer(mainCommandBuffer, 0));

	VkCommandBuffer cmd = mainCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pInheritanceInfo = nullptr; // Explicitly not inheriting, because this is not a secondary command buffer
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	// allow one-shot buffer optimizations

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	VkClearValue clearValue = {};
	float flash = static_cast<float>( abs(sin(frameNumber / 120.0f)) );
	clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };

	VkClearValue depthClear;
	depthClear.depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo rpInfo = {};
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.renderPass = renderPass;
	rpInfo.renderArea.extent = windowExtent;
	rpInfo.framebuffer = frameBuffers[swapchainImageIndex];
	rpInfo.clearValueCount = 2;
	VkClearValue clearValues[] = { clearValue, depthClear };
	rpInfo.pClearValues = &clearValues[0];

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	std::sort(renderables.begin(), renderables.end());
	DrawObjects(cmd, &renderables[0], static_cast<int>(renderables.size()));

	vkCmdEndRenderPass(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &presentSemaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &renderSemaphore;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, renderFence));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderSemaphore;
	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

	frameNumber++;
}


void VulkanEngine::UpdateCamera()
{
	const float ACCEL = 0.015f;
	const float DRAG = 0.9f;

	enum MOVE_DIR
	{
		FWD,
		LEFT,
		BACK,
		RIGHT,
		UP,
		DOWN,
		//
		COUNT
	};
	const SDL_Scancode scQwerty[MOVE_DIR::COUNT] = {
		SDL_SCANCODE_W,
		SDL_SCANCODE_A,
		SDL_SCANCODE_S,
		SDL_SCANCODE_D,
		SDL_SCANCODE_LALT,
		SDL_SCANCODE_SPACE
	};
	const SDL_Scancode scDvorak[MOVE_DIR::COUNT] = {
		SDL_SCANCODE_COMMA,
		SDL_SCANCODE_A,
		SDL_SCANCODE_O,
		SDL_SCANCODE_E,
		SDL_SCANCODE_HOME,
		SDL_SCANCODE_END
	};
	const glm::vec3 moveVec[MOVE_DIR::COUNT] = {
		glm::vec3( 0.0f,  0.0f,  1.0f),
		glm::vec3( 1.0f,  0.0f,  0.0f),
		glm::vec3( 0.0f,  0.0f, -1.0f),
		glm::vec3(-1.0f,  0.0f,  0.0f),
		glm::vec3( 0.0f, -1.0f,  0.0f),
		glm::vec3( 0.0f,  1.0f,  0.0f)
	};
	const SDL_Scancode* scanCodes = useDvorak ? scDvorak : scQwerty;
	for (int i = 0; i < MOVE_DIR::COUNT; ++i)
	{
		if (keyboardState[scanCodes[i]])
			camVel += moveVec[i] * ACCEL;
	}

	camVel *= DRAG;
	camPos += camVel;
}


void VulkanEngine::Run()
{
	SDL_Event e;
	bool bQuit = false;

	while (!bQuit)
	{
		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT)
			{
				bQuit = true;
			}
			else if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_SPACE)
				{
					selectedShader = (selectedShader + 1) % 2;
				}
				else if (e.key.keysym.sym == SDLK_d)
				{
					useDvorak = !useDvorak;
				}
			}
		}

		UpdateCamera();
		Draw();
	}
}


VkPipeline PipelineBuilder::BuildPipeline(VkDevice device, VkRenderPass pass)
{
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
	{
		OutputDebugString("Failed to create pipeline\n");
		return VK_NULL_HANDLE;
	}
	else
	{
		return newPipeline;
	}
}