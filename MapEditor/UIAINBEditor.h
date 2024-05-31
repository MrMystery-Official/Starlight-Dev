#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "AINB.h"
#include "imgui.h"
#include "imgui_node_editor.h"
#include <string>
#include <vector>

namespace ed = ax::NodeEditor;

namespace UIAINBEditor {

extern bool Open;
extern bool Focused;
extern bool FocusOnZero;
extern ed::EditorContext* Context;
extern AINBFile AINB;
extern bool RunAutoLayout;
extern void (*SaveCallback)();

void LoadAINBFile(std::string Path, bool AbsolutePath = false);
void LoadAINBFile(std::vector<unsigned char> Bytes);

void Initialize();
void Delete();

void UpdateNodsIds();
ImColor GetHeaderColor(AINBFile::Node& Node);
void UpdateNodeShapes();
void ManageLinkCreationDeletion();
void DrawPinIcon(uint32_t Id, bool IsOutput);
void DrawNode(AINBFile::Node& Node);
void DrawLinks(AINBFile::Node& Node, int& CurrentId);
void Save();

void DrawPropertiesWindowContent();
void DrawAinbEditorWindow();

}
