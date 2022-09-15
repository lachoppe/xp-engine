#pragma once

#include "vk_types.h"

namespace vkinit
{
	VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkPipelineShaderStageCreateInfo ShaderStateCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
	VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo();
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo(VkPrimitiveTopology topology);
	VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo(VkPolygonMode polygonMode);
	VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo();
	VkPipelineColorBlendAttachmentState ColorBlendAttachmentState();
	VkPipelineLayoutCreateInfo LayoutCreateInfo();
	VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);
	VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);
	VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
	VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo(bool depthTest, bool depthWrite, VkCompareOp compareOp);
	VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
	VkWriteDescriptorSet WriteDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
	VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
	VkSubmitInfo SubmitInfo(VkCommandBuffer* cmd);
	VkSamplerCreateInfo SamplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
	VkWriteDescriptorSet WriteDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);
}