#pragma once

#include <file/game/phive/util/PhiveWrapper.h>
#include <file/game/phive/util/PhiveBinaryVectorReader.h>
#include <file/game/phive/util/PhiveBinaryVectorWriter.h>
#include <file/game/phive/util/NavMeshGenerator.h>
#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>
#include <vector>
#include <utility>

/*

THIS IS AN OUTDATED IMPLEMENTATION, USE file/game/phive/navmesh/PhiveNavMesh INSTEAD

*/

namespace application::file::game::phive
{
    class PhiveNavMesh
    {
    public:
        struct hkBaseObject : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);

            }
        };


        struct hkUlong : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            unsigned long long m_primitiveBase;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));

            }
        };


        struct hkReferencedObject : public hkBaseObject {
            hkUlong m_sizeAndFlags;
            hkUlong m_refCount;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                hkBaseObject::Read(Reader);
                m_sizeAndFlags.Read(Reader);
                m_refCount.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                hkBaseObject::Write(Writer);
                m_sizeAndFlags.Write(Writer);
                m_refCount.Write(Writer);

            }
        };


        struct hkInt32 : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            int m_primitiveBase;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));

            }
        };


        struct hkaiIndex : public hkInt32 {

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                hkInt32::Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                hkInt32::Write(Writer);

            }
        };


        struct hkInt16 : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            short m_primitiveBase;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));

            }
        };


        struct hkaiNavMesh__Face : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkaiIndex m_startEdgeIndex;
            hkaiIndex m_startUserEdgeIndex;
            hkInt16 m_numEdges;
            hkInt16 m_numUserEdges;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                m_startEdgeIndex.Read(Reader);
                m_startUserEdgeIndex.Read(Reader);
                m_numEdges.Read(Reader);
                m_numUserEdges.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                m_startEdgeIndex.Write(Writer);
                m_startUserEdgeIndex.Write(Writer);
                m_numEdges.Write(Writer);
                m_numUserEdges.Write(Writer);

            }
        };


        struct hkaiPackedKey_ : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            unsigned int m_primitiveBase;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));

            }
        };


        struct hkUint8 : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            unsigned char m_primitiveBase;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));

            }
        };


        struct hkFlags : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkUint8 m_storage;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(1);
                m_storage.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(1);
                m_storage.Write(Writer);

            }
        };


        struct hkHalf16 : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkInt16 m_value;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(2);
                m_value.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(2);
                m_value.Write(Writer);

            }
        };


        struct hkaiNavMesh__Edge : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkaiIndex m_a;
            hkaiIndex m_b;
            hkaiPackedKey_ m_oppositeEdge;
            hkaiPackedKey_ m_oppositeFace;
            hkFlags m_flags;
            hkUint8 m_paddingByte;
            hkHalf16 m_userEdgeCost;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                m_a.Read(Reader);
                m_b.Read(Reader);
                m_oppositeEdge.Read(Reader);
                m_oppositeFace.Read(Reader);
                m_flags.Read(Reader);
                m_paddingByte.Read(Reader);
                m_userEdgeCost.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                m_a.Write(Writer);
                m_b.Write(Writer);
                m_oppositeEdge.Write(Writer);
                m_oppositeFace.Write(Writer);
                m_flags.Write(Writer);
                m_paddingByte.Write(Writer);
                m_userEdgeCost.Write(Writer);

            }
        };


        struct hkVector4f : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            float m_subTypeArray[16 / sizeof(float)];

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(16);
                Reader.ReadStruct(&m_subTypeArray[0], sizeof(m_subTypeArray));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(16);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_subTypeArray[0]), sizeof(m_subTypeArray));

            }
        };


        struct hkVector4 : public hkVector4f {

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                hkVector4f::Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                hkVector4f::Write(Writer);

            }
        };


        struct hkUint16 : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            unsigned short m_primitiveBase;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));

            }
        };


        struct hkEnum : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkUint16 m_storage;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(2);
                m_storage.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(2);
                m_storage.Write(Writer);

            }
        };


        struct hkUint32 : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            unsigned int m_primitiveBase;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));

            }
        };


        struct hkaiFaceEdgeIndexPair : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkaiIndex m_faceIndex;
            hkaiIndex m_edgeIndex;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                m_faceIndex.Read(Reader);
                m_edgeIndex.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                m_faceIndex.Write(Writer);
                m_edgeIndex.Write(Writer);

            }
        };


        struct hkaiStreamingSet__NavMeshConnection : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkaiFaceEdgeIndexPair m_aFaceEdgeIndex;
            hkaiFaceEdgeIndexPair m_bFaceEdgeIndex;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                m_aFaceEdgeIndex.Read(Reader);
                m_bFaceEdgeIndex.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                m_aFaceEdgeIndex.Write(Writer);
                m_bFaceEdgeIndex.Write(Writer);

            }
        };


        struct hkHandle : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            unsigned int m_value;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                Reader.ReadStruct(&m_value, sizeof(m_value));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_value), sizeof(m_value));

            }
        };


        struct hkFpMath__FpInt : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            unsigned int m_storage;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                Reader.ReadStruct(&m_storage, sizeof(m_storage));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_storage), sizeof(m_storage));

            }
        };


        struct hkFpMath__FpIntVec3 : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkFpMath__FpInt m_x;
            hkFpMath__FpInt m_y;
            hkFpMath__FpInt m_z;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                Reader.ReadStruct(&m_x, sizeof(m_x));
                Reader.ReadStruct(&m_y, sizeof(m_y));
                Reader.ReadStruct(&m_z, sizeof(m_z));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_x), sizeof(m_x));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_y), sizeof(m_y));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_z), sizeof(m_z));

            }
        };


        struct hkaiClipper__BiplanarInnerTypes__FacePlane : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkFpMath__FpIntVec3 m_a;
            hkFpMath__FpIntVec3 m_ab;
            hkFpMath__FpIntVec3 m_ac;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                m_a.Read(Reader);
                m_ab.Read(Reader);
                m_ac.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                m_a.Write(Writer);
                m_ab.Write(Writer);
                m_ac.Write(Writer);

            }
        };


        struct hkaiClipper__BiplanarInnerTypes__Basis : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkaiClipper__BiplanarInnerTypes__FacePlane m_facePlane;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                m_facePlane.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                m_facePlane.Write(Writer);

            }
        };


        template <typename T, int N>
        struct hkFpMath__Detail__MultiWordUint : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            T m_words[N];

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                Reader.ReadStruct(&m_words[0], sizeof(m_words));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_words[0]), sizeof(m_words));

            }
        };


        struct hkFpMath__FpUint : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkFpMath__Detail__MultiWordUint<unsigned long long, 4> m_storage;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                m_storage.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                m_storage.Write(Writer);

            }
        };


        struct hkaiNavMeshGeneration__Snapshotting__GetGeometryVolumeClippingSegmentsBiplanarInput : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkHandle m_geometryVolumeKey;
            hkaiClipper__BiplanarInnerTypes__Basis m_basis;
            hkFpMath__FpUint m_bottomExtrusion;
            hkFpMath__FpUint m_topExtrusion;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                m_geometryVolumeKey.Read(Reader);
                m_basis.Read(Reader);
                Reader.ReadStruct(&m_bottomExtrusion, sizeof(m_bottomExtrusion));
                Reader.ReadStruct(&m_topExtrusion, sizeof(m_topExtrusion));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                m_geometryVolumeKey.Write(Writer);
                m_basis.Write(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_bottomExtrusion), sizeof(m_bottomExtrusion));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_topExtrusion), sizeof(m_topExtrusion));

            }
        };


        struct hkFpMath__FpIntVec2 : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkFpMath__FpInt m_x;
            hkFpMath__FpInt m_y;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                Reader.ReadStruct(&m_x, sizeof(m_x));
                Reader.ReadStruct(&m_y, sizeof(m_y));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_x), sizeof(m_x));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_y), sizeof(m_y));

            }
        };


        struct hkFpMath__FpFusedRationalVec2 : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkFpMath__FpIntVec2 m_numerator;
            hkFpMath__FpUint m_denominator;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                m_numerator.Read(Reader);
                Reader.ReadStruct(&m_denominator, sizeof(m_denominator));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                m_numerator.Write(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_denominator), sizeof(m_denominator));

            }
        };


        struct hkVector2f : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            float m_x;
            float m_y;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                Reader.ReadStruct(&m_x, sizeof(m_x));
                Reader.ReadStruct(&m_y, sizeof(m_y));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_x), sizeof(m_x));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_y), sizeof(m_y));

            }
        };

        /*

        template<typename tKEY, typename tVAL>
        struct hkPair : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            tKEY m_0;
            tVAL m_0x31;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                m_0.Read(Reader);
                m_0x31.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                m_0.Write(Writer);
                m_0x31.Write(Writer);

            }
        };


        struct hkHashMapDetail__Index : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            int m_hashMod;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                Reader.Seek(8, application::util::BinaryVectorReader::Position::Current);
                Reader.ReadStruct(&m_hashMod, sizeof(m_hashMod));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                Writer.WriteInteger(0, sizeof(uint64_t));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_hashMod), sizeof(m_hashMod));
            }
        };


        template<typename tKEY, typename tVALUE>
        struct hkHashMap : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {

            hkHashBase<hkPair<tKEY, tVALUE>> mParent;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                mParent.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                mParent.Write(Writer);
            }
        };

        struct hkaiDynamicUserEdgeSetInfoBase : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(1);
                unsigned int Jumpback = Reader.GetPosition();
                Reader.Seek(Jumpback + 1, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(1);
                unsigned int Jumpback = Writer.GetPosition();
                Writer.Seek(Jumpback + 1, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        template<typename tKEY>
        struct hkHashSet : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
        {
            hkHashBase<hkaiIndex> mParent;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkHashSet";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };
        */

        struct hkHashMapDetail__Index : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            uint64_t mEntries;
            int mHashMod;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                Reader.ReadStruct(&mEntries, sizeof(mEntries));
                Reader.ReadStruct(&mHashMod, sizeof(mHashMod));
                Reader.Seek(Jumpback + 16, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mEntries), sizeof(mEntries));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mHashMod), sizeof(mHashMod));
                Writer.Seek(Jumpback + 16, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        template<typename tKEY, typename tVAL>
        struct hkPair : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable{
            tKEY m0;
            tVAL m0x31;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();

                if constexpr (std::is_class<tKEY>::value)
                {
                    m0.Read(Reader);
                }
                else
                {
                    Reader.ReadStruct(&m0, sizeof(m0));
                }

                if constexpr (std::is_class<tVAL>::value)
                {
                    m0x31.Read(Reader);
                }
                else
                {
                    Reader.ReadStruct(&m0x31, sizeof(m0x31));
                }

                Reader.Seek(Jumpback + 48, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();

                if constexpr (std::is_class<tKEY>::value)
                {
                    m0.Write(Writer);
                }
                else
                {
                    Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m0), sizeof(m0));
                }

                if constexpr (std::is_class<tVAL>::value)
                {
                    m0x31.Write(Writer);
                }
                else
                {
                    Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m0x31), sizeof(m0x31));
                }

                Writer.Seek(Jumpback + 48, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        template<typename tITEM>
        struct hkHashBase : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            std::vector<tITEM> m_items;
            hkHashMapDetail__Index m_index;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                Reader.ReadHkArray<tITEM>(&m_items, sizeof(tITEM), "hkPair");
                m_index.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                Writer.QueryHkArrayWrite("13");
                m_index.Write(Writer);

                Writer.WriteHkArray<tITEM>(&m_items, sizeof(tITEM), "hkPair", "13");
            }
        };

        template<typename tKEY, typename tVALUE>
        struct hkHashMap : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {

            hkHashBase<hkPair<tKEY, tVALUE>> mParent;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                mParent.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                mParent.Write(Writer);
            }
        };

        template<typename tKEY>
        struct hkHashSet : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkHashBase<tKEY> mParent;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                mParent.Write(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        struct hkaiDynamicUserEdgeSetInfoBase : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(1);
                unsigned int Jumpback = Reader.GetPosition();
                Reader.Seek(Jumpback + 1, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(1);
                unsigned int Jumpback = Writer.GetPosition();
                Writer.Seek(Jumpback + 1, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        struct hkaiPackedClusterKey_ : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            unsigned int m_primitiveBase;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));

            }
        };

        struct hkaiDynamicUserEdgeSetInfoBase__ClusterGraphEdge : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {

            hkaiPackedClusterKey_ mStartClusterKey;
            hkaiPackedClusterKey_ mEndClusterKey;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mStartClusterKey.Read(Reader);
                mEndClusterKey.Read(Reader);
                Reader.Seek(Jumpback + 8, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                mStartClusterKey.Write(Writer);
                mEndClusterKey.Write(Writer);
                Writer.Seek(Jumpback + 8, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        template <typename T>
        struct hkRefPtr : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            T m_ptr;
            std::string m_className;

            hkRefPtr() : m_className("NO_hkRefPtr_NAME_SET") {}
            hkRefPtr(const std::string& ClassName) : m_className(ClassName) {}

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                Reader.ReadHkPointer<T>(&m_ptr, sizeof(T));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                Writer.QueryHkPointerWrite("16");

                Writer.WriteHkPointer<T>(&m_ptr, sizeof(T), m_className, "16");
            }
        };

        struct hkaiDynamicUserEdgeSetInfoBase__SectionIdxPair : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkaiIndex mASectionIdx;
            hkaiIndex mBSectionIdx;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mASectionIdx.Read(Reader);
                mBSectionIdx.Read(Reader);
                Reader.Seek(Jumpback + 8, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                mASectionIdx.Write(Writer);
                mBSectionIdx.Write(Writer);
                Writer.Seek(Jumpback + 8, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        struct hkaiDynamicUserEdgeSetInfo__UserEdgePair : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkaiPackedKey_ mAFaceKey;
            hkaiPackedKey_ mBFaceKey;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mAFaceKey.Read(Reader);
                mBFaceKey.Read(Reader);
                Reader.Seek(Jumpback + 8, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                mAFaceKey.Write(Writer);
                mBFaceKey.Write(Writer);
                Writer.Seek(Jumpback + 8, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        struct hkaiDynamicUserEdgeSetInfo__ExternalEdges : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkReferencedObject mParent;
            hkaiDynamicUserEdgeSetInfoBase__SectionIdxPair mSectionIndices;
            hkHashSet<hkaiDynamicUserEdgeSetInfo__UserEdgePair> mEdges;
            hkHashMap<hkaiDynamicUserEdgeSetInfoBase__ClusterGraphEdge, int> mClusterGraphEdges;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mSectionIndices.Read(Reader);
                mEdges.Read(Reader);
                mClusterGraphEdges.Read(Reader);
                Reader.Seek(Jumpback + 96, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                mParent.Write(Writer);
                mSectionIndices.Write(Writer);
                mEdges.Write(Writer);
                mClusterGraphEdges.Write(Writer);
                Writer.Seek(Jumpback + 96, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        struct hkaiDynamicUserEdgeSetInfo__Section : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkHashSet<hkaiIndex> mFaceIndices;
            hkHashMap<hkaiDynamicUserEdgeSetInfoBase__ClusterGraphEdge, int> mClusterGraphEdges;
            hkHashMap<hkaiIndex, hkRefPtr<hkaiDynamicUserEdgeSetInfo__ExternalEdges>> mExternalEdges;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mFaceIndices.Read(Reader);
                mClusterGraphEdges.Read(Reader);
                mExternalEdges.Read(Reader);
                Reader.Seek(Jumpback + 96, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                mFaceIndices.Write(Writer);
                mClusterGraphEdges.Write(Writer);
                mExternalEdges.Write(Writer);
                Writer.Seek(Jumpback + 96, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        struct hkaiDynamicUserEdgeSetInfo : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkaiDynamicUserEdgeSetInfoBase mParent;
            hkHandle mSetId;
            hkHashMap<hkaiIndex, hkaiDynamicUserEdgeSetInfo__Section> mSections;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mSetId.Read(Reader);
                mSections.Read(Reader);
                Reader.Seek(Jumpback + 40, application::file::game::phive::util::PhiveBinaryVectorReader::Position::Begin);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                mParent.Write(Writer);
                mSetId.Write(Writer);
                mSections.Write(Writer);
                Writer.Seek(Jumpback + 40, application::file::game::phive::util::PhiveBinaryVectorWriter::Position::Begin);
            }
        };

        struct hkaiStreamingSet__VolumeConnection : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkaiIndex m_aCellIndex;
            hkaiIndex m_bCellIndex;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                m_aCellIndex.Read(Reader);
                m_bCellIndex.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                m_aCellIndex.Write(Writer);
                m_bCellIndex.Write(Writer);

            }
        };


        struct hkAabb : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkVector4 m_min;
            hkVector4 m_max;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(16);
                m_min.Read(Reader);
                m_max.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(16);
                m_min.Write(Writer);
                m_max.Write(Writer);

            }
        };


        struct hkaiStreamingSet : public hkReferencedObject {
            hkUint32 m_aSectionUid;
            hkUint32 m_bSectionUid;
            std::vector<hkaiStreamingSet__NavMeshConnection> m_meshConnections;
            hkHashMap<hkHandle, hkaiDynamicUserEdgeSetInfo> m_graphConnections;
            std::vector<hkaiStreamingSet__VolumeConnection> m_volumeConnections;
            std::vector<hkAabb> m_aConnectionAabbs;
            std::vector<hkAabb> m_bConnectionAabbs;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                hkReferencedObject::Read(Reader);
                m_aSectionUid.Read(Reader);
                m_bSectionUid.Read(Reader);
                Reader.ReadHkArray<hkaiStreamingSet__NavMeshConnection>(&m_meshConnections, sizeof(hkaiStreamingSet__NavMeshConnection), "hkaiStreamingSet__NavMeshConnection");
                m_graphConnections.Read(Reader);
                Reader.ReadHkArray<hkaiStreamingSet__VolumeConnection>(&m_volumeConnections, sizeof(hkaiStreamingSet__VolumeConnection), "hkaiStreamingSet__VolumeConnection");
                Reader.ReadHkArray<hkAabb>(&m_aConnectionAabbs, sizeof(hkAabb), "hkAabb");
                Reader.ReadHkArray<hkAabb>(&m_bConnectionAabbs, sizeof(hkAabb), "hkAabb");
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                hkReferencedObject::Write(Writer);
                m_aSectionUid.Write(Writer);
                m_bSectionUid.Write(Writer);
                Writer.QueryHkArrayWrite("9");
                m_graphConnections.Write(Writer);
                Writer.QueryHkArrayWrite("10");
                Writer.QueryHkArrayWrite("11");
                Writer.QueryHkArrayWrite("12");

                Writer.WriteHkArray<hkaiStreamingSet__NavMeshConnection>(&m_meshConnections, sizeof(hkaiStreamingSet__NavMeshConnection), "hkaiStreamingSet__NavMeshConnection", "9");
                Writer.WriteHkArray<hkaiStreamingSet__VolumeConnection>(&m_volumeConnections, sizeof(hkaiStreamingSet__VolumeConnection), "hkaiStreamingSet__VolumeConnection", "10");
                Writer.WriteHkArray<hkAabb>(&m_aConnectionAabbs, sizeof(hkAabb), "hkAabb", "11");
                Writer.WriteHkArray<hkAabb>(&m_bConnectionAabbs, sizeof(hkAabb), "hkAabb", "12");
            }
        };


        struct hkaiAnnotatedStreamingSet : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkEnum m_side;
            hkRefPtr<hkaiStreamingSet> m_streamingSet = hkRefPtr<hkaiStreamingSet>("hkaiStreamingSet");

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                m_side.Read(Reader);
                m_streamingSet.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                m_side.Write(Writer);
                Writer.QueryHkPointerWrite("8");

                Writer.WriteHkPointer<hkaiStreamingSet>(&m_streamingSet.m_ptr, sizeof(hkaiStreamingSet), "hkaiStreamingSet", "8");
            }
        };


        struct hkReal : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            float m_primitiveBase;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));

            }
        };


        struct hkaiUpVector : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkVector4 m_up;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(16);
                m_up.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(16);
                m_up.Write(Writer);

            }
        };


        struct hkUint64 : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            unsigned long long m_primitiveBase;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));

            }
        };


        struct hkaiNavMeshFaceIterator : public hkReferencedObject {

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                hkReferencedObject::Read(Reader);
                Reader.Seek(8, application::util::BinaryVectorReader::Position::Current);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                hkReferencedObject::Write(Writer);
                Writer.Seek(8, application::util::BinaryVectorWriter::Position::Current);
            }
        };


        struct hkcdStaticTree__AabbTreeBase : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(1);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(1);

            }
        };


        struct hkcdCompressedAabbCodecs__AabbCodecBase : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(1);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(1);

            }
        };


        template <typename T, int N>
        struct hkcdCompressedAabbCodecs__CompressedAabbCodec : public hkcdCompressedAabbCodecs__AabbCodecBase {
            T m_xyz[N];

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(1);
                hkcdCompressedAabbCodecs__AabbCodecBase::Read(Reader);
                Reader.ReadStruct(&m_xyz[0], sizeof(m_xyz));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(1);
                hkcdCompressedAabbCodecs__AabbCodecBase::Write(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_xyz[0]), sizeof(m_xyz));

            }
        };


        struct hkcdCompressedAabbCodecs__Aabb4BytesCodec : public hkcdCompressedAabbCodecs__CompressedAabbCodec<uint8_t, 3> {
            hkUint8 m_data;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(1);
                hkcdCompressedAabbCodecs__CompressedAabbCodec::Read(Reader);
                m_data.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(1);
                hkcdCompressedAabbCodecs__CompressedAabbCodec::Write(Writer);
                m_data.Write(Writer);

            }
        };

        struct hkcdCompressedAabbCodecs__Aabb6BytesCodec : public hkcdCompressedAabbCodecs__CompressedAabbCodec<uint8_t, 3> {
            hkUint8 m_hiData;
            hkUint16 m_loData;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(1);
                hkcdCompressedAabbCodecs__CompressedAabbCodec::Read(Reader);
                m_hiData.Read(Reader);
                m_loData.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(1);
                hkcdCompressedAabbCodecs__CompressedAabbCodec::Write(Writer);
                m_hiData.Write(Writer);
                m_loData.Write(Writer);

            }

            glm::vec3 DecompressMin(glm::vec3 ParentMin, glm::vec3 ParentMax)
            {
                float x =
                    (m_xyz[0] >> 4) * (float)(m_xyz[0] >> 4) * (1.0f / 226.0f) * (ParentMax.x - ParentMin.x) +
                    ParentMin.x;
                float y =
                    (m_xyz[1] >> 4) * (float)(m_xyz[1] >> 4) * (1.0f / 226.0f) * (ParentMax.y - ParentMin.y) +
                    ParentMin.y;
                float z =
                    (m_xyz[2] >> 4) * (float)(m_xyz[2] >> 4) * (1.0f / 226.0f) * (ParentMax.z - ParentMin.z) +
                    ParentMin.z;
                return glm::vec3(x, y, z);
            }

            glm::vec3 DecompressMax(glm::vec3 parentMin, glm::vec3 parentMax)
            {
                float x = -((m_xyz[0] & 0x0F) * (float)(m_xyz[0] & 0x0F)) * (1.0f / 226.0f) *
                    (parentMax.x - parentMin.x) + parentMax.x;
                float y = -((m_xyz[1] & 0x0F) * (float)(m_xyz[1] & 0x0F)) * (1.0f / 226.0f) *
                    (parentMax.y - parentMin.y) + parentMax.y;
                float z = -((m_xyz[2] & 0x0F) * (float)(m_xyz[2] & 0x0F)) * (1.0f / 226.0f) *
                    (parentMax.z - parentMin.z) + parentMax.z;
                return glm::vec3(x, y, z);
            }
        };


        struct hkcdStaticTree__AabbTree : public hkcdStaticTree__AabbTreeBase {
            std::vector<hkcdCompressedAabbCodecs__Aabb6BytesCodec> m_nodes;
            hkAabb m_domain;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(16);
                hkcdStaticTree__AabbTreeBase::Read(Reader);
                uint64_t Offset = Reader.ReadUInt64();
                if (Offset != 48)
                    Reader.Seek(24, application::util::BinaryVectorReader::Position::Current);
                else
                    Reader.Seek(-8, application::util::BinaryVectorReader::Position::Current);
                Reader.ReadHkArray<hkcdCompressedAabbCodecs__Aabb6BytesCodec>(&m_nodes, sizeof(hkcdCompressedAabbCodecs__Aabb6BytesCodec), "hkcdCompressedAabbCodecs__Aabb6BytesCodec");
                m_domain.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(16);
                hkcdStaticTree__AabbTreeBase::Write(Writer);
                uint32_t NodeArrayPosition = Writer.GetPosition() + 48;
                if (!m_nodes.empty())
                {
                    Writer.WriteInteger(0x30, sizeof(uint64_t));
                    Writer.WriteInteger(m_nodes.size(), sizeof(uint64_t));
                    Writer.mPhiveWrapper.mItems.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperItem{
                        .mTypeIndex = Writer.mPhiveWrapper.GetTypeNameIndex("hkcdCompressedAabbCodecs__Aabb6BytesCodec"),
                        .mFlags = (uint16_t)Writer.mPhiveWrapper.GetTypeNameFlag("hkcdCompressedAabbCodecs__Aabb6BytesCodec"),
                        .mDataOffset = (uint32_t)(Writer.GetPosition() + 32),
                        .mCount = (uint32_t)m_nodes.size()
                        });
                }
                else
                {
                    Writer.Seek(16, application::util::BinaryVectorWriter::Position::Current);
                }
                m_domain.Write(Writer);

                if (!m_nodes.empty())
                {
                    Writer.Seek(NodeArrayPosition, application::util::BinaryVectorWriter::Position::Begin);
                    for (hkcdCompressedAabbCodecs__Aabb6BytesCodec& Codec : m_nodes)
                    {
                        Codec.Write(Writer);
                    }
                }
                //Writer.WriteHkArray<hkcdCompressedAabbCodecs__Aabb6BytesCodec>(&m_nodes, sizeof(hkcdCompressedAabbCodecs__Aabb6BytesCodec), "hkcdCompressedAabbCodecs__Aabb6BytesCodec", "19");
            }
        };


        struct hkcdStaticTree__Aabb6BytesTree : public hkcdStaticTree__AabbTree {

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                hkcdStaticTree__AabbTree::Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                hkcdStaticTree__AabbTree::Write(Writer);

            }
        };


        struct hkcdStaticAabbTree__Impl : public hkReferencedObject {
            hkcdStaticTree__Aabb6BytesTree m_tree;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(16);
                hkReferencedObject::Read(Reader);
                Reader.Seek(16, application::util::BinaryVectorReader::Position::Current);
                m_tree.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(16);
                hkReferencedObject::Write(Writer);
                Writer.Seek(16, application::util::BinaryVectorWriter::Position::Current);
                m_tree.Write(Writer);

            }
        };


        struct hkcdStaticAabbTree : public hkReferencedObject {
            hkRefPtr<hkcdStaticAabbTree__Impl> m_treePtr = hkRefPtr<hkcdStaticAabbTree__Impl>("hkcdStaticAabbTree__Impl");

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                hkReferencedObject::Read(Reader);
                Reader.Seek(8, application::util::BinaryVectorReader::Position::Current);
                m_treePtr.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                hkReferencedObject::Write(Writer);
                Writer.Seek(8, application::util::BinaryVectorWriter::Position::Current);
                Writer.QueryHkPointerWrite("18");

                Writer.WriteHkPointer<hkcdStaticAabbTree__Impl>(&m_treePtr.m_ptr, sizeof(hkcdStaticAabbTree__Impl), "hkcdStaticAabbTree__Impl", "18");
            }
        };


        struct hkaiNavMeshStaticTreeFaceIterator : public hkaiNavMeshFaceIterator {
            hkRefPtr<hkcdStaticAabbTree> m_tree = hkRefPtr<hkcdStaticAabbTree>("hkcdStaticAabbTree");

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                hkaiNavMeshFaceIterator::Read(Reader);
                m_tree.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                hkaiNavMeshFaceIterator::Write(Writer);
                Writer.QueryHkPointerWrite("17");

                Writer.WriteHkPointer<hkcdStaticAabbTree>(&m_tree.m_ptr, sizeof(hkcdStaticAabbTree), "hkcdStaticAabbTree", "17");
            }
        };


        struct hkaiNavMeshClearanceCache__McpDataInteger : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkUint8 m_interpolant;
            hkUint8 m_clearance;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(1);
                m_interpolant.Read(Reader);
                m_clearance.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(1);
                m_interpolant.Write(Writer);
                m_clearance.Write(Writer);

            }
        };


        struct hkaiNavMeshClearanceCache__EdgeDataInteger : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkaiNavMeshClearanceCache__McpDataInteger m_mcp;
            hkUint8 m_startVertexClearance;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(1);
                m_mcp.Read(Reader);
                m_startVertexClearance.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(1);
                m_mcp.Write(Writer);
                m_startVertexClearance.Write(Writer);

            }
        };


        struct hkaiNavMeshClearanceCache : public hkReferencedObject {
            hkReal m_clearanceCeiling;
            hkReal m_clearanceBias;
            hkReal m_clearanceIntToRealMultiplier;
            hkReal m_clearanceRealToIntMultiplier;
            std::vector<unsigned int> m_faceOffsets;
            std::vector<hkUint8> m_edgePairClearances;
            int m_unusedEdgePairElements;
            std::vector<hkaiNavMeshClearanceCache__EdgeDataInteger> m_edgeData;
            int m_uncalculatedFacesLowerBound;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                hkReferencedObject::Read(Reader);
                m_clearanceCeiling.Read(Reader);
                m_clearanceBias.Read(Reader);
                m_clearanceIntToRealMultiplier.Read(Reader);
                m_clearanceRealToIntMultiplier.Read(Reader);
                Reader.ReadHkArray<unsigned int>(&m_faceOffsets, sizeof(unsigned int), "unsigned int");
                Reader.ReadHkArray<hkUint8>(&m_edgePairClearances, sizeof(hkUint8), "hkUint8");
                Reader.ReadStruct(&m_unusedEdgePairElements, sizeof(m_unusedEdgePairElements));
                Reader.ReadHkArray<hkaiNavMeshClearanceCache__EdgeDataInteger>(&m_edgeData, sizeof(hkaiNavMeshClearanceCache__EdgeDataInteger), "hkaiNavMeshClearanceCache__EdgeDataInteger");
                Reader.ReadStruct(&m_uncalculatedFacesLowerBound, sizeof(m_uncalculatedFacesLowerBound));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                hkReferencedObject::Write(Writer);
                m_clearanceCeiling.Write(Writer);
                m_clearanceBias.Write(Writer);
                m_clearanceIntToRealMultiplier.Write(Writer);
                m_clearanceRealToIntMultiplier.Write(Writer);
                Writer.QueryHkArrayWrite("22");
                Writer.QueryHkArrayWrite("23");
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_unusedEdgePairElements), sizeof(m_unusedEdgePairElements));
                Writer.QueryHkArrayWrite("24");
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_uncalculatedFacesLowerBound), sizeof(m_uncalculatedFacesLowerBound));

                Writer.WriteHkArray<unsigned int>(&m_faceOffsets, sizeof(unsigned int), "unsigned int", "22");
                Writer.WriteHkArray<hkUint8>(&m_edgePairClearances, sizeof(hkUint8), "hkUint8", "23");
                Writer.WriteHkArray<hkaiNavMeshClearanceCache__EdgeDataInteger>(&m_edgeData, sizeof(hkaiNavMeshClearanceCache__EdgeDataInteger), "hkaiNavMeshClearanceCache__EdgeDataInteger", "24");
            }
        };


        struct hkaiNavMeshClearanceCacheSeeding__CacheData : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkUint32 m_seedingDataIdentifier;
            hkRefPtr<hkaiNavMeshClearanceCache> m_initialCache = hkRefPtr<hkaiNavMeshClearanceCache>("hkaiNavMeshClearanceCache");

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                m_seedingDataIdentifier.Read(Reader);
                m_initialCache.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                m_seedingDataIdentifier.Write(Writer);
                Writer.QueryHkPointerWrite("21");

                Writer.WriteHkPointer<hkaiNavMeshClearanceCache>(&m_initialCache.m_ptr, sizeof(hkaiNavMeshClearanceCache), "hkaiNavMeshClearanceCache", "21");
            }
        };


        struct hkaiNavMeshClearanceCacheSeeding__CacheDataSet : public hkReferencedObject {
            std::vector<hkaiNavMeshClearanceCacheSeeding__CacheData> m_cacheDatas;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                hkReferencedObject::Read(Reader);
                Reader.ReadHkArray<hkaiNavMeshClearanceCacheSeeding__CacheData>(&m_cacheDatas, sizeof(hkaiNavMeshClearanceCacheSeeding__CacheData), "hkaiNavMeshClearanceCacheSeeding__CacheData");
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                hkReferencedObject::Write(Writer);
                Writer.QueryHkArrayWrite("20");

                Writer.WriteHkArray<hkaiNavMeshClearanceCacheSeeding__CacheData>(&m_cacheDatas, sizeof(hkaiNavMeshClearanceCacheSeeding__CacheData), "hkaiNavMeshClearanceCacheSeeding__CacheData", "20");
            }
        };


        struct hkaiNavMesh : public hkReferencedObject {
            std::vector<hkaiNavMesh__Face> m_faces;
            std::vector<hkaiNavMesh__Edge> m_edges;
            std::vector<hkVector4> m_vertices;
            std::vector<hkaiAnnotatedStreamingSet> m_streamingSets;
            std::vector<hkInt32> m_faceData;
            std::vector<hkInt32> m_edgeData;
            int m_faceDataStriding;
            int m_edgeDataStriding;
            unsigned char m_flags;
            hkAabb m_aabb;
            hkReal m_erosionRadius;
            hkaiUpVector m_up;
            hkUint64 m_userData;
            hkRefPtr<hkaiNavMeshStaticTreeFaceIterator> m_cachedFaceIterator = hkRefPtr<hkaiNavMeshStaticTreeFaceIterator>("hkaiNavMeshStaticTreeFaceIterator");
            hkRefPtr<hkaiNavMeshClearanceCacheSeeding__CacheDataSet> m_clearanceCacheSeedingDataSet = hkRefPtr<hkaiNavMeshClearanceCacheSeeding__CacheDataSet>("hkaiNavMeshClearanceCacheSeeding__CacheDataSet");

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(16);
                hkReferencedObject::Read(Reader);
                Reader.ReadHkArray<hkaiNavMesh__Face>(&m_faces, sizeof(hkaiNavMesh__Face), "hkaiNavMesh__Face");
                Reader.ReadHkArray<hkaiNavMesh__Edge>(&m_edges, sizeof(hkaiNavMesh__Edge), "hkaiNavMesh__Edge");
                Reader.ReadHkArray<hkVector4>(&m_vertices, sizeof(hkVector4), "hkVector4");
                Reader.ReadHkArray<hkaiAnnotatedStreamingSet>(&m_streamingSets, sizeof(hkaiAnnotatedStreamingSet), "hkaiAnnotatedStreamingSet");
                Reader.ReadHkArray<hkInt32>(&m_faceData, sizeof(hkInt32), "hkInt32");
                Reader.ReadHkArray<hkInt32>(&m_edgeData, sizeof(hkInt32), "hkInt32");
                Reader.Seek(8, application::util::BinaryVectorReader::Position::Current);
                Reader.ReadStruct(&m_faceDataStriding, sizeof(m_faceDataStriding));
                Reader.ReadStruct(&m_edgeDataStriding, sizeof(m_edgeDataStriding));
                Reader.ReadStruct(&m_flags, sizeof(m_flags));
                Reader.Seek(15, application::util::BinaryVectorReader::Position::Current); //Padding
                m_aabb.Read(Reader);
                m_erosionRadius.Read(Reader);
                m_up.Read(Reader);
                m_userData.Read(Reader);
                m_cachedFaceIterator.Read(Reader);
                m_clearanceCacheSeedingDataSet.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(16);
                hkReferencedObject::Write(Writer);
                Writer.QueryHkArrayWrite("0");
                Writer.QueryHkArrayWrite("1");
                Writer.QueryHkArrayWrite("2");
                Writer.QueryHkArrayWrite("3");
                Writer.QueryHkArrayWrite("4");
                Writer.QueryHkArrayWrite("5");
                Writer.Seek(8, application::util::BinaryVectorWriter::Position::Current);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_faceDataStriding), sizeof(m_faceDataStriding));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_edgeDataStriding), sizeof(m_edgeDataStriding));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_flags), sizeof(m_flags));
                Writer.Seek(15, application::util::BinaryVectorWriter::Position::Current);
                m_aabb.Write(Writer);
                m_erosionRadius.Write(Writer);
                m_up.Write(Writer);
                m_userData.Write(Writer);
                Writer.QueryHkPointerWrite("6");
                Writer.QueryHkPointerWrite("7");

                Writer.Align(16);
                Writer.WriteHkArray<hkaiNavMesh__Face>(&m_faces, sizeof(hkaiNavMesh__Face), "hkaiNavMesh__Face", "0");

                Writer.Align(16);
                Writer.WriteHkArray<hkaiNavMesh__Edge>(&m_edges, sizeof(hkaiNavMesh__Edge), "hkaiNavMesh__Edge", "1");

                Writer.Align(16);
                Writer.WriteHkArray<hkVector4>(&m_vertices, sizeof(hkVector4), "hkVector4", "2");

                Writer.WriteHkArray<hkaiAnnotatedStreamingSet>(&m_streamingSets, sizeof(hkaiAnnotatedStreamingSet), "hkaiAnnotatedStreamingSet", "3");

                Writer.WriteHkArray<hkInt32>(&m_faceData, sizeof(hkInt32), "hkInt32", "4");
                Writer.WriteHkArray<hkInt32>(&m_edgeData, sizeof(hkInt32), "hkInt32", "5");
            }

            void WriteIterators(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) {
                Writer.WriteHkPointer<hkaiNavMeshStaticTreeFaceIterator>(&m_cachedFaceIterator.m_ptr, sizeof(hkaiNavMeshStaticTreeFaceIterator), "hkaiNavMeshStaticTreeFaceIterator", "6");

                if (!m_clearanceCacheSeedingDataSet.m_ptr.m_cacheDatas.empty())
                    Writer.WriteHkPointer<hkaiNavMeshClearanceCacheSeeding__CacheDataSet>(&m_clearanceCacheSeedingDataSet.m_ptr, sizeof(hkaiNavMeshClearanceCacheSeeding__CacheDataSet), "hkaiNavMeshClearanceCacheSeeding__CacheDataSet", "7");
            }
        };


        struct hkaiClusterGraph__Node : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            int m_startEdgeIndex;
            int m_numEdges;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                Reader.ReadStruct(&m_startEdgeIndex, sizeof(m_startEdgeIndex));
                Reader.ReadStruct(&m_numEdges, sizeof(m_numEdges));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_startEdgeIndex), sizeof(m_startEdgeIndex));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_numEdges), sizeof(m_numEdges));

            }
        };


        struct hkaiClusterGraph__Edge : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkHalf16 m_cost;
            hkFlags m_flags;
            hkaiPackedClusterKey_ m_source;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(4);
                m_cost.Read(Reader);
                m_flags.Read(Reader);
                Reader.Seek(1, application::util::BinaryVectorReader::Position::Current);
                m_source.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(4);
                m_cost.Write(Writer);
                m_flags.Write(Writer);
                Writer.Seek(1, application::util::BinaryVectorWriter::Position::Current);
                m_source.Write(Writer);

            }
        };


        struct hkaiClusterGraph : public hkReferencedObject {
            std::vector<hkVector4> m_positions;
            std::vector<hkaiClusterGraph__Node> m_nodes;
            std::vector<hkaiClusterGraph__Edge> m_edges;
            std::vector<hkUint32> m_nodeData;
            std::vector<hkUint32> m_edgeData;
            std::vector<hkaiIndex> m_featureToNodeIndex;
            int m_nodeDataStriding;
            int m_edgeDataStriding;
            std::vector<hkaiAnnotatedStreamingSet> m_streamingSets;
            hkUint64 m_userData;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                hkReferencedObject::Read(Reader);
                Reader.ReadHkArray<hkVector4>(&m_positions, sizeof(hkVector4), "hkVector4");
                Reader.ReadHkArray<hkaiClusterGraph__Node>(&m_nodes, sizeof(hkaiClusterGraph__Node), "hkaiClusterGraph__Node");
                Reader.ReadHkArray<hkaiClusterGraph__Edge>(&m_edges, sizeof(hkaiClusterGraph__Edge), "hkaiClusterGraph__Edge");
                Reader.ReadHkArray<hkUint32>(&m_nodeData, sizeof(hkUint32), "hkUint32");
                Reader.ReadHkArray<hkUint32>(&m_edgeData, sizeof(hkUint32), "hkUint32");
                Reader.ReadHkArray<hkaiIndex>(&m_featureToNodeIndex, sizeof(hkaiIndex), "hkaiIndex");
                Reader.ReadStruct(&m_nodeDataStriding, sizeof(m_nodeDataStriding));
                Reader.ReadStruct(&m_edgeDataStriding, sizeof(m_edgeDataStriding));
                Reader.ReadHkArray<hkaiAnnotatedStreamingSet>(&m_streamingSets, sizeof(hkaiAnnotatedStreamingSet), "hkaiAnnotatedStreamingSet");
                m_userData.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                hkReferencedObject::Write(Writer);
                //Writer.Seek(8, BinaryVectorWriter::Position::Current);
                Writer.QueryHkArrayWrite("25");
                Writer.QueryHkArrayWrite("26");
                Writer.QueryHkArrayWrite("27");
                Writer.QueryHkArrayWrite("28");
                Writer.QueryHkArrayWrite("29");
                Writer.QueryHkArrayWrite("30");
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_nodeDataStriding), sizeof(m_nodeDataStriding));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_edgeDataStriding), sizeof(m_edgeDataStriding));
                Writer.Align(16);
                Writer.QueryHkArrayWrite("31");
                m_userData.Write(Writer);

                Writer.Align(16);
                Writer.WriteHkArray<hkVector4>(&m_positions, sizeof(hkVector4), "hkVector4", "25");
                Writer.Align(16);
                Writer.WriteHkArray<hkaiClusterGraph__Node>(&m_nodes, sizeof(hkaiClusterGraph__Node), "hkaiClusterGraph__Node", "26");
                Writer.Align(16);
                Writer.WriteHkArray<hkaiClusterGraph__Edge>(&m_edges, sizeof(hkaiClusterGraph__Edge), "hkaiClusterGraph__Edge", "27");
                if (m_edges.empty())
                {
                    /*
                    Writer.Seek(Writer.mArrayOffsets["27"], BinaryVectorWriter::Position::Begin);

                    Writer.WriteInteger(0, sizeof(uint64_t));
                    Writer.WriteInteger(Writer.mItemPointerIndex++, sizeof(uint32_t));
                    Writer.WriteInteger(0, sizeof(uint32_t));
                    */

                    /*
                    Writer.mPhiveWrapper.mItems.push_back(PhiveWrapper::PhiveWrapperItem{
                        .mTypeIndex = Writer.mPhiveWrapper.GetTypeNameIndex("hkaiClusterGraph__Edge"),
                        .mFlags = (uint16_t)Writer.mPhiveWrapper.GetTypeNameFlag("hkaiClusterGraph__Edge"),
                        .mDataOffset = 0,
                        .mCount = (uint32_t)0
                        });
                        */
                }
                Writer.WriteHkArray<hkUint32>(&m_nodeData, sizeof(hkUint32), "hkUint32", "28");
                Writer.WriteHkArray<hkUint32>(&m_edgeData, sizeof(hkUint32), "hkUint32", "29");
                Writer.Align(16);
                Writer.WriteHkArray<hkaiIndex>(&m_featureToNodeIndex, sizeof(hkaiIndex), "hkaiIndex", "30");
                if (m_featureToNodeIndex.empty())
                {
                    /*
                    Writer.Seek(Writer.mArrayOffsets["30"], BinaryVectorWriter::Position::Begin);

                    Writer.WriteInteger(0, sizeof(uint64_t));
                    Writer.WriteInteger(Writer.mItemPointerIndex++, sizeof(uint32_t));
                    Writer.WriteInteger(0, sizeof(uint32_t));
                    */

                    /*
                    Writer.mPhiveWrapper.mItems.push_back(PhiveWrapper::PhiveWrapperItem{
                        .mTypeIndex = Writer.mPhiveWrapper.GetTypeNameIndex("hkaiIndex"),
                        .mFlags = (uint16_t)Writer.mPhiveWrapper.GetTypeNameFlag("hkaiIndex"),
                        .mDataOffset = 0,
                        .mCount = (uint32_t)0
                        });
                        */
                }
                Writer.WriteHkArray<hkaiAnnotatedStreamingSet>(&m_streamingSets, sizeof(hkaiAnnotatedStreamingSet), "hkaiAnnotatedStreamingSet", "31");
            }
        };

        struct hkStringPtr : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            std::vector<char> m_stringAndFlag;

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                Reader.ReadHkArrayFromItemPointer<char>(&m_stringAndFlag, sizeof(char), "char");
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_stringAndFlag.data()), m_stringAndFlag.size());

            }

            std::string ToString() {
                return std::string(m_stringAndFlag.begin(), m_stringAndFlag.end());
            }
        };


        template <typename T>
        struct hkRefVariant : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            T m_ptr;
            std::string m_className;

            hkRefVariant(const std::string& ClassName) : m_className(ClassName) {}

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                Reader.ReadHkPointer<T>(&m_ptr, sizeof(T));
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                Writer.QueryHkPointerWrite("34");

                Writer.WriteHkPointer<T>(&m_ptr, sizeof(T), m_className, "34");
            }
        };

        template <typename T>
        struct hkRootLevelContainer__NamedVariant : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            std::string m_localClassName;

            hkRootLevelContainer__NamedVariant(const std::string& ClassName) : m_localClassName(ClassName) {}
            
            hkStringPtr m_name;
            hkStringPtr m_className;
            hkRefVariant<T> m_variant = hkRefVariant<T>(m_localClassName);

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);
                m_name.Read(Reader);
                m_className.Read(Reader);
                m_variant.Read(Reader);
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                Writer.QueryHkArrayToItemPointerWrite("33_" + m_name.ToString());
                Writer.QueryHkArrayToItemPointerWrite("34_" + m_name.ToString());
                Writer.QueryHkPointerWrite("35_" + m_name.ToString());
            }

            void WriteName(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) {
                Writer.WriteHkArrayToItemPointer<char>(&m_name.m_stringAndFlag, sizeof(char), "char", "33_" + m_name.ToString());
            }

            void WriteClassName(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) {
                Writer.WriteHkArrayToItemPointer<char>(&m_className.m_stringAndFlag, sizeof(char), "char", "34_" + m_name.ToString());
            }

            void WriteData(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) {
                Writer.WriteHkPointer<T>(&m_variant.m_ptr, sizeof(T), m_localClassName, "35_" + m_name.ToString());
            }

            void WriteLastNavMeshData(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) {
                if constexpr (std::is_base_of_v<hkaiNavMesh, T>) {
                    hkaiNavMesh* NavMeshPtr = (hkaiNavMesh*)&m_variant.m_ptr;
                    NavMeshPtr->WriteIterators(Writer);
                }
            }
        };

        //Manual
        struct hkNavMeshRootLevelContainer : public application::file::game::phive::util::PhiveBinaryVectorReader::hkReadable, public application::file::game::phive::util::PhiveBinaryVectorWriter::hkWriteable {
            hkRootLevelContainer__NamedVariant<hkaiNavMesh> m_namedVariantNavMesh = hkRootLevelContainer__NamedVariant<hkaiNavMesh>("hkaiNavMesh");
            hkRootLevelContainer__NamedVariant<hkaiClusterGraph> m_namedVariantClusterGraph = hkRootLevelContainer__NamedVariant<hkaiClusterGraph>("hkaiClusterGraph");

            virtual void Read(application::file::game::phive::util::PhiveBinaryVectorReader& Reader) override {
                Reader.Align(8);

                Reader.ReadHkArrayFromItemPointerAtIndex<hkRootLevelContainer__NamedVariant<hkaiNavMesh>, 0>(&m_namedVariantNavMesh, 24, "hkRootLevelContainer__NamedVariant");
                Reader.Seek(-8, application::util::BinaryVectorReader::Position::Current);
                Reader.ReadHkArrayFromItemPointerAtIndex<hkRootLevelContainer__NamedVariant<hkaiClusterGraph>, 1>(&m_namedVariantClusterGraph, 24, "hkRootLevelContainer__NamedVariant");
            }

            virtual void Write(application::file::game::phive::util::PhiveBinaryVectorWriter& Writer) override {
                Writer.Align(8);
                Writer.WriteInteger(Writer.mItemPointerIndex++, sizeof(uint64_t));
                Writer.WriteInteger(0, sizeof(uint64_t));

                Writer.mPhiveWrapper.mItems.push_back(application::file::game::phive::util::PhiveWrapper::PhiveWrapperItem{
                    .mTypeIndex = Writer.mPhiveWrapper.GetTypeNameIndex("hkRootLevelContainer__NamedVariant"),
                    .mFlags = (uint16_t)Writer.mPhiveWrapper.GetTypeNameFlag("hkRootLevelContainer__NamedVariant"),
                    .mDataOffset = 16,
                    .mCount = (uint32_t)2
                    });

                m_namedVariantNavMesh.Write(Writer);
                m_namedVariantClusterGraph.Write(Writer);

                m_namedVariantNavMesh.WriteName(Writer);
                m_namedVariantClusterGraph.WriteName(Writer);

                Writer.WriteInteger(0, sizeof(uint8_t));

                m_namedVariantNavMesh.WriteClassName(Writer);
                m_namedVariantClusterGraph.WriteClassName(Writer);

                m_namedVariantNavMesh.WriteData(Writer);
                m_namedVariantClusterGraph.WriteData(Writer);
                m_namedVariantNavMesh.WriteLastNavMeshData(Writer);
            }
        };

        application::file::game::phive::util::PhiveWrapper mPhiveWrapper;
        hkNavMeshRootLevelContainer mContainer;

        std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> ToVerticesIndices();
        void SetNavMeshModel(const std::vector<glm::vec3>& Vertices, const std::vector<uint32_t>& Indices, const application::file::game::phive::util::NavMeshGenerator::PhiveNavMeshGeneratorConfig& Config);
        std::vector<unsigned char> ToBinary();
        void WriteToFile(const std::string& Path);

        void Initialize(std::vector<unsigned char> Bytes);

        PhiveNavMesh(std::vector<unsigned char> Bytes);
        PhiveNavMesh() = default;
    };
}