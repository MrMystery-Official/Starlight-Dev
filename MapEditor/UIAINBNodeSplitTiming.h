#pragma once

#include "UIAINBNodeBase.h"

class UIAINBNodeSplitTiming : public UIAINBNodeBase {
public:
    UIAINBNodeSplitTiming(AINBFile::Node& Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow);

    void Render() override;
    void RebuildNode() override;
    void RenderLinks(std::vector<std::unique_ptr<UIAINBNodeBase>>& Nodes) override;
    void GenerateNodeShapeInfo() override;
    ImColor GetHeaderColor() override;
    void PostProcessLinkedNodeInfo(Pin& StartPin, AINBFile::LinkedNodeInfo& Info) override;

private:
    uint32_t mPinIdEnter = 0;
    uint32_t mPinIdUpdate = 0;
    uint32_t mPinIdLeave = 0;
};