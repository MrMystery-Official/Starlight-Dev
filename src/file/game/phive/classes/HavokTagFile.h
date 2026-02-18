#pragma once

#include <vector>
#include <file/game/phive/classes/HavokClasses.h>
#include <file/game/phive/classes/HavokDataHolder.h>
#include <util/Logger.h>
#include <algorithm>

namespace application::file::game::phive::classes
{
	template <typename T>
	struct HavokTagFile
	{
		struct Size 
		{
			unsigned int mData = 0;

			Size() = default;
			Size(int Data) : mData(Data) {}

			unsigned int GetSize()
			{
				return bswap_32(mData) & 0x3FFFFFFF;
			}

			unsigned char GetFlags()
			{
				return bswap_32(mData) >> 30;
			}

			void SetSize(unsigned int NewSize)
			{
				NewSize &= 0x3FFFFFFF;
				unsigned char Flags = GetFlags();

				uint32_t Combined = (static_cast<uint32_t>(Flags) << 30) | NewSize;
				mData = bswap_32(Combined);
			}
		};

		struct Header
		{
			Size mTagFileSize;
			char mTagFileMagic[4];

			Size mSDKVSize;
			char mSDKVMagic[4];
			char mSDKVValue[8];

			Size mDataSize;
			char mDataMagic[4];
		};

		struct NamedType
		{
			struct Template
			{
				std::string mName = "";
				std::string mValueString = "";

				uint8_t mIndex;
				uint8_t mValue;
			};

			std::string mName = "";

			uint8_t mIndex;
			uint8_t mTemplateCount;

			std::vector<Template> mTemplates;
		};

		Header mHeader;
		T mRootClass;
		bool mEnablePatches = false;

		std::vector<unsigned char> mConstantBlock;
		std::vector<std::string> mTypeStringTable;
		std::vector<std::string> mFieldStringTable;
		std::vector<NamedType> mTypes;
		HavokDataHolder mDataHolder;

		uint32_t mLastDataOffset = 0; //Only when patching, after terminator
		uint32_t mUnkBlock1AbsoluteOffset = 0;
		uint32_t mUnkBlock1AbsoluteSize = 0;
		std::vector<unsigned char> mUnkBlock1;

		void InitializeItemRegistration()
		{
			mDataHolder.GetItems().clear();
		}

		uint32_t RegisterItem(uint16_t TypeIndex, uint8_t Flags, uint32_t DataOffset, uint32_t Count)
		{
			mDataHolder.GetItems().emplace_back(TypeIndex, Flags, DataOffset, Count);

			return mDataHolder.GetItems().size() - 1;
		}

		void LoadFromBytes(const std::vector<unsigned char>& Bytes, bool EnablePatches)
		{
			mEnablePatches = EnablePatches;

			application::file::game::phive::classes::HavokClassBinaryVectorReader Reader(Bytes);

			Reader.ReadStruct(&mHeader, sizeof(Header));

			std::vector<unsigned char> DataSection(mHeader.mDataSize.GetSize() - 8);
			Reader.Seek(32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
			Reader.ReadStruct(DataSection.data(), DataSection.size());

			uint32_t DataSectionEnd = Reader.GetPosition();

			Reader.SeekToSection("TYPE", DataSectionEnd);
			Size TypeSectionSize(Reader.ReadUInt32());
			mConstantBlock.resize(TypeSectionSize.GetSize());
			Reader.Seek(-4, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Current);
			Reader.ReadStruct(mConstantBlock.data(), mConstantBlock.size());

			if (EnablePatches)
			{
				Reader.Seek(mUnkBlock1AbsoluteOffset, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
				mUnkBlock1.resize(mUnkBlock1AbsoluteSize);
				Reader.ReadStruct(mUnkBlock1.data(), mUnkBlock1.size());
			}

			Reader.SeekToSection("TST1", DataSectionEnd);
			Reader.ReadStringTable(mTypeStringTable);

			Reader.SeekToSection("FST1", DataSectionEnd);
			uint32_t FieldStringTableOffset = Reader.GetPosition();
			Reader.ReadStringTable(mFieldStringTable);

			Reader.SeekToSection("TNA1", DataSectionEnd);
			Reader.Seek(8, application::util::BinaryVectorReader::Position::Current);
			uint8_t TypeCount = Reader.ReadUInt8() - 1;
			mTypes.resize(TypeCount);
			for (uint8_t i = 0; i < TypeCount; i++)
			{
				mTypes[i].mIndex = Reader.ReadUInt8();
				mTypes[i].mTemplateCount = Reader.ReadUInt8();
				mTypes[i].mTemplates.resize(mTypes[i].mTemplateCount);

				uint32_t Index = std::max((uint8_t)0, (uint8_t)mTypes[i].mIndex);

				mTypes[i].mName = mTypeStringTable.size() > Index ? mTypeStringTable[Index] : mTypeStringTable.front(); //TODO, mIndex too big for string table

				for (int8_t j = 0; j < mTypes[i].mTemplateCount; j++)
				{
					mTypes[i].mTemplates[j].mIndex = Reader.ReadUInt8();
					mTypes[i].mTemplates[j].mValue = Reader.ReadUInt8();

					mTypes[i].mTemplates[j].mName = mTypeStringTable[mTypes[i].mTemplates[j].mIndex];

					if (mTypes[i].mTemplates[j].mName[0] == 't')
					{
						mTypes[i].mTemplates[j].mValueString = mTypeStringTable.size() > mTypes[i].mTemplates[j].mValue ? mTypeStringTable[mTypes[i].mTemplates[j].mValue] : mTypeStringTable.front();
					}
				}
			}

			Reader.SeekToSection("ITEM", FieldStringTableOffset);
			uint32_t ItemSectionOffset = Reader.GetPosition();
			Size ItemSectionSize(Reader.ReadUInt32());
			uint32_t ItemCount = (ItemSectionSize.GetSize() - 8) / 12;
			Reader.Seek(4, application::util::BinaryVectorReader::Position::Current);
			mDataHolder.GetItems().resize(ItemCount);
			for (size_t i = 0; i < mDataHolder.GetItems().size(); i++)
			{
				HavokDataHolder::Item& HavokItem = mDataHolder.GetItems()[i];
				HavokItem.mTypeIndex = Reader.ReadUInt16();
				Reader.ReadUInt8();
				HavokItem.mFlags = Reader.ReadUInt8();
				HavokItem.mDataOffset = Reader.ReadUInt32();
				HavokItem.mCount = Reader.ReadUInt32();
			}

			if (EnablePatches)
			{
				Reader.SeekToSection("PTCH", ItemSectionOffset);
				Size PatchSectionSize(Reader.ReadUInt32());
				Reader.Seek(4, application::util::BinaryVectorReader::Position::Current);

				uint32_t PatchEnd = Reader.GetSectionOffset("PTCH") + PatchSectionSize.GetSize();

				while (Reader.GetPosition() < PatchEnd)
				{
					HavokDataHolder::Patch NewPatch;
					NewPatch.mTypeIndex = Reader.ReadUInt32();
					NewPatch.mCount = Reader.ReadUInt32();
					NewPatch.mOffsets.resize(NewPatch.mCount);
					for (uint32_t& Offset : NewPatch.mOffsets)
					{
						Offset = Reader.ReadUInt32();
					}
					mDataHolder.GetPatches().push_back(NewPatch);
				}
			}

			mRootClass = application::file::game::phive::classes::HavokClasses::DecodeHavokTagFileWithRoot<T>(DataSection, mDataHolder);
		}

		void PrintItems()
		{
			application::util::Logger::Info("HavokTagFile", "--- Item List : Begin ---");
			application::util::Logger::Info("HavokTagFile", "   mTypeStringTable.front(): \"%s\"", mTypeStringTable.front().c_str());
			application::util::Logger::Info("HavokTagFile", "   mTypeStringTable.back(): \"%s\"", mTypeStringTable.back().c_str());
			for (HavokDataHolder::Item& HavokItem : mDataHolder.GetItems())
			{
				std::string TypeName = mTypes[HavokItem.mTypeIndex].mName;
				if (mTypes[HavokItem.mTypeIndex].mTemplateCount) TypeName += "<";
				for (int8_t i = 0; i < mTypes[HavokItem.mTypeIndex].mTemplateCount; i++)
				{
					TypeName += mTypes[HavokItem.mTypeIndex].mTemplates[i].mValueString + ", ";
				}
				if (mTypes[HavokItem.mTypeIndex].mTemplateCount) TypeName += ">";

				application::util::Logger::Info("HavokTagFile", "   Item: TypeName: \"%s\", Flags: %u, DataOffset: %u, Count: %u", TypeName.c_str(), HavokItem.mFlags, HavokItem.mDataOffset, HavokItem.mCount);
			}
			application::util::Logger::Info("HavokTagFile", "--- Item List : End ---");
		}

		void ToBinary(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer)
		{
			uint32_t HeaderOffset = Writer.GetPosition();
			Writer.Seek(32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Current);
			application::file::game::phive::classes::HavokClasses::EncodeHavokTagFileWithRoot<T>(Writer, mRootClass);
			Writer.Align(16);
			uint32_t DataSectionEndOffset = Writer.GetPosition();

			Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mConstantBlock.data()), mConstantBlock.size());
			Writer.Align(4);

			unsigned long long IndexSizeOffset = Writer.GetPosition();
			Writer.WriteInteger(0, sizeof(uint32_t));
			Writer.WriteBytes("INDX");
			unsigned long long ItemSizeOffset = Writer.GetPosition();
			Writer.WriteInteger(0, sizeof(uint32_t));
			Writer.WriteBytes("ITEM");

			for (HavokDataHolder::Item& HavokItem : mDataHolder.GetItems())
			{
				Writer.WriteInteger(HavokItem.mTypeIndex, sizeof(uint16_t));
				Writer.WriteByte(0x00);
				Writer.WriteInteger(HavokItem.mFlags, sizeof(uint8_t));
				Writer.WriteInteger(HavokItem.mDataOffset, sizeof(uint32_t));
				Writer.WriteInteger(HavokItem.mCount, sizeof(uint32_t));
			}

			uint32_t ItemSectionEndOffset = Writer.GetPosition();
			uint32_t EndTagFile = Writer.GetPosition();

			if (mEnablePatches)
			{
				Writer.Align(4);
				unsigned long long PatchSizeOffset = Writer.GetPosition();
				Writer.WriteInteger(0, sizeof(uint32_t));
				Writer.WriteBytes("PTCH");

				std::sort(mDataHolder.GetPatches().begin(), mDataHolder.GetPatches().end(),
					[](const HavokDataHolder::Patch& a, const HavokDataHolder::Patch& b) {
						return a.mTypeIndex < b.mTypeIndex;
					});

				for (HavokDataHolder::Patch& Patch : mDataHolder.GetPatches())
				{
					Writer.WriteInteger(Patch.mTypeIndex, sizeof(uint32_t));
					Writer.WriteInteger(Patch.mCount, sizeof(uint32_t));

					for (uint32_t& Offset : Patch.mOffsets)
					{
						Writer.WriteInteger(Offset, sizeof(uint32_t));
					}
				}
				uint32_t PatchEnd = Writer.GetPosition();
				EndTagFile = Writer.GetPosition();

				Writer.Seek(PatchSizeOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
				Size PatchSize(0x40000074);
				PatchSize.SetSize(PatchEnd - PatchSizeOffset);
				Writer.WriteInteger(PatchSize.mData, sizeof(uint32_t));

				Writer.Seek(PatchEnd, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
				Writer.WriteInteger(0, sizeof(uint32_t)); //Terminator

				if (!mUnkBlock1.empty())
				{
					Writer.Align(8);
					mLastDataOffset = Writer.GetPosition();
					Writer.WriteInteger(0, sizeof(uint32_t));
					Writer.WriteInteger(0x3F800000, sizeof(uint32_t));
					Writer.Seek(16, application::util::BinaryVectorWriter::Position::Current);
					Writer.WriteInteger(0x3F800000, sizeof(uint32_t));
					Writer.Seek(11, application::util::BinaryVectorWriter::Position::Current);
					Writer.WriteByte(0x80);
					Writer.Seek(4, application::util::BinaryVectorWriter::Position::Current);
					Writer.WriteInteger(0x3F800000, sizeof(uint32_t));
					Writer.Seek(4, application::util::BinaryVectorWriter::Position::Current);

					Writer.Align(8);

					mUnkBlock1AbsoluteOffset = Writer.GetPosition();
					Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mUnkBlock1.data()), mUnkBlock1.size());
				}
				else
				{
					mLastDataOffset = Writer.GetPosition();
				}
			}

			Writer.Seek(ItemSizeOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
			Size ItemSize(0x40000074);
			ItemSize.SetSize(ItemSectionEndOffset - ItemSizeOffset);
			Writer.WriteInteger(ItemSize.mData, sizeof(uint32_t));

			Writer.Seek(IndexSizeOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
			Size IndexSize;
			IndexSize.SetSize(EndTagFile - IndexSizeOffset);
			Writer.WriteInteger(IndexSize.mData, sizeof(uint32_t));

			mHeader.mTagFileSize.SetSize(EndTagFile - HeaderOffset);
			mHeader.mDataSize.SetSize(DataSectionEndOffset - HeaderOffset - 24);

			Writer.Seek(HeaderOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
			Writer.WriteInteger(mHeader.mTagFileSize.mData, sizeof(uint32_t));
			Writer.WriteBytes("TAG0");
			Writer.WriteInteger(mHeader.mSDKVSize.mData, sizeof(uint32_t));
			Writer.WriteBytes("SDKV");
			Writer.WriteBytes("20220100");
			Writer.WriteInteger(mHeader.mDataSize.mData, sizeof(uint32_t));
			Writer.WriteBytes("DATA");

			Writer.Seek(EndTagFile, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
		}

		std::vector<unsigned char> ToBinary()
		{
			application::file::game::phive::classes::HavokClassBinaryVectorWriter Writer;

			ToBinary(Writer);

			return Writer.GetData();
		}

		HavokTagFile() = default;
	};
}