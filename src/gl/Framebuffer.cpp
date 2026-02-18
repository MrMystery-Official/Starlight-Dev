#include "Framebuffer.h"

#include <util/Logger.h>
#include <glad/glad.h>

namespace application::gl
{
	Framebuffer::Framebuffer(const float& Width, const float& Height)
	{
		Create(Width, Height);
	}

	void Framebuffer::Create(const float& Width, const float& Height)
	{
		glGenFramebuffers(1, &mFramebufferObjectId);
		glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferObjectId);

		glGenTextures(1, &mTextureId);
		glBindTexture(GL_TEXTURE_2D, mTextureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureId, 0);

		glGenRenderbuffers(1, &mRenderbufferId);
		glBindRenderbuffer(GL_RENDERBUFFER, mRenderbufferId);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Width, Height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRenderbufferId);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			application::util::Logger::Error("Framebuffer", "Framebuffer not complete");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	void Framebuffer::RescaleFramebuffer(const float& Width, const float& Height)
	{
		Bind();

		glBindTexture(GL_TEXTURE_2D, mTextureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureId, 0);

		glBindRenderbuffer(GL_RENDERBUFFER, mRenderbufferId);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Width, Height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRenderbufferId);
	}

	void Framebuffer::Delete()
	{
		glDeleteFramebuffers(1, &mFramebufferObjectId);
		glDeleteTextures(1, &mTextureId);
		glDeleteRenderbuffers(1, &mRenderbufferId);
	}

	void Framebuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferObjectId);
	}

	void Framebuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	unsigned int Framebuffer::GetFramebufferTexture()
	{
		return mTextureId;
	}
}