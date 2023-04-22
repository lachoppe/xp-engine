#include "vk_textures.h"
#include <iostream>
#include "debug.h"

#include "vk_initializers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


bool vkutil::LoadImageFromFile(VulkanEngine& engine, const char* file, AllocatedImage& outImage)
{
	int width, height, channels;

	stbi_uc* pixels = stbi_load(file, &width, &height, &channels, STBI_rgb_alpha);

	if (!pixels)
	{
		OutputMessage("WARNING: Failed to load texture file: %s\n", file);
		return false;
	}

	void* pixel_ptr = pixels;
	VkDeviceSize imageSize = width * height * 4;
	VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

	AllocatedBuffer stagingBuffer = engine.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data;
	vmaMapMemory(engine.allocator, stagingBuffer.allocation, &data);
	memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));
	vmaUnmapMemory(engine.allocator, stagingBuffer.allocation);

	stbi_image_free(pixels);

	VkExtent3D imageExtent;
	imageExtent.width  = static_cast<uint32_t>(width);
	imageExtent.height = static_cast<uint32_t>(height);
	imageExtent.depth  = 1;

	VkImageCreateInfo dimgInfo = vkinit::ImageCreateInfo(imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

	AllocatedImage newImage;
	VmaAllocationCreateInfo dimgAllocInfo = {};
	dimgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaCreateImage(engine.allocator, &dimgInfo, &dimgAllocInfo, &newImage.image, &newImage.allocation, nullptr);

	engine.ImmediateSubmit([&](VkCommandBuffer cmd)
		{
			VkImageSubresourceRange range;
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			VkImageMemoryBarrier imageBarrierToTransfer = {};
			imageBarrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrierToTransfer.image = newImage.image;
			imageBarrierToTransfer.subresourceRange = range;
			imageBarrierToTransfer.srcAccessMask = 0;
			imageBarrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToTransfer);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;
			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageExtent = imageExtent;

			vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			VkImageMemoryBarrier imageBarrierToReadable = imageBarrierToTransfer;
			imageBarrierToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrierToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageBarrierToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrierToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToReadable);
		});

	engine.mainDeletionQueue.PushFunction([=]()
		{
			vmaDestroyImage(engine.allocator, newImage.image, newImage.allocation);
		});

	vmaDestroyBuffer(engine.allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	OutputMessage("Texture loaded: %s\n", file);

	outImage = newImage;

	return true;
}