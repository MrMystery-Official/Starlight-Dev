#pragma once

#include "UIAINBNodeBase.h"

class UIAINBNodeSequential : public UIAINBNodeBase {
public:
    UIAINBNodeSequential(AINBFile::Node& Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow);

    void Render() override;
    void RebuildNode() override;
    void RenderLinks(std::vector<std::unique_ptr<UIAINBNodeBase>>& Nodes) override;
    void GenerateNodeShapeInfo() override;
    ImColor GetHeaderColor() override;
    void UpdateVisuals() override;

private:
    uint16_t mSeqCount = 2;
    std::vector<uint32_t> mSeqToPinId;
};