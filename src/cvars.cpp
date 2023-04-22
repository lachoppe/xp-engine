#include "cvars.h"

#include <unordered_map>
#include <algorithm>
#include <sstream>
#include "imgui.h"
#include "imgui_internal.h"

#include "debug.h"

enum class CVarType : char
{
	INT,
	FLOAT,
	STRING,
};

static bool NameString(void* data, int n, const char** out_str)
{
	const char** names = reinterpret_cast<const char**>(data);
	*out_str = names[n];
	return true;
}

class CVarParameter
{
public:
	friend class CVarSystemImpl;

	int32_t arrayIndex{ -1 };

	CVarType type{ 0 };
	CVarFlags flags{ 0 };
	std::string name;
	std::string description;
 	bool (*comboFunc)(void* data, int n, const char** out_str) { NameString };
	std::vector<const char*> comboNames;
	std::vector<CVarParameter*> comboParams;

	CVarParameter()
	{
	}

	bool HasFlag(CVarFlags flag) const
	{
		return !!((uint32_t)flags & (uint32_t)flag);
	}
};

template<typename T>
struct CVarStorage
{
	T initial{ 0 };
	T current{ 0 };
	T min{ 0 };
	T max{ 0 };
	CVarParameter* parameter{ nullptr };
};

template<typename T>
struct CVarArray
{
	CVarStorage<T>* cvars;
	int32_t lastCVar{ 0 };

	CVarArray(size_t size)
	{
		cvars = new CVarStorage<T>[size]();
	}

	CVarStorage<T>* GetStorageForIndex(int32_t index)
	{
		return &cvars[index];
	}

	T GetCurrent(int32_t index)
	{
		return cvars[index].current;
	}

	T* GetCurrentPtr(int32_t index)
	{
		return &cvars[index].current;
	}

	void SetCurrent(int32_t index, const T& value)
	{
		cvars[index].current = value;
	}

	int AddEx(const T& initialValue, const T& currentValue, const T& minValue, const T& maxValue, CVarParameter* param)
	{
		int index = lastCVar;

		cvars[index].current = currentValue;
		cvars[index].initial = initialValue;
		cvars[index].min = minValue;
		cvars[index].max = maxValue;
		cvars[index].parameter = param;

		param->arrayIndex = index;
		lastCVar++;

		return index;
	}

	int Add(const T& initialValue, const T& currentValue, const T& minValue, const T& maxValue, CVarParameter* param)
	{
		return AddEx(initialValue, currentValue, minValue, maxValue, param);
	}

	int Add(const T& defaultValue, const T& currentValue, CVarParameter* param)
	{
		return AddEx( defaultValue, currentValue, defaultValue, defaultValue, param );
	}
};


class CVarSystemImpl : public CVarSystem
{
public:
	constexpr static int MAX_INT_CVARS = 100;
	CVarArray<int32_t> intCVars{ MAX_INT_CVARS };

	constexpr static int MAX_FLOAT_CVARS = 100;
	CVarArray<double> floatCVars{ MAX_FLOAT_CVARS };

	constexpr static int MAX_STRING_CVARS = 20;
	CVarArray<std::string> stringCVars{ MAX_STRING_CVARS };

	constexpr static int MAX_STRING_LEN = 128;

	template<typename T>
	CVarArray<T>* GetCVarArray();

	template<>
	CVarArray<int32_t>* GetCVarArray()
	{
		return &intCVars;
	}

	template<>
	CVarArray<double>* GetCVarArray()
	{
		return &floatCVars;
	}

	template<>
	CVarArray<std::string>* GetCVarArray()
	{
		return &stringCVars;
	}

	CVarParameter* GetCVar(const char* name) override final;
	CVarParameter* GetCVar(StringUtils::StringHash hash) override final;

	int32_t* GetIntCVar(StringUtils::StringHash hash) override final;
	double* GetFloatCVar(StringUtils::StringHash hash) override final;
	std::string* GetStringCVar(StringUtils::StringHash hash) override final;

	void SetIntCVar(StringUtils::StringHash hash, int32_t value) override final;
	void SetFloatCVar(StringUtils::StringHash hash, double value) override final;
	void SetStringCVar(StringUtils::StringHash hash, const char* value) override final;

	bool CreateIntCVar(const char* name, const char* description, int32_t defaultValue, int32_t currentValue, int32_t minValue, int32_t maxValue, CVarParameter** param) override final;
	CVarParameter* CreateFloatCVar(const char* name, const char* description, double defaultValue, double currentValue, double minValue, double maxValue) override final;
	CVarParameter* CreateStringCVar(const char* name, const char* description, const char* defaultValue, const char* currentValue) override final;

	void DrawImGuiEditor() override final;
	void EditParam(CVarParameter* param, float textWidth);

	template<typename T>
	int GetCVarIndex(uint32_t nameHash)
	{
		CVarParameter* param = GetCVar(nameHash);
		if (!param)
		{
			return -1;
		}
		else
		{
			return param->arrayIndex;
		}
	}

	template<typename T>
	T* GetCVarCurrent(uint32_t nameHash)
	{
		CVarParameter* param = GetCVar(nameHash);
		if (!param)
		{
			return nullptr;
		}
		else
		{
			return GetCVarArray<T>()->GetCurrentPtr(param->arrayIndex);
		}
	}

	template<typename T>
	void SetCVarCurrent(uint32_t nameHash, const T& value)
	{
		CVarParameter* param = GetCVar(nameHash);
		if (param)
		{
			return GetCVarArray<T>()->SetCurrent(param->arrayIndex, value);
		}
	}

private:
	// needs mutex

	bool InitCVar(const char* name, const char* description, CVarParameter** param);

	std::unordered_map<uint32_t, CVarParameter> savedCVars;
	std::vector<CVarParameter*> cachedEditParams;
};

CVarSystem* CVarSystem::Get()
{
	static CVarSystemImpl singleton{};
	return &singleton;
}

CVarParameter* CVarSystemImpl::GetCVar(const char* name)
{
	uint32_t hash = StringUtils::StringHash{ name };
	auto it = savedCVars.find(hash);

	if (it != savedCVars.end())
	{
		return &(*it).second;
	}

	return nullptr;
}

CVarParameter* CVarSystemImpl::GetCVar(StringUtils::StringHash hash)
{
	auto it = savedCVars.find(hash);

	if (it != savedCVars.end())
	{
		return &(*it).second;
	}

	return nullptr;
}

int32_t* CVarSystemImpl::GetIntCVar(StringUtils::StringHash hash)
{
	return GetCVarCurrent<int32_t>(hash);
}

double* CVarSystemImpl::GetFloatCVar(StringUtils::StringHash hash)
{
	return GetCVarCurrent<double>(hash);
}

std::string* CVarSystemImpl::GetStringCVar(StringUtils::StringHash hash)
{
	return GetCVarCurrent<std::string>(hash);
}

void CVarSystemImpl::SetIntCVar(StringUtils::StringHash hash, int32_t value)
{
	SetCVarCurrent<int32_t>(hash, value);
}

void CVarSystemImpl::SetFloatCVar(StringUtils::StringHash hash, double value)
{
	SetCVarCurrent<double>(hash, value);
}

void CVarSystemImpl::SetStringCVar(StringUtils::StringHash hash, const char* value)
{
	SetCVarCurrent<std::string>(hash, value);
}

bool CVarSystemImpl::InitCVar(const char* name, const char* description, CVarParameter** param)
{
	uint32_t nameHash = StringUtils::StringHash{ name };
	CVarParameter* oldParam = GetCVar(nameHash);
	if (oldParam)
	{
		*param = oldParam;
		return false;
	}

	savedCVars[nameHash] = CVarParameter{};

	CVarParameter& newParam = savedCVars[nameHash];

	newParam.name = name;
	newParam.description = description;

	*param = &newParam;
	return true;
}

bool CVarSystemImpl::CreateIntCVar(const char* name, const char* description, int32_t defaultValue, int32_t currentValue, int32_t minValue, int32_t maxValue, CVarParameter** param)
{
	// need mutex
	if (!InitCVar(name, description, param))
		return false;

	(*param)->type = CVarType::INT;
	GetCVarArray<int32_t>()->Add(defaultValue, currentValue, minValue, maxValue, *param);
	return true;
}

CVarParameter* CVarSystemImpl::CreateFloatCVar(const char* name, const char* description, double defaultValue, double currentValue, double minValue, double maxValue)
{
	// need mutex
	CVarParameter* param = nullptr;
	if (!InitCVar(name, description, &param))
		return param;
	param->type = CVarType::FLOAT;
	GetCVarArray<double>()->Add(defaultValue, currentValue, minValue, maxValue, param);
	return param;
}

CVarParameter* CVarSystemImpl::CreateStringCVar(const char* name, const char* description, const char* defaultValue, const char* currentValue)
{
	// need mutex
	CVarParameter* param = nullptr;
	if (!InitCVar(name, description, &param))
		return param;
	param->type = CVarType::STRING;
	GetCVarArray<std::string>()->Add(defaultValue, currentValue, param);
	GetCVarArray<std::string>()->GetCurrentPtr(param->arrayIndex)->reserve(MAX_STRING_LEN);
	return param;
}


template<typename T>
T GetCVarCurrentByIndex(int32_t index)
{
	return reinterpret_cast<CVarSystemImpl*>(CVarSystemImpl::Get())->GetCVarArray<T>()->GetCurrent(index);
}

template<typename T>
T* PtrGetCVarCurrentByIndex(int32_t index)
{
	return reinterpret_cast<CVarSystemImpl*>(CVarSystemImpl::Get())->GetCVarArray<T>()->GetCurrentPtr(index);
}

template<typename T>
CVarStorage<T>* GetCVarStorageByIndex(int32_t index)
{
	return reinterpret_cast<CVarSystemImpl*>(CVarSystemImpl::Get())->GetCVarArray<T>()->GetStorageForIndex(index);
}

template<typename T>
void SetCVarCurrentByIndex(int32_t index, const T& value)
{
	reinterpret_cast<CVarSystemImpl*>(CVarSystemImpl::Get())->GetCVarArray<T>()->SetCurrent(index, value);
}

template<typename T>
const char* AutoCVar<T>::GetName()
{
	if (!ApplyLookup()) return nullptr;
	return GetCVarStorageByIndex<CVarType>(index)->parameter->name.data();
}

template<typename T>
const char* AutoCVar<T>::GetDescription()
{
	if (!ApplyLookup()) return nullptr;
	return GetCVarStorageByIndex<CVarType>(index)->parameter->description.data();
}

template<typename T>
CVarStorage<T>* AutoCVar<T>::GetStorage()
{
	if (!ApplyLookup()) return nullptr;
	return GetCVarStorageByIndex<CVarType>(index);
}

template<typename T>
bool AutoCVar<T>::ApplyLookup()
{
	if (index >= 0)
		return true;

	CVarParameter* param = CVarSystemImpl::Get()->GetCVar(lookupName.c_str());
	WARN_M(param, "Lookup fail: '%s' doesn't exist", lookupName);
	if (param)
	{
		index = param->arrayIndex;
		lookupName.clear();
		return true;
	}

	return false;
}

AutoCVar_Int::AutoCVar_Int(const char* name, const char* description, int32_t defaultValue, CVarFlags flags)
{
	CVarParameter* param = nullptr;
	if (!CVarSystemImpl::Get()->CreateIntCVar(name, description, defaultValue, defaultValue, 0, 0, &param))
		return;

	// Strip out the HasRange flag in case it was manually set
	param->flags = static_cast<CVarFlags>((uint32_t)flags & ~((uint32_t)CVarFlags::_HasRange));
	index = param->arrayIndex;
}

AutoCVar_Int::AutoCVar_Int(const char* name, const char* description, int32_t defaultValue, int32_t minValue, int32_t maxValue, CVarFlags flags, bool (*comboFunction)(void* data, int n, const char** out_str))
{
	CVarParameter* param = nullptr;
	if (CVarSystemImpl::Get()->CreateIntCVar(name, description, defaultValue, defaultValue, minValue, maxValue, &param))
	{
		// First instantiation (not a reference)
		param->flags = static_cast<CVarFlags>((uint32_t)flags | (uint32_t)CVarFlags::_HasRange);
		param->comboFunc = comboFunction ? comboFunction : NameString;

		if (param->HasFlag(CVarFlags::EditCombo))
		{
			// ImGui can't use const char* here
 			static char* unknownName = "(missing)";

			// Find combo names
			param->comboParams.clear();
			for (int i = minValue; i <= maxValue; ++i)
			{
				std::ostringstream str;
				str << name << "_" << i;
				CVarParameter* optParam = CVarSystemImpl::Get()->GetCVar(str.str().c_str());
				param->comboParams.push_back(optParam);
				if (optParam)
 					param->comboNames.push_back(optParam->description.data());
				else
					param->comboNames.push_back(nullptr);
			}
		}
	}

	index = param->arrayIndex;
}

AutoCVar_Int::AutoCVar_Int(const char* name)
{
	lookupName = name;
	index = -1;
}

int32_t AutoCVar_Int::Get()
{
	if (!ApplyLookup()) return -1;
	return GetCVarCurrentByIndex<CVarType>(index);
}

int32_t* AutoCVar_Int::GetPtr()
{
	if (!ApplyLookup()) return nullptr;
	return PtrGetCVarCurrentByIndex<CVarType>(index);
}

void AutoCVar_Int::Set(int32_t value)
{
	if (!ApplyLookup()) return;
	SetCVarCurrentByIndex<CVarType>(index, value);
}

void AutoCVar_Int::Toggle()
{
	bool enabled = Get() != 0;
	Set(enabled ? 0 : 1);
}

CVarParameter* AutoCVar_Int::GetComboValueParam(int32_t overrideValue)
{
	const CVarStorage<int32_t>* storage = GetStorage();
	const CVarParameter* param = storage->parameter;

	int32_t value = -1;

	if (param->HasFlag(CVarFlags::EditCombo))
	{
		int comboIndex = (overrideValue >= 0) ? overrideValue : Get();

		if (comboIndex >= 0 || comboIndex < param->comboParams.size())
			return param->comboParams[comboIndex];
	}
	return nullptr;
}

// CVarStorage<int32_t>* AutoCVar_Int::GetComboValueStorage(int32_t overrideValue)
// {
// 	const CVarParameter* comboParam = GetComboValueParam(overrideValue);
// 	if (comboParam)
// 		return GetCVarStorageByIndex<int32_t>(comboParam->arrayIndex);
// 	return nullptr;
// }

int32_t AutoCVar_Int::GetComboValue()
{
	const CVarParameter* comboParam = GetComboValueParam();
	if (comboParam)
		return GetCVarCurrentByIndex<int32_t>(comboParam->arrayIndex);
	return -1;
}

// int32_t AutoCVar_Int::GetComboValueInitial(int32_t overrideValue)
// {
// 	const CVarParameter* comboParam = GetComboValueParam(overrideValue);
// 	if (comboParam)
// 		return GetCVarStorageByIndex<int32_t>(comboParam->arrayIndex)->initial;
// 	return -1;
// }

const char* AutoCVar_Int::GetComboValueName(int32_t overrideValue)
{
	std::vector<const char*>& names = GetStorage()->parameter->comboNames;
	int32_t mode = (overrideValue >= 0) ? overrideValue : Get();
	if (mode >= 0 && mode < names.size())
		return names[mode];
	return nullptr;
}

int32_t AutoCVar_Int::GetMin()
{
	if (!ApplyLookup()) return static_cast<CVarType>(0);
	return GetStorage()->min;
}

int32_t AutoCVar_Int::GetMax()
{
	if (!ApplyLookup()) return static_cast<CVarType>(0);
	return GetStorage()->max;
}



AutoCVar_Float::AutoCVar_Float(const char* name, const char* description, double defaultValue, CVarFlags flags)
{
	CVarParameter* param = CVarSystemImpl::Get()->CreateFloatCVar(name, description, defaultValue, defaultValue, 0.0, 0.0);
	// Strip out the HasRange flag in case it was manually set
	param->flags = static_cast<CVarFlags>((uint32_t)flags & ~((uint32_t)CVarFlags::_HasRange));
	index = param->arrayIndex;
}

AutoCVar_Float::AutoCVar_Float(const char* name, const char* description, double defaultValue, double minValue, double maxValue, CVarFlags flags)
{
	CVarParameter* param = CVarSystemImpl::Get()->CreateFloatCVar(name, description, defaultValue, defaultValue, minValue, maxValue);
	param->flags = static_cast<CVarFlags>((uint32_t)flags | (uint32_t)CVarFlags::_HasRange);
	index = param->arrayIndex;
}

AutoCVar_Float::AutoCVar_Float(const char* name)
{
	lookupName = name;
	index = -1;
}

double AutoCVar_Float::Get()
{
	if (!ApplyLookup()) return 0.0;
	return GetCVarCurrentByIndex<CVarType>(index);
}

double* AutoCVar_Float::GetPtr()
{
	if (!ApplyLookup()) return nullptr;
	return PtrGetCVarCurrentByIndex<CVarType>(index);
}

float AutoCVar_Float::GetFloat()
{
	if (!ApplyLookup()) return 0.0f;
	return static_cast<float>(Get());
}

float* AutoCVar_Float::GetFloatPtr()
{
	if (!ApplyLookup()) return nullptr;
	float* value = reinterpret_cast<float*>(GetPtr());
	return value;
}

void AutoCVar_Float::Set(double value)
{
	if (!ApplyLookup()) return;
	SetCVarCurrentByIndex<CVarType>(index, value);
}

double AutoCVar_Float::GetMin()
{
	if (!ApplyLookup()) return static_cast<CVarType>(0);
	return GetStorage()->min;
}

double AutoCVar_Float::GetMax()
{
	if (!ApplyLookup()) return static_cast<CVarType>(0);
	return GetStorage()->max;
}


AutoCVar_String::AutoCVar_String(const char* name, const char* description, const char* defaultValue, CVarFlags flags)
{
	CVarParameter* param = CVarSystemImpl::Get()->CreateStringCVar(name, description, defaultValue, defaultValue);
	param->flags = flags;
	index = param->arrayIndex;
}

AutoCVar_String::AutoCVar_String(const char* name)
{
	lookupName = name;
	index = -1;
}

const char* AutoCVar_String::Get()
{
	if (!ApplyLookup()) return nullptr;
	return PtrGetCVarCurrentByIndex<CVarType>(index)->data();
}

void AutoCVar_String::Set(std::string&& value)
{
	if (!ApplyLookup()) return;
	SetCVarCurrentByIndex<CVarType>(index, value);
}


void CVarSystemImpl::DrawImGuiEditor()
{
	static char searchText[MAX_STRING_LEN] = "";

	ImGui::InputText("Filter", searchText, MAX_STRING_LEN);
	const char* advancedText = "Advanced";
	ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(advancedText).x - 40);
	static bool showAdvanced = false;
	ImGui::Checkbox(advancedText, &showAdvanced);
	ImGui::Separator();
	cachedEditParams.clear();

	auto addToEditList = [&](auto param)
	{
		bool isHidden = param->HasFlag(CVarFlags::NoEdit);
		bool isAdvanced = param->HasFlag(CVarFlags::Advanced);

		if (!isHidden)
		{
			if (!(!showAdvanced && isAdvanced) && param->name.find(searchText) != std::string::npos)
			{
				cachedEditParams.push_back(param);
			}
		}
	};

	for (int i = 0; i < GetCVarArray<int32_t>()->lastCVar; ++i)
		addToEditList(GetCVarArray<int32_t>()->cvars[i].parameter);

	for (int i = 0; i < GetCVarArray<double>()->lastCVar; ++i)
		addToEditList(GetCVarArray<double>()->cvars[i].parameter);

	for (int i = 0; i < GetCVarArray<std::string>()->lastCVar; ++i)
		addToEditList(GetCVarArray<std::string>()->cvars[i].parameter);

	// Just alphabetical sort for now
	{
		std::sort(cachedEditParams.begin(), cachedEditParams.end(), [](CVarParameter* A, CVarParameter* B)
			{
				return A->name < B->name;
			});

		float maxTextWidth = 0;
		for (auto p : cachedEditParams)
			maxTextWidth = std::max(maxTextWidth, ImGui::CalcTextSize(p->name.c_str()).x);

		for (auto p : cachedEditParams)
			EditParam(p, maxTextWidth);
	}
}


void Label(const char* label, float textWidth)
{
	constexpr float Slack = 50;
	constexpr float EditorWidth = 250;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const ImGuiStyle& style = ImGui::GetStyle();
	float fullWidth = textWidth + Slack;

	ImVec2 textSize = ImGui::CalcTextSize(label);
	ImVec2 startPos = ImGui::GetCursorScreenPos();
	ImVec2 finalPos = { startPos.x + fullWidth, startPos.y };

	ImGui::Text(label);
	ImGui::SameLine();
	ImGui::SetCursorScreenPos(finalPos);
	ImGui::SetNextItemWidth(EditorWidth);
}


void CVarSystemImpl::EditParam(CVarParameter* p, float textWidth)
{
	const bool readOnlyFlag = ((uint32_t)p->flags) & (uint32_t)CVarFlags::EditReadOnly;
	const bool checkboxFlag = ((uint32_t)p->flags) & (uint32_t)CVarFlags::EditCheckbox;
	const bool dragFlag = ((uint32_t)p->flags) & (uint32_t)CVarFlags::EditFloatDrag;
	const bool comboFlag = ((uint32_t)p->flags) & (uint32_t)CVarFlags::EditCombo;
	const bool hasRange = (uint32_t)p->flags & (uint32_t)CVarFlags::_HasRange;

	switch (p->type)
	{
	case CVarType::INT:
		if (readOnlyFlag)
		{
			std::string fmt = p->name + " = %i";
			ImGui::Text(fmt.c_str(), GetCVarArray<int32_t>()->GetCurrent(p->arrayIndex));
		}
		else
		{
			if (checkboxFlag)
			{
				bool checkbox = GetCVarArray<int32_t>()->GetCurrent(p->arrayIndex) != 0;

				Label(p->name.c_str(), textWidth);
				ImGui::PushID(p->name.c_str());

				if (ImGui::Checkbox("", &checkbox))
					GetCVarArray<int32_t>()->SetCurrent(p->arrayIndex, checkbox ? 1 : 0);
				
				ImGui::PopID();
			}
			else if (comboFlag)
			{
				Label(p->name.c_str(), textWidth);
				ImGui::PushID(p->name.c_str());
				int* value = PtrGetCVarCurrentByIndex<int32_t>(p->arrayIndex);
				if (*value < 0)
					*value = 0;
				ImGui::Combo("", value, p->comboFunc, p->comboNames.data(), static_cast<int>(p->comboNames.size()));
				ImGui::PopID();
			}
			else
			{
				Label(p->name.c_str(), textWidth);
				ImGui::PushID(p->name.c_str());
				ImGui::InputInt("", GetCVarArray<int32_t>()->GetCurrentPtr(p->arrayIndex));
				ImGui::PopID();
			}
		}
		break;
	case CVarType::FLOAT:
		if (readOnlyFlag)
		{
			std::string fmt = p->name + " = %f";
			ImGui::Text(fmt.c_str(), GetCVarArray<double>()->GetCurrent(p->arrayIndex));
		}
		else
		{
			Label(p->name.c_str(), textWidth);
			ImGui::PushID(p->name.c_str());

			static double defaultMin = 0.0;
			static double defaultMax = 1.0;
			double *min = &defaultMin;
			double *max = &defaultMax;
			double step = 0.0;
			double stepFast = 0.0;
			if (hasRange)
			{
				CVarStorage<double>* storage = GetCVarArray<double>()->GetStorageForIndex(p->arrayIndex);
				min = &storage->min;
				max = &storage->max;
				step = (max - min) * 0.01;
				stepFast = (max - min) * 0.1;
			}

			if (dragFlag)
			{
				ImGuiSliderFlags flags = hasRange ? ImGuiSliderFlags_None : ImGuiSliderFlags_AlwaysClamp;
				ImGui::SliderScalar("", ImGuiDataType_Double, GetCVarArray<double>()->GetCurrentPtr(p->arrayIndex), min, max, "%.3f", flags);
			}
			else
			{
				ImGui::InputDouble("", GetCVarArray<double>()->GetCurrentPtr(p->arrayIndex), step, stepFast, "%.3f");
			}
			ImGui::PopID();

		}
		break;
	case CVarType::STRING:
		if (readOnlyFlag)
		{
			std::string fmt = p->name + " = %s";
			ImGui::Text(fmt.c_str(), GetCVarArray<std::string>()->GetCurrent(p->arrayIndex));
		}
		else
		{
			ImGui::PushID(p->name.c_str());
			std::string* string = GetCVarArray<std::string>()->GetCurrentPtr(p->arrayIndex);
			ImGui::InputText("", string->data(), MAX_STRING_LEN);
			ImGui::PopID();
		}
		break;
	default:
		break;
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(p->description.c_str());
	}
}
