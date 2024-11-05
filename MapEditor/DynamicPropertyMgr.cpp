#include "DynamicPropertyMgr.h"

#include <filesystem>
#include <algorithm>
#include <fstream>
#include "Editor.h"
#include "Byml.h"
#include "Logger.h"
#include "ZStdFile.h"

std::unordered_map<std::string, std::vector<DynamicPropertyMgr::MultiCaseString>> DynamicPropertyMgr::mActorProperties;
std::unordered_map<std::string, std::pair<int8_t, DynamicPropertyMgr::MultiCaseString>> DynamicPropertyMgr::mProperties;

DynamicPropertyMgr::MultiCaseString::MultiCaseString(std::string Str)
{
	mString = Str;
	mLowerCaseString = Str;
	std::transform(mLowerCaseString.begin(), mLowerCaseString.end(), mLowerCaseString.begin(),
		[](unsigned char c) { return std::tolower(c); });
}

bool DynamicPropertyMgr::MultiCaseString::operator==(const MultiCaseString& B)
{
	return B.mLowerCaseString == mLowerCaseString;
}

std::string DynamicPropertyMgr::ReadString(BinaryVectorReader& Reader)
{
	int Length = Reader.ReadUInt16();
	std::string Result;
	for (int i = 0; i < Length; i++)
	{
		Result += Reader.ReadInt8();
	}
	return Result;
}

void DynamicPropertyMgr::Initialize()
{
	std::ifstream File(Editor::GetWorkingDirFile("Definitions.edyprdef"), std::ios::binary);

	if (!File.eof() && !File.fail())
	{
		File.seekg(0, std::ios_base::end);
		std::streampos FileSize = File.tellg();

		std::vector<unsigned char> Bytes(FileSize);

		File.seekg(0, std::ios_base::beg);
		File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);
		File.close();


		BinaryVectorReader Reader(Bytes);
		Reader.Seek(8, BinaryVectorReader::Position::Begin); //Skip magic
		uint8_t Version = Reader.ReadUInt8();
		if (Version != 0x01)
		{
			Logger::Error("DynamicPropertyMgr", "Dynamic property definition file has wrong version, expected 0x01. Did you update the WorkingDir/Definitions.edyprdef file?");
			return;
		}

		uint32_t ActorPropertiesCount = Reader.ReadUInt32();
		uint32_t PropertyCount = Reader.ReadUInt32();

		for (uint32_t i = 0; i < ActorPropertiesCount; i++)
		{
			uint32_t Count = Reader.ReadUInt32();
			std::string Key = ReadString(Reader);

			std::vector<MultiCaseString> Val;
			Val.reserve(Count);

			for (uint32_t j = 0; j < Count; j++)
			{
				Val.push_back(MultiCaseString(ReadString(Reader)));
			}

			mActorProperties.insert({ Key, Val });
		}

		for (uint32_t i = 0; i < PropertyCount; i++)
		{
			std::string Key = ReadString(Reader);
			mProperties.insert({ Key, std::make_pair(Reader.ReadUInt8(), MultiCaseString(Key) )});
		}

		Logger::Info("DynamicPropertyMgr", "Initialized, found " + std::to_string(mProperties.size()) + " properties");
	}
}

#include <iostream>

void DynamicPropertyMgr::Generate()
{
	for (const auto& DirEntry : std::filesystem::recursive_directory_iterator(Editor::GetRomFSFile("Banc", false), std::filesystem::directory_options::skip_permission_denied))
	{
		if ((DirEntry.path().filename().string().find("_Dynamic") == std::string::npos && DirEntry.path().filename().string().find("_Static") == std::string::npos) ||
			DirEntry.path().filename().string().find(".byml.zs") == std::string::npos)
			continue;
		
		BymlFile Byml(ZStdFile::Decompress(DirEntry.path().string(), ZStdFile::Dictionary::BcettByaml).Data);
		if (!Byml.HasChild("Actors"))
			continue;

		for (BymlFile::Node& Child : Byml.GetNode("Actors")->GetChildren())
		{
			if (!Child.HasChild("Dynamic") || !Child.HasChild("Gyaml"))
				continue;

			std::string ActorName = Child.GetChild("Gyaml")->GetValue<std::string>();

			for (BymlFile::Node& DynamicNode : Child.GetChild("Dynamic")->GetChildren())
			{
				std::string Key = DynamicNode.GetKey();
				if (!mProperties.contains(Key))
				{
					uint8_t Type = 0;
					switch (DynamicNode.GetType())
					{
					case BymlFile::Type::StringIndex:
						Type = 0;
						break;
					case BymlFile::Type::Bool:
						Type = 1;
						break;
					case BymlFile::Type::Int32:
						Type = 2;
						break;
					case BymlFile::Type::Int64:
						Type = 3;
						break;
					case BymlFile::Type::UInt32:
						Type = 4;
						break;
					case BymlFile::Type::UInt64:
						Type = 5;
						break;
					case BymlFile::Type::Float:
						Type = 6;
						break;
					case BymlFile::Type::Array:
						Type = 7;
						break;
					default:
						Logger::Error("DynamicPropertyMgr", "Invalid data type");
						break;
					}
					mProperties[Key] = std::make_pair((uint8_t)Type, MultiCaseString(Key));
				}

				std::vector<MultiCaseString>& KnownProperties = mActorProperties[ActorName];
				MultiCaseString MultiKey(Key);
				if (std::find(KnownProperties.begin(), KnownProperties.end(), MultiKey) == KnownProperties.end())
					KnownProperties.push_back(MultiKey);
			}
		}
	}

	Logger::Info("DynamicPropertyMgr", "Found " + std::to_string(mProperties.size()) + " unique properties and scanned " + std::to_string(mActorProperties.size()));

	BinaryVectorWriter Writer;
	Writer.WriteBytes("EDYPRDEF"); //Magic
	Writer.WriteByte(0x01); //Version
	Writer.WriteInteger(mActorProperties.size(), sizeof(uint32_t));
	Writer.WriteInteger(mProperties.size(), sizeof(uint32_t));
	for (auto& [Key, Val] : mActorProperties)
	{
		Writer.WriteInteger(Val.size(), sizeof(uint32_t));

		Writer.WriteInteger(Key.size(), sizeof(uint16_t));
		Writer.WriteBytes(Key.c_str());

		for (MultiCaseString& Property : Val)
		{
			Writer.WriteInteger(Property.mString.size(), sizeof(uint16_t));
			Writer.WriteBytes(Property.mString.c_str());
		}
	}
	for (auto& [Key, Type] : mProperties)
	{
		Writer.WriteInteger(Key.size(), sizeof(uint16_t));
		Writer.WriteBytes(Key.c_str());
		Writer.WriteInteger(Type.first, sizeof(uint8_t));
	}

	std::ofstream FileOut(Editor::GetWorkingDirFile("Definitions.edyprdef"), std::ios::binary);
	std::vector<unsigned char> Binary = Writer.GetData();
	std::copy(Binary.cbegin(), Binary.cend(),
		std::ostream_iterator<unsigned char>(FileOut));
	FileOut.close();
}