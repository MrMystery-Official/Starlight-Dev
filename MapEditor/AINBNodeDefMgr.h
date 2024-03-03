#pragma once

#include <vector>
#include <string>
#include "AINB.h"
#include "BinaryVectorReader.h"
#include "EXB.h"

namespace AINBNodeDefMgr
{
	struct NodeDef
	{
		struct ImmediateParam
		{
			std::string Name;
			AINBFile::ValueType ValueType;
			std::string Class;
			std::vector<AINBFile::FlagsStruct> Flags;
		};

		struct InputParam {
			std::string Name;
			std::string Class;
			std::vector<AINBFile::FlagsStruct> Flags;
			AINBFile::AINBValue Value;
			AINBFile::ValueType ValueType;

			bool HasEXBFunction = false;
			uint32_t EXBIndex = 0xFFFF;
		};

		enum class CategoryEnum : uint8_t
		{
			AI = 0,
			Logic = 1,
			Sequence = 2
		};

		NodeDef::CategoryEnum Category;
		std::string Name;
		std::string DisplayName;
		uint32_t NameHash;
		uint16_t Type;
		std::vector<std::vector<ImmediateParam>> ImmediateParameters;
		std::vector<std::vector<InputParam>> InputParameters;
		std::vector<std::vector<AINBFile::OutputEntry>> OutputParameters; //All output data required
		std::vector<std::string> LinkedNodeParams; //Editor only
		std::vector<std::string> FileNames;
		std::vector<AINBFile::FlagsStruct> Flags;
		std::vector<CategoryEnum> AllowedAINBCategories;
	};

	extern std::vector<NodeDef> NodeDefinitions;
	extern EXB EXBFile;

	void DecodeAINB(std::vector<unsigned char> Bytes, std::string FileName, std::vector<EXB::CommandInfoStruct>* EXBCommands);
	std::string ReadString(BinaryVectorReader& Reader);
	NodeDef* GetNodeDefinition(std::string Name);
	void Initialize();
	void Generate();
	AINBFile::Node NodeDefToNode(AINBNodeDefMgr::NodeDef* Def);
}