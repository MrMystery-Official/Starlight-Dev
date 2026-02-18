#pragma once

#include <rendering/ainb/UIAINBEditorNodeBase.h>

namespace application::rendering::ainb::nodes
{
	class UIAINBEditorNodeSplitTiming : public application::rendering::ainb::UIAINBEditorNodeBase
	{
	public:
		UIAINBEditorNodeSplitTiming(int UniqueId, application::file::game::ainb::AINBFile::Node& Node);

		virtual void DrawImpl() override;
		virtual ImColor GetNodeColor() override;
		virtual int GetNodeIndex() override;
		virtual bool HasInternalParameters() override;
		virtual void RenderLinks(std::vector<std::unique_ptr<UIAINBEditorNodeBase>>& Nodes) override;
		virtual bool FinalizeNode() override;
		virtual bool HasFlowOutputParameters() override;
		virtual void PostProcessLinkedNodeInfo(Pin& StartPin, application::file::game::ainb::AINBFile::LinkedNodeInfo& Info) override;
	};
}