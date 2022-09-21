#include "vk_config.h"

#include "../third_party/imgui/imgui.h"

#include <windows.h>

void Config::Setup()
{
	SetupValue(lookSensitivity, 5, "Options", "Look Sensitivity");
	SetupValue(useDvorak, false, "Options", "Use Dvorak Layout");
	SetupValue(minLookSensitivityValue, 0.0005f, "Constants", "Min Look Sensitivity Value");
	SetupValue(maxLookSensitivityValue, 0.005f, "Constants", "Max Look Sensitivity Value");
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
	ImGui::Begin("Options", showOptions, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::SliderFloat("Look Sensitivity", &lookSensitivity, 0, 10, "%0.2f");
	ImGui::SameLine();
	if (ImGui::Button("... Reset"))
	{
		lookSensitivity = 5;
	}

	ImGui::Separator();

	ImGui::Checkbox("Use Dvorak", &useDvorak);

	ImGui::End();
}
