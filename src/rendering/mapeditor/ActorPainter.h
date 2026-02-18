#pragma once

#include "imgui.h"
#include <game/BancEntity.h>
#include <vector>
#include <gl/SimpleMesh.h>
#include <gl/Shader.h>

namespace application::rendering::map_editor
{
	class ActorPainter
	{
	public:
		static application::gl::Shader* gSphereShader;
		static application::gl::SimpleMesh gSphereMesh;

		static void Initialize();

		ActorPainter() = default;

		void SetEnabled(bool Enabled, void* MapEditorPtr);
		void DrawMenu(const float& w, const ImVec2& Pos, void* MapEditorPtr);
		void RenderActorPainter(float XPos, float YPos, int ScreenWidth, int ScreenHeight, void* MapEditorPtr);

		bool mEnabled = false;
	private:
		struct ActorEntry
		{
			std::string mGyml = "";
			float mYModelOffset = 0.0f;
			float mProbability = 1.0f;
		};

		int mTerrainMode = 0;
		uint64_t mTerrainActorHash = 0;
		application::game::BancEntity* mTerrainActorEntity = nullptr;
		std::vector<glm::vec3> mTerrainVertices;
		std::vector<uint32_t> mTerrainIndices;

		uint64_t mMergedActorHash = 0;
		application::game::BancEntity* mMergedActorEntity = nullptr;

		application::game::BancEntity::BancType mBancType = application::game::BancEntity::BancType::DYNAMIC;
		std::string mBancPath = "";

		float mRadius = 1.0f;
		uint32_t mIterations = 1;

		glm::vec3 mScaleMin = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 mScaleMax = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 mRotMin = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 mRotMax = glm::vec3(5.0f, 90.0f, 5.0f);
		
		std::vector<ActorEntry> mEntries;
	};
}