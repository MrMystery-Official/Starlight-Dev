#pragma once

#include <file/game/phive/classes/HavokDataHolder.h>
#include <util/BinaryVectorReader.h>
#include <vector>
#include <string>

namespace application::file::game::phive::classes
{
	class HavokClassBinaryVectorReader : public application::util::BinaryVectorReader
	{
	public:
		HavokClassBinaryVectorReader(std::vector<unsigned char> Bytes, bool BigEndian = false) : BinaryVectorReader(Bytes, BigEndian) {}

		void ReadRawArray(void* Ptr, unsigned int ArraySizeInBytes, unsigned int ArrayCount, bool IsPrimitive);
		int GetSectionOffset(const std::string& Magic, unsigned long long StartPos = 0);
		void SeekToSection(const std::string& Magic, unsigned long long StartPos = 0);
		void ReadStringTable(std::vector<std::string>& Dest);

		HavokDataHolder* mDataHolder = nullptr;
	};
}