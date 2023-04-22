#pragma once

#include "string_utils.h"

class CVarParameter;

template<typename T>
struct CVarStorage;

enum class CVarFlags : uint32_t
{
	None = 0,
	NoEdit = 1 << 1,
	EditReadOnly = 1 << 2,
	Advanced = 1 << 3,

	EditCheckbox = 1 << 8,
	EditFloatDrag = 1 << 9,		// uses min and max for slider range, else 0..1
	EditCombo = 1 << 10,		// uses <cvar>_<n> cvars for value (contiguous from 0) and label (description)

	// reserved
	_HasRange = 1 << 16,
};


class CVarSystem
{
public:
	static CVarSystem* Get();

	virtual CVarParameter* GetCVar(const char* name) = 0;
	virtual CVarParameter* GetCVar(StringUtils::StringHash hash) = 0;
	virtual int32_t* GetIntCVar(StringUtils::StringHash hash) = 0;
	virtual double* GetFloatCVar(StringUtils::StringHash hash) = 0;
	virtual std::string* GetStringCVar(StringUtils::StringHash hash) = 0;

	virtual void SetIntCVar(StringUtils::StringHash hash, int32_t value) = 0;
	virtual void SetFloatCVar(StringUtils::StringHash hash, double value) = 0;
	virtual void SetStringCVar(StringUtils::StringHash hash, const char* value) = 0;

	virtual bool CreateIntCVar(const char* name, const char* description, int32_t defaultValue, int32_t currentValue, int32_t minValue, int32_t maxValue, CVarParameter** param) = 0;
	virtual CVarParameter* CreateFloatCVar(const char* name, const char* description, double defaultValue, double currentValue, double minValue, double maxValue) = 0;
	virtual CVarParameter* CreateStringCVar(const char* name, const char* description, const char* defaultValue, const char* currentValue) = 0;

	virtual void DrawImGuiEditor() = 0;
};


template<typename T>
struct AutoCVar
{
protected:
	int index{ -1 };
	using CVarType = T;
	std::string lookupName;

	bool ApplyLookup();

public:
	virtual const char* GetName();
	virtual const char* GetDescription();
	virtual CVarStorage<T>* GetStorage();
};


struct AutoCVar_Int : AutoCVar<int32_t>
{
	AutoCVar_Int(const char* name, const char* description, int32_t defaultValue, CVarFlags flags = CVarFlags::None);
	AutoCVar_Int(const char* name, const char* description, int32_t defaultValue, int32_t minValue, int32_t maxValue, CVarFlags flags = CVarFlags::None, bool (*comboFunction)(void* data, int n, const char** out_str) = nullptr);
	AutoCVar_Int(const char* name);

	int32_t Get();
	int32_t* GetPtr();
	void Set(int32_t value);
	void Toggle();
	CVarParameter* GetComboValueParam(int32_t overrideValue = -1);
// 	CVarStorage<int32_t>* GetComboValueStorage(int32_t overrideValue = -1);
	int32_t GetComboValue();
// 	int32_t GetComboValueInitial(int32_t overrideValue = -1);
	const char* GetComboValueName(int32_t overrideValue = -1);
	int32_t GetMin();
	int32_t GetMax();
};

struct AutoCVar_Float : AutoCVar<double>
{
	AutoCVar_Float(const char* name, const char* description, double defaultValue, CVarFlags flags = CVarFlags::None);
	AutoCVar_Float(const char* name, const char* description, double defaultValue, double minValue, double maxValue, CVarFlags flags = CVarFlags::None);
	AutoCVar_Float(const char* name);

	double Get();
	double* GetPtr();
	void Set(double value);
	float GetFloat();
	float* GetFloatPtr();
	double GetMin();
	double GetMax();
};

struct AutoCVar_String : AutoCVar<std::string>
{
	AutoCVar_String(const char* name, const char* description, const char* defaultValue, CVarFlags flags = CVarFlags::None);
	AutoCVar_String(const char* name);

	const char* Get();
	void Set(std::string&& value);
};