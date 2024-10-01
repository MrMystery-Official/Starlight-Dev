#include "SplatoonShapeToTotK.h"

#include "Util.h"
#include <iostream>
#include <filesystem>
#include "Editor.h"
#include "ZStdFile.h"
#include "Logger.h"

/*
SplatoonShapeToTotK::Material SplatoonShapeToTotK::MetalMaterial = SplatoonShapeToTotK::Material{
	.MaterialArray = { 0x0C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.MatColFlagsNum = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
};

SplatoonShapeToTotK::Material SplatoonShapeToTotK::WoodMaterial = SplatoonShapeToTotK::Material{
.MaterialArray = { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
.MatColFlagsNum = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
};
*/

std::vector<const char*> SplatoonShapeToTotK::Materials =
{
	"Undefined",
	"Soil",
	"Grass",
	"Sand",
	"HeavySand",
	"Snow",
	"HeavySnow",
	"Stone",
	"StoneSlip",
	"StoneNoSlip",
	"SlipBoard",
	"Cart",
	"Metal",
	"MetalSlip",
	"MetalNoSlip",
	"WireNet",
	"Wood",
	"Ice",
	"Cloth",
	"Glass",
	"Bone",
	"Rope",
	"Character",
	"Ragdoll",
	"Surfing",
	"GuardianFoot",
	"LaunchPad",
	"Conveyer",
	"Rail",
	"Grudge",
	"Meat",
	"Vegetable",
	"Bomb",
	"MagicBall",
	"Barrier",
	"AirWall",
	"GrudgeSnow",
	"Tar",
	"Water",
	"HotWater",
	"IceWater",
	"Lava",
	"Bog",
	"ContaminatedWater",
	"DungeonCeil",
	"Gas",
	"InvalidateRestartPos",
	"HorseSpeedLimit",
	"ForbidDynamicCuttingAreaForHugeCharacter",
	"ForbidHorseReturnToSafePosByClimate",
	"Dragon",
	"ReferenceSurfing",
	"WaterSlip",
	"HorseDeleteImmediatelyOnDeath"
};

uint32_t SplatoonShapeToTotK::FindSection(BinaryVectorReader Reader, std::string Section)
{
	Reader.Seek(0, BinaryVectorReader::Position::Begin);
	char Data[4] = { 0 };
	while (!(Data[0] == Section.at(0) && Data[1] == Section.at(1) && Data[2] == Section.at(2) && Data[3] == Section.at(3)))
	{
		Reader.ReadStruct(Data, 4);
	}
	return Reader.GetPosition();
}

inline uint32_t SplatoonShapeToTotK::BitFieldToSize(uint32_t Input)
{
	return Input & 0x3FFFFFFF;
}

void SplatoonShapeToTotK::ReadStringTable(BinaryVectorReader Reader, std::vector<std::string>* Dest)
{
	uint32_t Size = BitFieldToSize(Reader.ReadUInt32(true));
	uint32_t TargetAddress = Reader.GetPosition() + Size - 4; //-4 = Size(3) + Flags(1)

	Reader.Seek(4, BinaryVectorReader::Position::Current); //Magic

	std::string Text = "";
	while (Reader.GetPosition() != TargetAddress)
	{
		char Character = Reader.ReadUInt8();
		if (Character == 0x00)
		{
			Dest->push_back(Text);
			Text.clear();
			continue;
		}

		Text += Character;
	}
}

std::vector<unsigned char> SplatoonShapeToTotK::Convert(std::string SplatoonFile, MaterialSettings PhiveMaterial)
{
	return Convert(Util::ReadFile(SplatoonFile), PhiveMaterial);
}

std::vector<unsigned char> SplatoonShapeToTotK::Convert(std::vector<unsigned char> Bytes, MaterialSettings PhiveMaterial)
{
	BinaryVectorReader Reader(Bytes);
	BinaryVectorWriter Writer;

	Reader.Seek(0x20, BinaryVectorReader::Position::Begin);

	std::vector<unsigned char> MaterialArray;
	MaterialArray.push_back(PhiveMaterial.MaterialIndex);
	MaterialArray.push_back(0x00);
	MaterialArray.push_back(0x00);
	MaterialArray.push_back(0x00);
	MaterialArray.push_back(0x00);
	MaterialArray.push_back(0x00);
	MaterialArray.push_back(0x00);
	MaterialArray.push_back(0x00);
	if (!PhiveMaterial.Climbable)
	{
		MaterialArray.push_back(0x01);
		MaterialArray.push_back(0x80);
	}

	std::vector<unsigned char> MaterialColFlagsNum = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	while (MaterialArray.size() % 16 != 0)
	{
		MaterialArray.push_back(0x00);
	}

	while (MaterialColFlagsNum.size() % 16 != 0)
	{
		MaterialColFlagsNum.push_back(0x00);
	}

	uint32_t TableSize0 = MaterialArray.size();
	uint32_t TableSize1 = MaterialColFlagsNum.size();

	std::cout << "Size: " << TableSize0 << "; " << TableSize1 << std::endl;

	std::string Path = "";

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	for (const auto& DirEntry : recursive_directory_iterator(Editor::GetRomFSFile("Phive/Shape/Dcc", false)))
	{
		if (!DirEntry.is_regular_file()) continue;
		if (DirEntry.path().filename().string().starts_with("Cave")) continue;
		std::vector<unsigned char> TmpBytes = ZStdFile::Decompress(DirEntry.path().string(), ZStdFile::Dictionary::Base).Data;
		BinaryVectorReader TmpReader(TmpBytes);
		TmpReader.Seek(0x20, BinaryVectorReader::Position::Begin);
		uint32_t TemplateTableSize0 = TmpReader.ReadUInt32();
		uint32_t TemplateTableSize1 = TmpReader.ReadUInt32();
		std::cout << "TemplateSize: " << TemplateTableSize0 << "; " << TemplateTableSize1 << std::endl;
		if (TemplateTableSize0 == TableSize0 && TemplateTableSize1 == TableSize1)
		{
			Path = DirEntry.path().string();
			break;
		}
	}

	if (Path.length() == 0) //Is this case even possible? - Theoretically yes, practically probably not
	{
		Logger::Error("PhiveShapeGenerator", "Could not find valid template file");
		return std::vector<unsigned char>();
	}

	std::cout << "FOUND\n";

	std::vector<unsigned char> TemplateBytes = ZStdFile::Decompress(Path, ZStdFile::Dictionary::Base).Data;
	BinaryVectorReader TemplateReader(TemplateBytes);

	Reader.Seek(0x58, BinaryVectorReader::Position::Begin);
	uint32_t HavokTagFileSize = Reader.ReadUInt32(); //Size and flags, but only used for size
	Reader.Seek(0x50, BinaryVectorReader::Position::Begin);
	std::vector<unsigned char> HavokTagFile;
	HavokTagFile.resize(HavokTagFileSize);
	Reader.Read(reinterpret_cast<char*>(HavokTagFile.data()), HavokTagFileSize);

	// Post processing havok tag file
	BinaryVectorWriter HavokTagFileWriter;
	HavokTagFileWriter.WriteRawUnsafeFixed(reinterpret_cast<const char*>(HavokTagFile.data()), HavokTagFile.size());
	Reader.Seek(0xE0, BinaryVectorReader::Position::Begin);
	uint32_t GeometrySectionOffset = Reader.ReadUInt32() - 4;
	GeometrySectionOffset += Reader.GetPosition();
	uint32_t GeometrySectionCount = Reader.ReadUInt32();
	Reader.Seek(GeometrySectionOffset, BinaryVectorReader::Position::Begin);
	for (int i = 0; i < GeometrySectionCount; i++)
	{
		Reader.Seek(20, BinaryVectorReader::Position::Current);
		uint32_t VertexCount = Reader.ReadUInt32() * 6;
		HavokTagFileWriter.Seek(Reader.GetPosition() - 0x50 - 4, BinaryVectorWriter::Position::Begin);
		HavokTagFileWriter.WriteInteger(VertexCount, sizeof(uint32_t));
		Reader.Seek(40, BinaryVectorReader::Position::Current);
	}
	HavokTagFile = HavokTagFileWriter.GetData();

	Writer.Seek(0x50, BinaryVectorWriter::Position::Begin); //Skipping header, writing later
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(HavokTagFile.data()), HavokTagFile.size());

	TemplateReader.Seek(FindSection(TemplateReader, "TYPE") - 8, BinaryVectorReader::Position::Begin);
	uint32_t TypeOffset = TemplateReader.GetPosition();
	std::vector<unsigned char> PhiveData(TemplateReader.GetSize() - TemplateReader.GetPosition());
	TemplateReader.Read(reinterpret_cast<char*>(PhiveData.data()), PhiveData.size());

	TemplateReader.Seek(FindSection(TemplateReader, "INDX") - 8, BinaryVectorReader::Position::Begin);
	uint32_t PhiveSize = TemplateReader.GetPosition() - TypeOffset;
	PhiveData.resize(PhiveSize);

	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(PhiveData.data()), PhiveData.size());

	Reader.Seek(FindSection(Reader, "INDX") - 6, BinaryVectorReader::Position::Begin);
	uint16_t ItemSectionSize = Reader.ReadUInt16(true);
	std::vector<unsigned char> ItemSection;
	ItemSection.resize(ItemSectionSize);
	Reader.Seek(-4, BinaryVectorReader::Position::Current);
	Reader.Read(reinterpret_cast<char*>(ItemSection.data()), ItemSection.size());

	Reader.Seek(FindSection(Reader, "TST1") - 8, BinaryVectorReader::Position::Begin); //-8 = Magic(4) + Size(3) + Flags(1)
	std::vector<std::string> TagStringTable;
	ReadStringTable(Reader, &TagStringTable);

	TemplateReader.Seek(FindSection(TemplateReader, "TST1") - 8, BinaryVectorReader::Position::Begin); //-8 = Magic(4) + Size(3) + Flags(1)
	std::vector<std::string> TemplateTagStringTable;
	ReadStringTable(TemplateReader, &TemplateTagStringTable);

	/* Converting item section */
	BinaryVectorWriter ItemWriter;
	BinaryVectorReader ItemReader(ItemSection);
	ItemWriter.WriteRawUnsafeFixed(reinterpret_cast<const char*>(ItemSection.data()), 28);
	ItemReader.Seek(28, BinaryVectorReader::Position::Current);
	uint16_t ItemNodeCount = (ItemSection.size() - 28) / 12;
	for (int i = 0; i < ItemNodeCount; i++)
	{
		uint16_t NameIndex = ItemReader.ReadUInt16();
		uint16_t Flags = ItemReader.ReadUInt16();
		std::string Name = TagStringTable[NameIndex];
		std::cout << Name << std::endl;

		std::string NewName = Name;
		if (Name == "hkFloat3") NewName = "hknpShapeType::Enum";

		uint16_t NewNameIndex = 0;
		for (int j = 0; j < TemplateTagStringTable.size(); j++)
		{
			if (TemplateTagStringTable[j] == NewName)
			{
				NewNameIndex = j;
				break;
			}
		}
		ItemWriter.WriteInteger(NewNameIndex, sizeof(uint16_t));
		ItemWriter.WriteInteger(Flags, sizeof(uint16_t));
		uint32_t DataOffset = ItemReader.ReadUInt32();
		ItemWriter.WriteInteger(DataOffset, sizeof(uint32_t)); //Offset

		uint32_t ItemCount = ItemReader.ReadUInt32();
		ItemWriter.WriteInteger(Name == "hkFloat3" ? ItemCount * 6 : ItemCount, sizeof(uint32_t));
	}

	ItemSection = ItemWriter.GetData();

	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(ItemSection.data()), ItemSection.size());
	
	while (Writer.GetPosition() % 16 != 0)
		Writer.WriteByte(0x0);

	uint32_t NewTable0Offset = Writer.GetPosition();
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(MaterialArray.data()), MaterialArray.size());
	uint32_t NewTable1Offset = Writer.GetPosition();
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(MaterialColFlagsNum.data()), MaterialColFlagsNum.size());
	uint32_t FileSize = Writer.GetPosition();

	Writer.Seek(0, BinaryVectorWriter::Position::Begin);

	Writer.WriteBytes("Phive");
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x01);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0xFF);
	Writer.WriteByte(0xFE);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x04);
	Writer.WriteInteger(0x30, sizeof(uint32_t));
	Writer.WriteInteger(NewTable0Offset, sizeof(uint32_t));
	Writer.WriteInteger(NewTable1Offset, sizeof(uint32_t));
	Writer.WriteInteger(FileSize, sizeof(uint32_t));
	Writer.WriteInteger(FileSize - 0x30 - MaterialArray.size() - MaterialColFlagsNum.size(), sizeof(uint32_t));
	Writer.WriteInteger(MaterialArray.size(), sizeof(uint32_t));
	Writer.WriteInteger(MaterialColFlagsNum.size(), sizeof(uint32_t));
	Writer.WriteInteger(0x00, sizeof(uint32_t));
	Writer.WriteInteger(0x00, sizeof(uint32_t));
	uint32_t JumpbackHeader = Writer.GetPosition();
	Writer.WriteInteger(_byteswap_ulong(FileSize - 0x30 - MaterialArray.size() - MaterialColFlagsNum.size() - 4), 4);
	Writer.Seek(JumpbackHeader, BinaryVectorWriter::Position::Begin);
	Writer.WriteByte(0x00);
	Writer.Seek(3, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes("TAG0");
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x10);
	Writer.WriteBytes("SDKV");
	Writer.WriteBytes("20220100");
	JumpbackHeader = Writer.GetPosition();
	Writer.WriteInteger(_byteswap_ulong(HavokTagFileSize + 8), 4);
	Writer.Seek(JumpbackHeader, BinaryVectorWriter::Position::Begin);
	Writer.WriteByte(0x40);
	Writer.Seek(3, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes("DATA");

	std::vector<unsigned char> Data = Writer.GetData();
	Data.resize(FileSize);

	return Data;
}