#include "VertexArrayObject.h"

#include <util/Logger.h>

namespace application::gl
{
	VertexArrayObject::VertexArrayObject(BufferObject Vbo)
	{
		Initalize();
		mBuffers.push_back(Vbo);
	}

	VertexArrayObject::VertexArrayObject(BufferObject Vbo, BufferObject Ebo)
	{
		Initalize();
		mIndexBuffer = Ebo;
		mBuffers.push_back(Vbo);
	}

	VertexArrayObject::VertexArrayObject(std::vector<BufferObject> Vbo)
	{
		Initalize();
		mBuffers.insert(mBuffers.end(), Vbo.begin(), Vbo.end());
	}

	VertexArrayObject::VertexArrayObject(std::vector<BufferObject> Vbo, BufferObject Ebo)
	{
		Initalize();
		mBuffers.insert(mBuffers.end(), Vbo.begin(), Vbo.end());
		mIndexBuffer = Ebo;
	}

	void VertexArrayObject::Initalize()
	{
		glGenVertexArrays(1, &mID);
	}

	void VertexArrayObject::Clear()
	{
		mAttributes.clear();
	}

	void VertexArrayObject::AddAttribute(uint32_t Location, int32_t Size, GLenum Type, bool Normalized, int32_t Stride, int32_t Offset, int32_t BufferIndex, int32_t Divisor)
	{
		mAttributes[BufferIndex].push_back(VertexAttribute
			{
			.mLocation = Location,
			.mElementCount = Size,
			.mType = Type,
			.mNormalized = Normalized,
			.mStride = (uint32_t)Stride,
			.mOffset = (uint32_t)Offset,
			.mDivisor = Divisor,
			.mBufferIndex = BufferIndex
			});
	}

	void VertexArrayObject::Enable()
	{
		glBindVertexArray(mID);
		for (auto& [Key, Val] : mAttributes)
		{
			mBuffers[Key].Bind();
			for (VertexAttribute& Attr : Val)
			{
				VertexAttributePointer(Attr);
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void VertexArrayObject::VertexAttributePointer(VertexAttribute& Attr)
	{
#ifdef _DEBUG
		if (mBuffers[Attr.mBufferIndex].mTarget != GL_ARRAY_BUFFER)
		{
			application::util::Logger::Error("VertexArrayObject", "Input buffer not an array buffer type");
			return;
		}
#endif

		glEnableVertexAttribArray(Attr.mLocation);

		/*
		if (Attr.Type == GL_INT ||
			Attr.Type == GL_UNSIGNED_SHORT && !Attr.Normalized ||
			Attr.Type == GL_UNSIGNED_BYTE && !Attr.Normalized ||
			Attr.Type == GL_UNSIGNED_INT && !Attr.Normalized)
		{
			glVertexAttribIPointer(Index, Attr.ElementCount, Attr.Type, Attr.Stride, (void*)Attr.Offset);
		}
		else
		{
		*/
		glVertexAttribPointer(Attr.mLocation, Attr.mElementCount, Attr.mType, Attr.mNormalized, Attr.mStride, (void*)Attr.mOffset);
		//}

		glVertexAttribDivisor(Attr.mLocation, Attr.mDivisor);
	}

	void VertexArrayObject::Bind()
	{
		glBindVertexArray(mID);
	}

	void VertexArrayObject::Unbind()
	{
		glBindVertexArray(0);
	}

	void VertexArrayObject::Use()
	{
		Bind();
#ifdef _DEBUG
		if (mIndexBuffer.mDataCount > 0)
#endif
			mIndexBuffer.Bind();
	}

	void VertexArrayObject::Delete()
	{
		glDeleteVertexArrays(1, &mID);
	}
}