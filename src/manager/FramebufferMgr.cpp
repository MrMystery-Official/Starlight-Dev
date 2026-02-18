#include "FramebufferMgr.h"

namespace application::manager
{
	std::unordered_map<std::string, application::gl::Framebuffer> FramebufferMgr::gFramebuffers;

	application::gl::Framebuffer* FramebufferMgr::GetFramebuffer(const std::string& Name)
	{
		return &gFramebuffers[Name];
	}

	application::gl::Framebuffer* FramebufferMgr::CreateFramebuffer(const float& Width, const float& Height, const std::string& Name)
	{
		gFramebuffers[Name].Create(Width, Height);

		return &gFramebuffers[Name];
	}

	bool FramebufferMgr::DeleteFramebuffer(application::gl::Framebuffer* Buffer)
	{
		for (auto Iter = gFramebuffers.begin(); Iter != gFramebuffers.end(); )
		{
			if (&Iter->second == Buffer)
			{
				Iter = gFramebuffers.erase(Iter);
				return true;
			}
			Iter++;
		}

		return false;
	}

	void FramebufferMgr::Cleanup()
	{
		for (auto& [Name, Buffer] : gFramebuffers)
		{
			Buffer.Delete();
		}

		gFramebuffers.clear();
	}
}