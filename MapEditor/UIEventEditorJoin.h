#pragma once

#include "UIEventEditorNodeBase.h"
#include "BFEVFL.h"

class UIEventEditorJoin : public UIEventEditorNodeBase
{
public:
	BFEVFLFile::JoinEvent* mJoin;
	int16_t mForkParentIndex = -1;

	UIEventEditorJoin(BFEVFLFile::JoinEvent& Join, uint32_t EditorId);

	virtual void Render() override;
	virtual NodeType GetNodeType() override;
	virtual ImColor GetHeaderColor() override;
	virtual void DrawLinks() override;
	virtual void DeleteOutputLink(uint32_t LinkId) override;
	virtual void CreateLink(uint32_t SourceLinkId, uint32_t DestinationLinkId) override;
	virtual bool IsPinLinked(uint32_t PinId) override;
};