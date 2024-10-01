#pragma once

#include <unordered_map>
#include <vector>
#include "BufferObject.h"
#include <variant>
#include <string>
#include "Shader.h"

class GLBfresVAO
{
public:
	struct VertexAttribute
	{
		uint32_t Location;
		int32_t ElementCount;
		GLenum Type;
		bool Normalized;
		uint32_t Stride;
		uint32_t Offset;
		bool Instance;
		int32_t Divisor;
		int32_t BufferIndex;
	};

	GLuint Handle = 0xFFFFFFFF;
	std::unordered_map<uint32_t, std::vector<VertexAttribute>> Attributes; //Buffer Index -> VertexAttribute[]
	std::vector<BufferObject> Buffers;
	BufferObject IndexBuffer;

	GLBfresVAO() {}
	GLBfresVAO(BufferObject Vbo);
	GLBfresVAO(BufferObject Vbo, BufferObject Ebo);
	GLBfresVAO(std::vector<BufferObject> Vbo);
	GLBfresVAO(std::vector<BufferObject> Vbo, BufferObject Ebo);

	void Init();
	void Clear();

	void AddAttribute(uint32_t Location, int32_t Size, GLenum Type, bool Normalized, int32_t Stride, int32_t Offset, int32_t BufferIndex = 0, int32_t Divisor = 0);

	void Enable(Shader* Shader);

	void VertexAttributePointer(VertexAttribute& Attr);

	void Bind();
	void Unbind();
	void Use();
	void Dispose();
};