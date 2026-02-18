#pragma once

#include <glad/glad.h>
#include <rendering/UIWindowBase.h>
#include <manager/TextureMgr.h>
#include <string>
#include <vector>
#include <set>
#include <glm/vec4.hpp>
#include <file/game/ainb/AINBFile.h>
#include <manager/AINBNodeMgr.h>
#include <optional>
#include <functional>
#include <rendering/popup/PopUpBuilder.h>
#include "UIAINBEditorNodeBase.h"
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;

namespace application::rendering::ainb
{
	class UIAINBEditor : public UIWindowBase
	{
	public:
		enum class NewNodeToLinkState : uint8_t
		{
			NONE = 0,
			INIT = 1,
			WAITING = 2
		};

		UIAINBEditor() = default;
		UIAINBEditor(const std::string& AINBPath);

		void LoadAINB(const std::string& AINBPath);
		void LoadAINB(const std::vector<unsigned char>& AINBBytes);

		void DrawGeneralWindow();
		void DrawNodeListWindow();
		void DrawGraphWindow();
		void DrawDetailsWindow();

		void InitializeImpl() override;
		void DrawImpl() override;
		void DeleteImpl() override;
		WindowType GetWindowType() override;

		std::optional<std::function<void(application::file::game::ainb::AINBFile& File)>> mSaveCallback = std::nullopt;
		std::optional<std::function<void(application::rendering::ainb::UIAINBEditor* Editor) >> mInitCallback = std::nullopt;
		bool mEnableSaveAs = true;
		bool mEnableSaveOverwrite = true;
		bool mEnableSaveInProject = true;
		bool mEnableOpen = true;

		static bool NodeEditorSaveSettings(const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer);
		static void Initialize();

		static const char* gCategoryDropdownItems[3];
		static const char* gValueTypeDropdownItems[6];
		static const char* gParamValueTypeDropdownItems[6];
		static application::gl::Texture* gHeaderTexture;

	private:
		enum class DetailsEditorContentType : uint8_t
		{
			NONE = 0,
			ENTRYPOINT = 1,
			BLACKBOARD = 2
		};

		struct DetailsEditorContentEntryPoint
		{
			application::file::game::ainb::AINBFile::Command* mCommand = nullptr;
		};

		struct DetailsEditorContentBlackBoard
		{
			application::file::game::ainb::AINBFile::GlobalEntry* mVariable = nullptr;
		};

		struct DetailsEditorContent
		{
			DetailsEditorContentType mType = DetailsEditorContentType::NONE;
			std::variant<int, DetailsEditorContentEntryPoint, DetailsEditorContentBlackBoard> mContent = 0;
		};
		
		struct GraphRenderAction
		{
			std::function<void()> mFunc;
			uint32_t mFrameDelay = 0;
		};

		struct VisualNode
		{
			int32_t mX = 0;
			int32_t mY = 0;
			int32_t _mRelY = 0;
			int32_t mWidth = 0;
			int32_t mHeight = 0;
			int32_t _mTreeHeight = 0;
			application::file::game::ainb::AINBFile::Node* mPtr = nullptr;

			VisualNode() = default;
			explicit VisualNode(application::file::game::ainb::AINBFile::Node& Node) : mPtr(&Node) {}
			explicit VisualNode(application::file::game::ainb::AINBFile::Node* Node) : mPtr(Node) {}
		};

		struct CommandGroup 
		{
			std::string CommandName;
			VisualNode* RootNode;
			std::set<VisualNode*> Nodes;  // all nodes belonging to this command
			glm::vec4 Bounds;  // x, y, w, h
		};

		void GraphDeselect(bool Nodes = true, bool Links = true);
		void DeleteNode(ed::NodeId NodeId);
		void DeleteNodeLink(ed::LinkId LinkId);
		void AddEditorNode(application::file::game::ainb::AINBFile::Node* Node, application::manager::AINBNodeMgr::NodeDef* Def = nullptr);
		void UpdateEditorNodeIndices();

		void AutoLayoutGreedyCollisionResolve(std::vector<VisualNode>& AllNodes);
		int32_t AutoLayoutCalcNodeWidth(VisualNode& Node);
		void AutoLayoutCalcTreeHeight(VisualNode* n, std::vector<VisualNode*>& Visited, std::vector<VisualNode>& Nodes);
		bool AutoLayoutCheckCollision(int32_t x, int32_t y, int32_t w, int32_t h, std::vector<glm::i32vec4>& Placed);
		int32_t AutoLayoutResolveCollision(int32_t x, int32_t y, int32_t w, int32_t h, std::vector<glm::i32vec4>& Placed);
		void AutoLayoutPlaceSubtree(VisualNode* n, int32_t x, int32_t y, std::vector<VisualNode*>& Placed, std::vector<glm::i32vec4>& Bounds, std::vector<VisualNode>& Nodes);
		void AutoLayout();
		void AutoLayoutCollectCommandNodes(VisualNode* Root, std::set<VisualNode*>& OutNodes, std::set<VisualNode*>& Visited, std::vector<VisualNode>& AllNodes);
		void AutoLayoutResolveGroupCollisions(std::vector<CommandGroup>& Groups);

		std::string GetWindowTitle();

		application::file::game::ainb::AINBFile mAINBFile;
		std::string mAINBPath = "";

		DetailsEditorContent mDetailsEditorContent;
		
		bool mNodeListOpen = true;
		bool mGeneralOpen = true;
		bool mGraphOpen = true;
		bool mDetailsOpen = true;

		bool mWantAutoLayout = false;

		ed::EditorContext* mNodeEditorContext = nullptr;
		int mNodeEditorUniqueId = 1;

		std::vector<std::unique_ptr<UIAINBEditorNodeBase>> mNodes;
		std::vector<GraphRenderAction> mGraphRenderActions;
		std::vector<std::function<void()>> mRenderEndActions;

		ed::NodeId mContextNodeId;
		ed::LinkId mContextLinkId;

		std::string mNodeListSearchBar;

		std::string mAddNodeSearchBar;
		bool mFocusAddNodeSearchBar = false;
		bool mShowAllNodes = false;
		std::unordered_map<std::string, int> mCategoriesOriginalState;

		ImVec2 mOpenPopupPosition;
		UIAINBEditorNodeBase::Pin mNewNodeLinkPin;
		NewNodeToLinkState mNewNodeLinkState = NewNodeToLinkState::NONE;
		int32_t mNewNodeStartIndex = -1;

		static application::rendering::popup::PopUpBuilder gCreateNewNodePopUp;
	};
}