#pragma once

#include "UIAINBNodeBase.h"

class UIAINBNodeSimultaneous : public UIAINBNodeBase {
public:
    UIAINBNodeSimultaneous(AINBFile::Node& Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow);

    void Render() override;
    void RebuildNode() override;
    void RenderLinks(std::vector<std::unique_ptr<UIAINBNodeBase>>& Nodes) override;
    void GenerateNodeShapeInfo() override;
    ImColor GetHeaderColor() override;
};