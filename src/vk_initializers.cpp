#include "vk_initializers.h"

VkCommandPoolCreateInfo vkinit::CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	info.queueFamilyIndex = queueFamilyIndex;
	info.flags = flags;

	return info;
}

VkCommandBufferAllocateInfo vkinit::CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	info.commandPool = pool;
	info.commandBufferCount = count;
	info.level = level;

	return info;
}

VkPipelineShaderStageCreateInfo vkinit::ShaderStateCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
	VkPipelineShaderStageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	// Shader entry point
	info.pName = "main";
	info.stage = stage;
	info.module = shaderModule;

	return info;
}

VkPipelineVertexInputStateCreateInfo vkinit::VertexInputStateCreateInfo()
{
	VkPipelineVertexInputStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	info.vertexAttributeDescriptionCount = 0;
	info.vertexBindingDescriptionCount = 0;

	return info;
}

VkPipelineInputAssemblyStateCreateInfo vkinit::InputAssemblyStateCreateInfo(VkPrimitiveTopology topology)
{
	VkPipelineInputAssemblyStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	info.topology = topology;
	info.primitiveRestartEnable = VK_FALSE;

	return info;
}

VkPipelineRasterizationStateCreateInfo vkinit::RasterizationStateCreateInfo(VkPolygonMode polygonMode)
{
	VkPipelineRasterizationStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	info.depthClampEnable = VK_FALSE;
	info.rasterizerDiscardEnable = VK_FALSE;	// discards all primitives prior to rasterization stage if enabled

	info.polygonMode = polygonMode;
	info.lineWidth = 1.0f;

	// No backface culling
	info.cullMode = VK_CULL_MODE_NONE;
	info.frontFace = VK_FRONT_FACE_CLOCKWISE;

	// No depth bias (useful for depth shadows)
	info.depthBiasEnable = VK_FALSE;
	info.depthBiasConstantFactor = 0.0f;
	info.depthBiasClamp = 0.0f;
	info.depthBiasSlopeFactor = 0.0f;

	return info;
}

VkPipelineMultisampleStateCreateInfo vkinit::MultisampleStateCreateInfo()
{
	VkPipelineMultisampleStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

	// Disabling multisampling (1 sample per pixel)
	info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	info.sampleShadingEnable = VK_FALSE;
	info.minSampleShading = 1.0f;
	info.pSampleMask = nullptr;
	info.alphaToCoverageEnable = VK_FALSE;
	info.alphaToOneEnable = VK_FALSE;

	return info;
}

VkPipelineColorBlendAttachmentState vkinit::ColorBlendAttachmentState()
{
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};

	// Write RGBA, no blending
	colorBlendAttachment.colorWriteMask =	VK_COLOR_COMPONENT_R_BIT |
											VK_COLOR_COMPONENT_G_BIT |
											VK_COLOR_COMPONENT_B_BIT |
											VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	return colorBlendAttachment;
}

VkPipelineLayoutCreateInfo vkinit::LayoutCreateInfo()
{
	VkPipelineLayoutCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	// empty defaults (could just return the bare info with sType, but leaving this here to receive parameters in future)
	info.flags = 0;
	info.setLayoutCount = 0;
	info.pSetLayouts = nullptr;
	info.pushConstantRangeCount = 0;
	info.pPushConstantRanges = nullptr;

	return info;
}

VkFenceCreateInfo vkinit::FenceCreateInfo(VkFenceCreateFlags flags)
{
	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	info.flags = flags;

	return info;
}

VkSemaphoreCreateInfo vkinit::SemaphoreCreateInfo(VkSemaphoreCreateFlags flags)
{
	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	info.flags = flags;

	return info;
}