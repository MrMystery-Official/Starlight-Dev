#include "TextureMgr.h"

#include <util/Logger.h>
#include <util/FileUtil.h>
#include <stb/stb_image.h>

namespace application::manager
{
	std::unordered_map<std::string, application::gl::Texture> TextureMgr::gTextures;
	std::unordered_map<application::file::game::texture::TexToGoFile::Surface*, application::gl::Texture> TextureMgr::gTexToGoSurfaceTextures;

    application::gl::Texture* TextureMgr::GetAssetTexture(const std::string& Name)
    {
        if (gTextures.contains(Name))
            return &gTextures[Name];

        gTextures.insert({ Name, application::gl::Texture(application::util::FileUtil::GetAssetFilePath("Textures/" + Name + ".png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE) });

        return &gTextures[Name];
    }

    application::gl::Texture* TextureMgr::GetTexToGoSurfaceTexture(application::file::game::texture::TexToGoFile::Surface* Surface, GLenum Slot, bool GenMipMaps, GLenum TextureFilter)
    {
        if (gTexToGoSurfaceTextures.contains(Surface))
            return &gTexToGoSurfaceTextures[Surface];

        gTexToGoSurfaceTextures.insert({ Surface, application::gl::Texture(Surface, Slot, GenMipMaps, TextureFilter) });

        return &gTexToGoSurfaceTextures[Surface];
    }

    void TextureMgr::Cleanup()
    {
        for (auto& [Key, Tex] : gTextures)
        {
            Tex.Delete();
            application::util::Logger::Info("TextureMgr", "Deleted texture " + Key);
        }

        for (auto& [Key, Tex] : gTexToGoSurfaceTextures)
        {
            Tex.Delete();
        }

        gTextures.clear();
    }
}