#pragma once

#include "imgui.h"
#include <glad/glad.h>
#include <stdint.h>

class Framebuffer {
public:
    Framebuffer(float Width, float Height);
    Framebuffer() { }
    unsigned int GetFrameTexture();
    void RescaleFramebuffer(float Width, float Height);
    void Bind() const;
    void Unbind() const;
    void Delete();

private:
    unsigned int m_FBO;
    unsigned int m_Texture;
    unsigned int m_RBO;
};
