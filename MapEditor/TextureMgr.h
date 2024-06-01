#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace TextureMgr {

struct Texture {
    GLuint ID;
    uint32_t Width;
    uint32_t Height;
    std::vector<unsigned char> Pixels;
};

extern std::unordered_map<std::string, Texture> Textures;

TextureMgr::Texture* GetTexture(std::string Name);
void Cleanup();

}
