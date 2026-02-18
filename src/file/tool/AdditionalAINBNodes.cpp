#include <file/tool/AdditionalAINBNodes.h>

#include <file/game/byml/BymlFile.h>
#include <util/FileUtil.h>
#include <util/Math.h>

namespace application::file::tool
{
    std::vector<application::manager::AINBNodeMgr::NodeDef> AdditionalAINBNodes::gAdditionalNodeDefs;

	void AdditionalAINBNodes::Load(const std::string& Path)
	{
        if (!application::util::FileUtil::FileExists(Path))
            return;

        gAdditionalNodeDefs.clear();

		application::file::game::byml::BymlFile File(Path);

		for (auto& Node : File.GetNode("Nodes")->GetChildren())
		{
            application::manager::AINBNodeMgr::NodeDef Def;
            Def.mProjectName = Node.GetChild("Project")->GetValue<std::string>();
            Def.mName = Node.GetChild("Name")->GetValue<std::string>();
            Def.mNameHash = application::util::Math::StringHashMurMur3_32(Def.mName);
            Def.mFileNames.push_back("Not processable, this is a custom node");

            for (auto& Category : Node.GetChild("Categories")->GetChildren())
            {
                std::string Val = Category.GetValue<std::string>();
                application::manager::AINBNodeMgr::NodeDef::Category Cat = application::manager::AINBNodeMgr::NodeDef::Category::LOGIC;
                if(Val == "AI") Cat = application::manager::AINBNodeMgr::NodeDef::Category::AI;
                if(Val == "Sequence") Cat = application::manager::AINBNodeMgr::NodeDef::Category::SEQUENCE;
                Def.mCategories.push_back(Cat);
            }

            Def.mNodeType = static_cast<application::file::game::ainb::AINBFile::NodeTypes>((uint16_t)Node.GetChild("Type")->GetValue<uint32_t>());

            if (Node.HasChild("ExecutionFlowParameters"))
            {
                for (auto& FlowParam : Node.GetChild("ExecutionFlowParameters")->GetChildren())
                {
                    Def.mFlowOutputParameters.push_back(FlowParam.GetValue<std::string>());
                }
            }

            if (Node.HasChild("InputParameters"))
            {
                for (auto& InputParam : Node.GetChild("InputParameters")->GetChildren())
                {
                    application::manager::AINBNodeMgr::NodeDef::ParameterDef Param;

                    Param.mName = InputParam.GetChild("Name")->GetValue<std::string>();
                    Param.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(InputParam.GetChild("Type")->GetValue<uint32_t>());
                    if(InputParam.HasChild("Class")) Param.mClass = InputParam.GetChild("Class")->GetValue<std::string>();

                    if (InputParam.HasChild("Flags"))
                    {
                        for (auto& Flag : InputParam.GetChild("Flags")->GetChildren())
                        {
                            std::string Val = Flag.GetValue<std::string>();
                            application::file::game::ainb::AINBFile::FlagsStruct ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::PulseThreadLocalStorage;

                            if(Val == "SetPointerFlagBitZero") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::SetPointerFlagBitZero;
                            if(Val == "IsValidUpdate") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsValidUpdate;
                            if(Val == "UpdatePostCurrentCommandCalc") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::UpdatePostCurrentCommandCalc;
                            if(Val == "IsPreconditionNode") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsPreconditionNode;
                            if(Val == "IsExternalAINB") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsExternalAINB;
                            if(Val == "IsResidentNode") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsResidentNode;

                            Param.mFlags.push_back(ResultingFlag);
                        }
                    }

                    Def.mInputParameters.push_back(Param);
                }
            }

            if (Node.HasChild("OutputParameters"))
            {
                for (auto& InputParam : Node.GetChild("OutputParameters")->GetChildren())
                {
                    application::manager::AINBNodeMgr::NodeDef::ParameterDef Param;

                    Param.mName = InputParam.GetChild("Name")->GetValue<std::string>();
                    Param.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(InputParam.GetChild("Type")->GetValue<uint32_t>());
                    if (InputParam.HasChild("Class")) Param.mClass = InputParam.GetChild("Class")->GetValue<std::string>();

                    if (InputParam.HasChild("Flags"))
                    {
                        for (auto& Flag : InputParam.GetChild("Flags")->GetChildren())
                        {
                            std::string Val = Flag.GetValue<std::string>();
                            application::file::game::ainb::AINBFile::FlagsStruct ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::PulseThreadLocalStorage;

                            if (Val == "SetPointerFlagBitZero") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::SetPointerFlagBitZero;
                            if (Val == "IsValidUpdate") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsValidUpdate;
                            if (Val == "UpdatePostCurrentCommandCalc") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::UpdatePostCurrentCommandCalc;
                            if (Val == "IsPreconditionNode") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsPreconditionNode;
                            if (Val == "IsExternalAINB") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsExternalAINB;
                            if (Val == "IsResidentNode") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsResidentNode;

                            Param.mFlags.push_back(ResultingFlag);
                        }
                    }

                    Def.mOutputParameters.push_back(Param);
                }
            }

            if (Node.HasChild("ImmediateParameters"))
            {
                for (auto& InputParam : Node.GetChild("ImmediateParameters")->GetChildren())
                {
                    application::manager::AINBNodeMgr::NodeDef::ParameterDef Param;

                    Param.mName = InputParam.GetChild("Name")->GetValue<std::string>();
                    Param.mValueType = static_cast<application::file::game::ainb::AINBFile::ValueType>(InputParam.GetChild("Type")->GetValue<uint32_t>());
                    if (InputParam.HasChild("Class")) Param.mClass = InputParam.GetChild("Class")->GetValue<std::string>();

                    if (InputParam.HasChild("Flags"))
                    {
                        for (auto& Flag : InputParam.GetChild("Flags")->GetChildren())
                        {
                            std::string Val = Flag.GetValue<std::string>();
                            application::file::game::ainb::AINBFile::FlagsStruct ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::PulseThreadLocalStorage;

                            if (Val == "SetPointerFlagBitZero") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::SetPointerFlagBitZero;
                            if (Val == "IsValidUpdate") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsValidUpdate;
                            if (Val == "UpdatePostCurrentCommandCalc") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::UpdatePostCurrentCommandCalc;
                            if (Val == "IsPreconditionNode") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsPreconditionNode;
                            if (Val == "IsExternalAINB") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsExternalAINB;
                            if (Val == "IsResidentNode") ResultingFlag = application::file::game::ainb::AINBFile::FlagsStruct::IsResidentNode;

                            Param.mFlags.push_back(ResultingFlag);
                        }
                    }

                    Def.mImmediateParameters.push_back(Param);
                }
            }

            gAdditionalNodeDefs.push_back(Def);
		}
	}

	void AdditionalAINBNodes::Save(const std::string& Path)
	{
        if (gAdditionalNodeDefs.empty())
            return;

        application::file::game::byml::BymlFile File;
        File.GetType() = application::file::game::byml::BymlFile::Type::Dictionary;

        application::file::game::byml::BymlFile::Node Root(application::file::game::byml::BymlFile::Type::Array, "Nodes");

        for (application::manager::AINBNodeMgr::NodeDef& Def : gAdditionalNodeDefs)
        {
            application::file::game::byml::BymlFile::Node BymlDef(application::file::game::byml::BymlFile::Type::Dictionary, Def.mName);

            BymlDef.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, "Project", Def.mProjectName });
            BymlDef.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, "Name", Def.mName });

            application::file::game::byml::BymlFile::Node Categories(application::file::game::byml::BymlFile::Type::Array, "Categories");

            for (auto& Category : Def.mCategories)
            {
                std::string Val = "Logic";
                if (Category == application::manager::AINBNodeMgr::NodeDef::Category::AI) Val = "AI";
                if (Category == application::manager::AINBNodeMgr::NodeDef::Category::SEQUENCE) Val = "Sequence";
                Categories.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, Val, Val });
            }

            BymlDef.AddChild(Categories);

            BymlDef.AddChild({ application::file::game::byml::BymlFile::Type::UInt32, "Type", static_cast<uint16_t>(Def.mNodeType) });

            if (!Def.mFlowOutputParameters.empty())
            {
                application::file::game::byml::BymlFile::Node FlowParams(application::file::game::byml::BymlFile::Type::Array, "ExecutionFlowParameters");

                for (std::string& Param : Def.mFlowOutputParameters)
                {
                    FlowParams.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, Param, Param });
                }

                BymlDef.AddChild(FlowParams);
            }

            if (!Def.mInputParameters.empty())
            {
                application::file::game::byml::BymlFile::Node InputParams(application::file::game::byml::BymlFile::Type::Array, "InputParameters");

                for (auto& Param : Def.mInputParameters)
                {
                    application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::Dictionary);

                    Node.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, "Name", Param.mName });
                    Node.AddChild({ application::file::game::byml::BymlFile::Type::UInt32, "Type", static_cast<uint32_t>(Param.mValueType) });
                    if(!Param.mClass.empty()) Node.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, "Class", Param.mClass });

                    if (!Param.mFlags.empty())
                    {
                        application::file::game::byml::BymlFile::Node Flags(application::file::game::byml::BymlFile::Type::Array);

                        for (auto& Flag : Param.mFlags)
                        {
                            std::string Val = "PulseThreadLocalStorage";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::SetPointerFlagBitZero) Val = "SetPointerFlagBitZero";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsValidUpdate) Val = "IsValidUpdate";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::UpdatePostCurrentCommandCalc) Val = "UpdatePostCurrentCommandCalc";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsPreconditionNode) Val = "IsPreconditionNode";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsExternalAINB) Val = "IsExternalAINB";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsResidentNode) Val = "IsResidentNode";

                            Flags.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, Val, Val });
                        }

                        Node.AddChild(Flags);
                    }

                    InputParams.AddChild(Node);
                }

                BymlDef.AddChild(InputParams);
            }

            if (!Def.mOutputParameters.empty())
            {
                application::file::game::byml::BymlFile::Node OutputParams(application::file::game::byml::BymlFile::Type::Array, "OutputParameters");

                for (auto& Param : Def.mOutputParameters)
                {
                    application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::Dictionary);

                    Node.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, "Name", Param.mName });
                    Node.AddChild({ application::file::game::byml::BymlFile::Type::UInt32, "Type", static_cast<uint32_t>(Param.mValueType) });
                    if (!Param.mClass.empty()) Node.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, "Class", Param.mClass });

                    if (!Param.mFlags.empty())
                    {
                        application::file::game::byml::BymlFile::Node Flags(application::file::game::byml::BymlFile::Type::Array);

                        for (auto& Flag : Param.mFlags)
                        {
                            std::string Val = "PulseThreadLocalStorage";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::SetPointerFlagBitZero) Val = "SetPointerFlagBitZero";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsValidUpdate) Val = "IsValidUpdate";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::UpdatePostCurrentCommandCalc) Val = "UpdatePostCurrentCommandCalc";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsPreconditionNode) Val = "IsPreconditionNode";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsExternalAINB) Val = "IsExternalAINB";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsResidentNode) Val = "IsResidentNode";

                            Flags.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, Val, Val });
                        }

                        Node.AddChild(Flags);
                    }

                    OutputParams.AddChild(Node);
                }

                BymlDef.AddChild(OutputParams);
            }

            if (!Def.mImmediateParameters.empty())
            {
                application::file::game::byml::BymlFile::Node ImmediateParams(application::file::game::byml::BymlFile::Type::Array, "ImmediateParameters");

                for (auto& Param : Def.mImmediateParameters)
                {
                    application::file::game::byml::BymlFile::Node Node(application::file::game::byml::BymlFile::Type::Dictionary);

                    Node.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, "Name", Param.mName });
                    Node.AddChild({ application::file::game::byml::BymlFile::Type::UInt32, "Type", static_cast<uint32_t>(Param.mValueType) });
                    if (!Param.mClass.empty()) Node.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, "Class", Param.mClass });

                    if (!Param.mFlags.empty())
                    {
                        application::file::game::byml::BymlFile::Node Flags(application::file::game::byml::BymlFile::Type::Array);

                        for (auto& Flag : Param.mFlags)
                        {
                            std::string Val = "PulseThreadLocalStorage";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::SetPointerFlagBitZero) Val = "SetPointerFlagBitZero";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsValidUpdate) Val = "IsValidUpdate";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::UpdatePostCurrentCommandCalc) Val = "UpdatePostCurrentCommandCalc";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsPreconditionNode) Val = "IsPreconditionNode";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsExternalAINB) Val = "IsExternalAINB";
                            if (Flag == application::file::game::ainb::AINBFile::FlagsStruct::IsResidentNode) Val = "IsResidentNode";

                            Flags.AddChild({ application::file::game::byml::BymlFile::Type::StringIndex, Val, Val });
                        }

                        Node.AddChild(Flags);
                    }

                    ImmediateParams.AddChild(Node);
                }

                BymlDef.AddChild(ImmediateParams);
            }

            Root.AddChild(BymlDef);
        }

        File.GetNodes().push_back(Root);

        File.WriteToFile(Path, application::file::game::byml::BymlFile::TableGeneration::AUTO);
	}
}