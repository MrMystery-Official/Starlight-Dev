#pragma once

#include "UIAINBNodeBase.h"
#include <unordered_map>

class UIAINBNodeS32Selector : public UIAINBNodeBase {
public:
    UIAINBNodeS32Selector(AINBFile::Node& Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow);

    void Render() override;
    void RebuildNode() override;
    void RenderLinks(std::vector<std::unique_ptr<UIAINBNodeBase>>& Nodes) override;
    void GenerateNodeShapeInfo() override;
    ImColor GetHeaderColor() override;
    void PostProcessLinkedNodeInfo(Pin& StartPin, AINBFile::LinkedNodeInfo& Info) override;

private:
    std::unordered_map<int32_t, uint32_t> mPinIds; //Condition(signed 32-bit integer) -> pin id
    std::vector<int32_t> mConditions;
    uint32_t mPinIdDefault;
};