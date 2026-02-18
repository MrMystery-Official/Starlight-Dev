#include "AINBNodeMgr.h"

#include "imgui.h"
#include <util/Logger.h>
#include <util/FileUtil.h>
#include <filesystem>
#include <fstream>
#include <util/BinaryVectorReader.h>
#include <util/BinaryVectorWriter.h>
#include <manager/ProjectMgr.h>

#include <file/game/sarc/SarcFile.h>
#include <file/game/zstd/ZStdBackend.h>
#include <file/tool/AdditionalAINBNodes.h>

namespace application::manager
{
    std::vector<AINBNodeMgr::NodeDef> AINBNodeMgr::gNodeDefinitions;
    std::vector<std::pair<std::string, std::function<bool(AINBNodeMgr::NodeDef*)>>> AINBNodeMgr::gCategories_;
    std::vector<std::pair<std::string, std::vector<AINBNodeMgr::NodeDef*>>> AINBNodeMgr::gNodeCategories; //Tag -> NodeDef*[]
    std::unordered_map<std::string, std::unordered_map<std::string, bool>> AINBNodeMgr::gHasNodeCategoryNodes; //for example, Logic -> { "Element": false, "Operate": true }
    std::unordered_map<std::string, AINBNodeMgr::NodeDef::Category> AINBNodeMgr::gFileToNodeCategory;
    float AINBNodeMgr::gMaxDisplayLength = 0.0f;
    const uint16_t AINBNodeMgr::gFileVersion = 0x0002;

    bool AINBNodeMgr::Initialize()
    {
        gFileToNodeCategory["AI"] = NodeDef::Category::AI;
        gFileToNodeCategory["Logic"] = NodeDef::Category::LOGIC;
        gFileToNodeCategory["Sequence"] = NodeDef::Category::SEQUENCE;

        gCategories_.push_back({ "Element", [](AINBNodeMgr::NodeDef* Def)
        {
            return Def->mNodeType != application::file::game::ainb::AINBFile::NodeTypes::UserDefined;
        } });
        gCategories_.push_back({ "Operate", [](AINBNodeMgr::NodeDef* Def)
        {
            return Def->mName.starts_with("Operate") && !Def->mName.ends_with(".module");
        } });
        gCategories_.push_back({ "Actor Logic", [](AINBNodeMgr::NodeDef* Def)
        {
            return Def->mName.starts_with("ActorLogic") && !Def->mName.ends_with(".module");
        } });
        gCategories_.push_back({ "Hold", [](AINBNodeMgr::NodeDef* Def)
        {
            return Def->mName.starts_with("Hold") && !Def->mName.ends_with(".module");
        } });
        gCategories_.push_back({ "Trigger", [](AINBNodeMgr::NodeDef* Def)
        {
            return Def->mName.starts_with("Trigger") && !Def->mName.ends_with(".module");
        } });

        gCategories_.push_back({ "Event", [](AINBNodeMgr::NodeDef* Def)
        {
            return Def->mName.starts_with("Event") && !Def->mName.ends_with(".module");
        } });
        gCategories_.push_back({ "Query", [](AINBNodeMgr::NodeDef* Def)
        {
            return (Def->mName.starts_with("Query") || Def->mName.starts_with("SeqQuery")) && !Def->mName.ends_with(".module");
        } });
        gCategories_.push_back({ "OneShot", [](AINBNodeMgr::NodeDef* Def)
        {
            return (Def->mName.starts_with("OneShot") || Def->mName.starts_with("Oneshot") || Def->mName.starts_with("SeqOneShot")) && !Def->mName.ends_with(".module");
        } });
        gCategories_.push_back({ "Execute", [](AINBNodeMgr::NodeDef* Def)
        {
            return (Def->mName.starts_with("Execute") || Def->mName.starts_with("SeqExecute")) && !Def->mName.ends_with(".module");
        } });
        gCategories_.push_back({ "Selector", [](AINBNodeMgr::NodeDef* Def)
        {
            return (Def->mName.starts_with("Selector") || Def->mName.starts_with("SeqSelector")) && !Def->mName.ends_with(".module") && Def->mNodeType == application::file::game::ainb::AINBFile::NodeTypes::UserDefined;
        } });
        gCategories_.push_back({ "Module", [](AINBNodeMgr::NodeDef* Def)
        {
            return Def->mName.ends_with(".module");
        } });

        for(auto& [Category, Func] : gCategories_)
        {
            gNodeCategories.push_back({ Category, {} });
        }
        gNodeCategories.push_back({ "Default", {} });

        //LoadFromFile();
#if defined(TOOL_MARIO_WONDER_AINB)
        LoadFromFile("DefinitionsWonder");
#else
        LoadFromFile();
#endif

        {
            AINBNodeMgr::NodeDef Def;
            Def.mName = "Element_Simultaneous";
            Def.mNodeType = application::file::game::ainb::AINBFile::NodeTypes::Element_Simultaneous;
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "EndPolicy",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::String
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "ResultPolicy",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::String
            });
            Def.mFlowOutputParameters.push_back("Control");
            Def.mCategories.push_back(AINBNodeMgr::NodeDef::Category::AI);
            Def.mCategories.push_back(AINBNodeMgr::NodeDef::Category::SEQUENCE);
            gNodeDefinitions.push_back(Def);
        }

        {
            AINBNodeMgr::NodeDef Def;
            Def.mName = "Element_BoolSelector";
            Def.mNodeType = application::file::game::ainb::AINBFile::NodeTypes::Element_BoolSelector;
            Def.mInputParameters.push_back(NodeDef::ParameterDef{
                .mName = "Input",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Bool
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "CalculateTiming",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Int
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "ChildFrameSync",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Bool
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "InputValue",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Bool
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "IsNoSelectWhenChildBusy",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Bool
            });
            Def.mFlowOutputParameters.push_back("True");
            Def.mFlowOutputParameters.push_back("False");
            Def.mCategories.push_back(AINBNodeMgr::NodeDef::Category::AI);
            Def.mCategories.push_back(AINBNodeMgr::NodeDef::Category::SEQUENCE);
            gNodeDefinitions.push_back(Def);
        }

        {
            AINBNodeMgr::NodeDef Def;
            Def.mName = "Element_S32Selector";
            Def.mNodeType = application::file::game::ainb::AINBFile::NodeTypes::Element_S32Selector;
            Def.mInputParameters.push_back(NodeDef::ParameterDef{
                .mName = "Input",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Int
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "CalculateTiming",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Int
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "InputValue",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Int
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "ChildFrameSync",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Bool
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "IsNoSelectIfSameInstance",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Bool
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "IsNoSelectWhenChildBusy",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Bool
            });
            Def.mFlowOutputParameters.push_back("Default");
            Def.mCategories.push_back(AINBNodeMgr::NodeDef::Category::AI);
            Def.mCategories.push_back(AINBNodeMgr::NodeDef::Category::SEQUENCE);
            gNodeDefinitions.push_back(Def);
        }

        {
            AINBNodeMgr::NodeDef Def;
            Def.mName = "Element_Sequential";
            Def.mNodeType = application::file::game::ainb::AINBFile::NodeTypes::Element_Sequential;
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "BusyPolicy",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Int
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "NumLoop",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Int
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "PlayPolicy",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Int
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "ResultPolicy",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Int
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "IsDealyJudge",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Bool
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "IsLoop",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Bool
            });
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "IsUpdateNextInFrame",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Bool
            });
            Def.mCategories.push_back(AINBNodeMgr::NodeDef::Category::AI);
            Def.mCategories.push_back(AINBNodeMgr::NodeDef::Category::SEQUENCE);
            gNodeDefinitions.push_back(Def);
        }

        {
            AINBNodeMgr::NodeDef Def;
            Def.mName = "Element_SplitTiming";
            Def.mNodeType = application::file::game::ainb::AINBFile::NodeTypes::Element_SplitTiming;
            Def.mImmediateParameters.push_back(NodeDef::ParameterDef{
                .mName = "ChildStateSyncPolicy",
                .mClass = "",
                .mValueType = application::file::game::ainb::AINBFile::ValueType::Int
            });
            Def.mFlowOutputParameters.push_back("Enter");
            Def.mFlowOutputParameters.push_back("Update");
            Def.mFlowOutputParameters.push_back("Leave");
            Def.mCategories.push_back(AINBNodeMgr::NodeDef::Category::AI);
            Def.mCategories.push_back(AINBNodeMgr::NodeDef::Category::SEQUENCE);
            gNodeDefinitions.push_back(Def);
        }

        for(AINBNodeMgr::NodeDef& Def : gNodeDefinitions)
        {
            bool Found = false;
            for(size_t i = 0; i < gCategories_.size(); i++)
            {
                if(gCategories_[i].second(&Def))
                {
                    Found = true;
                    gNodeCategories[i].second.push_back(&Def);
                    break;
                }
            }
            if(Found)
                continue;

            gNodeCategories.back().second.push_back(&Def);
        }

        auto CheckIfFileCategoryHasNodes = [](const std::string& FileCategory)
        {
            AINBNodeMgr::NodeDef::Category Cat = gFileToNodeCategory[FileCategory];

            for(auto& [Tag, Nodes] : gNodeCategories)
            {
                bool HasNodes = false;

                for(AINBNodeMgr::NodeDef* Def : Nodes)
                {
                    if(std::find(Def->mCategories.begin(), Def->mCategories.end(), Cat) != Def->mCategories.end())
                    {
                        HasNodes = true;
                        break;
                    }
                }

                gHasNodeCategoryNodes[FileCategory][Tag] = HasNodes;
            }
        };

        CheckIfFileCategoryHasNodes("AI");
        CheckIfFileCategoryHasNodes("Logic");
        CheckIfFileCategoryHasNodes("Sequence");

        return true;
    }

    void AINBNodeMgr::CalculateMaxDisplayLength()
    {
        gMaxDisplayLength = 0.0f;
        for(AINBNodeMgr::NodeDef& Def : gNodeDefinitions)
        {
            gMaxDisplayLength = std::max(gMaxDisplayLength, ImGui::CalcTextSize(Def.mName.c_str()).x);
        }
        gMaxDisplayLength += ImGui::GetStyle().IndentSpacing;
    }

    void AINBNodeMgr::SetDefaultValue(application::file::game::ainb::AINBFile::AINBValue& Value, application::file::game::ainb::AINBFile::ValueType ValueType)
    {
        switch (ValueType)
        {
        case application::file::game::ainb::AINBFile::ValueType::Bool:
            Value = false;
            break;
        case application::file::game::ainb::AINBFile::ValueType::Int:
            Value = (uint32_t)0;
            break;
        case application::file::game::ainb::AINBFile::ValueType::Float:
            Value = 0.0f;
            break;
        case application::file::game::ainb::AINBFile::ValueType::String:
            Value = "";
            break;
        case application::file::game::ainb::AINBFile::ValueType::Vec3f:
            Value = glm::vec3(0, 0, 0);
            break;
        case application::file::game::ainb::AINBFile::ValueType::UserDefined:
            Value = (uint32_t)0;
            break;
        default:
            application::util::Logger::Error("AINBNodeMgr", "Unknown input parameter value type: %u", (int)ValueType);
            break;
        }
    }

    void AINBNodeMgr::AddNode(application::file::game::ainb::AINBFile& AINB, NodeDef* Def)
    {
        if(Def == nullptr)
        {
            application::util::Logger::Error("AINBNodeMgr", "Could not add node, definition was a nullptr");
            return;
        }

        application::file::game::ainb::AINBFile::Node Node;
        Node.Type = (uint16_t)Def->mNodeType;
        Node.Name = Def->mName;
        Node.NodeIndex = AINB.Nodes.size();
        Node.NameHash = Def->mNameHash;

        Node.InputParameters.resize(application::file::game::ainb::AINBFile::ValueTypeCount);
        Node.OutputParameters.resize(application::file::game::ainb::AINBFile::ValueTypeCount);
        Node.ImmediateParameters.resize(application::file::game::ainb::AINBFile::ValueTypeCount);

        for(NodeDef::ParameterDef& Def : Def->mInputParameters)
        {
            application::file::game::ainb::AINBFile::InputEntry Entry;
            Entry.Name = Def.mName;
            Entry.Class = Def.mClass;
            Entry.ValueType = (int)Def.mValueType;
            Entry.Flags = Def.mFlags;
            for (auto Iter = Entry.Flags.begin(); Iter != Entry.Flags.end(); )
            {
                if (((application::file::game::ainb::AINBFile::FlagsStruct)*Iter) == application::file::game::ainb::AINBFile::FlagsStruct::SetPointerFlagBitZero)
                {
                    Iter = Entry.Flags.erase(Iter);
                    continue;
                }
                
                Iter++;
            }

            SetDefaultValue(Entry.Value, Def.mValueType);

            Node.InputParameters[Entry.ValueType].push_back(Entry);
        }

        for(NodeDef::ParameterDef& Def : Def->mImmediateParameters)
        {
            application::file::game::ainb::AINBFile::ImmediateParameter Entry;
            Entry.Name = Def.mName;
            Entry.Class = Def.mClass;
            Entry.ValueType = (int)Def.mValueType;
            Entry.Flags = Def.mFlags;

            SetDefaultValue(Entry.Value, Def.mValueType);

            Node.ImmediateParameters[Entry.ValueType].push_back(Entry);
        }

        for(NodeDef::ParameterDef& Def : Def->mOutputParameters)
        {
            application::file::game::ainb::AINBFile::OutputEntry Entry;
            Entry.Name = Def.mName;
            Entry.Class = Def.mClass;

            Node.OutputParameters[(int)Def.mValueType].push_back(Entry);
        }

        AINB.Nodes.push_back(Node);
    }

    void AINBNodeMgr::ReloadAdditionalProjectNodes()
    {
        gNodeDefinitions.erase(std::remove_if(gNodeDefinitions.begin(), gNodeDefinitions.end(),
            [](const application::manager::AINBNodeMgr::NodeDef& Def) { return !Def.mProjectName.empty(); }), gNodeDefinitions.end());

        for (const NodeDef& Def : application::file::tool::AdditionalAINBNodes::gAdditionalNodeDefs)
        {
            if (Def.mProjectName == application::manager::ProjectMgr::gProject)
            {
                gNodeDefinitions.push_back(Def);

                bool Found = false;
                for (size_t i = 0; i < gCategories_.size(); i++)
                {
                    if (gCategories_[i].second(&gNodeDefinitions.back()))
                    {
                        Found = true;
                        gNodeCategories[i].second.push_back(&gNodeDefinitions.back());
                        break;
                    }
                }
                if (Found)
                    continue;

                gNodeCategories.back().second.push_back(&gNodeDefinitions.back());
            }
        }
    }

    void AINBNodeMgr::DecodeAINB(std::vector<unsigned char> Bytes, std::string FileName)
    {
        application::file::game::ainb::AINBFile File;
        File.Initialize(Bytes, true);
        if (!File.Loaded || File.Nodes.empty()) return;
        AINBNodeMgr::NodeDef::Category FileCategory = AINBNodeMgr::NodeDef::Category::LOGIC;
        if(File.Header.FileCategory == "AI") FileCategory = AINBNodeMgr::NodeDef::Category::AI;
        else if (File.Header.FileCategory == "Sequence") FileCategory = AINBNodeMgr::NodeDef::Category::SEQUENCE;

        for (application::file::game::ainb::AINBFile::Node& Node : File.Nodes)
        {
            if(Node.Type != (uint16_t)application::file::game::ainb::AINBFile::NodeTypes::UserDefined)
                continue;
            
            bool NodeFound = false;
            for (AINBNodeMgr::NodeDef& Definition : gNodeDefinitions)
            {
                if (Definition.mName == Node.Name)
                {
                    NodeFound = true;
                    //if (Definition.FileNames.size() < 10) Definition.FileNames.push_back(FileName);
                    if(std::find(Definition.mFileNames.begin(), Definition.mFileNames.end(), FileName) == Definition.mFileNames.end())
                        Definition.mFileNames.push_back(FileName);

                    if (std::find(Definition.mCategories.begin(), Definition.mCategories.end(), FileCategory) == Definition.mCategories.end()) Definition.mCategories.push_back(FileCategory);
                    for (int i = 0; i < application::file::game::ainb::AINBFile::ValueTypeCount; i++)
                    {
                        for (application::file::game::ainb::AINBFile::OutputEntry& Param : Node.OutputParameters[i])
                        {
                            bool Found = false;
                            for (AINBNodeMgr::NodeDef::ParameterDef& ParamDef : Definition.mOutputParameters)
                            {
                                if (ParamDef.mName == Param.Name && (int)ParamDef.mValueType == i)
                                {
                                    Found = true;
                                    break;
                                }
                            }
                            if (Found) continue;

                            AINBNodeMgr::NodeDef::ParameterDef Def;
                            Def.mName = Param.Name;
                            Def.mClass = Param.Class;
                            Def.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(i);

                            Definition.mOutputParameters.push_back(Def);
                        }
                        for (application::file::game::ainb::AINBFile::InputEntry& Param : Node.InputParameters[i])
                        {
                            bool Found = false;
                            for (AINBNodeMgr::NodeDef::ParameterDef& ParamDef : Definition.mInputParameters)
                            {
                                if (ParamDef.mName == Param.Name && (int)ParamDef.mValueType == Param.ValueType)
                                {
                                    Found = true;
                                    break;
                                }
                            }
                            if (Found) continue;
                            

                            AINBNodeMgr::NodeDef::ParameterDef Def;
                            Def.mName = Param.Name;
                            Def.mClass = Param.Class;
                            Def.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(Param.ValueType);
                            Def.mFlags = Param.Flags;

                            Definition.mInputParameters.push_back(Def);
                        }
                        for (application::file::game::ainb::AINBFile::ImmediateParameter& Param : Node.ImmediateParameters[i])
                        {
                            bool Found = false;
                            for (AINBNodeMgr::NodeDef::ParameterDef& ParamDef : Definition.mImmediateParameters)
                            {
                                if (ParamDef.mName == Param.Name && (int)ParamDef.mValueType == Param.ValueType)
                                {
                                    Found = true;
                                    break;
                                }
                            }
                            if (Found) continue;

                            AINBNodeMgr::NodeDef::ParameterDef Def;
                            Def.mName = Param.Name;
                            Def.mClass = Param.Class;
                            Def.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(Param.ValueType);
                            Def.mFlags = Param.Flags;

                            Definition.mImmediateParameters.push_back(Def);
                        }
                    }


                    for (int NodeLinkType = 0; NodeLinkType < application::file::game::ainb::AINBFile::LinkedNodeTypeCount; NodeLinkType++)
                    {
                        for (application::file::game::ainb::AINBFile::LinkedNodeInfo& NodeLink : Node.LinkedNodes[NodeLinkType])
                        {
                            if (NodeLink.Type == application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink && NodeLink.ConnectionName != "" && NodeLink.ConnectionName != "MapEditor_AINB_NoVal")
                            {
                                if(std::find(Definition.mFlowOutputParameters.begin(), Definition.mFlowOutputParameters.end(), NodeLink.ConnectionName) != Definition.mFlowOutputParameters.end())
                                    continue;
                                
                                Definition.mFlowOutputParameters.push_back(NodeLink.ConnectionName);
                            }
                        }
                    }
                    break;
                }
            }
            if (NodeFound) continue;
            AINBNodeMgr::NodeDef Def;
            Def.mName = Node.Name;
            //if (Def.FileNames.size() < 10) Def.FileNames.push_back(FileName);
            if(std::find(Def.mFileNames.begin(), Def.mFileNames.end(), FileName) == Def.mFileNames.end())
                Def.mFileNames.push_back(FileName);
            Def.mNameHash = Node.NameHash;
            Def.mNodeType = static_cast<application::file::game::ainb::AINBFile::NodeTypes>(Node.Type);
            Def.mCategories.push_back(FileCategory);
            for (int i = 0; i < application::file::game::ainb::AINBFile::ValueTypeCount; i++)
            {
                for (application::file::game::ainb::AINBFile::OutputEntry& Param : Node.OutputParameters[i])
                {
                    AINBNodeMgr::NodeDef::ParameterDef ParamDef;
                    ParamDef.mName = Param.Name;
                    ParamDef.mClass = Param.Class;
                    ParamDef.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(i);

                    Def.mOutputParameters.push_back(ParamDef);
                }
                for (application::file::game::ainb::AINBFile::InputEntry& Param : Node.InputParameters[i])
                {
                    AINBNodeMgr::NodeDef::ParameterDef ParamDef;
                    ParamDef.mName = Param.Name;
                    ParamDef.mClass = Param.Class;
                    ParamDef.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(Param.ValueType);
                    ParamDef.mFlags = Param.Flags;

                    Def.mInputParameters.push_back(ParamDef);
                }
                for (application::file::game::ainb::AINBFile::ImmediateParameter& Param : Node.ImmediateParameters[i])
                {
                    AINBNodeMgr::NodeDef::ParameterDef ParamDef;
                    ParamDef.mName = Param.Name;
                    ParamDef.mClass = Param.Class;
                    ParamDef.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(Param.ValueType);
                    ParamDef.mFlags = Param.Flags;

                    Def.mImmediateParameters.push_back(ParamDef);
                }
            }
            int FlowIdx = 0;
            int FlowCount = 0;
            for (int NodeLinkType = 0; NodeLinkType < application::file::game::ainb::AINBFile::LinkedNodeTypeCount; NodeLinkType++)
            {
                for (application::file::game::ainb::AINBFile::LinkedNodeInfo& NodeLink : Node.LinkedNodes[NodeLinkType])
                {
                    if (NodeLink.Type == application::file::game::ainb::AINBFile::LinkedNodeMapping::StandardLink && NodeLink.ConnectionName != "" && NodeLink.ConnectionName != "MapEditor_AINB_NoVal")
                    {
                        if(std::find(Def.mFlowOutputParameters.begin(), Def.mFlowOutputParameters.end(), NodeLink.ConnectionName) != Def.mFlowOutputParameters.end())
                            continue;
                                
                        Def.mFlowOutputParameters.push_back(NodeLink.ConnectionName);
                    }
                }
            }

            gNodeDefinitions.push_back(Def);
        }
    }

    void AINBNodeMgr::GenerateNodeDefinitionFile()
    {
        using directory_iterator = std::filesystem::directory_iterator;
        for (const auto& DirEntry : directory_iterator(application::util::FileUtil::GetRomFSFilePath("Logic", false)))
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
                    DecodeAINB(Bytes, "Logic/" + DirEntry.path().filename().string());
                }
                else
                {
                    application::util::Logger::Error("AINBNodeMgr", "Could not open file \"%s\"", DirEntry.path().string().c_str());
                }
            }
        }

        for (const auto& DirEntry : directory_iterator(application::util::FileUtil::GetRomFSFilePath("AI", false)))
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
                    DecodeAINB(Bytes, "AI/" + DirEntry.path().filename().string());
                }
                else
                {
                    application::util::Logger::Error("AINBNodeMgr", "Could not open file \"%s\"", DirEntry.path().string().c_str());
                }
            }
        }

        for (const auto& DirEntry : directory_iterator(application::util::FileUtil::GetRomFSFilePath("Sequence", false)))
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
                    DecodeAINB(Bytes, "Sequence/" + DirEntry.path().filename().string());
                }
                else
                {
                    application::util::Logger::Error("AINBNodeMgr", "Could not open file \"%s\"", DirEntry.path().string().c_str());
                }
            }
        }

        for (const auto& DirEntry : directory_iterator(application::util::FileUtil::GetRomFSFilePath("Pack/Actor", false)))
        {
            if (DirEntry.is_regular_file())
            {
                if (DirEntry.path().string().ends_with(".pack.zs"))
                {
                    std::vector<unsigned char> Data = application::file::game::ZStdBackend::Decompress(DirEntry.path().string());
                    if (!Data.empty())
                    {
                        application::file::game::SarcFile File(Data);
                        for (application::file::game::SarcFile::Entry& Entry : File.GetEntries())
                        {
                            if (Entry.mBytes[0] == 'A' && Entry.mBytes[1] == 'I' && Entry.mBytes[2] == 'B')
                            {
                                DecodeAINB(Entry.mBytes, "Pack/Actor/" + DirEntry.path().filename().string() + "/" + Entry.mName);
                            }
                        }
                    }
                }
            }
        }

        WriteToFile();
    }

    void AINBNodeMgr::GenerateNodeDefinitionFileWonder(const std::string& RomFSPath)
    {
        using directory_iterator = std::filesystem::directory_iterator;

        for (const auto& DirEntry : directory_iterator(RomFSPath + "/AI"))
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
                    DecodeAINB(Bytes, "AI/" + DirEntry.path().filename().string());
                }
                else
                {
                    application::util::Logger::Error("AINBNodeMgr", "Could not open file \"%s\"", DirEntry.path().string().c_str());
                }
            }
        }

        for (const auto& DirEntry : directory_iterator(RomFSPath + "/Pack/Actor"))
        {
            if (DirEntry.is_regular_file())
            {
                if (DirEntry.path().string().ends_with(".pack.zs"))
                {
                    std::vector<unsigned char> Data = application::file::game::ZStdBackend::Decompress(DirEntry.path().string());
                    if (!Data.empty())
                    {
                        application::file::game::SarcFile File(Data);
                        for (application::file::game::SarcFile::Entry& Entry : File.GetEntries())
                        {
                            if (Entry.mBytes[0] == 'A' && Entry.mBytes[1] == 'I' && Entry.mBytes[2] == 'B')
                            {
                                DecodeAINB(Entry.mBytes, "Pack/Actor/" + DirEntry.path().filename().string() + "/" + Entry.mName);
                            }
                        }
                    }
                }
            }
        }

        WriteToFile("DefinitionsWonder");
    }

    void AINBNodeMgr::WriteToFile(const std::string& Name)
    {
        application::util::BinaryVectorWriter Writer;

        Writer.WriteBytes("EAINBDEF");
        Writer.WriteInteger(gFileVersion, sizeof(gFileVersion));

        Writer.WriteInteger(gNodeDefinitions.size(), sizeof(uint32_t));
        for(AINBNodeMgr::NodeDef& Def : gNodeDefinitions)
        {
            Writer.WriteInteger(Def.mName.size(), sizeof(uint16_t));
            Writer.WriteBytes(Def.mName.c_str());

            Writer.WriteInteger(Def.mCategories.size(), sizeof(uint16_t));
            for(AINBNodeMgr::NodeDef::Category& Cat : Def.mCategories)
            {
                Writer.WriteInteger((uint8_t)Cat, sizeof(uint8_t));
            }

            Writer.WriteInteger((uint16_t)Def.mNodeType, sizeof(uint16_t));

            Writer.WriteInteger(Def.mFlowOutputParameters.size(), sizeof(uint16_t));
            for(std::string& Str : Def.mFlowOutputParameters)
            {
                Writer.WriteInteger(Str.size(), sizeof(uint16_t));
                Writer.WriteBytes(Str.c_str());
            }

            Writer.WriteInteger(Def.mInputParameters.size(), sizeof(uint16_t));
            for(AINBNodeMgr::NodeDef::ParameterDef& Param : Def.mInputParameters)
            {
                Writer.WriteInteger(Param.mName.size(), sizeof(uint16_t));
                Writer.WriteBytes(Param.mName.c_str());
                Writer.WriteInteger(Param.mClass.size(), sizeof(uint16_t));
                Writer.WriteBytes(Param.mClass.c_str());
                Writer.WriteInteger((uint8_t)Param.mValueType, sizeof(uint8_t));

                Writer.WriteInteger(Param.mFlags.size(), sizeof(uint8_t));
                for (application::file::game::ainb::AINBFile::FlagsStruct Flag : Param.mFlags)
                {
                    Writer.WriteInteger((uint8_t)Flag, sizeof(uint8_t));
                }
            }

            Writer.WriteInteger(Def.mOutputParameters.size(), sizeof(uint16_t));
            for(AINBNodeMgr::NodeDef::ParameterDef& Param : Def.mOutputParameters)
            {
                Writer.WriteInteger(Param.mName.size(), sizeof(uint16_t));
                Writer.WriteBytes(Param.mName.c_str());
                Writer.WriteInteger(Param.mClass.size(), sizeof(uint16_t));
                Writer.WriteBytes(Param.mClass.c_str());
                Writer.WriteInteger((uint8_t)Param.mValueType, sizeof(uint8_t));
            }

            Writer.WriteInteger(Def.mImmediateParameters.size(), sizeof(uint16_t));
            for(AINBNodeMgr::NodeDef::ParameterDef& Param : Def.mImmediateParameters)
            {
                Writer.WriteInteger(Param.mName.size(), sizeof(uint16_t));
                Writer.WriteBytes(Param.mName.c_str());
                Writer.WriteInteger(Param.mClass.size(), sizeof(uint16_t));
                Writer.WriteBytes(Param.mClass.c_str());
                Writer.WriteInteger((uint8_t)Param.mValueType, sizeof(uint8_t));

                Writer.WriteInteger(Param.mFlags.size(), sizeof(uint8_t));
                for (application::file::game::ainb::AINBFile::FlagsStruct Flag : Param.mFlags)
                {
                    Writer.WriteInteger((uint8_t)Flag, sizeof(uint8_t));
                }
            }

            Writer.WriteInteger(Def.mFileNames.size(), sizeof(uint16_t));
            for(std::string& Str : Def.mFileNames)
            {
                Writer.WriteInteger(Str.size(), sizeof(uint16_t));
                Writer.WriteBytes(Str.c_str());
            }

            Writer.WriteInteger(Def.mNameHash, sizeof(uint32_t));
        }

        std::ofstream FileOut(application::util::FileUtil::GetWorkingDirFilePath(Name + ".eainbdef"), std::ios::binary);
        std::vector<unsigned char> Binary = Writer.GetData();
        std::copy(Binary.cbegin(), Binary.cend(),
            std::ostream_iterator<unsigned char>(FileOut));
        FileOut.close();
    }

    bool AINBNodeMgr::LoadFromFile(const std::string& Name)
    {
        std::ifstream File(application::util::FileUtil::GetWorkingDirFilePath(Name + ".eainbdef"), std::ios::binary);

        if (!File.eof() && !File.fail())
        {
            File.seekg(0, std::ios_base::end);
            std::streampos FileSize = File.tellg();

            std::vector<unsigned char> Bytes(FileSize);

            File.seekg(0, std::ios_base::beg);
            File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);
            File.close();


            application::util::BinaryVectorReader Reader(Bytes);
            Reader.Seek(8, application::util::BinaryVectorReader::Position::Begin); //Skip magic
            uint16_t FileVersion = Reader.ReadUInt16();
            if(FileVersion != gFileVersion)
            {
                application::util::Logger::Error("AINBNodeMgr", "Invalid version, got %u, expected %u", FileVersion, gFileVersion);
                return false;
            }

            auto ReadString = [&Reader]()
            {
                uint16_t Length = Reader.ReadUInt16();
                std::string Result;
                Result.resize(Length);
                Reader.ReadStruct(Result.data(), Length);
                return Result;
            };

            gNodeDefinitions.resize(Reader.ReadUInt32());
            for(size_t i = 0; i < gNodeDefinitions.size(); i++)
            {
                AINBNodeMgr::NodeDef& Def = gNodeDefinitions[i];
                Def.mName = ReadString();

                uint16_t CategoryLength = Reader.ReadUInt16();
                Def.mCategories.resize(CategoryLength);
                for(size_t j = 0; j < CategoryLength; j++)
                {
                    Def.mCategories[j] = static_cast<AINBNodeMgr::NodeDef::Category>(Reader.ReadUInt8());
                }

                Def.mNodeType = static_cast<application::file::game::ainb::AINBFile::NodeTypes>(Reader.ReadUInt16());

                uint16_t FlowParametersSize = Reader.ReadUInt16();
                Def.mFlowOutputParameters.resize(FlowParametersSize);
                for(size_t j = 0; j < FlowParametersSize; j++)
                {
                    Def.mFlowOutputParameters[j] = ReadString();
                }

                uint16_t InputParametersSize = Reader.ReadUInt16();
                Def.mInputParameters.resize(InputParametersSize);
                for(size_t j = 0; j < InputParametersSize; j++)
                {
                    AINBNodeMgr::NodeDef::ParameterDef& Param = Def.mInputParameters[j];
                    Param.mName = ReadString();
                    Param.mClass = ReadString();
                    Param.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(Reader.ReadUInt8());

                    uint8_t FlagsSize = Reader.ReadUInt8();
                    for (int k = 0; k < FlagsSize; k++)
                    {
                        Param.mFlags.push_back((application::file::game::ainb::AINBFile::FlagsStruct)Reader.ReadUInt8());
                    }
                }

                uint16_t OutputParametersSize = Reader.ReadUInt16();
                Def.mOutputParameters.resize(OutputParametersSize);
                for(size_t j = 0; j < OutputParametersSize; j++)
                {
                    AINBNodeMgr::NodeDef::ParameterDef& Param = Def.mOutputParameters[j];
                    Param.mName = ReadString();
                    Param.mClass = ReadString();
                    Param.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(Reader.ReadUInt8());
                }

                uint16_t ImmediateParametersSize = Reader.ReadUInt16();
                Def.mImmediateParameters.resize(ImmediateParametersSize);
                for(size_t j = 0; j < ImmediateParametersSize; j++)
                {
                    AINBNodeMgr::NodeDef::ParameterDef& Param = Def.mImmediateParameters[j];
                    Param.mName = ReadString();
                    Param.mClass = ReadString();
                    Param.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(Reader.ReadUInt8());

                    uint8_t FlagsSize = Reader.ReadUInt8();
                    for (int k = 0; k < FlagsSize; k++)
                    {
                        Param.mFlags.push_back((application::file::game::ainb::AINBFile::FlagsStruct)Reader.ReadUInt8());
                    }
                }

                uint16_t FileNamesSize = Reader.ReadUInt16();
                Def.mFileNames.resize(FileNamesSize);
                for(size_t j = 0; j < FileNamesSize; j++)
                {
                    Def.mFileNames[j] = ReadString();
                }

                Def.mNameHash = Reader.ReadUInt32();
            }
        }
        return false;
    }
}