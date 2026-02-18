#include "SimpleMesh.h"

namespace application::gl
{
	SimpleMesh::SimpleMesh(std::vector<float>& Vertices, std::vector<GLuint>& Indices)
	{
		Initialize(Vertices, Indices);
	}

	SimpleMesh::SimpleMesh(std::vector<glm::vec3>& Vertices, std::vector<GLuint>& Indices)
	{
		std::vector<float> VerticesConverted;
		VerticesConverted.reserve(Vertices.size() * 3);
		for (const glm::vec3& Vec : Vertices)
		{
			VerticesConverted.push_back(Vec.x);
			VerticesConverted.push_back(Vec.y);
			VerticesConverted.push_back(Vec.z);
		}

		Initialize(VerticesConverted, Indices);
	}

	void SimpleMesh::Initialize(std::vector<float>& Vertices, std::vector<GLuint>& Indices)
	{
		mVertexArrayObject.Initalize();
		mVertexArrayObject.mIndexBuffer.GenBuffer(GL_ELEMENT_ARRAY_BUFFER);
		mVertexArrayObject.mIndexBuffer.SetData<GLuint>(Indices);

		mVertexArrayObject.mBuffers.resize(1);
		application::gl::BufferObject& VertexBuffer = mVertexArrayObject.mBuffers[0];
		VertexBuffer.GenBuffer(GL_ARRAY_BUFFER);
		VertexBuffer.SetData<float>(Vertices);

		mIndexCount = Indices.size();

		mVertexArrayObject.AddAttribute(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0, 0);
	}

	void SimpleMesh::Draw()
	{
		mVertexArrayObject.Enable();
		mVertexArrayObject.Use();

		glDrawElements(GL_TRIANGLES, sizeof(int) * mIndexCount, GL_UNSIGNED_INT, 0);
	}

	void SimpleMesh::Delete()
	{
		mVertexArrayObject.Delete();

		mVertexArrayObject.mIndexBuffer.Delete();
		mVertexArrayObject.mBuffers[0].Delete();
		mVertexArrayObject.mBuffers.clear();
	}
}