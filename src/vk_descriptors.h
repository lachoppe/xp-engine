#pragma once

#include <vector>
#include <unordered_map>

#include "vk_types.h"


class DescriptorAllocator
{
public:
	struct PoolSizes
	{
		std::vector<std::pair<VkDescriptorType, float>> poolSizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.0f },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.0f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.0f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.0f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.0f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.0f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.0f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.0f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.0f },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f },
		};
	};

	void ResetPools();
	bool AllocateSets(VkDescriptorSet* sets, VkDescriptorSetLayout layout);
	void Init(VkDevice newDevice);
	void Cleanup();

	VkDevice device;
private:
	VkDescriptorPool GrabPool();

	VkDescriptorPool currentPool{ VK_NULL_HANDLE };
	PoolSizes descriptorSizes;
	std::vector<VkDescriptorPool> usedPools;
	std::vector<VkDescriptorPool> freePools;
};


class DescriptorLayoutCache
{
public:
	void Init(VkDevice newDevice);
	void Cleanup();

	VkDescriptorSetLayout CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info);

	struct DescriptorLayoutInfo
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		bool operator==(const DescriptorLayoutInfo& other) const;
		std::size_t Hash() const;
	};

private:
	struct DescriptorLayoutHash
	{
		std::size_t operator()(const DescriptorLayoutInfo& info) const
		{
			return info.Hash();
		}
	};

	VkDevice device;
	std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> layoutCache;
};


class DescriptorBuilder
{
public:
	static DescriptorBuilder Begin(DescriptorLayoutCache* cache, DescriptorAllocator* allocator);

	DescriptorBuilder& BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
	DescriptorBuilder& BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

	bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
	bool Build(VkDescriptorSet& set);

private:
	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	DescriptorLayoutCache* cache{ nullptr };
	DescriptorAllocator* alloc{ nullptr };
};