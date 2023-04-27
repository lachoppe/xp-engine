#pragma once

#include "vk_types.h"
#include "vk_engine.h"

namespace vkutil
{
	struct MipmapInfo
	{
		size_t dataSize;
		size_t dataOffset;
	};

	bool LoadImageFromAsset(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
	bool LoadImageFromFile(VulkanEngine& engine, const char* file, AllocatedImage& outImage);

	AllocatedImage UploadImage(int width, int height, VkFormat fmt, VulkanEngine& engine, AllocatedBuffer& stagingBuffer);
	AllocatedImage UploadMipmappedImage(int width, int height, VkFormat fmt, VulkanEngine& engine, AllocatedBuffer& stagingBuffer, ::std::vector<MipmapInfo> mips);
}