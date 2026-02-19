#pragma once

#include <rendering/UIWindowBase.h>
#include <file/game/bfres/BfresFile.h>
#include <manager/FramebufferMgr.h>
#include <game/Scene.h>
#include <gl/Shader.h>
#include <gl/Camera.h>
#include <gl/TerrainRenderer.h>
#include <rendering/ImGuizmo.h>
#include <rendering/popup/PopUpBuilder.h>
#include <rendering/mapeditor/UIAILinks.h>
#include <rendering/mapeditor/ActorPainter.h>
#include <rendering/mapeditor/TerrainEditor.h>
#include <util/Frustum.h>
#include <memory>
#include <play/PlaySession.h>
#include "imgui.h"

namespace application::rendering::map_editor
{
	class UIMapEditor : public UIWindowBase
	{
	public:
		static application::gl::Shader* gInstancedShader;
		static application::gl::Shader* gPickingShader;
		static application::gl::Shader* gSelectedShader;
		static application::gl::Shader* gNavMeshShader;
		static glm::vec3 gLightColor;

		struct RenderSettings
		{
			bool mAllowActorSelection = true;

			bool mRenderNavMesh = false;
			bool mRenderNavMeshWireframe = false;
			bool mRenderVisibleEntities = true;
			bool mRenderFarEntities = true;
			bool mRenderInvisibleEntities = true;
			bool mRenderAreas = true;
			bool mRenderTerrain = true;
			bool mRenderTerrainWireframe = false;
			bool mRenderTerrainCollision = false;
			bool mRenderTerrainCollisionWireframe = true;
			bool mRenderMergedActors = true;
		};

		enum class EditingMode : int
		{
			BANC_ENTITY = 0,
			ACTOR_PAINTER = 1,
			TERRAIN = 2
		};

		enum class EditorMode : int
		{
			EDIT = 0,
			PLAY = 1
		};

		UIMapEditor() = default;

		void DrawOutlinerWindow();
		void DrawToolsWindow();
		void DrawDetailsWindow();

		void RenderPickingFramebuffer(const ImVec2& WindowSize, const ImVec2& MousePos);
		void DrawViewportWindow();

		void InitializeImpl() override;
		void DrawImpl() override;
		void DeleteImpl() override;
		WindowType GetWindowType() override;

		application::game::Scene::BancEntityRenderInfo* GetSelectedActor();
		bool IsAnyActorSelected();
		void DeselectActor();
		void SelectActor(int32_t NewIndex);
		void SelectNearestMergedBancEntityInstance(std::vector<application::game::BancEntity>* MergedActor, application::game::BancEntity& Entity, const glm::vec3& Pos);

		application::game::Scene mScene;
		application::gl::Camera mCamera = application::gl::Camera(0, 0, glm::vec3(0.0f, 0.0f, 0.0f), nullptr);

		static void InitializeGlobal();

		RenderSettings mRenderSettings;
		EditingMode mEditingMode = EditingMode::BANC_ENTITY;
		EditorMode mEditorMode = EditorMode::EDIT;
	private:
		struct PropertiesWindowInfo
		{
			bool mSetDynamicColumnWidth = true;
			bool mSetPhiveColumnWidth = true;
			bool mSetExternalParametersColumnWidth = true;
			bool mSetPresenceColumnWidth = true;
			bool mSetPhivePlacementColumnWidth = true;
		};

		std::string GetWindowTitle();
		//void DrawBancEntity(application::game::BancEntity& Entity, application::gl::Shader* Shader);
		//void DrawInstancedBancEntity(std::vector<application::game::BancEntity*>& Entities, application::gl::Shader* Shader);
		void DrawBancEntityRenderInfo(application::game::Scene::BancEntityRenderInfo& RenderInfo);
		void DrawInstancedBancEntityRenderInfo(std::vector<application::game::Scene::BancEntityRenderInfo>& Entities);

		void DrawDynamicTypePropertiesHeader(const std::string& Title, std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map, bool& SyncBancEntity, bool& UpdateColumnWidth, ImGuiTreeNodeFlags Flags, int IndentationLevel = 1);
		void DrawDynamicTypePropertiesSeparator(const std::string& Title, std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map, bool& SyncBancEntity, bool& UpdateColumnWidth, int IndentationLevel = 1);
		void DrawDynamicTypePropertiesTable(const std::string& Title, std::map<std::string, std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3>>& Map, bool& SyncBancEntity, bool& UpdateColumnWidth, int IndentationLevel = 1);

		void EnterPlayMode();
		void ExitPlayMode();

		void DrawOverlay();

		bool IsBancEntityRenderInfoCulled(const application::game::Scene::BancEntityRenderInfo& RenderInfo);

		const glm::vec4 mClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		ImVec2 mSceneWindowSize;
		application::gl::Framebuffer* mSceneWindowFramebuffer = nullptr;

		int32_t mSelectedActorIndex = -1;

		float mGuizmoObjectMatrix[16] =
		{ 
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f 
		};
		ImGuizmo::MODE mGuizmoMode = ImGuizmo::WORLD;
		ImGuizmo::OPERATION mGuizmoOperation = ImGuizmo::TRANSLATE;

		PropertiesWindowInfo mPropertiesWindowInfo;

		std::string mOutlierSearchFieldText;

		application::rendering::map_editor::UIAILinks mUIAiLinks;

		application::rendering::map_editor::ActorPainter mActorPainter;

		application::rendering::map_editor::TerrainEditor mTerrainEditor;

		application::util::Frustum mFrustum;

		uint16_t mHotkeyBlockFrameCount = 0;

		std::unique_ptr<application::play::PlaySession> mPlaySession = nullptr;

		static application::rendering::popup::PopUpBuilder gAddNewDynamicTypeParameterPopUp;
		static application::rendering::popup::PopUpBuilder gAddBancEntityPopUp;
		static application::rendering::popup::PopUpBuilder gAddLinkPopUp;
		static application::rendering::popup::PopUpBuilder gAddRailPopUp;
		static application::rendering::popup::PopUpBuilder gLoadScenePopUp;
		static application::rendering::popup::PopUpBuilder gNewScenePopUp;
		static application::rendering::popup::PopUpBuilder gAddFarActorPopUp;
		static application::rendering::popup::PopUpBuilder gAddBancEntityToParentPopUp;
		static application::rendering::popup::PopUpBuilder gConfirmActorDeletePopUp;

		static application::rendering::popup::PopUpBuilder gConfigureGitIdentityPopUp;
	};
}