#pragma once

#include "vk_types.h"
#include "vk_engine.h"

namespace vkutil
{
	bool LoadImageFromAsset(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
	bool LoadImageFromFile(VulkanEngine& engine, const char* file, AllocatedImage& outImage);

	AllocatedImage UploadImage(int width, int height, VkFormat fmt, VulkanEngine& engine, AllocatedBuffer& stagingBuffer);
}