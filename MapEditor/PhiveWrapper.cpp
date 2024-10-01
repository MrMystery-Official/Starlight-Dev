#include "PhiveWrapper.h"

#include "Logger.h"
#include "Util.h"
#include "StarlightData.h"

uint32_t PhiveWrapper::FindSection(BinaryVectorReader Reader, std::string Section)
{
	Reader.Seek(0, BinaryVectorReader::Position::Begin);
	char Data[4] = { 0 };
	while (!(Data[0] == Section.at(0) && Data[1] == Section.at(1) && Data[2] == Section.at(2) && Data[3] == Section.at(3)))
	{
		Reader.ReadStruct(Data, 4);
	}
	return Reader.GetPosition() - 8; //-8 = Flags(1) - Size(3) - Magic(4)
}

std::pair<uint8_t, uint32_t> PhiveWrapper::ReadBitFieldFlagsSize(uint32_t Input)
{
	return std::make_pair(Input >> 30, _byteswap_ulong(Input) & 0x3FFFFFFF);
}

uint32_t PhiveWrapper::WriteBitFieldFlagsSize(uint8_t Flags, uint32_t Size)
{
	Size &= 0x3FFFFFFF;

	uint32_t Combined = (static_cast<uint32_t>(Flags) << 30) | Size;
	return _byteswap_ulong(Combined);
}

void PhiveWrapper::WriteStringTable(BinaryVectorWriter& Writer, std::string SectionMagic, std::vector<std::string>& StringTable)
{
	uint32_t Jumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes(SectionMagic.c_str());
	for (const std::string& Buf : StringTable)
	{
		Writer.WriteBytes(Buf.c_str());
		Writer.WriteByte(0x00);
	}
	Writer.Align(4, 0xFF);
	uint32_t End = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(WriteBitFieldFlagsSize(0x01, End - Jumpback), sizeof(uint32_t));
	Writer.Seek(End, BinaryVectorWriter::Position::Begin);
}

void PhiveWrapper::ReadStringTable(BinaryVectorReader& Reader, std::string SectionMagic, std::vector<std::string>& StringTable)
{
	Reader.Seek(FindSection(Reader, SectionMagic), BinaryVectorReader::Position::Begin);

	uint32_t Size = ReadBitFieldFlagsSize(Reader.ReadUInt32()).second;
	Reader.Seek(4, BinaryVectorReader::Position::Current); //Magic
	Size = Size - 8; //Flags(1) - Size(3) - Magic(4)

	std::string Buffer;
	for (uint32_t i = 0; i < Size; i++)
	{
		char Character = Reader.ReadUInt8();
		if (Character == 0xFF)
			continue;

		if (Character == 0x00)
		{
			StringTable.push_back(Buffer);
			Buffer.clear();
			continue;
		}

		Buffer += Character;
	}
}

uint16_t PhiveWrapper::GetTypeNameIndex(std::string Name)
{
	Util::ReplaceString(Name, "__", "::");
	for (size_t i = 0; i < mTypes.size(); i++)
	{
		if (mTypes[i].mName == Name)
			return i + 1;
	}
	Logger::Error("PhiveWrapper", "Could not find type index for class" + Name);
	return 0xFFFF;
}

uint8_t PhiveWrapper::GetTypeNameFlag(std::string Name)
{
	Util::ReplaceString(Name, "__", "::");

	if (Name == "hkRootLevelContainer" ||
		Name == "hkaiNavMesh" ||
		Name == "hkaiClusterGraph" ||
		Name == "hkaiNavMeshStaticTreeFaceIterator" ||
		Name == "hkcdStaticAabbTree" ||
		Name == "hkcdStaticAabbTree::Impl")
		return 16;

	if (Name == "hkRootLevelContainer::NamedVariant" ||
		Name == "char" ||
		Name == "hkaiNavMesh::Face" ||
		Name == "hkaiNavMesh::Edge" ||
		Name == "hkVector4" ||
		Name == "hkInt32" ||
		Name == "hkaiClusterGraph::Node" ||
		Name == "hkaiIndex" ||
		Name == "hkcdCompressedAabbCodecs::Aabb6BytesCodec" ||
		Name == "hkaiClusterGraph::Edge")
		return 32;

	Logger::Error("PhiveWrapper", "Could not calculate flag for class " + Name);
}

std::vector<unsigned char> PhiveWrapper::ToBinary()
{
	auto RoundUpToMultiple = [](uint32_t NumToRound, uint32_t Multiple)
		{
			if (Multiple == 0)
				return NumToRound;

			uint32_t Remainder = NumToRound % Multiple;
			if (Remainder == 0)
				return NumToRound;

			return NumToRound + Multiple - Remainder;
		};

	BinaryVectorWriter Writer;
	Writer.Seek(0x50, BinaryVectorWriter::Position::Begin);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mHavokTagFile.data()), mHavokTagFile.size());

	Writer.Align(16);

	uint32_t HavokTagFileEnd = Writer.GetPosition();

	Writer.WriteInteger(mTypeSize, sizeof(uint32_t));
	Writer.WriteBytes("TYPE");
	
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteInteger(_byteswap_ushort(mTypePointerPaddingSize), sizeof(uint16_t));
	Writer.WriteBytes("TPTR");
	Writer.Seek(mTypePointerPaddingSize - 8, BinaryVectorWriter::Position::Current);

	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x10);
	Writer.WriteBytes("TSHA");
	Writer.WriteInteger(0, sizeof(uint32_t));
	//Writer.WriteInteger(_byteswap_ulong(0xC93BB585), sizeof(uint32_t));
	Writer.WriteInteger(StarlightData::GetU32Signature(), sizeof(uint32_t));

	WriteStringTable(Writer, "TST1", mTypeStringTable);

	uint32_t TNAJumpback = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t));
	Writer.WriteBytes("TNA1");
	Writer.WriteInteger(mTypes.size() + 1, sizeof(uint8_t));
	for (PhiveWrapperNamedType& Type : mTypes)
	{
		Writer.WriteInteger(Type.mIndex, sizeof(uint8_t));
		Writer.WriteInteger(Type.mTemplateCount, sizeof(uint8_t));
		for (PhiveWrapperTypeTemplate& Template : Type.mTemplates)
		{
			Writer.WriteInteger(Template.mIndex, sizeof(uint8_t));
			Writer.WriteInteger(Template.mValue, sizeof(uint8_t));
		}
	}
	Writer.Align(4);
	uint32_t TNAEnd = Writer.GetPosition();
	Writer.Seek(TNAJumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteInteger(_byteswap_ushort(TNAEnd - TNAJumpback), sizeof(uint16_t));
	Writer.Seek(TNAEnd, BinaryVectorWriter::Position::Begin);

	WriteStringTable(Writer, "FST1", mFieldStringTable);

	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(mConstantBlock.data()), mConstantBlock.size());

	uint32_t IndexJumpback = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t));
	Writer.WriteBytes("INDX");

	uint16_t ItemSize = 8 + mItems.size() * 12;
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteInteger(_byteswap_ushort(ItemSize), sizeof(uint16_t));
	Writer.WriteBytes("ITEM");

	for (PhiveWrapperItem& Item : mItems)
	{
		Writer.WriteInteger(Item.mTypeIndex, sizeof(uint16_t));
		Writer.WriteInteger(_byteswap_ushort(Item.mFlags), sizeof(uint16_t));
		Writer.WriteInteger(Item.mDataOffset, sizeof(uint32_t));
		Writer.WriteInteger(Item.mCount, sizeof(uint32_t));
	}

	uint32_t IndexSize = ItemSize + 8;

	uint32_t PatchJumpback = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t));
	Writer.WriteBytes("PTCH");
	for (PhiveWrapperPatch& Patch : mPatches)
	{
		Writer.WriteInteger(Patch.mTypeIndex, sizeof(uint32_t));
		Writer.WriteInteger(Patch.mCount, sizeof(uint32_t));

		for (uint32_t& Offset : Patch.mOffsets)
		{
			Writer.WriteInteger(Offset, sizeof(uint32_t));
		}
	}
	uint32_t PatchEnd = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t)); //Terminator

	Writer.Seek(PatchJumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteInteger(_byteswap_ushort(PatchEnd - PatchJumpback), sizeof(uint16_t));
	Writer.Seek(PatchEnd, BinaryVectorWriter::Position::Begin);

	IndexSize += (PatchEnd - PatchJumpback);

	uint32_t FileEnd = Writer.GetPosition();

	Writer.Seek(IndexJumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(_byteswap_ulong(IndexSize), sizeof(uint32_t));

	Writer.Seek(FileEnd, BinaryVectorWriter::Position::Begin);
	uint32_t PhysicsDataEnd = Writer.GetPosition();
	Writer.Align(8);
	uint32_t LastDataOffset = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t));
	Writer.WriteInteger(0x3F800000, sizeof(uint32_t));
	Writer.Seek(16, BinaryVectorWriter::Position::Current);
	Writer.WriteInteger(0x3F800000, sizeof(uint32_t));
	Writer.Seek(11, BinaryVectorWriter::Position::Current);
	Writer.WriteByte(0x80);
	Writer.Seek(4, BinaryVectorWriter::Position::Current);
	Writer.WriteInteger(0x3F800000, sizeof(uint32_t));
	Writer.Seek(4, BinaryVectorWriter::Position::Current);

	FileEnd = Writer.GetPosition();

	Writer.Seek(0, BinaryVectorWriter::Position::Begin);

	Writer.WriteBytes("Phive");
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x01);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0xFF);
	Writer.WriteByte(0xFE);
	Writer.WriteByte(0x01);
	Writer.WriteByte(0x03);

	Writer.WriteInteger(48, sizeof(uint32_t)); //TagFileOffset

	Writer.WriteInteger(LastDataOffset, sizeof(uint32_t));
	Writer.WriteInteger(RoundUpToMultiple(FileEnd, 16), sizeof(uint32_t));
	Writer.WriteInteger(PhysicsDataEnd - 0x30, sizeof(uint32_t));
	Writer.WriteInteger(0x34, sizeof(uint32_t));

	Writer.Seek(0x30, BinaryVectorWriter::Position::Begin);

	Writer.WriteInteger(WriteBitFieldFlagsSize(0x00, PhysicsDataEnd - 0x30), sizeof(uint32_t));
	Writer.WriteBytes("TAG0");
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x10);
	Writer.WriteBytes("SDKV");
	Writer.WriteBytes("20220100");
	Writer.WriteInteger(_byteswap_ulong((uint32_t)(HavokTagFileEnd - 0x48)), 4);
	Writer.Seek(-4, BinaryVectorWriter::Position::Current);
	Writer.WriteByte(0x40);
	Writer.Seek(3, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes("DATA");

	std::vector<unsigned char> Data = Writer.GetData();
	Data.resize(FileEnd);

	return Data;
}

PhiveWrapper::PhiveWrapper(std::vector<unsigned char> Bytes, bool EnablePatches) : mEnablePatches(EnablePatches)
{
	BinaryVectorReader Reader(Bytes);

	Reader.Seek(FindSection(Reader, "ITEM"), BinaryVectorReader::Position::Begin);
	uint32_t ItemCount = (ReadBitFieldFlagsSize(Reader.ReadUInt32()).second - 8) / 12;
	Reader.Seek(4, BinaryVectorReader::Position::Current);
	mItems.resize(ItemCount);
	Reader.ReadStruct(mItems.data(), sizeof(PhiveWrapperItem) * ItemCount);

	for (PhiveWrapperItem& Item : mItems)
	{
		Item.mFlags = _byteswap_ushort(Item.mFlags);
	}
	
	uint32_t ConstantBlockStart = FindSection(Reader, "TBDY");
	uint32_t ConstantBlockEnd = FindSection(Reader, "INDX");
	uint32_t ConstantBlockSize = ConstantBlockEnd - ConstantBlockStart;
	mConstantBlock.resize(ConstantBlockSize);
	Reader.Seek(ConstantBlockStart, BinaryVectorReader::Position::Begin);
	Reader.ReadStruct(mConstantBlock.data(), ConstantBlockSize);

	ReadStringTable(Reader, "TST1", mTypeStringTable);
	ReadStringTable(Reader, "FST1", mFieldStringTable);

	Reader.Seek(FindSection(Reader, "TNA1"), BinaryVectorReader::Position::Begin);
	Reader.Seek(8, BinaryVectorReader::Position::Current);
	uint8_t TypeCount = Reader.ReadUInt8() - 1;
	mTypes.resize(TypeCount);
	for (uint8_t i = 0; i < TypeCount; i++)
	{
		mTypes[i].mIndex = Reader.ReadUInt8();
		mTypes[i].mTemplateCount = Reader.ReadUInt8();
		mTypes[i].mTemplates.resize(mTypes[i].mTemplateCount);

		mTypes[i].mName = mTypeStringTable[mTypes[i].mIndex];

		for (int8_t j = 0; j < mTypes[i].mTemplateCount; j++)
		{
			mTypes[i].mTemplates[j].mIndex = Reader.ReadUInt8();
			mTypes[i].mTemplates[j].mValue = Reader.ReadUInt8();
			
			mTypes[i].mTemplates[j].mName = mTypeStringTable[mTypes[i].mTemplates[j].mIndex];
		}
	}

	Reader.Seek(FindSection(Reader, "TPTR"), BinaryVectorReader::Position::Begin);
	mTypePointerPaddingSize = ReadBitFieldFlagsSize(Reader.ReadUInt32()).second;
	Reader.Seek(FindSection(Reader, "TYPE"), BinaryVectorReader::Position::Begin);
	mTypeSize = Reader.ReadUInt32();

	if (!EnablePatches)
		return;

	Reader.Seek(FindSection(Reader, "PTCH") + 8, BinaryVectorReader::Position::Begin);
	while (true)
	{
		uint32_t TypeIndex = Reader.ReadUInt32();
		if (TypeIndex == 0)
			break;

		uint32_t Count = Reader.ReadUInt32();

		PhiveWrapperPatch Patch;
		Patch.mTypeIndex = TypeIndex;
		Patch.mCount = Count;
		Patch.mOffsets.resize(Count);
		Reader.ReadStruct(Patch.mOffsets.data(), sizeof(uint32_t) * Count);

		mPatches.push_back(Patch);
	}
}