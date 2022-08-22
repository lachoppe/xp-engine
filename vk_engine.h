#pragma once

#include "vk_types.h"

class VulkanEngine
{
public:

	bool isInitialized{ false };
	int frameNumber{ 0 };

	VkExtent2D windowExtent{ 1700, 900 };

	struct SDL_Window* window{ nullptr };

	void Init();

	void Cleanup();

	void Draw();

	void Run();
};