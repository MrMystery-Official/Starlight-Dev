#pragma once

#include <vector>
#include <utility>
#include <util/BinaryVectorReader.h>
#include <util/BinaryVectorWriter.h>

namespace application::file::game::phive::util
{
	class PhiveWrapper
	{
	public:

		struct PhiveWrapperItem
		{
			uint16_t mTypeIndex;
			uint16_t mFlags;
			uint32_t mDataOffset;
			uint32_t mCount;
		};

		struct PhiveWrapperTypeTemplate
		{
			std::string mName = "";

			uint8_t mIndex;
			uint8_t mValue;
		};

		struct PhiveWrapperNamedType
		{
			std::string mName = "";

			uint8_t mIndex;
			uint8_t mTemplateCount;

			std::vector<PhiveWrapperTypeTemplate> mTemplates;
		};

		struct PhiveWrapperPatch
		{
			uint32_t mTypeIndex;
			uint32_t mCount;
			std::vector<uint32_t> mOffsets;
		};

		bool mEnablePatches;

		std::vector<PhiveWrapperItem> mItems;
		std::vector<std::string> mTypeStringTable;
		std::vector<std::string> mFieldStringTable;
		std::vector<PhiveWrapperNamedType> mTypes;
		std::vector<PhiveWrapperPatch> mPatches;
		uint16_t mTypePointerPaddingSize = 0;
		uint32_t mTypeSize = 0; //Big Endian

		std::vector<unsigned char> mConstantBlock; //I am too lazy to decode that right now
		std::vector<unsigned char> mHavokTagFile;

		uint32_t FindSection(application::util::BinaryVectorReader Reader, std::string Section);
		uint16_t GetTypeNameIndex(std::string Name);
		uint8_t GetTypeNameFlag(std::string Name);
		std::pair<uint8_t, uint32_t> ReadBitFieldFlagsSize(uint32_t Input);
		uint32_t WriteBitFieldFlagsSize(uint8_t Flags, uint32_t Size);
		void WriteStringTable(application::util::BinaryVectorWriter& Writer, std::string SectionMagic, std::vector<std::string>& StringTable);
		void ReadStringTable(application::util::BinaryVectorReader& Reader, std::string SectionMagic, std::vector<std::string>& StringTable);

		std::vector<unsigned char> ToBinary();

		void Initialize(const std::vector<unsigned char>& Bytes, bool EnablePatches);

		PhiveWrapper(const std::vector<unsigned char>& Bytes, bool EnablePatches);
		PhiveWrapper() = default;
	};
}