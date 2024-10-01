#include "AINBNodeDefMgr.h"

#include "Editor.h"
#include "Logger.h"
#include "BinaryVectorWriter.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include "SARC.h"
#include "ZStdFile.h"

std::vector<AINBNodeDefMgr::NodeDef> AINBNodeDefMgr::NodeDefinitions;
EXB AINBNodeDefMgr::EXBFile;

std::string AINBNodeDefMgr::ReadString(BinaryVectorReader& Reader)
{
	int Length = Reader.ReadUInt32();
	std::string Result;
	for (int i = 0; i < Length; i++)
	{
		Result += Reader.ReadInt8();
	}
	return Result;
}


void AINBNodeDefMgr::Initialize()
{
	std::ifstream File(Editor::GetWorkingDirFile("Definitions.eainbdef"), std::ios::binary);

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
		if (Version != 0x03)
		{
			Logger::Error("AINBNodeDefMgr", "AINB node definition file has wrong version, expected 0x03. Did you update the WorkingDir/Definitions.eainbdef file?");
			return;
		}

		AINBNodeDefMgr::NodeDefinitions.resize(Reader.ReadUInt32());

		std::vector<uint32_t> Offsets(AINBNodeDefMgr::NodeDefinitions.size());
		for (int i = 0; i < AINBNodeDefMgr::NodeDefinitions.size(); i++)
		{
			Offsets[i] = Reader.ReadUInt32();
		}

		for (int j = 0; j < AINBNodeDefMgr::NodeDefinitions.size(); j++)
		{
			Reader.Seek(Offsets[j], BinaryVectorReader::Position::Begin);
			AINBNodeDefMgr::NodeDef Definition;
			Definition.Name = ReadString(Reader);
			Definition.DisplayName = ReadString(Reader);
			Definition.NameHash = Reader.ReadUInt32();
			Definition.Type = Reader.ReadUInt16();
			Definition.Category = (AINBNodeDefMgr::NodeDef::CategoryEnum)Reader.ReadUInt8();
			uint8_t LinkedNodeParamCount = Reader.ReadUInt8();
			uint32_t FileNameCount = Reader.ReadUInt32();
			uint8_t FlagCount = Reader.ReadUInt8();
			uint8_t AllowedAINBCount = Reader.ReadUInt8();

			Definition.InputParameters.resize(6);
			Definition.OutputParameters.resize(6);
			Definition.ImmediateParameters.resize(6);

			for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
			{
				Definition.InputParameters[Type].resize(Reader.ReadUInt8());
				Definition.OutputParameters[Type].resize(Reader.ReadUInt8());
				Definition.ImmediateParameters[Type].resize(Reader.ReadUInt8());
			}
			for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
			{
				for (int i = 0; i < Definition.InputParameters[Type].size(); i++)
				{
					AINBNodeDefMgr::NodeDef::InputParam Param;
					Param.Name = ReadString(Reader);
					Param.Class = ReadString(Reader);
					Param.ValueType = (AINBFile::ValueType)Reader.ReadUInt8();
					Param.EXBIndex = Reader.ReadUInt16();
					Param.HasEXBFunction = Param.EXBIndex != 0xFFFF;

					switch (Param.ValueType)
					{
					case AINBFile::ValueType::Bool:
						Param.Value = (bool)Reader.ReadUInt8();
						break;
					case AINBFile::ValueType::Int:
						Param.Value = Reader.ReadUInt32();
						break;
					case AINBFile::ValueType::Float:
						Param.Value = Reader.ReadFloat();
						break;
					case AINBFile::ValueType::String:
						Param.Value = ReadString(Reader);
						break;
					case AINBFile::ValueType::Vec3f:
						Param.Value = Vector3F(Reader.ReadFloat(), Reader.ReadFloat(), Reader.ReadFloat());
						break;
					}
					uint8_t FlagsSize = Reader.ReadUInt8();
					for (int k = 0; k < FlagsSize; k++)
					{
						Param.Flags.push_back((AINBFile::FlagsStruct)Reader.ReadUInt8());
					}
					Definition.InputParameters[Type][i] = Param;
				}

				for (int i = 0; i < Definition.OutputParameters[Type].size(); i++)
				{
					AINBFile::OutputEntry Param;
					Param.Name = ReadString(Reader);
					Param.Class = ReadString(Reader);
					Param.SetPointerFlagsBitZero = (bool)Reader.ReadUInt8();
					Definition.OutputParameters[Type][i] = Param;
				}

				for (int i = 0; i < Definition.ImmediateParameters[Type].size(); i++)
				{
					AINBNodeDefMgr::NodeDef::ImmediateParam Param;
					Param.Name = ReadString(Reader);
					Param.Class = ReadString(Reader);
					Param.ValueType = (AINBFile::ValueType)Reader.ReadUInt8();
					uint8_t FlagsSize = Reader.ReadUInt8();
					for (int k = 0; k < FlagsSize; k++)
					{
						Param.Flags.push_back((AINBFile::FlagsStruct)Reader.ReadUInt8());
					}
					Definition.ImmediateParameters[Type][i] = Param;
				}
			}
			for (int i = 0; i < LinkedNodeParamCount; i++)
			{
				Definition.LinkedNodeParams.push_back(ReadString(Reader));
			}
			for (int i = 0; i < FileNameCount; i++)
			{
				Definition.FileNames.push_back(ReadString(Reader));
			}
			for (int i = 0; i < FlagCount; i++)
			{
				Definition.Flags.push_back((AINBFile::FlagsStruct)Reader.ReadUInt8());
			}
			for (int i = 0; i < AllowedAINBCount; i++)
			{
				uint8_t Num = Reader.ReadUInt8();
				Definition.AllowedAINBCategories.push_back((AINBNodeDefMgr::NodeDef::CategoryEnum)Num);
			}
			AINBNodeDefMgr::NodeDefinitions[j] = Definition;
		}

		std::vector<unsigned char> EXBBytes(Reader.GetSize() - Reader.GetPosition());
		Reader.ReadStruct(EXBBytes.data(), Reader.GetSize() - Reader.GetPosition());
		EXBFile = EXB(EXBBytes);

		Logger::Info("AINBNodeDefMgr", "Initialized, found " + std::to_string(AINBNodeDefMgr::NodeDefinitions.size()) + " nodes");
	}
}

AINBNodeDefMgr::NodeDef* AINBNodeDefMgr::GetNodeDefinition(std::string Name)
{
	for (AINBNodeDefMgr::NodeDef& Def : AINBNodeDefMgr::NodeDefinitions)
	{
		if (Def.DisplayName == Name)
		{
			return &Def;
		}
	}
	return nullptr;
}

void AINBNodeDefMgr::DecodeAINB(std::vector<unsigned char> Bytes, std::string FileName, std::vector<EXB::CommandInfoStruct>* EXBCommands)
{
	AINBFile File(Bytes, true);
	if (!File.Loaded || File.Nodes.empty()) return;
	AINBNodeDefMgr::NodeDef::CategoryEnum FileCategory = AINBNodeDefMgr::NodeDef::CategoryEnum::Logic;
	if(File.Header.FileCategory == "AI") FileCategory = AINBNodeDefMgr::NodeDef::CategoryEnum::AI;
	else if (File.Header.FileCategory == "Sequence") FileCategory = AINBNodeDefMgr::NodeDef::CategoryEnum::Sequence;
	for (AINBFile::Node Node : File.Nodes)
	{
		bool NodeFound = false;
		for (AINBNodeDefMgr::NodeDef& Definition : AINBNodeDefMgr::NodeDefinitions)
		{
			if (Definition.DisplayName == Node.GetName())
			{
				NodeFound = true;
				//if (Definition.FileNames.size() < 10) Definition.FileNames.push_back(FileName);
				Definition.FileNames.push_back(FileName);
				if (std::find(Definition.AllowedAINBCategories.begin(), Definition.AllowedAINBCategories.end(), FileCategory) == Definition.AllowedAINBCategories.end()) Definition.AllowedAINBCategories.push_back(FileCategory);
				for (int i = 0; i < AINBFile::ValueTypeCount; i++)
				{
					for (AINBFile::OutputEntry Param : Node.OutputParameters[i])
					{
						bool Found = false;
						for (AINBFile::OutputEntry DefParam : Definition.OutputParameters[i])
						{
							if (DefParam.Name == Param.Name)
							{
								Found = true;
								break;
							}
						}
						if (Found) continue;
						Definition.OutputParameters[i].push_back(Param);
					}
					for (AINBFile::InputEntry Param : Node.InputParameters[i])
					{
						bool Found = false;
						for (AINBNodeDefMgr::NodeDef::InputParam DefParam : Definition.InputParameters[i])
						{
							if (DefParam.Name == Param.Name)
							{
								Found = true;
								break;
							}
						}
						if (Found) continue;
						AINBNodeDefMgr::NodeDef::InputParam DefParam = { Param.Name, Param.Class, Param.Flags, Param.Value, (AINBFile::ValueType)Param.ValueType };
						
						if (!Param.Function.Instructions.empty())
						{
							EXBCommands->push_back(Param.Function);
							DefParam.HasEXBFunction = true;
							DefParam.EXBIndex = EXBCommands->size() - 1;
						}

						Definition.InputParameters[i].push_back(DefParam);
					}
					for (AINBFile::ImmediateParameter Param : Node.ImmediateParameters[i])
					{
						bool Found = false;
						for (AINBNodeDefMgr::NodeDef::ImmediateParam DefParam : Definition.ImmediateParameters[i])
						{
							if (DefParam.Name == Param.Name)
							{
								Found = true;
								break;
							}
						}
						if (Found) continue;
						Definition.ImmediateParameters[i].push_back({ Param.Name, (AINBFile::ValueType)Param.ValueType, Param.Class, Param.Flags });
					}
				}
				int FlowIdx = 0;
				int FlowCount = 0;
				for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++)
				{
					for (AINBFile::LinkedNodeInfo& NodeLink : Node.LinkedNodes[NodeLinkType])
					{
						if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink)
						{
							FlowCount++;
						}
					}
				}

				for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++)
				{
					for (AINBFile::LinkedNodeInfo& NodeLink : Node.LinkedNodes[NodeLinkType])
					{
						if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink)
						{
							std::string LinkName = "";
							switch (Node.Type) {
							case (int)AINBFile::NodeTypes::UserDefined:
							case (int)AINBFile::NodeTypes::Element_BoolSelector:
							case (int)AINBFile::NodeTypes::Element_SplitTiming:
								LinkName = NodeLink.Condition;
								break;
							case (int)AINBFile::NodeTypes::Element_Simultaneous:
								LinkName = "Control";
								break;
							case (int)AINBFile::NodeTypes::Element_Sequential:
								LinkName = "Seq " + std::to_string(FlowIdx);
								break;
							case (int)AINBFile::NodeTypes::Element_S32Selector:
							case (int)AINBFile::NodeTypes::Element_F32Selector:
								if (FlowIdx == FlowCount - 1) {
									LinkName = "Default";
									break;
								}
								LinkName = "= " + NodeLink.Condition;
								break;
							case (int)AINBFile::NodeTypes::Element_Fork:
								LinkName = "Fork";
								break;
							default:
								LinkName = "<name unavailable>";
								break;
							}

							FlowIdx++;

							if (std::find(Definition.LinkedNodeParams.begin(), Definition.LinkedNodeParams.end(), LinkName) != Definition.LinkedNodeParams.end())
							{
								continue;
							}
							Definition.LinkedNodeParams.push_back(LinkName);
						}
					}
				}
				for (AINBFile::FlagsStruct Flag : Node.Flags)
				{
					if (Flag != AINBFile::FlagsStruct::IsPreconditionNode)
						Definition.Flags.push_back(Flag);
				}
				break;
			}
		}
		if (NodeFound) continue;
		NodeDef Def;
		Def.Category = File.Header.FileCategory == "AI" ? AINBNodeDefMgr::NodeDef::CategoryEnum::AI : (File.Header.FileCategory == "Logic" ? AINBNodeDefMgr::NodeDef::CategoryEnum::Logic : AINBNodeDefMgr::NodeDef::CategoryEnum::Sequence);
		Def.Name = Node.Name;
		Def.DisplayName = Node.GetName();
		//if (Def.FileNames.size() < 10) Def.FileNames.push_back(FileName);
		Def.FileNames.push_back(FileName);
		Def.NameHash = Node.NameHash;
		Def.Type = Node.Type;
		Def.OutputParameters.resize(6);
		Def.InputParameters.resize(6);
		Def.ImmediateParameters.resize(6);
		Def.AllowedAINBCategories.push_back(FileCategory);
		for (int i = 0; i < AINBFile::ValueTypeCount; i++)
		{
			for (AINBFile::OutputEntry Param : Node.OutputParameters[i])
			{
				Def.OutputParameters[i].push_back(Param);
			}
			for (AINBFile::InputEntry Param : Node.InputParameters[i])
			{
				AINBNodeDefMgr::NodeDef::InputParam DefParam = { Param.Name, Param.Class, Param.Flags, Param.Value, (AINBFile::ValueType)Param.ValueType };

				if (!Param.Function.Instructions.empty())
				{
					EXBCommands->push_back(Param.Function);
					DefParam.HasEXBFunction = true;
					DefParam.EXBIndex = EXBCommands->size() - 1;
				}

				Def.InputParameters[i].push_back(DefParam);
			}
			for (AINBFile::ImmediateParameter Param : Node.ImmediateParameters[i])
			{
				Def.ImmediateParameters[i].push_back({ Param.Name, (AINBFile::ValueType)Param.ValueType, Param.Class, Param.Flags });
			}
		}
		int FlowIdx = 0;
		int FlowCount = 0;
		for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++)
		{
			for (AINBFile::LinkedNodeInfo& NodeLink : Node.LinkedNodes[NodeLinkType])
			{
				if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink)
				{
					FlowCount++;
				}
			}
		}

		for (int NodeLinkType = 0; NodeLinkType < AINBFile::LinkedNodeTypeCount; NodeLinkType++)
		{
			for (AINBFile::LinkedNodeInfo& NodeLink : Node.LinkedNodes[NodeLinkType])
			{
				if (NodeLink.Type == AINBFile::LinkedNodeMapping::StandardLink)
				{
					std::string LinkName = "";
					switch (Node.Type) {
					case (int)AINBFile::NodeTypes::UserDefined:
					case (int)AINBFile::NodeTypes::Element_BoolSelector:
					case (int)AINBFile::NodeTypes::Element_SplitTiming:
						LinkName = NodeLink.Condition;
						break;
					case (int)AINBFile::NodeTypes::Element_Simultaneous:
						LinkName = "Control";
						break;
					case (int)AINBFile::NodeTypes::Element_Sequential:
						LinkName = "Seq " + std::to_string(FlowIdx);
						break;
					case (int)AINBFile::NodeTypes::Element_S32Selector:
					case (int)AINBFile::NodeTypes::Element_F32Selector:
						if (FlowIdx == FlowCount - 1) {
							LinkName = "Default";
							break;
						}
						LinkName = "= " + NodeLink.Condition;
						break;
					case (int)AINBFile::NodeTypes::Element_Fork:
						LinkName = "Fork";
						break;
					default:
						LinkName = "<name unavailable>";
						break;
					}

					FlowIdx++;

					if (std::find(Def.LinkedNodeParams.begin(), Def.LinkedNodeParams.end(), LinkName) != Def.LinkedNodeParams.end())
					{
						continue;
					}
					Def.LinkedNodeParams.push_back(LinkName);
				}
			}
		}
		for (AINBFile::FlagsStruct Flag : Node.Flags)
		{
			if (Flag != AINBFile::FlagsStruct::IsPreconditionNode)
				Def.Flags.push_back(Flag);
		}
		AINBNodeDefMgr::NodeDefinitions.push_back(Def);
	}
}

void AINBNodeDefMgr::Generate()
{
	std::vector<EXB::CommandInfoStruct> EXBCommands;

	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	for (const auto& DirEntry : recursive_directory_iterator(Editor::GetRomFSFile("Logic", false)))
	{
		if (DirEntry.is_regular_file())
		{
			std::ifstream File(DirEntry.path().string(), std::ios::binary);

			if (!File.eof() && !File.fail())
			{
				File.seekg(0, std::ios_base::end);
				std::streampos FileSize = File.tellg();

				std::vector<unsigned char> Bytes(FileSize);

				File.seekg(0, std::ios_base::beg);
				File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

				File.close();

				//this->AINBFile::AINBFile(Bytes);
				DecodeAINB(Bytes, "Logic/" + DirEntry.path().filename().string(), &EXBCommands);
			}
			else
			{
				Logger::Error("AINBNodeDefMgr", "Could not open file \"" + DirEntry.path().string() + "\"");
			}
		}
	}
	for (const auto& DirEntry : recursive_directory_iterator(Editor::GetRomFSFile("AI", false)))
	{
		if (DirEntry.is_regular_file())
		{
			std::ifstream File(DirEntry.path().string(), std::ios::binary);

			if (!File.eof() && !File.fail())
			{
				File.seekg(0, std::ios_base::end);
				std::streampos FileSize = File.tellg();

				std::vector<unsigned char> Bytes(FileSize);

				File.seekg(0, std::ios_base::beg);
				File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

				File.close();

				//this->AINBFile::AINBFile(Bytes);
				DecodeAINB(Bytes, "AI/" + DirEntry.path().filename().string(), &EXBCommands);
			}
			else
			{
				Logger::Error("AINBNodeDefMgr", "Could not open file \"" + DirEntry.path().string() + "\"");
			}
		}
	}
	for (const auto& DirEntry : recursive_directory_iterator(Editor::GetRomFSFile("Pack/Actor", false)))
	{
		if (DirEntry.is_regular_file())
		{
			if (DirEntry.path().string().ends_with(".pack.zs"))
			{
				ZStdFile::Result Result = ZStdFile::Decompress(DirEntry.path().string(), ZStdFile::Dictionary::Pack);
				if (Result.Data.size() > 0)
				{
					SarcFile File(Result.Data);
					for (SarcFile::Entry& Entry : File.GetEntries())
					{
						if (Entry.Bytes[0] == 'A' && Entry.Bytes[1] == 'I' && Entry.Bytes[2] == 'B')
						{
							DecodeAINB(Entry.Bytes, "Pack/Actor/" + DirEntry.path().filename().string() + "/" + Entry.Name, &EXBCommands);
						}
					}
				}
			}
		}
	}

	BinaryVectorWriter Writer;
	Writer.WriteBytes("EAINBDEF"); //Magic
	Writer.WriteByte(0x03); //Version
	Writer.WriteInteger(NodeDefinitions.size(), sizeof(uint32_t));
	Writer.Seek(NodeDefinitions.size() * 4, BinaryVectorWriter::Position::Current);

	std::vector<uint32_t> Offsets;
	for (AINBNodeDefMgr::NodeDef Def : NodeDefinitions)
	{
		Offsets.push_back(Writer.GetPosition());
		Writer.WriteInteger(Def.Name.length(), sizeof(uint32_t)); //Name length
		Writer.WriteBytes(Def.Name.c_str());
		Writer.WriteInteger(Def.DisplayName.length(), sizeof(uint32_t)); //Name length
		Writer.WriteBytes(Def.DisplayName.c_str());
		Writer.WriteInteger(Def.NameHash, sizeof(uint32_t));
		Writer.WriteInteger(Def.Type, sizeof(uint16_t));
		Writer.WriteByte((uint8_t)Def.Category);
		Writer.WriteByte((uint8_t)Def.LinkedNodeParams.size());
		Writer.WriteInteger(Def.FileNames.size(), sizeof(uint32_t));
		Writer.WriteByte((uint8_t)Def.Flags.size());
		Writer.WriteByte((uint8_t)Def.AllowedAINBCategories.size());
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			Writer.WriteInteger(Def.InputParameters[Type].size(), sizeof(uint8_t));
			Writer.WriteInteger(Def.OutputParameters[Type].size(), sizeof(uint8_t));
			Writer.WriteInteger(Def.ImmediateParameters[Type].size(), sizeof(uint8_t));
		}
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			for (AINBNodeDefMgr::NodeDef::InputParam Param : Def.InputParameters[Type])
			{
				Writer.WriteInteger(Param.Name.size(), sizeof(uint32_t));
				Writer.WriteBytes(Param.Name.c_str());
				Writer.WriteInteger(Param.Class.size(), sizeof(uint32_t));
				Writer.WriteBytes(Param.Class.c_str());
				Writer.WriteInteger((uint8_t)Param.ValueType, sizeof(uint8_t));
				Writer.WriteInteger(Param.EXBIndex, sizeof(uint16_t));
				switch (Param.ValueType)
				{
				case AINBFile::ValueType::Bool:
					Writer.WriteInteger(*reinterpret_cast<bool*>(&Param.Value), sizeof(bool));
					break;
				case AINBFile::ValueType::Int:
					Writer.WriteInteger(*reinterpret_cast<int*>(&Param.Value), sizeof(int32_t));
					break;
				case AINBFile::ValueType::Float:
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Param.Value), sizeof(float));
					break;
				case AINBFile::ValueType::String:
					Writer.WriteInteger(reinterpret_cast<std::string*>(&Param.Value)->length(), sizeof(uint32_t));
					Writer.WriteBytes(reinterpret_cast<std::string*>(&Param.Value)->c_str());
					break;
				case AINBFile::ValueType::Vec3f:
					Vector3F Vec3f = *reinterpret_cast<Vector3F*>(&Param.Value);
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Vec3f.GetRawData()[0]), sizeof(float));
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Vec3f.GetRawData()[1]), sizeof(float));
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Vec3f.GetRawData()[2]), sizeof(float));
					break;
				}
				Writer.WriteInteger(Param.Flags.size(), sizeof(uint8_t));
				for (AINBFile::FlagsStruct Flag : Param.Flags)
				{
					Writer.WriteInteger((uint8_t)Flag, sizeof(uint8_t));
				}
			}
			for (AINBFile::OutputEntry Param : Def.OutputParameters[Type])
			{
				Writer.WriteInteger(Param.Name.size(), sizeof(uint32_t));
				Writer.WriteBytes(Param.Name.c_str());
				Writer.WriteInteger(Param.Class.size(), sizeof(uint32_t));
				Writer.WriteBytes(Param.Class.c_str());
				Writer.WriteInteger((uint8_t)Param.SetPointerFlagsBitZero, sizeof(uint8_t));
			}
			for (AINBNodeDefMgr::NodeDef::ImmediateParam Param : Def.ImmediateParameters[Type])
			{
				Writer.WriteInteger(Param.Name.size(), sizeof(uint32_t));
				Writer.WriteBytes(Param.Name.c_str());
				Writer.WriteInteger(Param.Class.size(), sizeof(uint32_t));
				Writer.WriteBytes(Param.Class.c_str());
				Writer.WriteInteger((uint8_t)Param.ValueType, sizeof(uint8_t));
				Writer.WriteInteger(Param.Flags.size(), sizeof(uint8_t));
				for (AINBFile::FlagsStruct Flag : Param.Flags)
				{
					Writer.WriteInteger((uint8_t)Flag, sizeof(uint8_t));
				}
			}
		}
		for (std::string ParamName : Def.LinkedNodeParams)
		{
			Writer.WriteInteger(ParamName.length(), sizeof(uint32_t)); //ParamName length
			Writer.WriteBytes(ParamName.c_str());
		}
		for (std::string FileName : Def.FileNames)
		{
			Writer.WriteInteger(FileName.length(), sizeof(uint32_t)); //FileName length
			Writer.WriteBytes(FileName.c_str());
		}
		for (AINBFile::FlagsStruct Flag : Def.Flags)
		{
			Writer.WriteByte((uint8_t)Flag);
		}
		for (AINBNodeDefMgr::NodeDef::CategoryEnum Cat : Def.AllowedAINBCategories)
		{
			Writer.WriteByte((uint8_t)Cat);
		}
	}

	EXBFile.Commands = EXBCommands;
	std::vector<unsigned char> EXBBytes = EXBFile.ToBinary(0);
	Writer.WriteRawUnsafeFixed((const char*)EXBBytes.data(), EXBBytes.size());

	Writer.Seek(13, BinaryVectorWriter::Position::Begin);
	for (uint32_t Offset : Offsets)
	{
		Writer.WriteInteger(Offset, sizeof(uint32_t));
	}

	std::ofstream FileOut(Editor::GetWorkingDirFile("Definitions.eainbdef"), std::ios::binary);
	std::vector<unsigned char> Binary = Writer.GetData();
	std::copy(Binary.cbegin(), Binary.cend(),
		std::ostream_iterator<unsigned char>(FileOut));
	FileOut.close();
}

AINBFile::Node AINBNodeDefMgr::NodeDefToNode(AINBNodeDefMgr::NodeDef* Def)
{
	if (Def == nullptr)
	{
		Logger::Error("AINBNodeDefMgr", "Could not convert node definition to node instance, node definition was a nullptr");
		return AINBFile::Node();
	}

	AINBFile::Node NewNode;
	NewNode.Name = Def->Name;
	NewNode.NameHash = Def->NameHash;
	NewNode.Type = Def->Type;
	NewNode.OutputParameters = Def->OutputParameters;
	NewNode.EditorFlowLinkParams = Def->LinkedNodeParams;
	NewNode.Flags = Def->Flags;

	NewNode.InputParameters.resize(6);
	NewNode.ImmediateParameters.resize(6);

	for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
	{
		for (AINBNodeDefMgr::NodeDef::InputParam Param : Def->InputParameters[Type])
		{
			AINBFile::InputEntry Entry;
			Entry.Name = Param.Name;
			Entry.Class = Param.Class;
			Entry.Flags = Param.Flags;
			Entry.ValueType = (int)Param.ValueType;
			Entry.Value = Param.Value;
			Entry.NodeIndex = -1;
			Entry.ParameterIndex = 0;

			/*
			if (Param.HasEXBFunction)
			{
				Entry.Function = EXBFile.Commands[Param.EXBIndex];
			}
			*/

			NewNode.InputParameters[Type].push_back(Entry);
		}
		for (AINBNodeDefMgr::NodeDef::ImmediateParam Param : Def->ImmediateParameters[Type])
		{
			AINBFile::ImmediateParameter Entry;
			Entry.Name = Param.Name;
			Entry.Class = Param.Class;
			Entry.Flags = Param.Flags;
			Entry.ValueType = (int)Param.ValueType;
			NewNode.ImmediateParameters[Type].push_back(Entry);
		}
	}

	return NewNode;
}