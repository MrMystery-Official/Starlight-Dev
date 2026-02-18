#pragma once

#include <string>
#include <unordered_map>
#include <gl/Framebuffer.h>

namespace application::manager
{
	namespace FramebufferMgr
	{
		extern std::unordered_map<std::string, application::gl::Framebuffer> gFramebuffers;

		application::gl::Framebuffer* GetFramebuffer(const std::string& Name);
		application::gl::Framebuffer* CreateFramebuffer(const float& Width, const float& Height, const std::string& Name);
		bool DeleteFramebuffer(application::gl::Framebuffer* Buffer);

		void Cleanup();
	}
}