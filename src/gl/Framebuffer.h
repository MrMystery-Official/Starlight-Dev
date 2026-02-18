#pragma once

namespace application::gl
{
	class Framebuffer
	{
	public:
		Framebuffer() = default;
		Framebuffer(const float& Width, const float& Height);

		void Create(const float& Width, const float& Height);
		void RescaleFramebuffer(const float& Width, const float& Height);
		void Delete();

		void Bind();
		void Unbind();

		unsigned int GetFramebufferTexture();
	private:
		unsigned int mTextureId = 0xFFFFFFFF;
		unsigned int mFramebufferObjectId = 0xFFFFFFFF;
		unsigned int mRenderbufferId = 0xFFFFFFFF;
	};
}