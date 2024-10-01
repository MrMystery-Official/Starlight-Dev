#pragma once

#include "UIEventEditorNodeBase.h"
#include "BFEVFL.h"

class UIEventEditorSwitch : public UIEventEditorNodeBase
{
public:
	BFEVFLFile::SwitchEvent* mSwitch;
	std::vector<uint32_t> mOutputCasePinIds;

	UIEventEditorSwitch(BFEVFLFile::SwitchEvent& Switch, uint32_t EditorId);

	virtual void Render() override;
	virtual NodeType GetNodeType() override;
	virtual ImColor GetHeaderColor() override;
	virtual void DrawLinks() override;
	virtual void DeleteOutputLink(uint32_t LinkId) override;
	virtual void CreateLink(uint32_t SourceLinkId, uint32_t DestinationLinkId) override;
	virtual bool IsPinLinked(uint32_t PinId) override;
};