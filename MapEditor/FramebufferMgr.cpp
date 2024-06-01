#include "FramebufferMgr.h"

#include "Logger.h"

std::unordered_map<std::string, Framebuffer> FramebufferMgr::Framebuffers;

Framebuffer* FramebufferMgr::GetFramebuffer(std::string Name)
{
    if (!Framebuffers.count(Name)) {
        Logger::Error("FramebufferMgr", "Tried to access non-existing framebuffer " + Name);
        return nullptr;
    }

    return &Framebuffers[Name];
}

Framebuffer* FramebufferMgr::CreateFramebuffer(int Width, int Height, std::string Name)
{
    Framebuffers.insert({ Name, Framebuffer(Width, Height) });
    Logger::Info("FramebufferMgr", "Created framebuffer " + Name);
    return &Framebuffers[Name];
}

void FramebufferMgr::Cleanup()
{
    for (auto& [Name, Buffer] : Framebuffers) {
        Buffer.Unbind();
        Buffer.Delete();
        Logger::Info("FramebufferMgr", "Deleted framebuffer " + Name);
    }
    Framebuffers.clear();
}