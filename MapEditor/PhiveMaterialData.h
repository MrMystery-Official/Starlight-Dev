#pragma once

#include <vector>

namespace PhiveMaterialData
{
	extern std::vector<const char*> mMaterialNames;

	extern std::vector<const char*> mMaterialFlagNames;
	extern std::vector<uint64_t> mMaterialFlagMasks;

	extern std::vector<const char*> mMaterialFilterNames;
	extern std::vector<uint64_t> mMaterialFilterMasks;
	extern std::vector<bool> mMaterialFilterIsBase;

	struct Material
	{
		uint32_t mMaterialId;
		bool mFlags[64] = { false };
		bool mCollisionFlags[67] = { false };

		uint32_t mUnknown0 = 0; //Maybe sub material or smth?
	};
}