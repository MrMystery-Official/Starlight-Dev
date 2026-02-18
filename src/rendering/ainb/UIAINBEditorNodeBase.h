#pragma once

#include "imgui_node_editor.h"
#include "imgui_internal.h"
#include <file/game/ainb/AINBFile.h>
#include <string>
#include <unordered_map>

namespace ed = ax::NodeEditor;

namespace application::rendering::ainb
{
	class UIAINBEditorNodeBase
	{
	public:
		struct NodeShapeInfo
		{
			ImVec2 mHeaderMin;
			ImVec2 mHeaderMax;
		};

		enum class PinType : uint8_t
		{
			Int = 0,
			Bool = 1,
			Float = 2,
			String = 3,
			Vec3f = 4,
			UserDefined = 5,
			Flow = 6
		};

		struct Pin
		{
			ed::PinKind mKind;
			PinType mType;
			application::file::game::ainb::AINBFile::Node* mNode;
			void* mObjectPtr = nullptr;
			int32_t mParameterIndex = -1;
			bool mAllowMultipleLinks = true;
			bool mAlreadyLinked = false;
		};

		enum class LinkType
		{
			Parameter,
			Flow
		};

		struct Link
		{
			void* mObjectPtr;
			LinkType mType;
			uint16_t mNodeIndex;
			uint16_t mParameterIndex;
		};

		UIAINBEditorNodeBase(int UniqueId, application::file::game::ainb::AINBFile::Node& Node);
		virtual ~UIAINBEditorNodeBase() = default;
		
		void Reset();

		void DrawNodeHeader(const std::string& Title, application::file::game::ainb::AINBFile::Node* Node = nullptr);
		std::string GetValueTypeName(application::file::game::ainb::AINBFile::ValueType ValueType);
		std::string GetValueTypeName(uint8_t ValueType);
        ImColor GetValueTypeColor(application::file::game::ainb::AINBFile::ValueType ValueType);
        ImColor GetPinTypeColor(PinType Type);
        ImColor GetValueTypeColor(uint8_t ValueType);
        UIAINBEditorNodeBase::PinType ValueTypeToPinType(application::file::game::ainb::AINBFile::ValueType Type);
        UIAINBEditorNodeBase::PinType ValueTypeToPinType(uint8_t Type);
		void DrawPinIcon(bool Connected, uint32_t Alpha, PinType Type);
		ImRect DrawPin(uint32_t Id, bool Connected, uint32_t Alpha, PinType Type, ed::PinKind Kind, std::string Name, bool IsHeaderPin = false);
		void DrawParameterValue(application::file::game::ainb::AINBFile::ValueType Type, const std::string& Name, uint32_t Id, void* Dest);
		void Draw();
		void DrawInternalParameterSeparator();

		void DrawInputParameter(uint8_t Type, uint8_t Index);
		void DrawInternalParameter(uint8_t Type, uint8_t Index);
		void DrawOutputParameter(uint8_t Type, uint8_t Index);
		void DrawOutputFlowParameter(const std::string& Text, bool Linked, uint8_t Index, bool AllowMultipleLinks = false);

		virtual void DrawImpl() = 0;
		virtual ImColor GetNodeColor() = 0;
		virtual int GetNodeIndex();
		virtual bool HasInternalParameters();
		virtual void RenderLinks(std::vector<std::unique_ptr<UIAINBEditorNodeBase>>& Nodes) = 0;
		virtual void PostProcessLinkedNodeInfo(Pin& StartPin, application::file::game::ainb::AINBFile::LinkedNodeInfo& Info) {}
		virtual bool FinalizeNode() { return true; }
		virtual bool HasFlowOutputParameters() = 0;

		int mUniqueId = 0;
		int mNodeId;
		bool mEnableFlow = false;
		bool mFlowLinked = false;
		bool mIsEntryPoint = false;
		NodeShapeInfo mNodeShapeInfo;
		std::vector<std::vector<uint32_t>> mOutputParameters; //6 value types -> vector of output pin ids
		std::unordered_map<std::string, uint32_t> mOutputFlowParameters; //name -> pin id
		std::vector<std::vector<uint32_t>> mInputParameters; // 6 value types -> vector of input pin ids
		std::vector<std::vector<uint32_t>> mLinkedOutputParams; // 6 value types -> vector of output param indexes
		std::unordered_map<uint32_t, Pin> mPins;
		std::unordered_map<uint32_t, Link> mLinks;
		application::file::game::ainb::AINBFile::Node* mNode;
	private:
		ImVec2 mInternalParameterStartPos;
	};
}