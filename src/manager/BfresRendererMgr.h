#pragma once

#include <unordered_map>
#include <file/game/bfres/BfresFile.h>
#include <gl/BfresRenderer.h>

namespace application::manager
{
	namespace BfresRendererMgr
	{
		extern std::unordered_map<application::file::game::bfres::BfresFile*, application::gl::BfresRenderer> gModels;

		bool IsRendererLoaded(application::file::game::bfres::BfresFile* File);
		application::gl::BfresRenderer* GetRenderer(application::file::game::bfres::BfresFile* File);
		void Cleanup();
	}
}