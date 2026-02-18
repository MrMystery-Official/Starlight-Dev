#pragma once

#include <rendering/ainb/UIAINBEditorNodeBase.h>

namespace application::rendering::ainb::nodes
{
	class UIAINBEditorNodeSimultaneous : public application::rendering::ainb::UIAINBEditorNodeBase
	{
	public:
		UIAINBEditorNodeSimultaneous(int UniqueId, application::file::game::ainb::AINBFile::Node& Node);

		virtual void DrawImpl() override;
		virtual ImColor GetNodeColor() override;
		virtual int GetNodeIndex() override;
		virtual bool HasInternalParameters() override;
		virtual void RenderLinks(std::vector<std::unique_ptr<UIAINBEditorNodeBase>>& Nodes) override;
		virtual bool FinalizeNode() override;
		virtual bool HasFlowOutputParameters() override;
	};
}