#include "vk_config.h"

#include "../third_party/imgui/imgui.h"

#include <windows.h>
#include "vk_engine.h"
#include "debug.h"


void Config::Setup()
{
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
// 	const int syncModeCount = cvar_syncMode.GetMax();
// 	bool anyAvailable = false;
	defaultSyncMode = defaultMode;

	// The plan is to validate what's in the specified combo box using an array of valid values
	// If there's a combo value that's not in the list of valid values, it should be removed from the cvar's combo list
	// 
	// Can't access CVarStorage* from out here
	// Can't use the VkPresentModeKHR type in cvars
	// Therefore, the appropriate mechanism is probably a callback mechanism:
	//		AutoCVar_Int::ValidateComboValues(bool (callback*(int32_t value, valid list, valid list len)), valid list, valid list len)
	// Then ValidateComboValues can run the callback against each combo value using the valid list, which returns a bool
	// If the combo value isn't valid, then that combo gets removed from the combo lists

// 	for (int i = 0; i < syncModeCount; ++i)
// 	{
// if (i == 1) continue; // --------------------------------------------------------------------------------------------------------------------------
// 		
// 		int comboMode = cvar_syncMode.GetComboValueInitial(i);
// 		if (comboMode >= 0)
// 		{
// 			for (const auto& mode : modes)
// 			{
// 				if (mode == static_cast<VkPresentModeKHR>(comboMode))
// 				{
// 					syncModes[i].available = true;
// 					anyAvailable = true;
// 					break;
// 				}
// 			}
// 		}
// 		cvar_syncMode.SetComboValue(i);
// 	}

// 	WARN_M(anyAvailable, "No valid sync mode found!  Falling back to mode %d", defaultMode);

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

	CVarSystem::Get()->DrawImGuiEditor();

	ImGui::End();
}

void Config::SetNextSyncMode()
{
	int32_t& syncMode = *cvar_syncMode.GetPtr();

	if (syncMode >= 0)
		syncMode = (syncMode + 1) % (cvar_syncMode.GetMax() + 1);
	ValidateSyncMode();
}

void Config::ValidateSyncMode()
{
	int& syncMode = *cvar_syncMode.GetPtr();
	const int modeCount = cvar_syncMode.GetMax() + 1;
	const int modeStart = syncMode;

	if (syncMode < 0)
		syncMode = 0;

	if (syncMode >= modeCount)
		syncMode = modeCount - 1;

	// Find the next available syncMode
	for (int i = 0; i < modeCount; ++i)
	{
		syncMode = (modeStart + i) % modeCount;
		int32_t modeValue = cvar_syncMode.GetComboValue();
		if (modeValue >= 0 && modeValue < modeCount)
			return;
	}

	syncMode = -1;
}

VkPresentModeKHR Config::GetSyncMode()
{
	int32_t mode = cvar_syncMode.GetComboValue();

	if (mode >= 0)
		return static_cast<VkPresentModeKHR>(mode);
	return defaultSyncMode;
}

const char* Config::GetSyncModeName()
{
	const char* name = cvar_syncMode.GetComboValueName();
	if (name)
		return name;
	return "Fallback Mode";
}
