#pragma once

#include <string>
#include <vector>
#include "vk_types.h"

typedef struct SyncMode
{
	const char* name;
	const VkPresentModeKHR mode;
	bool available;
};

template<typename T> class ConfigValue
{
public:
	std::string category;
	std::string name;
	T* value;

	ConfigValue(std::string c, std::string n, T* v) : category(c), name(n), value(v) {}
};

class Config
{
private:
	static SyncMode syncModes[3];

	const char* CONFIG_FILENAME = ".\\config.ini";
		
	std::vector<ConfigValue<int>* > ints;
	std::vector<ConfigValue<float>* > floats;
	std::vector<ConfigValue<bool>* > bools;
	
	VkPresentModeKHR defaultSyncMode;

	class VulkanEngine* engine{ nullptr };
public:
	// Options
	float lookSensitivity;
	bool useDvorak;
	int syncMode;

	// Constants
	float minLookSensitivityValue;
	float maxLookSensitivityValue;

	void Setup();
	void SetEngine(class VulkanEngine* vkEngine);
	void ConfigureSyncModes(std::vector<VkPresentModeKHR> modes, VkPresentModeKHR defaultMode);

	void SetupValue(int& value, int default, std::string category, std::string name);
	void SetupValue(float& value, float default, std::string category, std::string name);
	void SetupValue(bool& value, bool default, std::string category, std::string name);

	void Save();
	void Load();

	void ShowOptions(bool* showOptions = nullptr);

	void SetNextSyncMode();
	void ValidateSyncMode();
	VkPresentModeKHR GetSyncMode();
	const char* GetSyncModeName();

	Config() { Setup(); }
};