#pragma once

#include <unordered_map>
#include <string>
#include "Framebuffer.h"

namespace FramebufferMgr
{
	extern std::unordered_map<std::string, Framebuffer> Framebuffers;

	Framebuffer* GetFramebuffer(std::string Name);
	Framebuffer* CreateFramebuffer(int Width, int Height, std::string Name);
	void Cleanup();
}