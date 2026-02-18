#pragma once

#include <vector>

namespace application::file::game::phive::util
{
	namespace PhiveMaterialData
	{
		extern std::vector<const char*> gMaterialNames;

		extern std::vector<const char*> gMaterialFlagNames;
		extern std::vector<uint64_t> gMaterialFlagMasks;

		extern std::vector<const char*> gMaterialFilterNames;
		extern std::vector<uint64_t> gMaterialFilterMasks;
		extern std::vector<bool> gMaterialFilterIsBase;

		struct PhiveMaterial
		{
			uint32_t mMaterialId = 0; //Default: Undefined -> gMaterialNames[0]
			bool mFlags[64] = { false };
			bool mCollisionFlags[67] = { false };

			uint32_t mUnknown0 = 0; //Maybe sub material or smth?
		};
	}
}