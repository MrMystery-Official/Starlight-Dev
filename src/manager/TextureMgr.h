#pragma once

#include <string>
#include <unordered_map>
#include <gl/Texture.h>
#include <file/game/texture/TexToGoFile.h>

namespace application::manager
{
	namespace TextureMgr
	{
		extern std::unordered_map<std::string, application::gl::Texture> gTextures;
		extern std::unordered_map<application::file::game::texture::TexToGoFile::Surface*, application::gl::Texture> gTexToGoSurfaceTextures;

		application::gl::Texture* GetAssetTexture(const std::string& Name);
		application::gl::Texture* GetTexToGoSurfaceTexture(application::file::game::texture::TexToGoFile::Surface* Surface, GLenum Slot = GL_TEXTURE0, bool GenMipMaps = true, GLenum TextureFilter = GL_LINEAR);
		void Cleanup();
	}
}