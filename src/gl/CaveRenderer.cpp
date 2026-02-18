#include "CaveRenderer.h"

#include <glad/glad.h>
#include <manager/ShaderMgr.h>
#include <glm/gtc/type_ptr.hpp>

namespace application::gl
{
	void CaveRenderer::Initialize(application::file::game::cave::CrbinFile& File)
	{
		std::vector<float> Vertices;
		std::vector<uint32_t> Indices;

		/*

with open(path, "rb") as f:
    stream = ReadStream(f.read())

    for i in range(TRIANGLE_COUNT):
        for j in range(3):
            stream.seek((BASE_INDEX + i * 3 + j) * INDEX_STRIDE)
            index = stream.read_u16()
            stream.seek(index * VERTEX_STRIDE + 0x10)
            data0 = stream.read_u32()
            data1 = stream.read_u32()
            x = (data0 >> 6 & 0x1fff) * SCALE + BASE_POS[0]
            y = (data0 >> 19 & 0x1fff) * SCALE + BASE_POS[1]
            z = (data1 & 0x1fff) * SCALE + BASE_POS[2]

            # this is just INVERSE * POS, numpy just calls it .dot()
            pos = np.array(INVERSE.dot(np.array([x, y, z, 1.0])))
            
            output += f"v {pos[0][0] + -302.73} {pos[0][1] + 76.65} {pos[0][2] - 950.19}\n"
		*/

		//Generating mesh
		{
			const glm::mat3x4& TransformOriginal = File.mHeader.mTransform.mMatrix;
			glm::mat4x4 Transform;
			Transform[0] = TransformOriginal[0];
			Transform[1] = TransformOriginal[1];
			Transform[2] = TransformOriginal[2];
			Transform[3].x = 0.0f;
			Transform[3].y = 0.0f;
			Transform[3].z = 0.0f;
			Transform[3].w = 1.0f;

			glm::mat3x4 Inverse = glm::inverse(Transform);

			//File.mHeader.mChunkMesh.mChunkFiles

			const int IndexStride = 2;
			const int VertexStride = 0x1c;

			for (size_t i = 0; i < File.mHeader.mChunkMesh.mChunks.mEntries.size(); i++)
			{
				application::file::game::cave::CrbinFile::ResChunk& Chunk = File.mHeader.mChunkMesh.mChunks.mEntries[i];
				if (Chunk.mLOD != 7)
					continue;

				const float Sidelength = File.mHeader.mChunkMesh.mMinSidelength * std::powf(2, (File.mHeader.mChunkMesh.mSubdivisions - Chunk.mLOD));
				const float Scale = Sidelength / 4096.0f;
				glm::vec3 BasePos = (File.mHeader.mChunkMesh.mBasePos.mVec - glm::vec3(Sidelength)) * glm::vec3(0.49993896) + (glm::vec3(Chunk.mX * Sidelength, Chunk.mY * Sidelength, Chunk.mZ * Sidelength));

				application::file::game::cave::CrbinFile::ResIndexInfo& IndexInfo = File.mHeader.mChunkMesh.mIndexInfo.mEntries[i];
				for (size_t j = IndexInfo.mBaseVertexStreamIndex; j < IndexInfo.mBaseVertexStreamIndex + IndexInfo.mVertexStreamCount; j++)
				{
					application::util::BinaryVectorReader Reader(File.mChunkFiles[File.mHeader.mChunkMesh.mStreamInfo.mEntries[j].mPageFileIndex].mData);
					
					uint32_t IndexOffset = Vertices.size() / 3;

					for (uint32_t k = 0; k < File.mHeader.mChunkMesh.mStreamInfo.mEntries[j].mTriangleCount; k++)
					{
						for (uint8_t a = 0; a < 3; a++)
						{
							Reader.Seek((File.mHeader.mChunkMesh.mStreamInfo.mEntries[j].mBaseIndex + k * 3 + a) * IndexStride, application::util::BinaryVectorReader::Position::Begin);
							uint16_t Index = Reader.ReadUInt16();
							Reader.Seek(Index * VertexStride + 0x10, application::util::BinaryVectorReader::Position::Begin);
							uint32_t Data0 = Reader.ReadUInt32();
							uint32_t Data1 = Reader.ReadUInt32();

							float x = (Data0 >> 6 & 0x1fff) * Scale + BasePos.x;
							float y = (Data0 >> 19 & 0x1fff) * Scale + BasePos.y;
							float z = (Data1 & 0x1fff) * Scale + BasePos.z;

							glm::vec4 Pos = Inverse * glm::vec4(x, y, z, 1.0f);
							Vertices.push_back(Pos.x);
							Vertices.push_back(Pos.y);
							Vertices.push_back(Pos.z);
						}

						Indices.push_back(Vertices.size() / 3 - 1);
						Indices.push_back(Vertices.size() / 3 - 2);
						Indices.push_back(Vertices.size() / 3 - 3);
					}

					/*
					for (uint32_t k = 0; k < File.mHeader.mChunkMesh.mStreamInfo.mEntries[j].mTriangleCount; k++)
					{
						Indices.push_back(IndexOffset + k * 3 + 0);
						Indices.push_back(IndexOffset + k * 3 + 1);
						Indices.push_back(IndexOffset + k * 3 + 2);
					}
					*/
				}

				break;
			}
		}

		glGenVertexArrays(1, &mCaveVAO);
		glBindVertexArray(mCaveVAO);

		glGenBuffers(1, &mCaveEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mCaveEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(uint32_t), Indices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &mCaveVBO);
		glBindBuffer(GL_ARRAY_BUFFER, mCaveVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * Vertices.size(), &Vertices[0], GL_STATIC_DRAW);

		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		mShader = application::manager::ShaderMgr::GetShader("Cave");
		mIndexCount = Indices.size();
	}

	void CaveRenderer::PrepareGlobal(application::gl::Camera& Camera)
	{
		mShader->Bind();

		Camera.Matrix(mShader, "camMatrix");
	}

	void CaveRenderer::Draw(const glm::mat4& Matrix)
	{
		glUniformMatrix4fv(glGetUniformLocation(mShader->mID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(Matrix));

		glBindVertexArray(mCaveVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mCaveEBO);
		glDrawElements(GL_TRIANGLES, sizeof(int) * mIndexCount, GL_UNSIGNED_INT, 0);
	}

	void CaveRenderer::Delete()
	{
		glDeleteBuffers(1, &mCaveVBO);
		glDeleteBuffers(1, &mCaveEBO);
		glDeleteVertexArrays(1, &mCaveVAO);
	}
}