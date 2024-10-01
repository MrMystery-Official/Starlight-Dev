#pragma once

#include "UIAINBNodeBase.h"

class UIAINBNodeBoolSelector : public UIAINBNodeBase {
public:
    UIAINBNodeBoolSelector(AINBFile::Node& Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow);

    void Render() override;
    void RebuildNode() override;
    void RenderLinks(std::vector<std::unique_ptr<UIAINBNodeBase>>& Nodes) override;
    void GenerateNodeShapeInfo() override;
    ImColor GetHeaderColor() override;
    void PostProcessLinkedNodeInfo(Pin& StartPin, AINBFile::LinkedNodeInfo& Info) override;
    void PostProcessNode() override;

private:
    uint32_t mPinIdTrue = 0;
    uint32_t mPinIdFalse = 0;
};