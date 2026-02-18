#pragma once

#include <vector>
#include <gl/Shader.h>
#include <gl/Camera.h>
#include <glm/mat4x4.hpp>
#include <file/game/cave/CrbinFile.h>

namespace application::gl
{
	struct CaveRenderer
	{
		unsigned int mCaveVAO, mCaveVBO, mCaveEBO = 0;
		uint32_t mIndexCount = 0;
		application::gl::Shader* mShader = nullptr;

		void Initialize(application::file::game::cave::CrbinFile& File);
		void PrepareGlobal(application::gl::Camera& Camera);
		void Draw(const glm::mat4& Matrix);
		void Delete();
	};
}