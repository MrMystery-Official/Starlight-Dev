#pragma once

#include "UIAINBNodeBase.h"

class UIAINBNodeDefault : public UIAINBNodeBase{
public:
    UIAINBNodeDefault(AINBFile::Node& Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow);

    void Render() override;
    void RenderLinks(std::vector<std::unique_ptr<UIAINBNodeBase>>& Nodes) override;
    void GenerateNodeShapeInfo() override;
    ImColor GetHeaderColor() override;

private:
    ImColor mHeaderColor;
};