#include "EventNodeDefMgr.h"

#include <filesystem>
#include <fstream>
#include "Editor.h"
#include "ZStdFile.h"
#include "BinaryVectorWriter.h"

std::vector<EventNodeDefMgr::NodeDef> EventNodeDefMgr::mEventNodes;
std::unordered_map<std::string, std::vector<std::string>> EventNodeDefMgr::mActorNodes;

std::string EventNodeDefMgr::ReadString(BinaryVectorReader& Reader)
{
	int Length = Reader.ReadUInt16();
	std::string Result;
	for (int i = 0; i < Length; i++)
	{
		Result += Reader.ReadInt8();
	}
	return Result;
}

void EventNodeDefMgr::Initialize()
{
	std::ifstream File(Editor::GetWorkingDirFile("Definitions.ebfevfldef"), std::ios::binary);

	if (!File.eof() && !File.fail())
	{
		File.seekg(0, std::ios_base::end);
		std::streampos FileSize = File.tellg();

		std::vector<unsigned char> Bytes(FileSize);

		File.seekg(0, std::ios_base::beg);
		File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);
		File.close();

		BinaryVectorReader Reader(Bytes);

		Reader.Seek(10, BinaryVectorReader::Position::Current);
		uint8_t Version = Reader.ReadUInt8();
		if (Version != 0x01)
		{
			Logger::Error("EventNodeDefMgr", "Wrong version, expected 0x01, got " + std::to_string(Version));
			return;
		}

		mEventNodes.resize(Reader.ReadUInt32());
		for (NodeDef& Def : mEventNodes)
		{
			Def.mActorName = ReadString(Reader);
			Def.mEventName = ReadString(Reader);
			Def.mIsAction = (bool)Reader.ReadUInt8();
			Def.mParameters.resize(Reader.ReadUInt16());
			for (auto& [Key, Val] : Def.mParameters)
			{
				Key = static_cast<BFEVFLFile::ContainerDataType>(Reader.ReadUInt8());
				Val = ReadString(Reader);
			}

			mActorNodes[Def.mActorName].push_back(Def.mEventName);
		}

		Logger::Info("EventNodeDefMgr", "Initialized, found " + std::to_string(mEventNodes.size()) + " nodes");
	}
}

void EventNodeDefMgr::DecodeEvent(BFEVFLFile& Event)
{
	for (auto& [Type, Node] : Event.mFlowcharts[0].mEvents)
	{
		if (Type == BFEVFLFile::EventType::Action)
		{
			BFEVFLFile::ActionEvent* Action = &std::get<BFEVFLFile::ActionEvent>(Node);
			
			if (mActorNodes.contains(Event.mFlowcharts[0].mActors[Action->mActorIndex].mName))
			{
				std::vector<std::string>& ExistingActorNodes = mActorNodes[Event.mFlowcharts[0].mActors[Action->mActorIndex].mName];
				if (std::find(ExistingActorNodes.begin(), ExistingActorNodes.end(), Event.mFlowcharts[0].mActors[Action->mActorIndex].mActions[Action->mActorActionIndex]) != ExistingActorNodes.end())
				{
					continue;
				}
			}

			NodeDef Def;
			Def.mIsAction = true;
			Def.mActorName = Event.mFlowcharts[0].mActors[Action->mActorIndex].mName;
			Def.mEventName = Event.mFlowcharts[0].mActors[Action->mActorIndex].mActions[Action->mActorActionIndex];

			for (auto& [Key, Item] : Action->mParameters.mItems)
			{
				Def.mParameters.push_back(std::make_pair(Item.mType, Key ));
			}

			mEventNodes.push_back(Def);
			mActorNodes[Def.mActorName].push_back(Def.mEventName);
		}
		else if (Type == BFEVFLFile::EventType::Switch)
		{
			BFEVFLFile::SwitchEvent* Switch = &std::get<BFEVFLFile::SwitchEvent>(Node);

			if (mActorNodes.contains(Event.mFlowcharts[0].mActors[Switch->mActorIndex].mName))
			{
				std::vector<std::string>& ExistingActorNodes = mActorNodes[Event.mFlowcharts[0].mActors[Switch->mActorIndex].mName];
				if (std::find(ExistingActorNodes.begin(), ExistingActorNodes.end(), Event.mFlowcharts[0].mActors[Switch->mActorIndex].mQueries[Switch->mActorQueryIndex]) != ExistingActorNodes.end())
				{
					continue;
				}
			}

			NodeDef Def;
			Def.mIsAction = false;
			Def.mActorName = Event.mFlowcharts[0].mActors[Switch->mActorIndex].mName;
			Def.mEventName = Event.mFlowcharts[0].mActors[Switch->mActorIndex].mQueries[Switch->mActorQueryIndex];

			for (auto& [Key, Item] : Switch->mParameters.mItems)
			{
				Def.mParameters.push_back(std::make_pair(Item.mType, Key));
			}

			mEventNodes.push_back(Def);
			mActorNodes[Def.mActorName].push_back(Def.mEventName);
		}
	}
}

void EventNodeDefMgr::Generate()
{
	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	for (const auto& DirEntry : recursive_directory_iterator(Editor::GetRomFSFile("Event/EventFlow", false)))
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

				std::cout << "Decoding" << DirEntry.path().string() << std::endl;

				BFEVFLFile Event(ZStdFile::Decompress(Bytes, ZStdFile::Dictionary::Base).Data);
				DecodeEvent(Event);

				//this->AINBFile::AINBFile(Bytes);
				//DecodeAINB(Bytes, "Logic/" + DirEntry.path().filename().string(), &EXBCommands);

				break;
			}
			else
			{
				Logger::Error("EventNodeDefMgr", "Could not open file \"" + DirEntry.path().string() + "\"");
			}
		}
	}
	BinaryVectorWriter Writer;
	Writer.WriteBytes("EBFEVFLDEF");
	Writer.WriteByte(0x01); //Version
	Writer.WriteInteger(mEventNodes.size(), sizeof(uint32_t));
	for (NodeDef& Def : mEventNodes)
	{
		Writer.WriteInteger(Def.mActorName.length(), sizeof(uint16_t));
		Writer.WriteBytes(Def.mActorName.c_str());
		Writer.WriteInteger(Def.mEventName.length(), sizeof(uint16_t));
		Writer.WriteBytes(Def.mEventName.c_str());
		Writer.WriteByte(Def.mIsAction);
		Writer.WriteInteger(Def.mParameters.size(), sizeof(uint16_t));
		for (auto& [Type, Key] : Def.mParameters)
		{
			Writer.WriteInteger((uint8_t)Type, sizeof(uint8_t));
			Writer.WriteInteger(Key.length(), sizeof(uint16_t));
			Writer.WriteBytes(Key.c_str());
		}
	}

	std::ofstream FileOut(Editor::GetWorkingDirFile("Definitions.ebfevfldef"), std::ios::binary);
	std::vector<unsigned char> Binary = Writer.GetData();
	std::copy(Binary.cbegin(), Binary.cend(),
		std::ostream_iterator<unsigned char>(FileOut));
	FileOut.close();
}