#pragma once

#include <vector>
#include <cstdint>

namespace application::file::game::phive::classes
{
	class HavokDataHolder
	{
	public:
		struct Item
		{
			uint16_t mTypeIndex = 0;
			uint8_t mFlags = 0;
			uint32_t mDataOffset = 0;
			uint32_t mCount = 0;

			Item() = default;
			Item(uint16_t TypeIndex, uint8_t Flags, uint32_t DataOffset, uint32_t Count) : mTypeIndex(TypeIndex), mFlags(Flags), mDataOffset(DataOffset), mCount(Count) {}
		};

		struct Patch
		{
			uint32_t mTypeIndex;
			uint32_t mCount;
			std::vector<uint32_t> mOffsets;
		};

		std::vector<Item>& GetItems();
		std::vector<Patch>& GetPatches();
		void AddPatch(uint32_t TypeIndex, uint32_t Offset);

	private:
		std::vector<Item> mItems;
		std::vector<Patch> mPatches;
	};
}