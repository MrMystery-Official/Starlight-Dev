#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <stb/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <unordered_map>
#include <map>

#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"
#include "BinaryVectorReader.h"
#include "TextureToGo.h"
#include "Vector3F.h"

#include "Mesh.h"

class BfresFile
{
public:
	static BfresFile CreateDefaultModel(std::string DefaultName, uint8_t Red, uint8_t Green, uint8_t Blue, uint8_t Alpha);

	struct BfresTexture
	{
		TextureToGo* Texture;
		std::vector<float> TexCoordinates;
		std::string Sampler = "";
	};

	struct Material
	{
		std::string Name;
		bool IsTransparent = false;
		std::vector<BfresFile::BfresTexture> Textures;
	};

	struct LOD
	{
		std::vector<std::vector<unsigned int>> Faces;
		std::vector<Mesh> GL_Meshes;

		std::vector<uint32_t> OpaqueObjects;
		std::vector<uint32_t> TransparentObjects;
	};

	struct SkeletonBone
	{
		uint16_t Index = 0;
		int16_t ParentIndex = -1;
		int16_t SmoothMatrixIndex = -1;
		int16_t RigidMatrixIndex = -1;
		int16_t BillboardIndex = -1;
		uint16_t NumUserData = 0;
		uint32_t Flags = 0;
		float Scale[3] = { 1, 1, 1 };
		float Rotation[4] = { 0, 0, 0, 1 };
		float Position[3] = { 0, 0, 0 };
	};

	struct Model
	{
		std::vector<LOD> LODs;
		std::vector<std::vector<float>> Vertices;
		std::vector<Material> Materials;
		float BoundingBoxSphereRadius;
	};

	std::vector<BfresFile::Model>& GetModels();
	bool& IsDefaultModel();
	void Delete();
	void CreateOpenGLObjects();

	BfresFile(std::vector<unsigned char> Bytes);
	BfresFile(std::string Path, std::vector<unsigned char> Bytes);
	BfresFile(std::string Path);
	BfresFile() {}
private:
	enum class VertexBufferFormat : uint16_t
	{
		Format_16_16_16_16_Single = 5381,
		Format_10_10_10_2_SNorm = 3586,
		Format_8_8_8_8_UInt = 2819,
		Format_8_8_8_8_SNorm = 2818,
		Format_8_8_8_8_UNorm = 2817,
		Format_8_8_UNorm = 2305,
		Format_8_8_SNorm = 2306,
		Format_8_UInt = 515,
		Format_16_16_Single = 4613,
		Format_16_16_UNorm = 4609,
		Format_16_16_SNorm = 4610,
		Format_32_32_Single = 5893,
		Format_32_32_32_Single = 6149
	};

	struct VertexBufferSize
	{
		uint32_t Size;
		uint32_t GPUFlags;
	};

	struct VertexBufferAttribute
	{
		std::string Name;
		uint16_t Format;
		uint16_t Offset;
		uint16_t BufferIndex;
	};

	struct VertexBuffer {
		uint32_t BufferOffset;
		uint32_t Size;
		uint32_t Stride;
		std::vector<unsigned char> Data;
	};

	std::vector<BfresFile::Model> m_Models;
	std::vector<BfresFile::SkeletonBone> m_SkeletonBones;
	std::vector<uint16_t> m_Rigids;
	bool m_DefaultModel = false;
	std::string m_Path = "";

	void GenerateBoundingBox();
	float ShortToFloat(uint8_t Byte1, uint8_t Byte2);
	float UInt32ToFloat(unsigned char Byte1, unsigned char Byte2, unsigned char Byte3, unsigned char Byte4);
	uint16_t CombineToUInt16(uint8_t Byte1, uint8_t Byte2);
	std::string ReadString(BinaryVectorReader& Reader, uint64_t Offset);
};

namespace GLTextureLibrary
{
	extern std::unordered_map<TextureToGo*, Texture> Textures;

	bool IsTextureLoaded(TextureToGo* TexToGo);
	Texture* GetTexture(TextureToGo* TexToGo, std::string Sampler = "", GLenum Slot = 0);
	Texture* AddTexture(TextureToGo& Tex, std::string Sampler = "", GLenum Slot = 0);
	std::unordered_map<TextureToGo*, Texture>& GetTextures();
	void Cleanup();
};

namespace BfresLibrary
{
	extern std::map<std::string, BfresFile> Models;

	void Initialize();

	bool IsModelLoaded(std::string ModelName);
	BfresFile* GetModel(std::string ModelName);
	void Cleanup();
};