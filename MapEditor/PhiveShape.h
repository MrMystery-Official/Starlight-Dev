#pragma once

#include "BinaryVectorReader.h"
#include "BinaryVectorWriter.h"
#include <map>
#include <string>
#include <vector>

class PhiveShape {
public:
    struct PhiveHeader {
        char Magic[6]; // Includes null terminator
        uint16_t Reserve1 = 0;
        uint16_t BOM = 0;
        uint8_t MajorVersion = 0;
        uint8_t MinorVersion = 0;
        uint32_t HktOffset = 0;
        uint32_t TableOffset0 = 0;
        uint32_t TableOffset1 = 0;
        uint32_t FileSize = 0;
        uint32_t HktSize = 0;
        uint32_t TableSize0 = 0;
        uint32_t TableSize1 = 0;
        uint32_t Reserve2 = 0;
        uint32_t Reserve3 = 0;
    };

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

    struct hkReadableWriteableObject {
        virtual void Read(BinaryVectorReader& Reader) = 0;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) = 0;
    };

    template <typename T>
    struct hkRelArrayView : public hkReadableWriteableObject {
        int32_t Offset;
        int32_t Size;

        std::vector<T> Elements;

        int32_t BaseOffset = 0;
        uint32_t LastElementOffset = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
        void Skip(BinaryVectorWriter& Writer);
    };

    template <typename T>
    struct hkRelArrayViewVertexBuffer : public hkReadableWriteableObject {
        int32_t Offset;
        int32_t Size;

        std::vector<T> Elements;

        int32_t BaseOffset = 0;
        uint32_t LastElementOffset = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
        void Skip(BinaryVectorWriter& Writer);
    };

    template <typename T>
    struct hkRelPtr : public hkReadableWriteableObject {
        int64_t Offset;
        T Data;

        int32_t BaseOffset = 0;
        uint32_t LastElementOffset = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
        void Skip(BinaryVectorWriter& Writer);
    };

    template <typename T>
    struct hkRelArray : public hkReadableWriteableObject {
        uint64_t Offset = 0;
        int32_t Size = 0;
        int32_t CapacityAndFlags = 0;

        std::vector<T> Elements;

        int32_t BaseOffset = 0;
        uint32_t LastElementOffset = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
        void Skip(BinaryVectorWriter& Writer);
    };

    struct hkBaseObject : public hkReadableWriteableObject {
        uint64_t VTFReserve = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hkReferencedObject : public hkReadableWriteableObject {
        hkBaseObject BaseObject;
        uint64_t SizeAndFlags = 0;
        uint64_t RefCount = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpShapePropertyBase : public hkReadableWriteableObject {
        hkReferencedObject ReferencedObject;
        uint16_t PropertyKey = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpShapePropertiesEntry : public hkReadableWriteableObject {
        hkRelPtr<hknpShapePropertyBase> Object;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpShapeProperties : public hkReadableWriteableObject {
        hkRelArray<hknpShapePropertiesEntry> Properties;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpShape : public hkReadableWriteableObject {
        hkReferencedObject ReferencedObject;
        hknpShapeTypeEnum Type;
        hknpCollisionDispatchTypeEnum DispatchType;
        hknpShapeFlagsEnum Flags;
        uint8_t NumShapeKeyBits = 0;
        uint8_t Reserve0[3] = { 0 }; // uint24_t
        float ConvexRadius = 0;
        uint32_t Reserve1 = 0;
        uint64_t UserData = 0;
        hknpShapeProperties Properties;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpCompositeShape : public hkReadableWriteableObject {
        hknpShape Shape;
        uint32_t ShapeTagCodecInfo = 0;
        uint32_t Reserve2 = 0;
        uint64_t Reserve3 = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpMeshShapeVertexConversionUtil : public hkReadableWriteableObject {
        float BitScale16[4] = { 0 };
        float BitScale16Inv[4] = { 0 };

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpMeshShapeShapeTagTableEntry : public hkReadableWriteableObject {
        uint32_t MeshPrimitiveKey;
        uint16_t ShapeTag;
        uint16_t Reserve0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hkcdFourAabb : public hkReadableWriteableObject {
        float MinX[4] = { 0 };
        float MaxX[4] = { 0 };
        float MinY[4] = { 0 };
        float MaxY[4] = { 0 };
        float MinZ[4] = { 0 };
        float MaxZ[4] = { 0 };

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hkcdSimdTreeNode : public hkReadableWriteableObject {
        hkcdFourAabb FourAabb;
        uint32_t Data[4] = { 0 };
        bool IsLeaf = false;
        bool IsActive = false;
        uint16_t Reserve0;
        uint32_t Reserve1;
        uint64_t Reserve2;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hkcdSimdTree : public hkReadableWriteableObject {
        hkRelArray<hkcdSimdTreeNode> Nodes;
        bool IsCompact = false;
        uint8_t Reserve0[3] = { 0 };
        uint32_t Reserve1 = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpTransposedFourAabbs8 : public hkReadableWriteableObject {
        uint32_t MinX = 0;
        uint32_t MaxX = 0;
        uint32_t MinY = 0;
        uint32_t MaxY = 0;
        uint32_t MinZ = 0;
        uint32_t MaxZ = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpAabb8TreeNode : public hkReadableWriteableObject {
        hknpTransposedFourAabbs8 TransposedFourAabbs8;
        uint8_t Data[4] = { 0 };

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpMeshShapeGeometrySectionPrimitive : public hkReadableWriteableObject {
        uint16_t IdA;
        uint16_t IdB;
        uint16_t IdC;
        uint16_t IdD;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpMeshShapeGeometrySectionVertex16_3 : public hkReadableWriteableObject {
        uint16_t X = 0;
        uint16_t Y = 0;
        uint16_t Z = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct EditorTreeNode {
        int32_t ChildA = -1;
        int32_t ChildB = -1;
    };

    struct hknpMeshShapeGeometrySection : public hkReadableWriteableObject {
        hkRelArrayView<hknpAabb8TreeNode> SectionBvh;
        hkRelArrayView<hknpMeshShapeGeometrySectionPrimitive> Primitives;
        hkRelArrayViewVertexBuffer<hknpMeshShapeGeometrySectionVertex16_3> VertexBuffer;
        hkRelArrayView<uint8_t> InteriorPrimitiveBitField;
        uint32_t SectionOffset[3];
        float BitScale8Inv[3];
        int16_t BitOffset[3];
        uint16_t Reserve0;

        // Editor data
        uint32_t SectionBvhOffset = 0;
        uint32_t PrimitivesOffset = 0;
        uint32_t VertexBufferOffset = 0;
        uint32_t InteriorPrimitiveBitFieldOffset = 0;

        std::vector<EditorTreeNode> BVHTree;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpMeshShapePrimitiveMapping : public hkReadableWriteableObject {
        hkReferencedObject ReferencedObject;
        hkRelArray<uint32_t> SectionStart;
        hkRelArray<uint32_t> BitString;
        uint32_t BitsPerEntry;
        uint32_t TriangleIndexBitMask;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    struct hknpMeshShape : public hkReadableWriteableObject {
        hknpCompositeShape CompositeShape;
        hknpMeshShapeVertexConversionUtil VertexConversionUtil;
        hkRelArrayView<hknpMeshShapeShapeTagTableEntry> ShapeTagTable;
        hkcdSimdTree TopLevelTree;
        hkRelArrayView<hknpMeshShapeGeometrySection> GeometrySections;
        hkRelPtr<hknpMeshShapePrimitiveMapping> PrimitiveMapping;

        // Editor data
        uint32_t ShapeTagTableOffset = 0;
        uint32_t TopLevelTreeOffset = 0;
        uint32_t GeometrySectionsOffset = 0;

        virtual void Read(BinaryVectorReader& Reader) override;
        virtual void Write(BinaryVectorWriter& Writer, int32_t Offset = -1) override;
    };

    std::vector<float> ToVertices();
    std::vector<unsigned int> ToIndices();

    std::vector<unsigned char> ToBinary(std::vector<float> Vertices, std::vector<unsigned int> Indices);
    void WriteToFile(std::string Path, std::vector<float> Vertices, std::vector<unsigned int> Indices);
    hknpMeshShape& GetMeshShape();

    PhiveShape(std::vector<unsigned char> Bytes);

private:
    struct Item {
        uint32_t Flags = 0;
        uint32_t DataOffset = 0;
        uint32_t Count = 0;

        int32_t TypeIndex = -1;
    };

    struct TNAItem {
        struct ParameterStruct {
            uint32_t Index = 0;
            uint32_t Value = 0;

            std::string Parameter = "";
            std::string ValueString = "MapEditor_BPHSH_NoVal";
        };

        uint32_t Index = 0;
        uint32_t TemplateCount = 0;
        std::string Tag = "MapEditor_BPHSH_NoVal";
        std::vector<ParameterStruct> Parameters;
    };

    PhiveHeader m_Header;
    std::vector<std::string> m_TagStringTable;
    std::vector<std::string> m_FieldStringTable;
    std::vector<Item> m_Items;
    std::vector<TNAItem> m_TNAItems;
    std::vector<unsigned char> m_TagBody; // Unknown what this data does, but it is the same for every collision
    std::vector<unsigned char> m_TagHashes; // Hashes are always the same
    std::vector<unsigned char> m_PhiveTable1;
    std::vector<unsigned char> m_PhiveTable2;

    hknpMeshShape MeshShape;

    void WritePackedInt(BinaryVectorWriter& Writer, int64_t Value);
    static int64_t ReadPackedInt(BinaryVectorReader& Reader);
    static void AlignWriter(BinaryVectorWriter& Writer, uint32_t Alignment, uint8_t Aligner = 0x00);

    static void ParseTreeNode(std::vector<uint32_t>& TreeNodeEntryIndices, hknpMeshShapeGeometrySection& Section, uint32_t Index, bool BlockChild = false);

    void ReadPhiveTables(BinaryVectorReader& Reader);
    void ReadTBDYSection(BinaryVectorReader Reader);
    void ReadTHSHSection(BinaryVectorReader Reader);
    void ReadTNASection(BinaryVectorReader Reader);
    void WriteTNASection(BinaryVectorWriter& Writer);
    uint32_t FindSection(BinaryVectorReader Reader, std::string Section);
    uint32_t BitFieldToSize(uint32_t Input);
    void ReadStringTable(BinaryVectorReader Reader, std::vector<std::string>* Dest);
    void WriteStringTable(BinaryVectorWriter& Writer, std::string Magic, std::vector<std::string>* Source);
    void ReadItemSection(BinaryVectorReader Reader);
    uint32_t WriteItemSection(BinaryVectorWriter& Writer);
};
