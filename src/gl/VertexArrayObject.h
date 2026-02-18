#pragma once

#include <vector>
#include <gl/BufferObject.h>
#include <gl/Shader.h>
#include <unordered_map>

namespace application::gl
{
	struct VertexArrayObject
	{
		struct VertexAttribute
		{
			uint32_t mLocation;
			int32_t mElementCount;
			GLenum mType;
			bool mNormalized;
			uint32_t mStride;
			uint32_t mOffset;
			bool mInstance;
			int32_t mDivisor;
			int32_t mBufferIndex;
		};

		GLuint mID = 0xFFFFFFFF;
		std::unordered_map<uint32_t, std::vector<VertexAttribute>> mAttributes; //Buffer Index -> VertexAttribute[]
		std::vector<application::gl::BufferObject> mBuffers;
		application::gl::BufferObject mIndexBuffer;

		VertexArrayObject() = default;
		VertexArrayObject(application::gl::BufferObject Vbo);
		VertexArrayObject(application::gl::BufferObject Vbo, application::gl::BufferObject Ebo);
		VertexArrayObject(std::vector<application::gl::BufferObject> Vbo);
		VertexArrayObject(std::vector<application::gl::BufferObject> Vbo, application::gl::BufferObject Ebo);

		void Initalize();
		void Clear();

		void AddAttribute(uint32_t Location, int32_t Size, GLenum Type, bool Normalized, int32_t Stride, int32_t Offset, int32_t BufferIndex = 0, int32_t Divisor = 0);

		void Enable();

		void VertexAttributePointer(VertexAttribute& Attr);

		void Bind();
		void Unbind();
		void Use();
		void Delete();
	};
}