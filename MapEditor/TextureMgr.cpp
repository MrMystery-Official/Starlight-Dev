#include "TextureMgr.h"

#include "Logger.h"
#include "Editor.h"
#include <stb/stb_image.h>

std::unordered_map<std::string, TextureMgr::Texture> TextureMgr::Textures;

TextureMgr::Texture* TextureMgr::GetTexture(std::string Name)
{
    if (Textures.count(Name))
        return &Textures[Name];

    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(Editor::GetAssetFile("Textures/" + Name + ".png").c_str(), &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return nullptr;

    TextureMgr::Texture Tex;
    Tex.Pixels = std::vector<unsigned char>(image_data, image_data + image_width * image_height);

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Upload pixels into texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    Tex.Width = image_width;
    Tex.Height = image_height;
    Tex.ID = image_texture;

    Textures.insert({ Name, Tex });

    Logger::Info("TextureMgr", "Loaded texture " + Name);

    return &Textures[Name];
}

void TextureMgr::Cleanup()
{
    for (auto& [Key, Tex] : Textures) {
        Logger::Info("TextureMgr", "Deleted texture " + std::to_string(Tex.ID));
        glDeleteTextures(1, &Tex.ID);
    }
    Textures.clear();
}