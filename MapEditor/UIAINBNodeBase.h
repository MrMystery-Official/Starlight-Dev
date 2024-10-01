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

class UIAINBNodeBase {
public:
    struct NodeShapeInfo {
        float FrameWidth = 0;
        ImVec2 HeaderMin;
        ImVec2 HeaderMax;
    };

    enum class PinType {
        Int,
        Bool,
        Float,
        String,
        Vec3f,
        UserDefined,
        Flow
    };

    enum class LinkType {
        Parameter,
        Flow
    };

    enum class NodeType {
        Default,
        Virtual
    };

    struct Pin {
        ed::PinKind Kind;
        PinType Type;
        AINBFile::Node* Node;
        void* ObjectPtr = nullptr;
        int32_t ParameterIndex = -1;
        bool AllowMultipleLinks = true;
        bool AlreadyLinked = false;
    };

    struct Link {
        void* ObjectPtr;
        LinkType Type;
        uint16_t NodeIndex;
        uint16_t ParameterIndex;
    };

    AINBFile::Node* mNode;
    uint32_t mEditorId;
    NodeShapeInfo mNodeShapeInfo;
    float mMinTextBoxWidth = 150.0f;
    std::vector<std::vector<uint32_t>> mOutputParameters; //6 value types -> vector of output pin ids
    std::vector<std::vector<uint32_t>> mInputParameters; // 6 value types -> vector of input pin ids
    std::vector<std::vector<uint32_t>> mLinkedOutputParams; // 6 value types -> vector of output param indexes
    std::unordered_map<uint32_t, Pin> mPins;
    std::unordered_map<uint32_t, Link> mLinks;
    TextureMgr::Texture& mHeaderBackground;
    bool& mEnableFlow;
    bool mFlowLinked = false;

    UIAINBNodeBase(AINBFile::Node* Node, uint32_t EditorId, TextureMgr::Texture& HeaderBackground, bool& EnableFlow);

    virtual void Render() = 0;
    virtual void RebuildNode() {}
    virtual void UpdateVisuals() {}
    virtual void PostProcessLinkedNodeInfo(Pin& StartPin, AINBFile::LinkedNodeInfo& Info) {}
    virtual void PostProcessNode() {}
    virtual NodeType GetNodeType();
    virtual void RenderLinks(std::vector<std::unique_ptr<UIAINBNodeBase>>& Nodes) = 0;
    virtual void GenerateNodeShapeInfo() = 0;
    virtual ImColor GetHeaderColor() = 0;

    void DrawNodeHeader();
    void DrawImmediateSeperator(ImVec2 Pos);
    void DrawPinIcon(bool Connected, uint32_t Alpha, PinType Type);
    ImRect DrawPin(uint32_t Id, bool Connected, uint32_t Alpha, PinType Type, ed::PinKind Kind, std::string Name);
    void DrawParameterValue(AINBFile::ValueType Type, std::string Name, uint32_t Id, void* Dest);

    std::string GetValueTypeName(AINBFile::ValueType ValueType);
    std::string GetValueTypeName(uint8_t ValueType);

    float GetParamValueWidth(AINBFile::ValueType ValueType);
    float GetParamValueWidth(uint8_t ValueType);

    ImColor GetValueTypeColor(AINBFile::ValueType ValueType);
    ImColor GetPinTypeColor(PinType Type);
    ImColor GetValueTypeColor(uint8_t ValueType);

    PinType ValueTypeToPinType(AINBFile::ValueType Type);
    PinType ValueTypeToPinType(uint8_t Type);

    void ClearParameterIds();
};