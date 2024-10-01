#pragma once

#include "ImGuiNodeEditorExt.h"
#include "imgui_internal.h"
#include "AINB.h"
#include "imgui_node_editor.h"
#include "TextureMgr.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ed = ax::NodeEditor;

class UIEventEditorNodeBase {
public:
    static std::unique_ptr<UIEventEditorNodeBase>& GetNodeByPinId(uint32_t PinId);
    static std::unique_ptr<UIEventEditorNodeBase>& GetNodeByLinkId(uint32_t PinId);

    enum class NodeType : uint8_t
    {
        EntryPoint = 0,
        SubFlow = 1,
        Action = 2,
        Fork = 3,
        Join = 4,
        Switch = 5
    };
    struct NodeShapeInfo {
        ImVec2 HeaderMin;
        ImVec2 HeaderMax;
    };

    uint32_t mEditorId;
    NodeShapeInfo mNodeShapeInfo;
    int32_t mInputPinId = -1;
    int32_t mOutputPinId = -1;
    bool mHasParentNode = false;

    UIEventEditorNodeBase(uint32_t EditorId) : mEditorId(EditorId) {}

    virtual void DrawHeader();
    virtual void Render() = 0;
    virtual NodeType GetNodeType() = 0;
    virtual void DrawPinIcon(bool Connected, uint32_t Alpha, bool Input);
    virtual ImColor GetHeaderColor() = 0;
    virtual void DrawPin(uint32_t Id, bool Connected, uint32_t Alpha, ed::PinKind Kind);
    virtual void DrawLinks() {}
    virtual void DeleteOutputLink(uint32_t LinkId) {}
    virtual void CreateLink(uint32_t SourceLinkId, uint32_t DestinationLinkId) {}
    virtual bool IsPinLinked(uint32_t PinId)
    {
        return false;
    }
};