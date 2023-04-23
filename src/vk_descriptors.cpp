#include "vk_descriptors.h"

#include <algorithm>

#include "debug.h"

#define POOL_SET_COUNT	1000


VkDescriptorPool CreatePool(VkDevice device, const DescriptorAllocator::PoolSizes poolSizes, int count, VkDescriptorPoolCreateFlags flags)
{
	std::vector<VkDescriptorPoolSize> sizes;
	sizes.reserve(poolSizes.poolSizes.size());

	for (auto ps : poolSizes.poolSizes)
		sizes.push_back({ ps.first, static_cast<uint32_t>(ps.second * count)});

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.flags = flags;
	createInfo.maxSets = count;
	createInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
	createInfo.pPoolSizes = sizes.data();

	VkDescriptorPool descriptorPool;
	vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool);

	return descriptorPool;
}


void DescriptorAllocator::Init(VkDevice newDevice)
{
	device = newDevice;
}

void DescriptorAllocator::Cleanup()
{
	for (auto p : freePools)
		vkDestroyDescriptorPool(device, p, nullptr);

	for (auto p : usedPools)
		vkDestroyDescriptorPool(device, p, nullptr);
}

VkDescriptorPool DescriptorAllocator::GrabPool()
{
	if (freePools.size() > 0)
	{
		VkDescriptorPool pool = freePools.back();
		freePools.pop_back();
		return pool;
	}

	return CreatePool(device, descriptorSizes, POOL_SET_COUNT, 0);
}

bool DescriptorAllocator::AllocateSets(VkDescriptorSet* sets, VkDescriptorSetLayout layout)
{
	if (currentPool == VK_NULL_HANDLE)
	{
		currentPool = GrabPool();
		usedPools.push_back(currentPool);
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.pSetLayouts = &layout;
	allocInfo.descriptorPool = currentPool;
	allocInfo.descriptorSetCount = 1;

	VkResult allocResult = vkAllocateDescriptorSets(device, &allocInfo, sets);
	bool needRealloc = false;

	switch (allocResult)
	{
	case VK_SUCCESS:
		return true;
	case VK_ERROR_FRAGMENTED_POOL:
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		needRealloc = true;
		break;
	default:
		return false;
	}

	if (needRealloc)
	{
		currentPool = GrabPool();
		usedPools.push_back(currentPool);
		allocInfo.descriptorPool = currentPool;

		allocResult = vkAllocateDescriptorSets(device, &allocInfo, sets);
		if (allocResult == VK_SUCCESS)
			return true;
	}

	return false;
}

void DescriptorAllocator::ResetPools()
{
	for (auto p : usedPools)
	{
		// Might not need to check return on this
		VK_CHECK(vkResetDescriptorPool(device, p, 0));
		freePools.push_back(p);
	}

	usedPools.clear();
	currentPool = VK_NULL_HANDLE;
}


void DescriptorLayoutCache::Init(VkDevice newDevice)
{
	device = newDevice;
}

void DescriptorLayoutCache::Cleanup()
{
	for (auto pair : layoutCache)
		vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
}

VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info)
{
	DescriptorLayoutInfo layoutInfo;
	layoutInfo.bindings.reserve(info->bindingCount);

	bool isSorted = true;
	int lastbinding = -1;

	for (int i = 0; i < static_cast<int>(info->bindingCount); ++i)
	{
		layoutInfo.bindings.push_back(info->pBindings[i]);

		if (static_cast<int>(info->pBindings[i].binding) > lastbinding)
			lastbinding = info->pBindings[i].binding;
		else
			isSorted = false;
	}

	if (!isSorted)
	{
		std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) {
			return a.binding < b.binding;
			});
	}

	auto it = layoutCache.find(layoutInfo);
	if (it != layoutCache.end())
	{
		return (*it).second;
	}
	else
	{
		VkDescriptorSetLayout layout;
		VK_CHECK(vkCreateDescriptorSetLayout(device, info, nullptr, &layout));

		layoutCache[layoutInfo] = layout;
		return layout;
	}
}

bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
{
	if (other.bindings.size() != bindings.size())
		return false;

	for (int i = 0; i < bindings.size(); ++i)
	{
		if (other.bindings[i].binding != bindings[i].binding)
			return false;

		if (other.bindings[i].descriptorType != bindings[i].descriptorType)
			return false;

		if (other.bindings[i].descriptorCount != bindings[i].descriptorCount)
			return false;

		if (other.bindings[i].stageFlags != bindings[i].stageFlags)
			return false;

// 		if (other.bindings[i].pImmutableSamplers != bindings[i].pImmutableSamplers)
// 			return false;
	}

	return true;
}

size_t DescriptorLayoutCache::DescriptorLayoutInfo::Hash() const
{
	using std::size_t;
	using std::hash;

	size_t result = hash<size_t>()(bindings.size());

	for (const VkDescriptorSetLayoutBinding& b : bindings)
	{
		size_t binding_hash =
			b.binding
			| b.descriptorType << 8
			| b.descriptorCount << 16
			| b.stageFlags << 24;

		result ^= hash<size_t>()(binding_hash);
	}

	return result;
}


DescriptorBuilder DescriptorBuilder::Begin(DescriptorLayoutCache* cache, DescriptorAllocator* allocator)
{
	DescriptorBuilder builder;
	builder.cache = cache;
	builder.alloc = allocator;

	return builder;
}

DescriptorBuilder& DescriptorBuilder::BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
	VkDescriptorSetLayoutBinding newBinding = {};
	newBinding.descriptorCount = 1;
	newBinding.descriptorType = type;
	newBinding.stageFlags = stageFlags;
	newBinding.pImmutableSamplers = nullptr;
	newBinding.binding = binding;
	bindings.push_back(newBinding);

	VkWriteDescriptorSet newWrite = {};
	newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	newWrite.pNext = nullptr;
	newWrite.descriptorCount = 1;
	newWrite.descriptorType = type;
	newWrite.dstBinding = binding;
	newWrite.pBufferInfo = bufferInfo;
	writes.push_back(newWrite);

	return *this;
}

DescriptorBuilder& DescriptorBuilder::BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
	VkDescriptorSetLayoutBinding newBinding = {};
	newBinding.descriptorCount = 1;
	newBinding.descriptorType = type;
	newBinding.stageFlags = stageFlags;
	newBinding.pImmutableSamplers = nullptr;
	newBinding.binding = binding;
	bindings.push_back(newBinding);

	VkWriteDescriptorSet newWrite = {};
	newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	newWrite.pNext = nullptr;
	newWrite.descriptorCount = 1;
	newWrite.descriptorType = type;
	newWrite.dstBinding = binding;
	newWrite.pImageInfo = imageInfo;
	writes.push_back(newWrite);

	return *this;
}

bool DescriptorBuilder::Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.pBindings = bindings.data();
	layoutInfo.bindingCount = static_cast<int32_t>(bindings.size());

	layout = cache->CreateDescriptorLayout(&layoutInfo);

	bool success = alloc->AllocateSets(&set, layout);
	if (!success)
		return false;

	for (VkWriteDescriptorSet wds : writes)
	{
		wds.dstSet = set;
	}

	vkUpdateDescriptorSets(alloc->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

	return true;
}

bool DescriptorBuilder::Build(VkDescriptorSet& set)
{
	VkDescriptorSetLayout layout = {};
	return Build(set, layout);
}