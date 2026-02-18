#pragma once

#include <glad/glad.h>
#include <vector>

namespace application::gl
{
	class BufferObject
	{
	public:
		GLuint mID = 0xFFFFFFFF;
		uint32_t mDataCount = 0;
		uint32_t mDataStride = 0;
		GLenum mTarget = GL_NONE;

		void Bind();
		void Unbind();
		virtual void Delete();

		uint32_t GetDataSizeInBytes();
		void SetData(std::vector<uint32_t> Data, GLuint BufferUsage = GL_STATIC_DRAW);
		void SetData(std::vector<unsigned char> Data, GLuint BufferUsage = GL_STATIC_DRAW);

		template<typename T>
		void SetData(std::vector<T>& Data, GLuint BufferUsage = GL_STATIC_DRAW)
		{
			mDataCount = Data.size();
			mDataStride = sizeof(T);

			Bind();
			glBufferData(mTarget, (GLuint)GetDataSizeInBytes(), Data.data(), BufferUsage);
			Unbind();
		}

		template<typename T>
		void SetStruct(std::vector<T> Data, uint32_t Size, GLuint BufferUsage = GL_STATIC_DRAW)
		{
			Bind();
			glBufferData(mTarget, (GLuint)Size, Data.data(), BufferUsage);
			Unbind();
		}

		template<typename T>
		void SetSubData(T Data, uint32_t Offset)
		{
			std::vector<T> Vec;
			Vec.push_back(Data);
			SetSubData<T>(Vec, Offset);
		}

		template<typename T>
		void SetSubData(std::vector<T> Data, uint32_t Offset)
		{
			uint32_t Size = sizeof(T);

			Bind();
			glBufferSubData(mTarget, Offset, (GLuint)Size, Data.data());
			Unbind();
		}

		virtual void GenBuffer(GLenum Target);

		BufferObject() = default;
	};
}