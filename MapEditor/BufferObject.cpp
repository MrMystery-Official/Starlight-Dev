#include "BufferObject.h"

void BufferObject::Bind()
{
	glBindBuffer(m_Target, m_ID);
}

void BufferObject::Unbind()
{
	glBindBuffer(m_Target, 0);
}

void BufferObject::Dispose()
{
	glDeleteBuffers(1, &m_ID);
}

uint32_t BufferObject::GetDataSizeInBytes()
{
	return m_DataStride * m_DataCount;
}

void BufferObject::SetData(std::vector<uint32_t> Data, GLuint BufferUsage)
{
	m_DataCount = Data.size();
	m_DataStride = sizeof(uint32_t);

	Bind();
	glBufferData(m_Target, (GLuint)GetDataSizeInBytes(), Data.data(), BufferUsage);
	Unbind();
}

void BufferObject::SetData(std::vector<unsigned char> Data, GLuint BufferUsage)
{
	m_DataCount = Data.size();
	m_DataStride = sizeof(unsigned char);

	Bind();
	glBufferData(m_Target, (GLuint)GetDataSizeInBytes(), Data.data(), BufferUsage);
	Unbind();
}

void BufferObject::GenBuffer(GLenum Target)
{
	m_Target = Target;
	glGenBuffers(1, &m_ID);
}