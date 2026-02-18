#pragma once

#include <rendering/ainb/UIAINBEditorNodeBase.h>
#include <vector>
#include <string>
#include <manager/AINBNodeMgr.h>

namespace application::rendering::ainb::nodes
{
	class UIAINBEditorNodeDefault : public application::rendering::ainb::UIAINBEditorNodeBase
	{
	public:
		UIAINBEditorNodeDefault(int UniqueId, application::file::game::ainb::AINBFile::Node& Node);

		virtual void DrawImpl() override;
		virtual ImColor GetNodeColor() override;
		virtual int GetNodeIndex() override;
		virtual bool HasInternalParameters() override;
		virtual void RenderLinks(std::vector<std::unique_ptr<UIAINBEditorNodeBase>>& Nodes) override;
		virtual bool HasFlowOutputParameters() override;
		virtual void PostProcessLinkedNodeInfo(Pin& StartPin, application::file::game::ainb::AINBFile::LinkedNodeInfo& Info) override;

		void InjectFixedFlowParameters(application::manager::AINBNodeMgr::NodeDef* Def);

	private:
		ImColor mColor;
		std::vector<std::string> mFixedFlowOutputParameters;
	};
}