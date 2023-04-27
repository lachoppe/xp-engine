#include "vk_textures.h"

#include <iostream>
#include "debug.h"
#include "vk_initializers.h"
#include "asset_core.h"
#include "texture_asset.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


bool vkutil::LoadImageFromAsset(VulkanEngine& engine, const char* filepath, AllocatedImage& outImage)
{
	assets::AssetFile asset;
	std::vector<MipmapInfo> mips;

	START_TIMER( load )
	bool loaded = assets::LoadBinary(filepath, asset);
	if (!loaded)
	{
		OutputMessage("Error loading cooked image asset: %s", filepath);
		return false;
	}
	END_TIMER("Texture load", load)

	assets::TextureInfo info = assets::ReadTextureInfo(&asset);

	VkDeviceSize compressedImageSize = info.dataSize;
	VkFormat imageFmt{};
	switch (info.textureFormat)
	{
	case assets::TextureFormat::RGBA8:
		imageFmt = VK_FORMAT_R8G8B8A8_SRGB; // VK_FORMAT_R8G8B8A8_UNORM
		break;
	default:
		return false;
	}

	START_TIMER(upload)
// 	VK_MEMORY_PROPERTY_HOST_CACHED_BIT
	AllocatedBuffer stagingBuffer = engine.CreateBuffer(compressedImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	void* data;
	vmaMapMemory(engine.allocator, stagingBuffer.allocation, &data);
	size_t offset = 0;
	for (int i = 0; i < info.pages.size(); ++i)
	{
		MipmapInfo mip;
		mip.dataOffset = offset;
		mip.dataSize = info.pages[i].originalSize;
		mips.push_back(mip);

		assets::UnpackTexturePage(&info, i, asset.blob.data(), reinterpret_cast<char*>(data) + offset);

		offset += mip.dataSize;
	}

	vmaUnmapMemory(engine.allocator, stagingBuffer.allocation);

	outImage = UploadMipmappedImage(info.pages[0].width, info.pages[0].height, imageFmt, engine, stagingBuffer, mips);

	vmaDestroyBuffer(engine.allocator, stagingBuffer.buffer, stagingBuffer.allocation);
	END_TIMER("Texture upload", upload)

	return true;
}

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

AllocatedImage vkutil::UploadImage(int width, int height, VkFormat fmt, VulkanEngine& engine, AllocatedBuffer& stagingBuffer)
{
	VkExtent3D imageExtent;
	imageExtent.width = static_cast<uint32_t>(width);
	imageExtent.height = static_cast<uint32_t>(height);
	imageExtent.depth = 1;

	VkImageCreateInfo dimgInfo = vkinit::ImageCreateInfo(fmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

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

	engine.mainDeletionQueue.PushFunction([=, &engine]()
		{
			vmaDestroyImage(engine.allocator, newImage.image, newImage.allocation);
		});

	newImage.mipLevels = 1;
	return newImage;
}

AllocatedImage vkutil::UploadMipmappedImage(int width, int height, VkFormat fmt, VulkanEngine& engine, AllocatedBuffer& stagingBuffer, std::vector<MipmapInfo> mips)
{
	VkExtent3D imageExtent;
	imageExtent.width = static_cast<uint32_t>(width);
	imageExtent.height = static_cast<uint32_t>(height);
	imageExtent.depth = 1;

	VkImageCreateInfo dimgInfo = vkinit::ImageCreateInfo(fmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
	dimgInfo.mipLevels = static_cast<uint32_t>(mips.size());

	AllocatedImage newImage;
	VmaAllocationCreateInfo dimgAllocInfo = {};
	dimgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaCreateImage(engine.allocator, &dimgInfo, &dimgAllocInfo, &newImage.image, &newImage.allocation, nullptr);

	engine.ImmediateSubmit([&](VkCommandBuffer cmd)
		{
			VkImageSubresourceRange range;
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = dimgInfo.mipLevels;
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

			for (int i = 0; i < mips.size(); ++i)
			{
				VkBufferImageCopy copyRegion = {};
				copyRegion.bufferOffset = mips[i].dataOffset;
				copyRegion.bufferRowLength = 0;
				copyRegion.bufferImageHeight = 0;
				copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount = 1;
				copyRegion.imageSubresource.mipLevel = i;
				copyRegion.imageExtent = imageExtent;

				vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

				imageExtent.width /= 2;
				imageExtent.height /= 2;
			}

			VkImageMemoryBarrier imageBarrierToReadable = imageBarrierToTransfer;
			imageBarrierToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrierToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageBarrierToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrierToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToReadable);
		});

	engine.mainDeletionQueue.PushFunction([=, &engine]()
		{
			vmaDestroyImage(engine.allocator, newImage.image, newImage.allocation);
		});

	newImage.mipLevels = dimgInfo.mipLevels;

	return newImage;
}