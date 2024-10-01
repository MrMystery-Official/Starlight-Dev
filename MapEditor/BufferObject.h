#pragma once

#include <glad/glad.h>
#include <vector>

class BufferObject
{
public:
	GLuint m_ID = 0xFFFFFFFF;
	uint32_t m_DataCount = 0;
	uint32_t m_DataStride = 0;
	GLenum m_Target;

	void Bind();
	void Unbind();
	virtual void Dispose();

	uint32_t GetDataSizeInBytes();
	void SetData(std::vector<uint32_t> Data, GLuint BufferUsage = GL_STATIC_DRAW);
	void SetData(std::vector<unsigned char> Data, GLuint BufferUsage = GL_STATIC_DRAW);

	template<typename T>
	void SetData(std::vector<T> Data, GLuint BufferUsage = GL_STATIC_DRAW)
	{
		m_DataCount = Data.size();
		m_DataStride = sizeof(T);

		Bind();
		glBufferData(m_Target, (GLuint)GetDataSizeInBytes(), Data.data(), BufferUsage);
		Unbind();
	}

	template<typename T>
	void SetStruct(std::vector<T> Data, uint32_t Size, GLuint BufferUsage = GL_STATIC_DRAW)
	{
		Bind();
		glBufferData(m_Target, (GLuint)Size, Data.data(), BufferUsage);
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
		glBufferSubData(m_Target, Offset, (GLuint)Size, Data.data());
		Unbind();
	}

	virtual void GenBuffer(GLenum Target);

	BufferObject() {}
};