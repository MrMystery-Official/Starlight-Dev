#include "BufferObject.h"

namespace application::gl
{
	void BufferObject::Bind()
	{
		glBindBuffer(mTarget, mID);
	}

	void BufferObject::Unbind()
	{
		glBindBuffer(mTarget, 0);
	}

	void BufferObject::Delete()
	{
		glDeleteBuffers(1, &mID);
	}

	uint32_t BufferObject::GetDataSizeInBytes()
	{
		return mDataStride * mDataCount;
	}

	void BufferObject::SetData(std::vector<uint32_t> Data, GLuint BufferUsage)
	{
		mDataCount = Data.size();
		mDataStride = sizeof(uint32_t);

		Bind();
		glBufferData(mTarget, (GLuint)GetDataSizeInBytes(), Data.data(), BufferUsage);
		Unbind();
	}

	void BufferObject::SetData(std::vector<unsigned char> Data, GLuint BufferUsage)
	{
		mDataCount = Data.size();
		mDataStride = sizeof(unsigned char);

		Bind();
		glBufferData(mTarget, (GLuint)GetDataSizeInBytes(), Data.data(), BufferUsage);
		Unbind();
	}

	void BufferObject::GenBuffer(GLenum Target)
	{
		mTarget = Target;
		glGenBuffers(1, &mID);
	}
}