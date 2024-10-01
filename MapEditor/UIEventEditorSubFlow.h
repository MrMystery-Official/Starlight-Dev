#pragma once

#include "UIEventEditorNodeBase.h"
#include "BFEVFL.h"

class UIEventEditorSubFlow : public UIEventEditorNodeBase
{
public:
	BFEVFLFile::SubFlowEvent* mSubFlow;

	UIEventEditorSubFlow(BFEVFLFile::SubFlowEvent& SubFlow, uint32_t EditorId);

	virtual void Render() override;
	virtual NodeType GetNodeType() override;
	virtual ImColor GetHeaderColor() override;
	virtual void DrawLinks() override;
	virtual void DeleteOutputLink(uint32_t LinkId) override;
	virtual void CreateLink(uint32_t SourceLinkId, uint32_t DestinationLinkId) override;
	virtual bool IsPinLinked(uint32_t PinId) override;
};