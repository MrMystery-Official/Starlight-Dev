#pragma once

#include "AINB.h"
#include "UIAINBNodeBase.h"
#include "imgui_node_editor.h"
#include "TextureMgr.h"
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
	extern TextureMgr::Texture* HeaderTexture;
	extern bool EnableFlow;
	extern ed::LinkId ContextLinkId;
	extern ed::NodeId ContextNodeId;
	extern uint32_t Id;
	extern std::vector<std::unique_ptr<UIAINBNodeBase>> Nodes;

	void AddNode(AINBFile::Node& Node, uint32_t Id);

	void LoadAINBFile(std::string Path, bool AbsolutePath = false);
	void LoadAINBFile(std::vector<unsigned char> Bytes);

	void Initialize();
	void Delete();

	void DeleteNodeLink(ed::LinkId LinkId);
	void DeleteNode(ed::NodeId NodeId);

	void UpdateLinkedParameters();
	void ManageLinkCreationDeletion();
	void Save();

	void DrawPropertiesWindowContent();
	void DrawAinbEditorWindow();

}