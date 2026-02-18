#include "BfresRendererMgr.h"

#include <util/Logger.h>

namespace application::manager
{
	std::unordered_map<application::file::game::bfres::BfresFile*, application::gl::BfresRenderer> BfresRendererMgr::gModels;

	bool BfresRendererMgr::IsRendererLoaded(application::file::game::bfres::BfresFile* File)
	{
		return gModels.contains(File);
	}

	application::gl::BfresRenderer* BfresRendererMgr::GetRenderer(application::file::game::bfres::BfresFile* File)
	{
		if (gModels.contains(File))
			return &gModels[File];

		gModels.insert({ File, application::gl::BfresRenderer(File) });
		return &gModels[File];
	}

	void BfresRendererMgr::Cleanup()
	{
		for (auto& [File, Renderer] : gModels)
		{
			Renderer.Delete();
		}
		gModels.clear();

		application::util::Logger::Info("BfresRendererMgr", "Cleaned up");
	}
}