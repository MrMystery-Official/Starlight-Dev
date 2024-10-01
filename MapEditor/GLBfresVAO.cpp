#include "GLBfresVAO.h"

#include "Logger.h"
#include <iostream>

GLBfresVAO::GLBfresVAO(BufferObject Vbo)
{
	Init();
	Buffers.push_back(Vbo);
}

GLBfresVAO::GLBfresVAO(BufferObject Vbo, BufferObject Ebo)
{
	Init();
	IndexBuffer = Ebo;
	Buffers.push_back(Vbo);
}

GLBfresVAO::GLBfresVAO(std::vector<BufferObject> Vbo)
{
	Init();
	Buffers.insert(Buffers.end(), Vbo.begin(), Vbo.end());
}

GLBfresVAO::GLBfresVAO(std::vector<BufferObject> Vbo, BufferObject Ebo)
{
	Init();
	Buffers.insert(Buffers.end(), Vbo.begin(), Vbo.end());
	IndexBuffer = Ebo;
}

void GLBfresVAO::Init()
{
	glGenVertexArrays(1, &Handle);
}

void GLBfresVAO::Clear()
{
	Attributes.clear();
}

void GLBfresVAO::AddAttribute(uint32_t Location, int32_t Size, GLenum Type, bool Normalized, int32_t Stride, int32_t Offset, int32_t BufferIndex, int32_t Divisor)
{
	Attributes[BufferIndex].push_back(VertexAttribute{
		.Location = Location,
		.ElementCount = Size,
		.Type = Type,
		.Normalized = Normalized,
		.Stride = (uint32_t)Stride,
		.Offset = (uint32_t)Offset,
		.Divisor = Divisor,
		.BufferIndex = BufferIndex
		});
}

void GLBfresVAO::Enable(Shader* Shader)
{
	glBindVertexArray(Handle);
	for (auto& [Key, Val] : Attributes)
	{
		Buffers[Key].Bind();
		for (VertexAttribute& Attr : Val)
		{
			VertexAttributePointer(Attr);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GLBfresVAO::VertexAttributePointer(VertexAttribute& Attr)
{
#ifdef _DEBUG
	if (Buffers[Attr.BufferIndex].m_Target != GL_ARRAY_BUFFER)
	{
		Logger::Error("GLBfresVAO", "Input buffer not an array buffer type!");
		return;
	}
#endif

	//Buffers[Attr.BufferIndex].Bind();
	glEnableVertexAttribArray(Attr.Location);

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
	glVertexAttribPointer(Attr.Location, Attr.ElementCount, Attr.Type, Attr.Normalized, Attr.Stride, (void*)Attr.Offset);
	//}

	glVertexAttribDivisor(Attr.Location, Attr.Divisor);
}

void GLBfresVAO::Bind()
{
	glBindVertexArray(Handle);
}

void GLBfresVAO::Unbind()
{
	glBindVertexArray(0);
}

void GLBfresVAO::Use()
{
	Bind();
#ifdef _DEBUG
	if (IndexBuffer.m_DataCount > 0)
#endif
		IndexBuffer.Bind();
}

void GLBfresVAO::Dispose()
{
	glDeleteVertexArrays(1, &Handle);
}