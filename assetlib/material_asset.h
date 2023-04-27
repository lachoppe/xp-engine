#pragma once

#include <unordered_map>
#include "asset_core.h"


namespace assets
{
	enum class TransparencyMode :uint8_t
	{
		Opaque = 0,
		Transparent,
		Masked
	};

	struct MaterialInfo
	{
		std::string baseEffect;
		std::unordered_map<std::string, std::string> textures;
		std::unordered_map<std::string, std::string> customProps;
		TransparencyMode transparency;
	};

	MaterialInfo ReadMaterialInfo(AssetFile* asset);
	AssetFile PackMaterial(MaterialInfo* info);
}