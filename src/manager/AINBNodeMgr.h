#pragma once

#include <vector>
#include <utility>
#include <string>
#include <functional>
#include <unordered_map>
#include <file/game/ainb/AINBFile.h>

namespace application::manager
{
    namespace AINBNodeMgr
    {
        struct NodeDef
        {
            struct ParameterDef
            {
                std::string mName;
                std::string mClass;
                application::file::game::ainb::AINBFile::ValueType mValueType;
                std::vector<application::file::game::ainb::AINBFile::FlagsStruct> mFlags;
            };
            enum class Category : uint8_t
            {
                LOGIC = 0,
                AI = 1,
                SEQUENCE = 2
            };

            //If loaded from the vanilla node definition file and not Starlights node database, this is set to the project name, else it is empty
            std::string mProjectName = "";

            std::string mName;
            std::vector<Category> mCategories;
            application::file::game::ainb::AINBFile::NodeTypes mNodeType;
            std::vector<std::string> mFlowOutputParameters;
            std::vector<ParameterDef> mInputParameters;
            std::vector<ParameterDef> mOutputParameters;
            std::vector<ParameterDef> mImmediateParameters;
            std::vector<std::string> mFileNames;
            uint32_t mNameHash;
        };
        
        extern std::vector<NodeDef> gNodeDefinitions;
        extern std::vector<std::pair<std::string, std::function<bool(NodeDef*)>>> gCategories_;
        extern std::vector<std::pair<std::string, std::vector<NodeDef*>>> gNodeCategories; //for example, LOGIC -> gNodeCategory
        extern std::unordered_map<std::string, std::unordered_map<std::string, bool>> gHasNodeCategoryNodes; //for example, Logic -> { "Element": false, "Operate": true }
        extern std::unordered_map<std::string, NodeDef::Category> gFileToNodeCategory;
        extern float gMaxDisplayLength;
        extern const uint16_t gFileVersion;

        bool Initialize();
        void DecodeAINB(std::vector<unsigned char> Bytes, std::string FileName);
        void GenerateNodeDefinitionFile();
        void GenerateNodeDefinitionFileWonder(const std::string& RomFSPath);
        void CalculateMaxDisplayLength();
        void SetDefaultValue(application::file::game::ainb::AINBFile::AINBValue& Value, application::file::game::ainb::AINBFile::ValueType ValueType);
        void AddNode(application::file::game::ainb::AINBFile& AINB, NodeDef* Def);

        void ReloadAdditionalProjectNodes();

        void WriteToFile(const std::string& Name = "Definitions");
        bool LoadFromFile(const std::string& Name = "Definitions");
    } // namespace AINBNodeMgr
} // namespace application::manager
