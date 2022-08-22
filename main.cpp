#include "vk_engine.h"

int main(int argc, char* argv[])
{
	VulkanEngine engine;

	engine.Init();

	engine.Run();

	engine.Cleanup();

	return 0;
}