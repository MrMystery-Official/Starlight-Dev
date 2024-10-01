#pragma once

#include "imgui_node_editor.h"
#include "UIEventEditorNodeBase.h"
#include "BFEVFL.h"
#include <vector>

namespace ed = ax::NodeEditor;

namespace UIEventEditor
{
	extern bool mOpen;
	extern bool mFocused;
	extern bool mPropertiesFirstTime;
	extern ed::EditorContext* mContext;
	extern BFEVFLFile mEventFile;
	extern std::vector<std::unique_ptr<UIEventEditorNodeBase>> mNodes;
	extern uint32_t mId;
	extern std::vector<const char*> mActorNames;
	extern ed::LinkId ContextLinkId;
	extern ed::NodeId ContextNodeId;

	void Initialize();
	void Delete();
	void DrawEventEditorWindow();
	void DrawPropertiesWindowContent();
	void ManageLinkCreationDeletion();
	void LoadEventFile(std::vector<unsigned char> Bytes);
}