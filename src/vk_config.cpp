#include "vk_config.h"

#include "../third_party/imgui/imgui.h"

#include <windows.h>
#include "vk_engine.h"

SyncMode Config::syncModes[] = {
	{
		"No sync (IMMEDIATE)",
		VK_PRESENT_MODE_IMMEDIATE_KHR,
		false
	},
	{
		"V-sync (FIFO)",
		VK_PRESENT_MODE_FIFO_KHR,
		false
	},
	{
		"V-sync - no wait (MAILBOX)",
		VK_PRESENT_MODE_MAILBOX_KHR,
		false
	}
};

void Config::Setup()
{
	SetupValue(lookSensitivity, 5, "Options", "lookSensitivity");
	SetupValue(useDvorak, false, "Options", "useDvorak");
	SetupValue(syncMode, 0, "Options", "syncMode");
	SetupValue(minLookSensitivityValue, 0.0005f, "Constants", "lookSensitivityMin");
	SetupValue(maxLookSensitivityValue, 0.005f, "Constants", "lookSensitivityMax");
}

void Config::SetEngine(VulkanEngine* vkEngine)
{
	engine = vkEngine;
}

void Config::SetupValue(int& value, int default, std::string category, std::string name)
{
	value = default;
	ints.push_back(new ConfigValue<int>( category, name, &value ));
}

void Config::SetupValue(float& value, float default, std::string category, std::string name)
{
	value = default;
	floats.push_back(new ConfigValue<float>(category, name, &value));
}


void Config::SetupValue(bool& value, bool default, std::string category, std::string name)
{
	value = default;
	bools.push_back(new ConfigValue<bool>(category, name, &value));
}

void Config::ConfigureSyncModes(std::vector<VkPresentModeKHR> modes, VkPresentModeKHR defaultMode)
{
	const int syncModeCount = IM_ARRAYSIZE(syncModes);
	bool anyAvailable = false;
	defaultSyncMode = defaultMode;

	for (int i = 0; i < syncModeCount; ++i)
	{
		for (const auto& mode : modes)
		{
			if (mode == syncModes[i].mode)
			{
				syncModes[i].available = true;
				anyAvailable = true;
				break;
			}
		}
	}

	WARN_M(anyAvailable, "No valid sync mode found!  Falling back to mode %d", defaultMode);

	ValidateSyncMode();
}


void Config::Save()
{
	for (auto value : ints)
		WritePrivateProfileString(value->category.c_str(), value->name.c_str(), std::to_string(*value->value).c_str(), CONFIG_FILENAME);

	for (auto value : floats)
		WritePrivateProfileString(value->category.c_str(), value->name.c_str(), std::to_string(*value->value).c_str(), CONFIG_FILENAME);

	for (auto value : bools)
		WritePrivateProfileString(value->category.c_str(), value->name.c_str(), *value->value ? "true" : "false", CONFIG_FILENAME);
}


void Config::Load()
{
	char value_l[32] = { '\0' };

	for (auto value : ints)
	{
		if (GetPrivateProfileString(value->category.c_str(), value->name.c_str(), "", value_l, 32, CONFIG_FILENAME) > 0)
			*value->value = atoi(value_l);
	}

	for (auto value : floats)
	{
		if (GetPrivateProfileString(value->category.c_str(), value->name.c_str(), "", value_l, 32, CONFIG_FILENAME) > 0)
			*value->value = static_cast<float>(atof(value_l));
	}

	for (auto value : bools)
	{
		if (GetPrivateProfileString(value->category.c_str(), value->name.c_str(), "", value_l, 32, CONFIG_FILENAME) > 0)
			*value->value = !strcmp( value_l, "true");
	}
}


void Config::ShowOptions(bool* showOptions)
{
	ValidateSyncMode();

	ImGui::Begin("Options", showOptions, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::SliderFloat("Look Sensitivity", &lookSensitivity, 0, 10, "%0.2f");
	ImGui::SameLine();
	if (ImGui::Button("... Reset"))
	{
		lookSensitivity = 5;
	}

	ImGui::Separator();

	ImGui::Checkbox("Use Dvorak Layout", &useDvorak);
	struct ComboFuncs
	{
		static bool ItemGetter(void* data, int n, const char** out_str)
		{
			const SyncMode* modes = static_cast<SyncMode*>(data);

			if (modes[n].available)
				*out_str = modes[n].name;
			else
				*out_str = "Not available";

			return true;
		}
	};
	if (syncMode < 0)
		syncMode = 0;
	ImGui::Combo("Display Sync Mode", &syncMode, &ComboFuncs::ItemGetter, syncModes, IM_ARRAYSIZE(syncModes));
	ImGui::End();
}

void Config::SetNextSyncMode()
{
	if (syncMode >= 0)
		syncMode = (syncMode + 1) % IM_ARRAYSIZE(syncModes);
	ValidateSyncMode();
}

void Config::ValidateSyncMode()
{
	if (syncMode < 0)
		syncMode = 0;

	if (syncMode >= IM_ARRAYSIZE(syncModes))
		syncMode = IM_ARRAYSIZE(syncModes) - 1;

	// Find the next available syncMode
	for (int i = 0; i < IM_ARRAYSIZE(syncModes); ++i)
	{
		int index = (syncMode + i) % IM_ARRAYSIZE(syncModes);
		if (syncModes[index].available)
		{
			syncMode = index;
			return;
		}
	}

	syncMode = -1;
}

VkPresentModeKHR Config::GetSyncMode()
{
	if (syncMode >= 0)
		return syncModes[syncMode].mode;
	return defaultSyncMode;
}

const char* Config::GetSyncModeName()
{
	if (syncMode >= 0)
		return syncModes[syncMode].name;
	return "Fallback Mode";
}
