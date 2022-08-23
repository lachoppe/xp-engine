#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <windows.h>
#include <debugapi.h>
#include <stdio.h>

#include "vk_types.h"
#include "vk_initializers.h"

#include "VKBootstrap.h"


#define VK_CHECK(x)															\
	do																		\
	{																		\
		VkResult err = x;													\
		if (err)															\
		{																	\
			const int msgLen = 64;											\
			wchar_t str[msgLen];											\
			swprintf(str, msgLen, L"Detected Vulkan error: %d\n", err);		\
			OutputDebugString(str);											\
			abort();														\
		}																	\
	} while (0);



void VulkanEngine::Init()
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

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

	const int msgLen = 1024;
	wchar_t str[msgLen];
	swprintf(str, msgLen, L"[%S: %S]\n%S\n", ms, mt, pCallbackData->pMessage);
	OutputDebugString(str);

	return VK_FALSE; // Applications must return false here
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
}


void VulkanEngine::Cleanup()
{
	if (isInitialized)
	{
		vkDestroySwapchainKHR(device, swapchain, nullptr);

		for (int i = 0; i < swapchainImageViews.size(); ++i)
		{
			vkDestroyImageView(device, swapchainImageViews[i], nullptr);
		}

		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkb::destroy_debug_utils_messenger(instance, debugMessenger);
 		vkDestroyInstance(instance, nullptr);
		SDL_DestroyWindow(window);
		isInitialized = false;
	}
}


void VulkanEngine::Draw()
{

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
				bQuit = true;
		}

		Draw();
	}
}