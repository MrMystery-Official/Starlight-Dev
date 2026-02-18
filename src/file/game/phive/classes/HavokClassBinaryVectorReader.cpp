#include "HavokClassBinaryVectorReader.h"

#include <file/game/phive/classes/HavokClasses.h>

namespace application::file::game::phive::classes
{
	void HavokClassBinaryVectorReader::ReadRawArray(void* Ptr, unsigned int ArraySizeInBytes, unsigned int ArrayCount, bool IsPrimitive)
	{
		if (IsPrimitive)
		{
			ReadStruct(Ptr, ArraySizeInBytes);
			return;
		}

		unsigned int ElementSize = ArraySizeInBytes / ArrayCount;

		for (unsigned int i = 0; i < ArrayCount; i++)
		{
			void* ElementPointer = static_cast<char*>(Ptr) + i * ElementSize;
			application::file::game::phive::classes::HavokClasses::hkReadableWriteableObject* Object = static_cast<application::file::game::phive::classes::HavokClasses::hkReadableWriteableObject*>(ElementPointer);
			
			Object->Read(*this);
		}		
	}

	int HavokClassBinaryVectorReader::GetSectionOffset(const std::string& Magic, unsigned long long StartPos)
	{
		Seek(StartPos, application::util::BinaryVectorReader::Position::Begin);
		char Data[4] = { 0 };
		while (!(Data[0] == Magic.at(0) && Data[1] == Magic.at(1) && Data[2] == Magic.at(2) && Data[3] == Magic.at(3)))
		{
			ReadStruct(Data, 4);
		}
		return GetPosition() - 8; //-8 = Flags(1) - Size(3) - Magic(4)
	}

	void HavokClassBinaryVectorReader::SeekToSection(const std::string& Magic, unsigned long long StartPos)
	{
		Seek(GetSectionOffset(Magic, StartPos), application::util::BinaryVectorReader::Position::Begin);
	}

	void HavokClassBinaryVectorReader::ReadStringTable(std::vector<std::string>& Dest)
	{
		uint32_t Size = bswap_32(ReadUInt32()) & 0x3FFFFFFF;
		Seek(4, application::util::BinaryVectorReader::Position::Current); //Magic
		Size = Size - 8; //Flags(1) - Size(3) - Magic(4)

		std::string Buffer;
		for (uint32_t i = 0; i < Size; i++)
		{
			char Character = ReadUInt8();
			if (Character == 0xFF)
				continue;

			if (Character == 0x00)
			{
				Dest.push_back(Buffer);
				Buffer.clear();
				continue;
			}

			Buffer += Character;
		}
	}
}