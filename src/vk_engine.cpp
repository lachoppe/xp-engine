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


void VulkanEngine::Init()
{
	SDL_Init(SDL_INIT_VIDEO);
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

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
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
		fbInfo.pAttachments = &swapchainImageViews[i];
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
	VkShaderModule triangleFragShader;
	LoadShaderModule("colored_triangle.frag", &triangleFragShader);

	VkShaderModule triangleVertShader;
	LoadShaderModule("colored_triangle.vert", &triangleVertShader);

	VkShaderModule redTriangleFragShader;
	LoadShaderModule("triangle.frag", &redTriangleFragShader);

	VkShaderModule redTriangleVertShader;
	LoadShaderModule("triangle.vert", &redTriangleVertShader);


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::LayoutCreateInfo();
	VkPipelineLayout trianglePipelineLayout;
	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &trianglePipelineLayout));

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
	pipelineBuilder.pipelineLayout = trianglePipelineLayout;

	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, triangleVertShader));
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));
	trianglePipeline = pipelineBuilder.BuildPipeline(device, renderPass);

	pipelineBuilder.shaderStages.clear();
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, redTriangleVertShader));
	pipelineBuilder.shaderStages.push_back(vkinit::ShaderStateCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));
	redTrianglePipeline = pipelineBuilder.BuildPipeline(device, renderPass);

	vkDestroyShaderModule(device, redTriangleVertShader, nullptr);
	vkDestroyShaderModule(device, redTriangleFragShader, nullptr);
	vkDestroyShaderModule(device, triangleVertShader, nullptr);
	vkDestroyShaderModule(device, triangleFragShader, nullptr);

	mainDeletionQueue.PushFunction([=]()
		{
			vkDestroyPipeline(device, redTrianglePipeline, nullptr);
			vkDestroyPipeline(device, trianglePipeline, nullptr);

			vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);
		});
}


void VulkanEngine::Cleanup()
{
	if (isInitialized)
	{
		vkWaitForFences(device, 1, &renderFence, VK_TRUE, 1000000000);

		mainDeletionQueue.Flush();

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

	VkRenderPassBeginInfo rpInfo = {};
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.renderPass = renderPass;
	rpInfo.renderArea.extent = windowExtent;
	rpInfo.framebuffer = frameBuffers[swapchainImageIndex];
	rpInfo.clearValueCount = 1;
	rpInfo.pClearValues = &clearValue;

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	if (selectedShader == 0)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
	}
	else
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, redTrianglePipeline);
	}

	vkCmdDraw(cmd, 3, 1, 0, 0);

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
			}
		}
		
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