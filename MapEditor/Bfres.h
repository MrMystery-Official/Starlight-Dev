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
#include <variant>

#include "BfresBinaryVectorReader.h"
#include "TextureToGo.h"

#include "Mesh.h"

class BfresFile
{
public:
	static std::unordered_map<uint64_t, std::string> ExternalBinaryStrings;
	static void Initialize();

	enum BfresAttribFormat : uint32_t
	{
		// 8 bits (8 x 1)
		Format_8_UNorm = 0x00000102, //
		Format_8_UInt = 0x00000302, //
		Format_8_SNorm = 0x00000202, //
		Format_8_SInt = 0x00000402, //
		Format_8_UIntToSingle = 0x00000802,
		Format_8_SIntToSingle = 0x00000A02,
		// 8 bits (4 x 2)
		Format_4_4_UNorm = 0x00000001,
		// 16 bits (16 x 1)
		Format_16_UNorm = 0x0000010A,
		Format_16_UInt = 0x0000020A,
		Format_16_SNorm = 0x0000030A,
		Format_16_SInt = 0x0000040A,
		Format_16_Single = 0x0000050A,
		Format_16_UIntToSingle = 0x00000803,
		Format_16_SIntToSingle = 0x00000A03,
		// 16 bits (8 x 2)
		Format_8_8_UNorm = 0x00000109, //
		Format_8_8_UInt = 0x00000309, //
		Format_8_8_SNorm = 0x00000209, //
		Format_8_8_SInt = 0x00000409, //
		Format_8_8_UIntToSingle = 0x00000804,
		Format_8_8_SIntToSingle = 0x00000A04,
		// 32 bits (16 x 2)
		Format_16_16_UNorm = 0x00000112, //
		Format_16_16_SNorm = 0x00000212, //
		Format_16_16_UInt = 0x00000312,
		Format_16_16_SInt = 0x00000412,
		Format_16_16_Single = 0x00000512, //
		Format_16_16_UIntToSingle = 0x00000807,
		Format_16_16_SIntToSingle = 0x00000A07,
		// 32 bits (10/11 x 3)
		Format_10_11_11_Single = 0x00000809,
		// 32 bits (8 x 4)
		Format_8_8_8_8_UNorm = 0x0000010B, //
		Format_8_8_8_8_SNorm = 0x0000020B, //
		Format_8_8_8_8_UInt = 0x0000030B, //
		Format_8_8_8_8_SInt = 0x0000040B, //
		Format_8_8_8_8_UIntToSingle = 0x0000080B,
		Format_8_8_8_8_SIntToSingle = 0x00000A0B,
		// 32 bits (10 x 3 + 2)
		Format_10_10_10_2_UNorm = 0x0000000B,
		Format_10_10_10_2_UInt = 0x0000090B,
		Format_10_10_10_2_SNorm = 0x0000020E, // High 2 bits are UNorm //
		Format_10_10_10_2_SInt = 0x0000099B,
		// 64 bits (16 x 4)
		Format_16_16_16_16_UNorm = 0x00000115, //
		Format_16_16_16_16_SNorm = 0x00000215, //
		Format_16_16_16_16_UInt = 0x00000315, //
		Format_16_16_16_16_SInt = 0x00000415, //
		Format_16_16_16_16_Single = 0x00000515, //
		Format_16_16_16_16_UIntToSingle = 0x0000080E,
		Format_16_16_16_16_SIntToSingle = 0x00000A0E,
		// 32 bits (32 x 1)
		Format_32_UInt = 0x00000314,
		Format_32_SInt = 0x00000416,
		Format_32_Single = 0x00000516,
		// 64 bits (32 x 2)
		Format_32_32_UInt = 0x00000317, //
		Format_32_32_SInt = 0x00000417, //
		Format_32_32_Single = 0x00000517, //
		// 96 bits (32 x 3)
		Format_32_32_32_UInt = 0x00000318, //
		Format_32_32_32_SInt = 0x00000418, //
		Format_32_32_32_Single = 0x00000518, //
		// 128 bits (32 x 4)
		Format_32_32_32_32_UInt = 0x00000319, //
		Format_32_32_32_32_SInt = 0x00000419, //
		Format_32_32_32_32_Single = 0x00000519 //
	};

	enum BfresIndexFormat : uint32_t
	{
		UnsignedByte = 0,
		UInt16 = 1,
		UInt32 = 2
	};

	enum BfresPrimitiveType : uint32_t
	{
		Triangles = 0x03,
		TriangleStrip = 0x04
	};

	enum TexWrap : signed char
	{
		Repeat,
		Mirror,
		Clamp,
		ClampToEdge,
		MirrorOnce,
		MirrorOnceClampToEdge
	};

	enum CompareFunction : uint8_t
	{
		Never,
		Less,
		Equal,
		LessOrEqual,
		Greater,
		NotEqual,
		GreaterOrEqual,
		Always
	};

	enum TexBorderType : uint8_t
	{
		White,
		Transparent,
		Opaque
	};

	enum MaxAnisotropic : uint8_t
	{
		Ratio_1_1 = 0x1,
		Ratio_2_1 = 0x2,
		Ratio_4_1 = 0x4,
		Ratio_8_1 = 0x8,
		Ratio_16_1 = 0x10
	};


	enum ShaderParamType : uint8_t
	{
		Bool,
		Bool2,
		Bool3,
		Bool4,
		Int,
		Int2,
		Int3,
		Int4,
		UInt,
		UInt2,
		UInt3,
		UInt4,
		Float,
		Float2,
		Float3,
		Float4,
		Reserved2,
		Float2x2,
		Float2x3,
		Float2x4,
		Reserved3,
		Float3x2,
		Float3x3,
		Float3x4,
		Reserved4,
		Float4x2,
		Float4x3,
		Float4x4,
		Srt2D,
		Srt3D,
		TexSrt,
		TexSrtEx
	};

	struct BinaryHeader
	{
		uint64_t Magic; //MAGIC + padding

		uint8_t VersionMicro;
		uint8_t VersionMinor;
		uint16_t VersionMajor;

		uint16_t ByteOrder;
		uint8_t Alignment;
		uint8_t TargetAddressSize;
		uint32_t NameOffset;
		uint16_t Flag;
		uint16_t BlockOffset;
		uint32_t RelocationTableOffset;
		uint32_t FileSize;
	};

	struct ResHeader
	{
		uint64_t NameOffset;

		uint64_t ModelOffset;
		uint64_t ModelDictionaryOffset;

		uint64_t Reserved0;
		uint64_t Reserved1;
		uint64_t Reserved2;
		uint64_t Reserved3;

		uint64_t SkeletalAnimOffset;
		uint64_t SkeletalAnimDictionaryOffset;
		uint64_t MaterialAnimOffset;
		uint64_t MaterialAnimDictionarymOffset;
		uint64_t BoneVisAnimOffset;
		uint64_t BoneVisAnimDictionarymOffset;
		uint64_t ShapeAnimOffset;
		uint64_t ShapeAnimDictionarymOffset;
		uint64_t SceneAnimOffset;
		uint64_t SceneAnimDictionarymOffset;

		uint64_t MemoryPoolOffset;
		uint64_t MemoryPoolInfoOffset;

		uint64_t EmbeddedFilesOffset;
		uint64_t EmbeddedFilesDictionaryOffset;

		uint64_t UserPointer;

		uint64_t StringTableOffset;
		uint32_t StringTableSize;

		uint16_t ModelCount;

		uint16_t Reserved4;
		uint16_t Reserved5;

		uint16_t SkeletalAnimCount;
		uint16_t MaterialAnimCount;
		uint16_t BoneVisAnimCount;
		uint16_t ShapeAnimCount;
		uint16_t SceneAnimCount;
		uint16_t EmbeddedFileCount;

		uint8_t ExternalFlags;
		uint8_t Reserved6;
	};

	struct BufferMemoryPool
	{
		uint32_t Flag = 0;
		uint32_t Size = 0;
		uint64_t Offset = 0;

		uint64_t Reserved1 = 0;
		uint64_t Reserved2 = 0;
	};

	struct ModelHeader
	{
		uint32_t Magic;
		uint32_t Reserved;
		uint64_t NameOffset;
		uint64_t PathOffset;

		uint64_t SkeletonOffset;
		uint64_t VertexArrayOffset;
		uint64_t ShapeArrayOffset;
		uint64_t ShapeDictionaryOffset;
		uint64_t MaterialArrayOffset;
		uint64_t MaterialDictionaryOffset;
		uint64_t ShaderAssignArrayOffset;

		uint64_t UserDataArrayOffset;
		uint64_t UserDataDictionaryOffset;

		uint64_t UserPointer;

		uint16_t VertexBufferCount;
		uint16_t ShapeCount;
		uint16_t MaterialCount;
		uint16_t ShaderAssignCount;
		uint16_t UserDataCount;

		uint16_t Reserved1;
		uint32_t Reserved2;
	};

	struct VertexBufferHeader
	{
		uint32_t Magic;
		uint32_t Reserved;

		uint64_t AttributeArrayOffset;
		uint64_t AttributeArrayDictionary;

		uint64_t MemoryPoolPointer;

		uint64_t RuntimeBufferArray;
		uint64_t UserBufferArray;

		uint64_t VertexBufferInfoArray;
		uint64_t VertexBufferStrideArray;
		uint64_t UserPointer;

		uint32_t BufferOffset;
		uint8_t VertexAttributeCount;
		uint8_t VertexBufferCount;

		uint16_t Index;
		uint32_t VertexCount;

		uint16_t Reserved1;
		uint16_t VertexBufferAlignment;
	};

	struct ShapeHeader
	{
		uint32_t Magic;
		uint32_t Flags;
		uint64_t NameOffset;
		uint64_t PathOffset;

		uint64_t MeshArrayOffset;
		uint64_t SkinBoneIndicesOffset;

		uint64_t KeyShapeArrayOffset;
		uint64_t KeyShapeDictionaryOffset;

		uint64_t BoundingBoxOffset;
		uint64_t BoundingSphereOffset;

		uint64_t UserPointer;

		uint16_t Index;
		uint16_t MaterialIndex;
		uint16_t BoneIndex;
		uint16_t VertexBufferIndex;
		uint16_t SkinBoneIndex;

		uint8_t MaxSkinInfluence;
		uint8_t MeshCount;
		uint8_t KeyShapeCount;
		uint8_t KeyAttributeCount;

		uint16_t Reserved;
	};

	struct MeshHeader
	{
		uint64_t SubMeshArrayOffset;
		uint64_t MemoryPoolOffset;
		uint64_t BufferRuntimeOffset;
		uint64_t BufferInfoOffset;

		uint32_t BufferOffset;

		BfresPrimitiveType PrimType;
		BfresIndexFormat IndexFormat;
		uint32_t IndexCount;
		uint32_t BaseIndex;
		uint16_t SubMeshCount;
		uint16_t Reserved;
	};

	struct MaterialHeader
	{
		uint32_t Magic; //FMAT
		uint32_t Flags;
		uint64_t NameOffset;

		uint64_t ShaderInfoOffset;

		uint64_t TextureRuntimeDataOffset;
		uint64_t TextureNamesOffset;
		uint64_t SamplerRuntimeDataOffset;
		uint64_t SamplerOffset;
		uint64_t SamplerDictionaryOffset;
		uint64_t RenderInfoBufferOffset;
		uint64_t RenderInfoNumOffset;
		uint64_t RenderInfoDataOffsetTable;
		uint64_t ParamDataOffset;
		uint64_t ParamIndicesOffset;
		uint64_t Reserved;
		uint64_t UserDataOffset;
		uint64_t UserDataDictionaryOffset;
		uint64_t VolatileFlagsOffset;
		uint64_t UserPointer;
		uint64_t SamplerIndicesOffset;
		uint64_t TextureIndicesOffset;

		uint16_t Index;
		uint8_t SamplerCount;
		uint8_t TextureRefCount;
		uint16_t Reserved1;
		uint16_t UserDataCount;

		uint16_t RenderInfoDataSize;
		uint16_t Reserved2;
		uint32_t Reserved3;
	};

	struct ShaderInfoHeader
	{
		uint64_t ShaderAssignOffset;
		uint64_t AttributeAssignOffset;
		uint64_t AttributeAssignIndicesOffset;
		uint64_t SamplerAssignOffset;
		uint64_t SamplerAssignIndicesOffset;
		uint64_t OptionBoolChoiceOffset;
		uint64_t OptionStringChoiceOffset;
		uint64_t OptionIndicesOffset;

		uint32_t Padding;

		uint8_t NumAttributeAssign;
		uint8_t NumSamplerAssign;
		uint16_t NumOptionBooleans;
		uint16_t NumOptions;

		uint16_t Padding2;
		uint32_t Padding3;
	};

	struct ShaderAssignHeader
	{
		uint64_t ShaderArchiveNameOffset;
		uint64_t ShaderModelNameOffset;
		uint64_t RenderInfoOffset;
		uint64_t RenderInfoDictOffset;
		uint64_t ShaderParamOffset;
		uint64_t ShaderParamDictOffset;
		uint64_t AttributeAssignDictOffset;
		uint64_t SamplerAssignDictOffset;
		uint64_t OptionsDictOffset;

		uint16_t RenderInfoCount;
		uint16_t ParamCount;
		uint16_t ShaderParamSize;
		uint16_t Padding1;
		uint32_t Padding2;
	};

	struct SkeletonHeader
	{
		uint32_t Magic;
		uint32_t Flags;
		uint64_t BoneDictionaryOffset;
		uint64_t BoneArrayOffset;
		uint64_t MatrixToBoneListOffset;
		uint64_t InverseModelMatricesOffset;
		uint64_t Reserved;
		uint64_t UserPointer;

		uint16_t NumBones;
		uint16_t NumSmoothMatrices;
		uint16_t NumRigidMatrices;

		uint16_t Padding1;
		uint32_t Padding2;
	};

	struct BoneHeader
	{
		uint64_t NameOffset;
		uint64_t UserDataDictionaryOffset;
		uint64_t UserDataArrayOffset;
		uint64_t Reserved;

		uint16_t Index;

		short ParentIndex;
		short SmoothMatrixIndex;
		short RigidMatrixIndex;
		short BillboardIndex;

		uint16_t NumUserData;

		uint32_t Flags;

		float ScaleX;
		float ScaleY;
		float ScaleZ;

		float RotationX;
		float RotationY;
		float RotationZ;
		float RotationW;

		float PositionX;
		float PositionY;
		float PositionZ;
	};


	struct VertexBuffer : public BfresBinaryVectorReader::ReadableObject
	{
		struct BufferData
		{
			int32_t Stride = 0;
			std::vector<unsigned char> Data;
		};

		struct VertexAttribute : public BfresBinaryVectorReader::ReadableObject
		{
			std::string Name = "";
			BfresAttribFormat Format;
			uint16_t Offset = 0;
			uint16_t BufferIndex = 0;

			virtual void Read(BfresBinaryVectorReader& Reader) override;
			std::vector<glm::vec4> GetData(VertexBuffer Buffer);
		};

		struct VertexBufferInfo : public BfresBinaryVectorReader::ReadableObject
		{
			uint32_t Size = 0;

			virtual void Read(BfresBinaryVectorReader& Reader) override;
		};

		struct VertexStrideInfo : public BfresBinaryVectorReader::ReadableObject
		{
			uint32_t Stride = 0;

			virtual void Read(BfresBinaryVectorReader& Reader) override;
		};

		VertexBufferHeader Header;
		uint32_t VertexCount = 0;
		std::vector<BufferData> Buffers;
		BfresBinaryVectorReader::ResDict<VertexAttribute> Attributes;
		std::vector<VertexBufferInfo> BufferInfo;
		std::vector<VertexStrideInfo> BufferStrides;

		virtual void Read(BfresBinaryVectorReader& Reader) override;
		void InitBuffers(BfresBinaryVectorReader& Reader, BufferMemoryPool PoolInfo);
		std::vector<glm::vec4> TryGetAttributeData(std::string Name);

		std::vector<glm::vec4> GetPositions();
		std::vector<glm::vec4> GetNormals();
		std::vector<glm::vec4> GetTexCoords(int Channel);
		std::vector<glm::vec4> GetColors(int Channel);
		std::vector<glm::vec4> GetBoneWeights(int Channel = 0);
		std::vector<glm::vec4> GetBoneIndices(int Channel = 0);
		std::vector<glm::vec4> GetTangents();
		std::vector<glm::vec4> GetBitangent();
	};

	struct Material : public BfresBinaryVectorReader::ReadableObject
	{
		struct Sampler : public BfresBinaryVectorReader::ReadableObject
		{
			enum class MipFilterModes : uint16_t
			{
				None = 0,
				Points = 1,
				Linear = 2
			};

			enum class ExpandFilterModes : uint16_t
			{
				Points = 1 << 2,
				Linear = 2 << 2
			};

			enum class ShrinkFilterModes : uint16_t
			{
				Points = 1 << 4,
				Linear = 2 << 4
			};

			uint16_t FlagsShrinkMask = 0b0000000000110000;
			uint16_t FlagsExpandMask = 0b0000000000001100;
			uint16_t FlagsMipmapMask = 0b0000000000000011;

			uint16_t FilterFlags = 0;

			MipFilterModes Mipmap;
			ExpandFilterModes MagFilter;
			ShrinkFilterModes MinFilter;

			TexWrap WrapModeU;
			TexWrap WrapModeV;
			TexWrap WrapModeW;

			CompareFunction CompareFunc;
			TexBorderType BorderColorType;
			MaxAnisotropic Anisotropic;

			float MinLOD;
			float MaxLOD;
			float LODBias;

			virtual void Read(BfresBinaryVectorReader& Reader) override;
		};

		struct ShaderParam : public BfresBinaryVectorReader::ReadableObject
		{
			struct Srt2D
			{
				glm::vec2 Scaling;
				float Rotation;
				glm::vec2 Translation;
			};
			struct Srt3D
			{
				glm::vec3 Scaling;
				glm::vec3 Rotation;
				glm::vec3 Translation;
			};
			struct TexSrt
			{
				enum TexSrtMode : uint32_t
				{
					ModeMaya,
					Mode3dsMax,
					ModeSoftimage
				};

				TexSrtMode Mode;
				glm::vec2 Scaling;
				float Rotation;
				glm::vec2 Translation;
			};

			ShaderParamType Type;
			uint16_t DataOffset;
			std::string Name;
			std::variant<bool, float, int32_t, uint32_t, uint8_t, std::vector<bool>, std::vector<float>, std::vector<int32_t>, std::vector<uint32_t>, std::vector<uint8_t>, Srt2D, Srt3D, TexSrt> DataValue;

			virtual void Read(BfresBinaryVectorReader& Reader) override;
			void ReadShaderParamData(BfresBinaryVectorReader& Reader);
			std::variant<bool, float, int32_t, uint32_t, uint8_t, std::vector<bool>, std::vector<float>, std::vector<int32_t>, std::vector<uint32_t>, std::vector<uint8_t>, Srt2D, Srt3D, TexSrt> ReadParamData(ShaderParamType Type, BfresBinaryVectorReader& Reader);
		};

		struct RenderInfo : public BfresBinaryVectorReader::ReadableObject
		{
			enum RenderInfoType : uint8_t
			{
				Int32,
				Single,
				String
			};

			std::string Name;
			RenderInfoType Type;
			std::variant<std::vector<int32_t>, std::vector<float>, std::vector<std::string>> _Value;
			std::variant<std::vector<int32_t>, std::vector<float>, std::vector<std::string>> Data;

			std::vector<float> GetValueSingles();
			std::vector<int32_t> GetValueInt32s();
			std::vector<std::string> GetValueStrings();

			void ReadData(BfresBinaryVectorReader& Reader, uint32_t Count);

			virtual void Read(BfresBinaryVectorReader& Reader) override;
		};

		struct ShaderAssign
		{
			BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> ShaderOptions;
			BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> SamplerAssign;
			BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> AttributeAssign;

			std::string ShaderArchiveName;
			std::string ShadingModelName;
		};

		std::string Name;
		bool Visible = true;
		BfresBinaryVectorReader::ResDict<Sampler> Samplers;
		BfresBinaryVectorReader::ResDict<ShaderParam> ShaderParams;
		BfresBinaryVectorReader::ResDict<RenderInfo> RenderInfos;

		std::vector<std::string> Textures;
		ShaderAssign ShaderAssign;

		MaterialHeader Header;
		ShaderInfoHeader HeaderShaderInfo;
		ShaderAssignHeader HeaderShaderAssign;

		void ReadShaderOptions(BfresBinaryVectorReader& Reader);
		void ReadRenderInfo(BfresBinaryVectorReader& Reader);
		void ReadShaderParameters(BfresBinaryVectorReader& Reader);
		BfresBinaryVectorReader::ResDict<BfresBinaryVectorReader::ResString> ReadAssign(BfresBinaryVectorReader& Reader, uint64_t StringListOffset, uint64_t StringDictOffset, uint64_t IndicesOffset, int32_t NumValues);
		//void LoadTextures(BfresFile& File, std::string TexDir);

		virtual void Read(BfresBinaryVectorReader& Reader) override;

		std::string GetRenderInfoString(std::string Key);
		float GetRenderInfoFloat(std::string Key);
		int32_t GetRenderInfoInt(std::string Key);
	};

	struct Skeleton : public BfresBinaryVectorReader::ReadableObject
	{
		struct Bone : public BfresBinaryVectorReader::ReadableObject
		{
			enum BoneFlagsRotation : uint32_t
			{
				Quaternion,
				EulerXYZ = 1 << 12
			};

			uint32_t _FlagsMask = 0b00000000000000000000000000000001;
			uint32_t _FlagsMaskRotate = 0b00000000000000000111000000000000;

			uint32_t _Flags = 0;
			std::string Name;
			BoneFlagsRotation FlagsRotation;
			glm::vec3 Position;
			glm::vec4 Rotate;
			glm::vec3 Scale;

			uint32_t Index;
			int16_t SmoothMatrixIndex;
			int16_t RigidMatrixIndex;
			int16_t ParentIndex;

			BoneHeader Header;

			glm::mat4 WorldMatrix;
			glm::mat4 InverseMatrix;
			bool CalculatedMatrices = false;

			void CalculateLocalMatrix(BfresBinaryVectorReader::ResDict<Bone>& Bones);

			virtual void Read(BfresBinaryVectorReader& Reader) override;
		};

		std::vector<uint16_t> MatrixToBoneList;
		uint16_t NumSmoothMatrices = 0;
		uint16_t NumRigidMatrices = 0;
		SkeletonHeader Header;
		BfresBinaryVectorReader::ResDict<Bone> Bones;

		void CalculateMatrices(bool CalculateInverse = false);
		virtual void Read(BfresBinaryVectorReader& Reader) override;
	};

	struct Shape : public BfresBinaryVectorReader::ReadableObject
	{
		struct Mesh : public BfresBinaryVectorReader::ReadableObject
		{
			struct IndexBufferInfo : public BfresBinaryVectorReader::ReadableObject
			{
				uint32_t Size = 0;
				uint32_t Flag = 0;

				virtual void Read(BfresBinaryVectorReader& Reader) override;
			};

			struct SubMesh : public BfresBinaryVectorReader::ReadableObject
			{
				uint32_t Count = 0;
				uint32_t Offset = 0;

				virtual void Read(BfresBinaryVectorReader& Reader) override;
			};

			uint32_t IndexCount = 0;
			std::vector<unsigned char> IndexBuffer;
			BfresIndexFormat IndexFormat;
			BfresPrimitiveType PrimitiveType;
			MeshHeader Header;
			std::vector<SubMesh> SubMeshes;
			IndexBufferInfo BufferInfo;

			virtual void Read(BfresBinaryVectorReader& Reader) override;
			void InitBuffers(BfresBinaryVectorReader& Reader, BufferMemoryPool Pool);
			std::vector<uint32_t> GetIndices();
		};

		struct ShapeBounding : public BfresBinaryVectorReader::ReadableObject
		{
			glm::vec3 Center;
			glm::vec3 Extent;

			virtual void Read(BfresBinaryVectorReader& Reader) override;
		};

		std::string Name;
		uint8_t SkinCount = 0;
		int32_t VertexBufferIndex = 0;
		int32_t MaterialIndex = 0;
		int32_t BoneIndex = 0;
		BfresFile::VertexBuffer Buffer;
		ShapeHeader Header;
		std::vector<Mesh> Meshes;
		std::vector<glm::vec4> BoundingSpheres;
		std::vector<ShapeBounding> BoundingBoxes;
		std::vector<glm::vec4> Vertices;

		virtual void Read(BfresBinaryVectorReader& Reader) override;
		void Init(BfresBinaryVectorReader& Reader, BfresFile::VertexBuffer Buffer, BufferMemoryPool Pool, BfresFile::Skeleton& Skeleton);
	};

	struct EmbeddedFile : public BfresBinaryVectorReader::ReadableObject
	{
		std::vector<unsigned char> Data;

		virtual void Read(BfresBinaryVectorReader& Reader) override;
	};

	struct Model : public BfresBinaryVectorReader::ReadableObject
	{
		std::string Name;
		BfresFile::ModelHeader Header;
		std::vector<VertexBuffer> VertexBuffers;
		BfresBinaryVectorReader::ResDict<Shape> Shapes;
		BfresBinaryVectorReader::ResDict<Material> Materials;
		Skeleton ModelSkeleton;
		float BoundingBoxSphereRadius = 0.0f;

		virtual void Read(BfresBinaryVectorReader& Reader) override;
		void Init(BfresFile& Bfres, BfresBinaryVectorReader& Reader, BufferMemoryPool PoolInfo, std::string TexDir);
	};

	BinaryHeader BinHeader;
	ResHeader Header;
	BufferMemoryPool BufferMemoryPoolInfo;

	BfresBinaryVectorReader::ResDict<Model> Models;
	BfresBinaryVectorReader::ResDict<EmbeddedFile> EmbeddedFiles;

	bool mDefaultModel = false;

	std::string mTexDir = "";

	void DecompressMeshCodec(std::string Path, std::vector<unsigned char> Bytes);

	BfresFile(std::vector<unsigned char> Bytes);
	BfresFile(std::string Path, std::vector<unsigned char> Bytes, std::string TexDir = "");
	BfresFile(std::string Path);
	BfresFile() {}
private:
	std::string m_Path = "";
};

namespace BfresLibrary
{
	extern std::map<std::string, BfresFile> Models;

	void Initialize();

	bool IsModelLoaded(std::string ModelName);
	BfresFile* GetModel(std::string ModelName);
	void Cleanup();
};