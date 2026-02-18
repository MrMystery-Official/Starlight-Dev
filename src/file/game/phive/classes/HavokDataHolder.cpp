#include "HavokDataHolder.h"

namespace application::file::game::phive::classes
{
	std::vector<HavokDataHolder::Item>& HavokDataHolder::GetItems()
	{
		return mItems;
	}

	std::vector<HavokDataHolder::Patch>& HavokDataHolder::GetPatches()
	{
		return mPatches;
	}

	void HavokDataHolder::AddPatch(uint32_t TypeIndex, uint32_t Offset)
	{
		for(Patch& ExistingPatch : mPatches)
		{
			if (ExistingPatch.mTypeIndex == TypeIndex)
			{
				ExistingPatch.mOffsets.push_back(Offset);
				ExistingPatch.mCount++;
				return;
			}
		}

		Patch NewPatch;
		NewPatch.mTypeIndex = TypeIndex;
		NewPatch.mCount = 1;
		NewPatch.mOffsets.push_back(Offset);

		mPatches.push_back(NewPatch);
	}
}