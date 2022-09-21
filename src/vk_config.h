#pragma once

#include <string>
#include <vector>


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
	const char* CONFIG_FILENAME = ".\\config.ini";

	std::vector<ConfigValue<int>* > ints;
	std::vector<ConfigValue<float>* > floats;
	std::vector<ConfigValue<bool>* > bools;

public:
	// Options
	float lookSensitivity;
	bool useDvorak;

	// Constants
	float minLookSensitivityValue;
	float maxLookSensitivityValue;


	void Setup();

	void SetupValue(int& value, int default, std::string category, std::string name);
	void SetupValue(float& value, float default, std::string category, std::string name);
	void SetupValue(bool& value, bool default, std::string category, std::string name);

	void Save();
	void Load();

	void ShowOptions(bool* showOptions = nullptr);

	Config() { Setup(); }
};