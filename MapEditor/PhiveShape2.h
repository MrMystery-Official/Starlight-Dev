#pragma once

#include <vector>
#include <string>
#include <map>
#include <utility>
#include "BinaryVectorReader.h"
#include "BinaryVectorWriter.h"

class PhiveShape2
{
public:
	struct PhiveShapeHeader
	{
		char m_Magic[6]; //Includes null terminator
		uint16_t m_Reserve1 = 0;
		uint16_t m_BOM = 0;
		uint8_t m_MajorVersion = 0;
		uint8_t m_MinorVersion = 0;
		uint32_t m_HktOffset = 0;
		uint32_t m_TableOffset0 = 0;
		uint32_t m_TableOffset1 = 0;
		uint32_t m_FileSize = 0;
		uint32_t m_HktSize = 0;
		uint32_t m_TableSize0 = 0;
		uint32_t m_TableSize1 = 0;
		uint32_t m_Reserve2 = 0;
		uint32_t m_Reserve3 = 0;
	};

	struct HavokTagFileHeader
	{
		uint8_t m_TagFlags;
		uint32_t m_TagSize;

		uint8_t m_SDKVFlags;
		uint32_t m_SDKVSize;

		uint8_t m_DataFlags;
		uint32_t m_DataSize;
	};

	//Havok
	enum class hknpShapeTypeEnum : uint8_t {
		CONVEX = 0x0,
		SPHERE = 0x1,
		CAPSULE = 0x2,
		CYLINDER = 0x3,
		TRIANGLE = 0x4,
		BOX = 0x5,
		DEBRIS = 0x6,
		COMPOSITE = 0x7,
		MESH = 0x8,
		COMPRESSED_MESH_DEPRECATED = 0x9,
		EXTERN_MESH = 0xa,
		COMPOUND = 0xb,
		DISTANCE_FIELD_BASE = 0xc,
		HEIGHT_FIELD = 0xd,
		DISTANCE_FIELD = 0xe,
		PARTICLE_SYSTEM = 0xf,
		SCALED_CONVEX = 0x10,
		TRANSFORMED = 0x11,
		MASKED = 0x12,
		MASKED_COMPOUND = 0x13,
		BREAKABLE_COMPOUND = 0x14,
		LOD = 0x15,
		DUMMY = 0x16,
		USER_0 = 0x17,
		USER_1 = 0x18,
		USER_2 = 0x19,
		USER_3 = 0x1a,
		USER_4 = 0x1b,
		USER_5 = 0x1c,
		USER_6 = 0x1d,
		USER_7 = 0x1e,
		NUM_SHAPE_TYPES = 0x1f,
		INVALID = 0x20,
	};

	enum class hknpCollisionDispatchTypeEnum : uint8_t {
		NONE = 0x0,
		DEBRIS = 0x1,
		CONVEX = 0x2,
		COMPOSITE = 0x3,
		DISTANCE_FIELD = 0x4,
		USER_0 = 0x5,
		USER_1 = 0x6,
		USER_2 = 0x7,
		USER_3 = 0x8,
		USER_4 = 0x9,
		USER_5 = 0xa,
		USER_6 = 0xb,
		USER_7 = 0xc,
		NUM_TYPES = 0xd,
	};
	enum class hknpShapeFlagsEnum : uint16_t {
		IS_CONVEX_SHAPE = 0x1,
		IS_CONVEX_POLYTOPE_SHAPE = 0x2,
		IS_COMPOSITE_SHAPE = 0x4,
		IS_HEIGHT_FIELD_SHAPE = 0x8,
		USE_SINGLE_POINT_MANIFOLD = 0x10,
		IS_INTERIOR_TRIANGLE = 0x20,
		SUPPORTS_COLLISIONS_WITH_INTERIOR_TRIANGLES = 0x40,
		USE_NORMAL_TO_FIND_SUPPORT_PLANE = 0x80,
		IS_TRIANGLE_OR_QUAD_SHAPE = 0x200,
		IS_QUAD_SHAPE = 0x400,
		IS_SDF_EDGE_COLLISION_ENABLED = 0x800,
		HAS_SURFACE_VELOCITY = 0x8000,
		USER_FLAG_0 = 0x1000,
		USER_FLAG_1 = 0x2000,
		USER_FLAG_2 = 0x4000,
		INHERITED_FLAGS = 0x8090,
	};

	struct hkReadableWriteableObject
	{
		virtual void Read(BinaryVectorReader& Reader) = 0;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) = 0;
		virtual void Skip(BinaryVectorWriter& Writer) {}
	};

	template<typename T>
	struct hkRelArrayView : public hkReadableWriteableObject {
		uint32_t m_Offset;
		uint32_t m_Size;
		std::vector<T> m_Elements;

		int32_t m_BaseOffset = 0;
		uint32_t m_LastElementOffset = 0;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
		virtual void Skip(BinaryVectorWriter& Writer) override;
	};

	template<typename T>
	struct hkRelArrayViewVertexBuffer : public hkReadableWriteableObject {
		uint32_t m_Offset;
		uint32_t m_Size;
		std::vector<T> m_Elements;

		int32_t m_BaseOffset = 0;
		uint32_t m_LastElementOffset = 0;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
		virtual void Skip(BinaryVectorWriter& Writer) override;
	};

	template<typename T>
	struct hkRelPtr : public hkReadableWriteableObject
	{
		uint64_t m_Offset = 0;
		T m_Data;

		int32_t m_BaseOffset = 0;
		uint32_t m_LastElementOffset = 0;

		bool m_NullPtr = true;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
		virtual void Skip(BinaryVectorWriter& Writer) override;
	};

	template<typename T>
	struct hkRelArray : public hkReadableWriteableObject
	{
		uint64_t m_Offset;
		uint32_t m_Size;
		uint32_t m_CapacityAndFlags;
		std::vector<T> m_Elements;

		int32_t m_BaseOffset = 0;
		uint32_t m_LastElementOffset = 0;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
		virtual void Skip(BinaryVectorWriter& Writer) override;
	};

	struct hkBaseObject : public hkReadableWriteableObject
	{
		uint64_t m_VTFReserve = 0;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hkReferencedObject : public hkReadableWriteableObject
	{
		hkBaseObject m_BaseObject;
		uint64_t m_SizeAndFlags = 0;
		uint64_t m_RefCount = 0;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpShapePropertyBase : public hkReadableWriteableObject
	{
		hkReferencedObject m_ReferencedObject;
		uint16_t m_PropertyKey = 0;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpShapePropertiesEntry : public hkReadableWriteableObject
	{
		hkRelPtr<hknpShapePropertyBase> m_Object;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpShapeProperties : public hkReadableWriteableObject
	{
		hkRelArray<hknpShapePropertiesEntry> m_Properties;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpShape : public hkReadableWriteableObject
	{
		hkReferencedObject m_ReferencedObject;
		hknpShapeTypeEnum m_Type;
		hknpCollisionDispatchTypeEnum m_DispatchType;
		hknpShapeFlagsEnum m_Flags;
		uint8_t m_NumShapeKeyBits;
		uint8_t m_Reserve0[3]; //uint24_t
		float m_ConvexRadius;
		uint32_t m_Reserve1;
		uint64_t m_UserData;
		hknpShapeProperties m_Properties;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpCompositeShape : public hkReadableWriteableObject
	{
		hknpShape m_Shape;
		uint32_t m_ShapeTagCodecInfo = 0;
		uint32_t m_Reserve2 = 0;
		uint64_t m_Reserve3 = 0;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpMeshShapeVertexConversionUtil : public hkReadableWriteableObject
	{
		float m_BitScale16[4];
		float m_BitScale16Inv[4];

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpMeshShapeShapeTagTableEntry : public hkReadableWriteableObject
	{
		uint32_t m_MeshPrimitiveKey;
		uint16_t m_ShapeTag;
		uint16_t m_Reserve0;

		uint32_t m_Offset;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hkcdFourAabb : public hkReadableWriteableObject
	{
		float m_MinX[4];
		float m_MaxX[4];
		float m_MinY[4];
		float m_MaxY[4];
		float m_MinZ[4];
		float m_MaxZ[4];

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hkcdSimdTreeNode : public hkReadableWriteableObject
	{
		hkcdFourAabb m_FourAabb;
		uint32_t m_Data[4];
		uint8_t m_IsLeaf;
		uint8_t m_IsActive;
		uint16_t m_Reserve0;
		uint32_t m_Reserve1;
		uint64_t m_Reserve2;

		uint32_t m_Offset;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hkcdSimdTree : public hkReadableWriteableObject
	{
		hkRelArray<hkcdSimdTreeNode> m_Nodes;
		bool m_IsCompact = false;
		uint8_t m_Reserve0[3] = { 0 };
		uint32_t m_Reserve1 = 0;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpTransposedFourAabbs8 : public hkReadableWriteableObject
	{
		uint32_t m_MinX = 0;
		uint32_t m_MaxX = 0;
		uint32_t m_MinY = 0;
		uint32_t m_MaxY = 0;
		uint32_t m_MinZ = 0;
		uint32_t m_MaxZ = 0;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpAabb8TreeNode : public hkReadableWriteableObject
	{
		hknpTransposedFourAabbs8 m_TransposedFourAabbs8;
		uint8_t m_Data[4];

		uint32_t m_Offset;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpMeshShapeGeometrySectionPrimitive : public hkReadableWriteableObject
	{
		uint16_t m_IdA;
		uint16_t m_IdB;
		uint16_t m_IdC;
		uint16_t m_IdD;

		uint32_t m_Offset;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpMeshShapeGeometrySectionVertex16_3 : public hkReadableWriteableObject
	{
		uint16_t m_X = 0;
		uint16_t m_Y = 0;
		uint16_t m_Z = 0;

		uint32_t m_Offset;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpMeshShapeGeometrySectionInteriorPrimitiveBitField : public hkReadableWriteableObject
	{
		uint8_t m_BitField;

		uint32_t m_Offset;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpMeshShapeGeometrySection : public hkReadableWriteableObject
	{
		hkRelArrayView<hknpAabb8TreeNode> m_SectionBvh;
		hkRelArrayView<hknpMeshShapeGeometrySectionPrimitive> m_Primitives;
		hkRelArrayViewVertexBuffer<hknpMeshShapeGeometrySectionVertex16_3> m_VertexBuffer;
		hkRelArrayView<hknpMeshShapeGeometrySectionInteriorPrimitiveBitField> m_InteriorPrimitiveBitField;
		uint32_t m_SectionOffset[3];
		float m_BitScale8Inv[3];
		int16_t m_BitOffset[3];
		uint16_t m_Reserve0;

		uint32_t m_Offset;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpMeshShapePrimitiveMapping : public hkReadableWriteableObject
	{
		hkReferencedObject m_ReferencedObject;
		hkRelArray<uint32_t> m_SectionStart;
		hkRelArray<uint32_t> m_BitString;
		uint32_t m_BitsPerEntry;
		uint32_t m_TriangleIndexBitMask;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	struct hknpMeshShape : public hkReadableWriteableObject
	{
		hknpCompositeShape m_CompositeShape;
		hknpMeshShapeVertexConversionUtil m_VertexConversionUtil;
		hkRelArrayView<hknpMeshShapeShapeTagTableEntry> m_ShapeTagTable;
		hkcdSimdTree m_TopLevelTree;
		hkRelArrayView<hknpMeshShapeGeometrySection> m_GeometrySections;
		hkRelPtr<hknpMeshShapePrimitiveMapping> m_PrimitiveMapping;

		virtual void Read(BinaryVectorReader& Reader) override;
		virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
	};

	//Phive
	struct PhiveMaterial
	{
		std::string Material;
		uint32_t SubMaterialIndex;
		uint64_t MaterialFlags;
		uint64_t CollisionFlags;
	};

	hknpMeshShape& GetMeshShape();
	std::vector<unsigned char> ToBinary();
	void WriteToFile(std::string Path);

	std::vector<float> ToVertices();
	std::vector<unsigned int> ToIndices();

	void InjectModel(std::vector<float> Vertices, std::vector<unsigned char> Indices);

	PhiveShape2(std::vector<unsigned char> Bytes);
	PhiveShape2() {}
private:
	struct PhiveTagNameMappingItem
	{
		struct Parameter
		{
			std::string m_Parameter;
			std::string m_Value = "";
			uint32_t m_ValueIndex = 0;
		};

		std::string m_Tag;
		std::vector<Parameter> m_Parameters;
	};

	const std::vector<std::string> m_MaterialList =
	{
		"Undefined",
		"Soil",
		"Grass",
		"Sand",
		"HeavySand",
		"Snow",
		"HeavySnow",
		"Stone",
		"StoneSlip",
		"StoneNoSlip",
		"SlipBoard",
		"Cart",
		"Metal",
		"MetalSlip",
		"MetalNoSlip",
		"WireNet",
		"Wood",
		"Ice",
		"Cloth",
		"Glass",
		"Bone",
		"Rope",
		"Character",
		"Ragdoll",
		"Surfing",
		"GuardianFoot",
		"LaunchPad",
		"Conveyer",
		"Rail",
		"Grudge",
		"Meat",
		"Vegetable",
		"Bomb",
		"MagicBall",
		"Barrier",
		"AirWall",
		"GrudgeSnow",
		"Tar",
		"Water",
		"HotWater",
		"IceWater",
		"Lava",
		"Bog",
		"ContaminatedWater",
		"DungeonCeil",
		"Gas",
		"InvalidateRestartPos",
		"HorseSpeedLimit",
		"ForbidDynamicCuttingAreaForHugeCharacter",
		"ForbidHorseReturnToSafePosByClimate",
		"Dragon",
		"ReferenceSurfing",
		"WaterSlip",
		"HorseDeleteImmediatelyOnDeath"
	};

	PhiveShapeHeader m_PhiveShapeHeader;
	HavokTagFileHeader m_HavokTagFileHeader;

	//Havok
	hknpMeshShape m_HavokMeshShape;

	//Phive
	std::vector<PhiveMaterial> m_Materials;
	std::vector<std::string> m_TagStringTable;
	std::vector<std::string> m_FieldStringTable;
	std::vector<uint8_t> m_TagHash;
	std::vector<PhiveTagNameMappingItem> m_NameMapping;
	std::vector<unsigned char> m_TagBody;

	void WritePackedInt(BinaryVectorWriter& Writer, int64_t Value);
	int64_t ReadPackedInt(BinaryVectorReader& Reader);
	uint32_t FindSection(BinaryVectorReader Reader, std::string Section);
	std::pair<uint8_t, uint32_t> ReadBitFieldFlagsSize(uint32_t Input);
	void ReadStringTable(BinaryVectorReader& Reader, std::vector<std::string>& Dest);
	void ReadHavokTagFileHeader(BinaryVectorReader& Reader);
	void ReadPhiveMaterials(BinaryVectorReader& Reader);
	void ReadTagHash(BinaryVectorReader& Reader);
	void ReadTagNameMapping(BinaryVectorReader& Reader);
	void ReadTagBody(BinaryVectorReader& Reader);

	void WriteStringTable(BinaryVectorWriter& Writer, std::vector<std::string>& Strings, std::string Magic);
	void WriteTagNameMapping(BinaryVectorWriter& Writer);
	void WriteTagBody(BinaryVectorWriter& Writer);
	void WriteTagHash(BinaryVectorWriter& Writer);
	std::pair<uint32_t, uint32_t> WriteMaterials(BinaryVectorWriter& Writer);
	uint32_t WriteItemData(BinaryVectorWriter& Writer);
};