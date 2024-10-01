#pragma once

#include "UIEventEditorNodeBase.h"
#include "BFEVFL.h"
#include <unordered_map>

class UIEventEditorFork : public UIEventEditorNodeBase
{
public:
	BFEVFLFile::ForkEvent* mFork;
	std::vector<uint32_t> mOutputPinIds;

	UIEventEditorFork(BFEVFLFile::ForkEvent& Fork, uint32_t EditorId);

	std::vector<int16_t> GetLastEventIndices(int16_t Index);
	virtual void Render() override;
	virtual NodeType GetNodeType() override;
	virtual ImColor GetHeaderColor() override;
	virtual void DrawLinks() override;
	virtual void DeleteOutputLink(uint32_t LinkId) override;
	virtual void CreateLink(uint32_t SourceLinkId, uint32_t DestinationLinkId) override;
	virtual bool IsPinLinked(uint32_t PinId) override;
};