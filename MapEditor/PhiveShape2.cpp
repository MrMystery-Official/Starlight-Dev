#include "PhiveShape2.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include "Logger.h"
#include "UIMapView.h"
#include "BVNode.h"

std::pair<uint8_t, uint32_t> PhiveShape2::ReadBitFieldFlagsSize(uint32_t Input)
{
	return std::make_pair(Input >> 30, _byteswap_ulong(Input) & 0x3FFFFFFF);
}

uint32_t WriteBitFieldFlagsSize(uint8_t Flags, uint32_t Size)
{
	Size &= 0x3FFFFFFF;

	uint32_t Combined = (static_cast<uint32_t>(Flags) << 30) | Size;
	return _byteswap_ulong(Combined);
}

//WritePackedInt taken from https://github.com/blueskythlikesclouds/TagTools/blob/havoc/Havoc/Extensions/BinaryWriterEx.cs
void PhiveShape2::WritePackedInt(BinaryVectorWriter& Writer, int64_t Value)
{
	if (Value < 0x80)
	{
		Writer.WriteInteger((uint8_t)Value, sizeof(uint8_t));
	}
	else if (Value < 0x4000)
	{
		Writer.WriteInteger((uint8_t)(((Value >> 8) & 0xFF) | 0x80), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)(Value & 0xFF), sizeof(uint8_t));
	}
	else if (Value < 0x200000)
	{
		Writer.WriteInteger((uint8_t)(((Value >> 16) & 0xFF) | 0xC0), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 8) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)(Value & 0xFF), sizeof(uint8_t));
	}
	else if (Value < 0x8000000)
	{
		Writer.WriteInteger((uint8_t)(((Value >> 24) & 0xFF) | 0xE0), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 16) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 8) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)(Value & 0xFF), sizeof(uint8_t));
	}
	else if (Value < 0x800000000)
	{
		Writer.WriteInteger((uint8_t)(((Value >> 32) & 0xFF) | 0xE8), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 24) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 16) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 8) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)(Value & 0xFF), sizeof(uint8_t));
	}
	else if (Value < 0x10000000000)
	{
		Writer.WriteInteger((uint8_t)0xF8, sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 32) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 24) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 16) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 8) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)(Value & 0xFF), sizeof(uint8_t));
	}
	else if (Value < 0x800000000000000)
	{
		Writer.WriteInteger((uint8_t)(((Value >> 56) & 0xFF) | 0xF0), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 48) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 40) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 32) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 24) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 16) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 8) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)(Value & 0xFF), sizeof(uint8_t));
	}
	else
	{
		Writer.WriteInteger((uint8_t)0xF9, sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 56) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 48) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 40) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 32) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 24) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 16) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)((Value >> 8) & 0xFF), sizeof(uint8_t));
		Writer.WriteInteger((uint8_t)(Value & 0xFF), sizeof(uint8_t));
	}
}

//ReadPackedInt taken from https://github.com/blueskythlikesclouds/TagTools/blob/havoc/Havoc/Extensions/BinaryReaderEx.cs
int64_t PhiveShape2::ReadPackedInt(BinaryVectorReader& Reader)
{
	uint8_t FirstByte = Reader.ReadUInt8();

	if ((FirstByte & 0x80) == 0)
		return FirstByte;

	switch (FirstByte >> 3)
	{
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		return ((FirstByte << 8) | Reader.ReadUInt8()) & 0x3FFF;

	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
		return ((FirstByte << 16) | (Reader.ReadUInt8() << 8) | Reader.ReadUInt8()) & 0x1FFFFF;

	case 0x1C:
		return ((FirstByte << 24) | (Reader.ReadUInt8() << 16) | (Reader.ReadUInt8() << 8) |
			Reader.ReadUInt8()) &
			0x7FFFFFF;

	case 0x1D:
		return ((FirstByte << 32) | (Reader.ReadUInt8() << 24) | (Reader.ReadUInt8() << 16) |
			(Reader.ReadUInt8() << 8) | Reader.ReadUInt8()) & 0x7FFFFFFFFFFFFFF;

	case 0x1E:
		return ((FirstByte << 56) | (Reader.ReadUInt8() << 48) | (Reader.ReadUInt8() << 40) |
			(Reader.ReadUInt8() << 32) | (Reader.ReadUInt8() << 24) | (Reader.ReadUInt8() << 16) |
			(Reader.ReadUInt8() << 8) | Reader.ReadUInt8()) & 0x7FFFFFFFFFFFFFF;

	case 0x1F:
		return (FirstByte & 7) == 0
			? ((FirstByte << 40) | (Reader.ReadUInt8() << 32) | (Reader.ReadUInt8() << 24) |
				(Reader.ReadUInt8() << 16) |
				(Reader.ReadUInt8() << 8) | Reader.ReadUInt8()) & 0xFFFFFFFFFF
			: (FirstByte & 7) == 1
			? (Reader.ReadUInt8() << 56) | (Reader.ReadUInt8() << 48) | (Reader.ReadUInt8() << 40) |
			(Reader.ReadUInt8() << 32) | (Reader.ReadUInt8() << 24) | (Reader.ReadUInt8() << 16) |
			(Reader.ReadUInt8() << 8) | Reader.ReadUInt8()
			: 0;

	default:
		return 0;
	}
}

template<typename T>
void PhiveShape2::hkRelArrayView<T>::Read(BinaryVectorReader& Reader)
{
	m_Offset = Reader.ReadUInt32();
	m_Size = Reader.ReadUInt32();
	uint32_t Jumback = Reader.GetPosition();
	if (m_Offset > 0)
	{
		Reader.Seek(m_Offset - 8, BinaryVectorReader::Position::Current);
		m_Elements.resize(m_Size);
		for (int i = 0; i < m_Size; i++)
		{
			T Object;
			if constexpr (std::is_integral_v<T>)
			{
				Reader.ReadStruct(&Object, sizeof(T));
				m_Elements[i] = Object;
				continue;
			}

			hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Object);
			if (BaseObj)
			{
				BaseObj->Read(Reader);
			}
			else
			{
				std::cout << "Cannot cast to hkReadableWriteableObject" << std::endl;
			}
			m_Elements[i] = Object;
		}
	}
	Reader.Seek(Jumback, BinaryVectorReader::Position::Begin);
}
template<typename T>
void PhiveShape2::hkRelArrayView<T>::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	if (m_BaseOffset > 0) Writer.Seek(m_BaseOffset, BinaryVectorWriter::Position::Begin);

	if (m_Elements.empty())
	{
		Writer.WriteInteger(0, sizeof(uint32_t));
		Writer.WriteInteger(0, sizeof(uint32_t));
		m_LastElementOffset = Offset;
		return;
	}

	Writer.WriteInteger(Offset - Writer.GetPosition(), sizeof(uint32_t));
	Writer.WriteInteger(m_Elements.size(), sizeof(uint32_t));

	uint32_t Jumpback = Writer.GetPosition();

	Writer.Seek(Offset, BinaryVectorWriter::Position::Begin);
	for (T& Element : m_Elements)
	{
		if constexpr (std::is_integral_v<T>)
		{
			Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Element), sizeof(T));
			continue;
		}

		hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Element);
		if (BaseObj)
		{
			BaseObj->Write(Writer);
		}
		else
		{
			std::cout << "Cannot cast to hkReadableWriteableObject" << std::endl;
		}
	}
	m_LastElementOffset = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
}
template<typename T>
void PhiveShape2::hkRelArrayView<T>::Skip(BinaryVectorWriter& Writer)
{
	m_BaseOffset = Writer.GetPosition();
	Writer.Seek(8, BinaryVectorWriter::Position::Current);
}

template<typename T>
void PhiveShape2::hkRelArrayViewVertexBuffer<T>::Read(BinaryVectorReader& Reader)
{
	m_Offset = Reader.ReadInt32();
	m_Size = Reader.ReadInt32() / 6;
	uint32_t Jumback = Reader.GetPosition();
	if (m_Offset > 0)
	{
		Reader.Seek(m_Offset - 8, BinaryVectorReader::Position::Current);
		m_Elements.resize(m_Size);
		for (int i = 0; i < m_Size; i++)
		{
			T Object;
			if constexpr (std::is_integral_v<T>)
			{
				Reader.ReadStruct(&Object, sizeof(T));
				m_Elements[i] = Object;
				continue;
			}

			hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Object);
			if (BaseObj)
			{
				BaseObj->Read(Reader);
			}
			else
			{
				std::cout << "Cannot cast to hkReadableWriteableObject" << std::endl;
			}
			m_Elements[i] = Object;
		}
	}
	Reader.Seek(Jumback, BinaryVectorReader::Position::Begin);
}
template<typename T>
void PhiveShape2::hkRelArrayViewVertexBuffer<T>::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	if (m_BaseOffset > 0) Writer.Seek(m_BaseOffset, BinaryVectorWriter::Position::Begin);

	Writer.WriteInteger(Offset - Writer.GetPosition(), sizeof(uint32_t));
	Writer.WriteInteger(m_Elements.size() * 6, sizeof(uint32_t));

	uint32_t Jumpback = Writer.GetPosition();

	Writer.Seek(Offset, BinaryVectorWriter::Position::Begin);
	for (T& Element : m_Elements)
	{
		if constexpr (std::is_integral_v<T>)
		{
			Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Element), sizeof(T));
			continue;
		}

		hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Element);
		if (BaseObj)
		{
			BaseObj->Write(Writer);
		}
		else
		{
			std::cout << "Cannot cast to hkReadableWriteableObject" << std::endl;
		}
	}
	m_LastElementOffset = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
}
template<typename T>
void PhiveShape2::hkRelArrayViewVertexBuffer<T>::Skip(BinaryVectorWriter& Writer)
{
	m_BaseOffset = Writer.GetPosition();
	Writer.Seek(8, BinaryVectorWriter::Position::Current);
}

template<typename T>
void PhiveShape2::hkRelPtr<T>::Read(BinaryVectorReader& Reader)
{
	m_Offset = Reader.ReadInt64();
	uint32_t Jumback = Reader.GetPosition();
	if (m_Offset > 0)
	{
		Reader.Seek(m_Offset, BinaryVectorReader::Position::Current);
		T Object;
		hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Object);
		if (BaseObj)
		{
			BaseObj->Read(Reader);
			m_NullPtr = false;
		}
		else
		{
			std::cout << "Cannot cast to hkReadableWriteableObject" << std::endl;
		}
		m_Data = *reinterpret_cast<T*>(BaseObj);
	}
	Reader.Seek(Jumback, BinaryVectorReader::Position::Begin);
}
template<typename T>
void PhiveShape2::hkRelPtr<T>::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	if (m_BaseOffset > 0) Writer.Seek(m_BaseOffset, BinaryVectorWriter::Position::Begin);

	if (m_NullPtr)
	{
		Writer.WriteInteger(0, sizeof(uint64_t));
		m_LastElementOffset = Offset;
		return;
	}

	Writer.WriteInteger((uint64_t)(Offset - Writer.GetPosition()), sizeof(uint64_t));

	uint32_t Jumpback = Writer.GetPosition();

	Writer.Seek(Offset, BinaryVectorWriter::Position::Begin);
	hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&m_Data);
	if (BaseObj)
	{
		BaseObj->Write(Writer);
	}
	else
	{
		std::cout << "Cannot cast to hkReadableWriteableObject" << std::endl;
	}
	m_LastElementOffset = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
}
template<typename T>
void PhiveShape2::hkRelPtr<T>::Skip(BinaryVectorWriter& Writer)
{
	m_BaseOffset = Writer.GetPosition();
	Writer.Seek(8, BinaryVectorWriter::Position::Current);
}

template<typename T>
void PhiveShape2::hkRelArray<T>::Read(BinaryVectorReader& Reader)
{
	m_Offset = Reader.ReadUInt64();
	m_Size = Reader.ReadInt32();
	m_CapacityAndFlags = Reader.ReadInt32();
	uint32_t Jumback = Reader.GetPosition();
	if (m_Offset > 0)
	{
		Reader.Seek(m_Offset - 16, BinaryVectorReader::Position::Current);
		m_Elements.resize(m_Size);
		for (int i = 0; i < m_Size; i++)
		{
			T Object;
			hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Object);
			if (BaseObj)
			{
				BaseObj->Read(Reader);
			}
			else
			{
				std::cout << "Cannot cast to hkReadableWriteableObject" << std::endl;
			}
			m_Elements[i] = Object;
		}
	}
	Reader.Seek(Jumback, BinaryVectorReader::Position::Begin);
}
template<typename T>
void PhiveShape2::hkRelArray<T>::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	if (m_BaseOffset > 0) Writer.Seek(m_BaseOffset, BinaryVectorWriter::Position::Begin);

	Writer.WriteInteger(!m_Elements.empty() ? (uint64_t)(Offset - Writer.GetPosition()) : 0, sizeof(uint64_t));
	Writer.WriteInteger((int32_t)m_Elements.size(), sizeof(int32_t));
	Writer.WriteInteger((int32_t)m_CapacityAndFlags, sizeof(int32_t));

	uint32_t Jumpback = Writer.GetPosition();

	Writer.Seek(Offset, BinaryVectorWriter::Position::Begin);
	for (T& Element : m_Elements)
	{
		if constexpr (std::is_integral_v<T>)
		{
			Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Element), sizeof(T));
			continue;
		}

		hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Element);
		if (BaseObj)
		{
			BaseObj->Write(Writer);
		}
		else
		{
			std::cout << "Cannot cast to hkReadableWriteableObject" << std::endl;
		}
	}
	m_LastElementOffset = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
}
template<typename T>
void PhiveShape2::hkRelArray<T>::Skip(BinaryVectorWriter& Writer)
{
	m_BaseOffset = Writer.GetPosition();
	Writer.Seek(16, BinaryVectorWriter::Position::Current);
}

void PhiveShape2::hkBaseObject::Read(BinaryVectorReader& Reader)
{
	m_VTFReserve = Reader.ReadUInt64();
}
void PhiveShape2::hkBaseObject::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	Writer.WriteInteger(m_VTFReserve, sizeof(uint64_t));
	std::cout << "WROTE hkBaseObject\n";
}

void PhiveShape2::hkReferencedObject::Read(BinaryVectorReader& Reader)
{
	m_BaseObject.Read(Reader);
	m_SizeAndFlags = Reader.ReadUInt64();
	m_RefCount = Reader.ReadUInt64();
}
void PhiveShape2::hkReferencedObject::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_BaseObject.Write(Writer);
	Writer.WriteInteger(m_SizeAndFlags, sizeof(uint64_t));
	Writer.WriteInteger(m_RefCount, sizeof(uint64_t));
	std::cout << "WROTE hkReferencedObject\n";
}

void PhiveShape2::hknpShapePropertyBase::Read(BinaryVectorReader& Reader)
{
	m_ReferencedObject.Read(Reader);
	m_PropertyKey = Reader.ReadUInt16();
}
void PhiveShape2::hknpShapePropertyBase::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_ReferencedObject.Write(Writer);
	Writer.WriteInteger(m_PropertyKey, sizeof(uint16_t));
	std::cout << "WROTE hknpShapePropertyBase\n";
}

void PhiveShape2::hknpShapePropertiesEntry::Read(BinaryVectorReader& Reader)
{
	m_Object.Read(Reader);
}
void PhiveShape2::hknpShapePropertiesEntry::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Object.Write(Writer, Writer.GetPosition() + 8);
	std::cout << "WROTE hknpShapePropertiesEntry\n";
}

void PhiveShape2::hknpShapeProperties::Read(BinaryVectorReader& Reader)
{
	m_Properties.Read(Reader);
}
void PhiveShape2::hknpShapeProperties::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Properties.Write(Writer, Writer.GetPosition() + 16);
	std::cout << "WROTE hknpShapeProperties\n";
}

void PhiveShape2::hknpShape::Read(BinaryVectorReader& Reader)
{
	m_ReferencedObject.Read(Reader);
	m_Type = static_cast<PhiveShape2::hknpShapeTypeEnum>(Reader.ReadUInt8());
	m_DispatchType = static_cast<PhiveShape2::hknpCollisionDispatchTypeEnum>(Reader.ReadUInt8());
	m_Flags = static_cast<PhiveShape2::hknpShapeFlagsEnum>(Reader.ReadUInt16());

	m_NumShapeKeyBits = Reader.ReadUInt8();
	Reader.ReadStruct(&m_Reserve0, 3);
	m_ConvexRadius = Reader.ReadFloat();
	m_Reserve1 = Reader.ReadUInt32();
	m_UserData = Reader.ReadUInt64();
	m_Properties.Read(Reader);
}
void PhiveShape2::hknpShape::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_ReferencedObject.Write(Writer);
	Writer.WriteInteger((uint8_t)m_Type, sizeof(uint8_t));
	Writer.WriteInteger((uint8_t)m_DispatchType, sizeof(uint8_t));
	Writer.WriteInteger((uint16_t)m_Flags, sizeof(uint16_t));

	Writer.WriteInteger(m_NumShapeKeyBits, sizeof(uint8_t));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_Reserve0), 3);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_ConvexRadius), sizeof(float));
	Writer.WriteInteger(m_Reserve1, sizeof(uint32_t));
	Writer.WriteInteger(m_UserData, sizeof(uint64_t));
	m_Properties.Write(Writer);

	std::cout << "WROTE hknpShape\n";
}

void PhiveShape2::hknpCompositeShape::Read(BinaryVectorReader& Reader)
{
	m_Shape.Read(Reader);
	m_ShapeTagCodecInfo = Reader.ReadUInt32();
	m_Reserve2 = Reader.ReadUInt32();
	m_Reserve3 = Reader.ReadUInt64();
}
void PhiveShape2::hknpCompositeShape::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Shape.Write(Writer);
	Writer.WriteInteger(m_ShapeTagCodecInfo, sizeof(uint32_t));
	Writer.WriteInteger(m_Reserve2, sizeof(uint32_t));
	Writer.WriteInteger(m_Reserve3, sizeof(uint64_t));
	std::cout << "WROTE hknpCompositeShape\n";
}

void PhiveShape2::hknpMeshShapeVertexConversionUtil::Read(BinaryVectorReader& Reader)
{
	Reader.ReadStruct(m_BitScale16, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(m_BitScale16Inv, 4 * 4); //4 * sizeof(float)
}
void PhiveShape2::hknpMeshShapeVertexConversionUtil::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_BitScale16[0]), 4 * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_BitScale16Inv[0]), 4 * 4);
	std::cout << "WROTE hknpMeshShapeVertexConversionUtil\n";
}

void PhiveShape2::hknpMeshShapeShapeTagTableEntry::Read(BinaryVectorReader& Reader)
{
	m_MeshPrimitiveKey = Reader.ReadUInt32();
	m_ShapeTag = Reader.ReadUInt16();
	m_Reserve0 = Reader.ReadUInt16();
}
void PhiveShape2::hknpMeshShapeShapeTagTableEntry::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Offset = Writer.GetPosition();
	Writer.WriteInteger(m_MeshPrimitiveKey, sizeof(uint32_t));
	Writer.WriteInteger(m_ShapeTag, sizeof(uint16_t));
	Writer.WriteInteger(m_Reserve0, sizeof(uint16_t));
	std::cout << "WROTE hknpMeshShapeShapeTagTableEntry\n";
}

void PhiveShape2::hkcdFourAabb::Read(BinaryVectorReader& Reader)
{
	Reader.ReadStruct(m_MinX, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(m_MaxX, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(m_MinY, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(m_MaxY, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(m_MinZ, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(m_MaxZ, 4 * 4); //4 * sizeof(float)
}
void PhiveShape2::hkcdFourAabb::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&m_MinX[0]), 4 * sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&m_MaxX[0]), 4 * sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&m_MinY[0]), 4 * sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&m_MaxY[0]), 4 * sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&m_MinZ[0]), 4 * sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&m_MaxZ[0]), 4 * sizeof(float));
	std::cout << "WROTE hkcdFourAabb\n";
}

void PhiveShape2::hkcdSimdTreeNode::Read(BinaryVectorReader& Reader)
{
	m_FourAabb.Read(Reader);
	Reader.ReadStruct(m_Data, 4 * 4); //4 * sizeof(uint32_t)
	m_IsLeaf = Reader.ReadUInt8();
	m_IsActive = Reader.ReadUInt8();
	m_Reserve0 = Reader.ReadUInt16();
	m_Reserve1 = Reader.ReadUInt32();
	m_Reserve2 = Reader.ReadUInt64();
}
void PhiveShape2::hkcdSimdTreeNode::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Offset = Writer.GetPosition();
	m_FourAabb.Write(Writer);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_Data[0]), 4 * 4);
	Writer.WriteInteger(m_IsLeaf, sizeof(uint8_t));
	Writer.WriteInteger(m_IsActive, sizeof(uint8_t));
	Writer.WriteInteger(m_Reserve0, sizeof(uint16_t));
	Writer.WriteInteger(m_Reserve1, sizeof(uint32_t));
	Writer.WriteInteger(m_Reserve2, sizeof(uint64_t));
	std::cout << "WROTE hkcdSimdTreeNode\n";
}

void PhiveShape2::hkcdSimdTree::Read(BinaryVectorReader& Reader)
{
	m_Nodes.Read(Reader);
	m_IsCompact = Reader.ReadUInt8();
	Reader.ReadStruct(m_Reserve0, 3); //3 * sizeof(uint8_t)
	m_Reserve1 = Reader.ReadUInt32();
}
void PhiveShape2::hkcdSimdTree::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Nodes.Skip(Writer);
	Writer.WriteInteger(m_IsCompact, sizeof(uint8_t));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_Reserve0), 3);
	Writer.WriteInteger(m_Reserve1, sizeof(uint32_t));
	std::cout << "WROTE hkcdSimdTree\n";
}

void PhiveShape2::hknpTransposedFourAabbs8::Read(BinaryVectorReader& Reader)
{
	Reader.ReadStruct(&m_MinX[0], sizeof(uint8_t) * 4);
	Reader.ReadStruct(&m_MaxX[0], sizeof(uint8_t) * 4);
	Reader.ReadStruct(&m_MinY[0], sizeof(uint8_t) * 4);
	Reader.ReadStruct(&m_MaxY[0], sizeof(uint8_t) * 4);
	Reader.ReadStruct(&m_MinZ[0], sizeof(uint8_t) * 4);
	Reader.ReadStruct(&m_MaxZ[0], sizeof(uint8_t) * 4);
}
void PhiveShape2::hknpTransposedFourAabbs8::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_MinX), sizeof(uint8_t) * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_MaxX), sizeof(uint8_t) * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_MinY), sizeof(uint8_t) * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_MaxY), sizeof(uint8_t) * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_MinZ), sizeof(uint8_t) * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_MaxZ), sizeof(uint8_t) * 4);
	std::cout << "WROTE hknpTransposedFourAabbs8\n";
}

void PhiveShape2::hknpAabb8TreeNode::Read(BinaryVectorReader& Reader)
{
	m_TransposedFourAabbs8.Read(Reader);
	Reader.ReadStruct(m_Data, 4); //4 * sizeof(uint8_t)
}
void PhiveShape2::hknpAabb8TreeNode::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Offset = Writer.GetPosition();
	m_TransposedFourAabbs8.Write(Writer);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_Data), 4);
	std::cout << "WROTE hknpAabb8TreeNode\n";
}

void PhiveShape2::hknpMeshShapeGeometrySectionPrimitive::Read(BinaryVectorReader& Reader)
{
	m_IdA = Reader.ReadUInt8();
	m_IdB = Reader.ReadUInt8();
	m_IdC = Reader.ReadUInt8();
	m_IdD = Reader.ReadUInt8();
}
void PhiveShape2::hknpMeshShapeGeometrySectionPrimitive::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Offset = Writer.GetPosition();
	Writer.WriteInteger(m_IdA, sizeof(uint8_t));
	Writer.WriteInteger(m_IdB, sizeof(uint8_t));
	Writer.WriteInteger(m_IdC, sizeof(uint8_t));
	Writer.WriteInteger(m_IdD, sizeof(uint8_t));
	std::cout << "WROTE hknpMeshShapeGeometrySectionPrimitive\n";
}

void PhiveShape2::hknpMeshShapeGeometrySectionVertex16_3::Read(BinaryVectorReader& Reader)
{
	m_X = Reader.ReadUInt16();
	m_Y = Reader.ReadUInt16();
	m_Z = Reader.ReadUInt16();
}
void PhiveShape2::hknpMeshShapeGeometrySectionVertex16_3::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Offset = Writer.GetPosition();
	Writer.WriteInteger(m_X, sizeof(uint16_t));
	Writer.WriteInteger(m_Y, sizeof(uint16_t));
	Writer.WriteInteger(m_Z, sizeof(uint16_t));
	std::cout << "WROTE hknpMeshShapeGeometrySectionVertex16_3\n";
}

void PhiveShape2::hknpMeshShapeGeometrySectionInteriorPrimitiveBitField::Read(BinaryVectorReader& Reader)
{
	m_BitField = Reader.ReadUInt8();
}
void PhiveShape2::hknpMeshShapeGeometrySectionInteriorPrimitiveBitField::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Offset = Writer.GetPosition();
	Writer.WriteInteger(m_BitField, sizeof(uint8_t));
}

void PhiveShape2::hknpMeshShapeGeometrySection::InjectBVH(std::vector<PhiveShape2::QuadBVHNode>& Nodes, glm::vec3 Min, glm::vec3 Max)
{
	m_SectionBvh.m_Elements.clear();
	//Encoding bounding boxes
	for (PhiveShape2::QuadBVHNode& Node : Nodes)
	{
		m_SectionBvh.m_Elements.resize(m_SectionBvh.m_Elements.size() + 1);
		hknpAabb8TreeNode& TreeNode = m_SectionBvh.m_Elements.back();

		std::vector<uint32_t> PrimitiveOrChild(4);
		PrimitiveOrChild[0] = Node.mIsLeaf ? Node.mPrimitiveIndices[0] : (Node.mChildrenIndices[0] != 0xFFFF ? Node.mChildrenIndices[0] : 0);
		PrimitiveOrChild[1] = Node.mIsLeaf ? Node.mPrimitiveIndices[1] : (Node.mChildrenIndices[1] != 0xFFFF ? Node.mChildrenIndices[1] : 0);
		PrimitiveOrChild[2] = Node.mIsLeaf ? Node.mPrimitiveIndices[2] : (Node.mChildrenIndices[2] != 0xFFFF ? Node.mChildrenIndices[2] : 0);
		PrimitiveOrChild[3] = Node.mIsLeaf ? Node.mPrimitiveIndices[3] : (Node.mChildrenIndices[3] != 0xFFFF ? Node.mChildrenIndices[3] : 0);

		std::vector<std::pair<uint32_t, uint32_t>> IndexToPrimitive;
		IndexToPrimitive.push_back({ 0, PrimitiveOrChild[0] });
		IndexToPrimitive.push_back({ 1, PrimitiveOrChild[1] });
		IndexToPrimitive.push_back({ 2, PrimitiveOrChild[2] });
		IndexToPrimitive.push_back({ 3, PrimitiveOrChild[3] });

		std::sort(IndexToPrimitive.begin(), IndexToPrimitive.end(), [](auto& left, auto& right) {
			return left.second < right.second;
			});

		//If leaf, make sure data[2] is bigger than data[3]
		if (Node.mIsLeaf)
		{
			if (IndexToPrimitive[3].second > IndexToPrimitive[2].second)
			{
				std::pair<uint32_t, uint32_t> Tmp = IndexToPrimitive[2];
				IndexToPrimitive[2] = IndexToPrimitive[3];
				IndexToPrimitive[3] = Tmp;
			}
		}

		for (size_t i = 0; i < IndexToPrimitive.size(); i++)
		{
			TreeNode.m_TransposedFourAabbs8.m_MinX[i] = (uint8_t)(std::round((Node.mMin[IndexToPrimitive[i].first].x - Min.x) / (Max.x - Min.x) * 255.0f));
			TreeNode.m_TransposedFourAabbs8.m_MaxX[i] = (uint8_t)(std::round((Node.mMax[IndexToPrimitive[i].first].x - Min.x) / (Max.x - Min.x) * 255.0f));

			TreeNode.m_TransposedFourAabbs8.m_MinY[i] = (uint8_t)(std::round((Node.mMin[IndexToPrimitive[i].first].y - Min.y) / (Max.y - Min.y) * 255.0f));
			TreeNode.m_TransposedFourAabbs8.m_MaxY[i] = (uint8_t)(std::round((Node.mMax[IndexToPrimitive[i].first].y - Min.y) / (Max.y - Min.y) * 255.0f));

			TreeNode.m_TransposedFourAabbs8.m_MinZ[i] = (uint8_t)(std::round((Node.mMin[IndexToPrimitive[i].first].z - Min.z) / (Max.z - Min.z) * 255.0f));
			TreeNode.m_TransposedFourAabbs8.m_MaxZ[i] = (uint8_t)(std::round((Node.mMax[IndexToPrimitive[i].first].z - Min.z) / (Max.z - Min.z) * 255.0f));

			TreeNode.m_Data[i] = IndexToPrimitive[i].second;
		}
	}
}
void PhiveShape2::hknpMeshShapeGeometrySection::Read(BinaryVectorReader& Reader)
{
	m_SectionBvh.Read(Reader);
	m_Primitives.Read(Reader);
	m_VertexBuffer.Read(Reader);
	m_InteriorPrimitiveBitField.Read(Reader);

	Reader.ReadStruct(m_SectionOffset, 3 * 4); //3 * sizeof(uint32_t)
	Reader.ReadStruct(m_BitScale8Inv, 3 * 4); //3 * sizeof(float)
	Reader.ReadStruct(m_BitOffset, 3 * 2); //3 * sizeof(int16_t)
	m_Reserve0 = Reader.ReadUInt16();
}
void PhiveShape2::hknpMeshShapeGeometrySection::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_Offset = Writer.GetPosition();
	m_SectionBvh.Skip(Writer);
	m_Primitives.Skip(Writer);
	m_VertexBuffer.Skip(Writer);
	m_InteriorPrimitiveBitField.Skip(Writer);

	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&m_SectionOffset[0]), 3 * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&m_BitScale8Inv[0]), 3 * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&m_BitOffset[0]), 3 * 2);
	Writer.WriteInteger(m_Reserve0, sizeof(uint16_t));

	m_SectionBvh.Write(Writer, Writer.GetPosition());
	Writer.Seek(m_SectionBvh.m_LastElementOffset, BinaryVectorWriter::Position::Begin);
	Writer.Align(16);

	m_Primitives.Write(Writer, Writer.GetPosition());
	Writer.Seek(m_Primitives.m_LastElementOffset, BinaryVectorWriter::Position::Begin);
	Writer.Align(16);

	m_VertexBuffer.Write(Writer, Writer.GetPosition());
	Writer.Seek(m_VertexBuffer.m_LastElementOffset, BinaryVectorWriter::Position::Begin);
	Writer.Align(16);

	m_InteriorPrimitiveBitField.Write(Writer, Writer.GetPosition());
	Writer.Align(16);

	std::cout << "WROTE hknpMeshShapeGeometrySection\n";
}

void PhiveShape2::hknpMeshShapePrimitiveMapping::Read(BinaryVectorReader& Reader)
{
	m_ReferencedObject.Read(Reader);
	m_SectionStart.Read(Reader);
	m_BitString.Read(Reader);
	m_BitsPerEntry = Reader.ReadUInt32();
	m_TriangleIndexBitMask = Reader.ReadUInt32();
}
void PhiveShape2::hknpMeshShapePrimitiveMapping::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_ReferencedObject.Write(Writer);
	m_SectionStart.Skip(Writer);
	m_BitString.Skip(Writer);
	Writer.WriteInteger(m_BitsPerEntry, sizeof(uint32_t));
	Writer.WriteInteger(m_TriangleIndexBitMask, sizeof(uint32_t));

	m_SectionStart.Write(Writer, Writer.GetPosition());
	Writer.Seek(m_SectionStart.m_LastElementOffset, BinaryVectorWriter::Position::Begin);

	m_BitString.Write(Writer, Writer.GetPosition());
	std::cout << "WROTE hknpMeshShapePrimitiveMapping\n";
}

void PhiveShape2::hknpMeshShape::Read(BinaryVectorReader& Reader)
{
	m_CompositeShape.Read(Reader);
	m_VertexConversionUtil.Read(Reader);
	m_ShapeTagTable.Read(Reader);
	m_TopLevelTree.Read(Reader);
	m_GeometrySections.Read(Reader);
	m_PrimitiveMapping.Read(Reader);
}
void PhiveShape2::hknpMeshShape::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	m_CompositeShape.Write(Writer);
	m_VertexConversionUtil.Write(Writer);
	m_ShapeTagTable.Skip(Writer);
	m_TopLevelTree.Write(Writer);
	m_GeometrySections.Skip(Writer);
	m_PrimitiveMapping.Skip(Writer);

	m_ShapeTagTable.Write(Writer, Writer.GetPosition());
	Writer.Seek(m_ShapeTagTable.m_LastElementOffset, BinaryVectorWriter::Position::Begin);
	Writer.Align(16);

	m_TopLevelTree.m_Nodes.Write(Writer, Writer.GetPosition());
	Writer.Seek(m_TopLevelTree.m_Nodes.m_LastElementOffset, BinaryVectorWriter::Position::Begin);
	Writer.Align(16);
	
	/*
	m_GeometrySections.Write(Writer, Writer.GetPosition());
	Writer.Seek(m_GeometrySections.m_Elements.back().m_InteriorPrimitiveBitField.m_LastElementOffset, BinaryVectorWriter::Position::Begin);
	Writer.Align(16);
	*/

	std::vector<uint32_t> GeometrySectionBaseOffsets;

	for (hknpMeshShapeGeometrySection& Section : m_GeometrySections.m_Elements)
	{
		GeometrySectionBaseOffsets.push_back(Writer.GetPosition());
		Section.m_Offset = Writer.GetPosition();
		Writer.Seek(32, BinaryVectorWriter::Position::Current);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Section.m_SectionOffset[0]), 3 * 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Section.m_BitScale8Inv[0]), 3 * 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Section.m_BitOffset[0]), 3 * 2);
		Writer.WriteInteger(Section.m_Reserve0, sizeof(uint16_t));
	}

	for (int i = 0; i < m_GeometrySections.m_Elements.size(); i++)
	{
		uint32_t SectionOffset = Writer.GetPosition();
		for (hknpAabb8TreeNode& Node : m_GeometrySections.m_Elements[i].m_SectionBvh.m_Elements)
		{
			Node.Write(Writer);
		}
		Writer.Align(16);
		uint32_t Jumpback = Writer.GetPosition();
		Writer.Seek(GeometrySectionBaseOffsets[i], BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(SectionOffset - Writer.GetPosition(), sizeof(uint32_t));
		Writer.WriteInteger(m_GeometrySections.m_Elements[i].m_SectionBvh.m_Elements.size(), sizeof(uint32_t));
		Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	}

	for (int i = 0; i < m_GeometrySections.m_Elements.size(); i++)
	{
		uint32_t SectionOffset = Writer.GetPosition();

		for (hknpMeshShapeGeometrySectionPrimitive& Primitive : m_GeometrySections.m_Elements[i].m_Primitives.m_Elements)
		{
			Primitive.Write(Writer);
		}

		Writer.Align(16);
		uint32_t Jumpback = Writer.GetPosition();
		Writer.Seek(GeometrySectionBaseOffsets[i] + 8, BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(SectionOffset - Writer.GetPosition(), sizeof(uint32_t));
		Writer.WriteInteger(m_GeometrySections.m_Elements[i].m_Primitives.m_Elements.size(), sizeof(uint32_t));
		Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	}

	for (int i = 0; i < m_GeometrySections.m_Elements.size(); i++)
	{
		uint32_t SectionOffset = Writer.GetPosition();

		for (hknpMeshShapeGeometrySectionVertex16_3& Vertex : m_GeometrySections.m_Elements[i].m_VertexBuffer.m_Elements)
		{
			Vertex.Write(Writer);
		}

		Writer.Align(16);
		uint32_t Jumpback = Writer.GetPosition();
		Writer.Seek(GeometrySectionBaseOffsets[i] + 16, BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(SectionOffset - Writer.GetPosition(), sizeof(uint32_t));
		Writer.WriteInteger(m_GeometrySections.m_Elements[i].m_VertexBuffer.m_Elements.size() * 6, sizeof(uint32_t));
		Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	}

	for (int i = 0; i < m_GeometrySections.m_Elements.size(); i++)
	{
		uint32_t SectionOffset = Writer.GetPosition();
		for (hknpMeshShapeGeometrySectionInteriorPrimitiveBitField& BitField : m_GeometrySections.m_Elements[i].m_InteriorPrimitiveBitField.m_Elements)
		{
			BitField.Write(Writer);
		}

		Writer.Align(16);
		uint32_t Jumpback = Writer.GetPosition();
		Writer.Seek(GeometrySectionBaseOffsets[i] + 24, BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(SectionOffset - Writer.GetPosition(), sizeof(uint32_t));
		Writer.WriteInteger(m_GeometrySections.m_Elements[i].m_InteriorPrimitiveBitField.m_Elements.size(), sizeof(uint32_t));
		Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	}

	uint32_t Jumpback = Writer.GetPosition();
	Writer.Seek(0xE0, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(GeometrySectionBaseOffsets[0] - Writer.GetPosition(), sizeof(uint32_t));
	Writer.WriteInteger(m_GeometrySections.m_Elements.size(), sizeof(uint32_t));
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);

	m_PrimitiveMapping.Write(Writer, Writer.GetPosition());
	Writer.Seek(m_PrimitiveMapping.m_LastElementOffset, BinaryVectorWriter::Position::Begin);
	std::cout << "WROTE hknpMeshShape\n";
}

void PhiveShape2::ReadPhiveMaterials(BinaryVectorReader& Reader)
{
	uint32_t MaterialCount = m_PhiveShapeHeader.m_TableSize0 / 16;
	
	m_Materials.resize(MaterialCount);
	
	for (uint32_t i = 0; i < MaterialCount; i++)
	{
		PhiveMaterialData::Material Material;

		Reader.Seek(m_PhiveShapeHeader.m_TableOffset0 + i * 16, BinaryVectorReader::Position::Begin);
		Material.mMaterialId = Reader.ReadUInt32();
		Material.mUnknown0 = Reader.ReadUInt32();
		uint64_t Flags = Reader.ReadUInt64();
		for (size_t j = 0; j < PhiveMaterialData::mMaterialFlagMasks.size(); j++)
		{
			Material.mFlags[j] = (Flags & PhiveMaterialData::mMaterialFlagMasks[j]) == PhiveMaterialData::mMaterialFlagMasks[j];
		}
		
		Reader.Seek(m_PhiveShapeHeader.m_TableOffset1 + i * 8, BinaryVectorReader::Position::Begin);
		uint64_t CollisionFlags = Reader.ReadUInt64();

		Material.mCollisionFlags[0] = true; //HitAll - 0xFFFFFFFF
		
		m_Materials[i] = Material;
	}
}

void PhiveShape2::ReadStringTable(BinaryVectorReader& Reader, std::vector<std::string>& Dest)
{
	uint32_t Size = ReadBitFieldFlagsSize(Reader.ReadUInt32()).second;
	Reader.Seek(4, BinaryVectorReader::Position::Current); //Magic
	Size = Size - 8; //Flags(1) - Size(3) - Magic(4)

	std::string Buffer;
	for (uint32_t i = 0; i < Size; i++)
	{
		char Character = Reader.ReadUInt8();
		if (Character == 0xFF) 
			continue;

		if (Character == 0x00)
		{
			Dest.push_back(Buffer);
			Buffer.clear();
			continue;
		}

		Buffer += Character;
	}
}

void PhiveShape2::ReadTagHash(BinaryVectorReader& Reader)
{
	uint32_t Size = ReadBitFieldFlagsSize(Reader.ReadUInt32()).second;
	Reader.Seek(4, BinaryVectorReader::Position::Current); //Magic
	Size = Size - 8; //Flags(1) - Size(3) - Magic(4)
	m_TagHash.resize(Size);
	Reader.ReadStruct(m_TagHash.data(), Size);
}

void PhiveShape2::ReadTagBody(BinaryVectorReader& Reader)
{
	uint32_t Size = ReadBitFieldFlagsSize(Reader.ReadUInt32()).second;
	Reader.Seek(4, BinaryVectorReader::Position::Current); //Magic
	Size = Size - 8; //Flags(1) - Size(3) - Magic(4)
	m_TagBody.resize(Size);
	Reader.ReadStruct(m_TagBody.data(), Size);
}

void PhiveShape2::ReadTagNameMapping(BinaryVectorReader& Reader)
{
	uint32_t Size = ReadBitFieldFlagsSize(Reader.ReadUInt32()).second;
	Reader.Seek(4, BinaryVectorReader::Position::Current); //Magic
	Size = Size - 8; //Flags(1) - Size(3) - Magic(4)

	uint32_t ElementCount = ReadPackedInt(Reader);
	m_NameMapping.resize(ElementCount);

	for (uint32_t i = 0; i < ElementCount; i++)
	{
		PhiveTagNameMappingItem Item;

		uint32_t Index = ReadPackedInt(Reader);
		uint32_t TemplateCount = ReadPackedInt(Reader);

		Item.m_Tag = m_TagStringTable[Index];
		Item.m_Parameters.resize(TemplateCount);
		for (size_t j = 0; j < Item.m_Parameters.size(); j++)
		{
			uint32_t ParameterIndex = ReadPackedInt(Reader);
			Item.m_Parameters[j].m_ValueIndex = ReadPackedInt(Reader);

			Item.m_Parameters[j].m_Parameter = m_TagStringTable[ParameterIndex];
			if (Item.m_Parameters[j].m_Parameter[0] == 't')
			{
				Item.m_Parameters[j].m_Value = m_TagStringTable[Item.m_Parameters[j].m_ValueIndex - 1];
			}
		}

		m_NameMapping[i] = Item;
	}
}

void PhiveShape2::WriteStringTable(BinaryVectorWriter& Writer, std::vector<std::string>& Strings, std::string Magic)
{
	uint32_t Jumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes(Magic.c_str());
	for (const std::string& Buf : Strings)
	{
		Writer.WriteBytes(Buf.c_str());
		Writer.WriteByte(0x00);
	}
	Writer.Align(4, 0xFF);
	uint32_t End = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(WriteBitFieldFlagsSize(0x01, End - Jumpback), sizeof(uint32_t));
	Writer.Seek(End, BinaryVectorWriter::Position::Begin);
}

void PhiveShape2::WriteTagNameMapping(BinaryVectorWriter& Writer)
{
	uint32_t Jumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes("TNA1");
	WritePackedInt(Writer, m_NameMapping.size());

	auto GetStringTableIndex = [](const std::vector<std::string>& StringTable, const std::string& Buffer)
		{
			auto Iter = std::find(StringTable.begin(), StringTable.end(), Buffer);
			if (Iter != StringTable.end())
				return (int32_t)(Iter - StringTable.begin());

			return -1;
		};

	for (const PhiveTagNameMappingItem& Item : m_NameMapping)
	{
		int32_t Index = GetStringTableIndex(m_TagStringTable, Item.m_Tag);
		if (Index == -1)
		{
			Logger::Error("PhiveShape", "Invalid tag mapping name: " + Item.m_Tag);
			continue;
		}
		uint32_t TemplateCount = Item.m_Parameters.size();

		WritePackedInt(Writer, Index);
		WritePackedInt(Writer, TemplateCount);

		for (const PhiveTagNameMappingItem::Parameter& Parameter : Item.m_Parameters)
		{
			int32_t ParameterIndex = GetStringTableIndex(m_TagStringTable, Parameter.m_Parameter);
			if (ParameterIndex == -1)
			{
				Logger::Error("PhiveShape", "Invalid tag parameter mapping name: " + Parameter.m_Parameter);
				continue;
			}
			uint32_t ParameterValue = GetStringTableIndex(m_TagStringTable, Parameter.m_Value) + 1;

			WritePackedInt(Writer, ParameterIndex);
			WritePackedInt(Writer, ParameterValue == 0 ? Parameter.m_ValueIndex : ParameterValue);
		}
	}

	Writer.Align(4);
	uint32_t End = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(WriteBitFieldFlagsSize(0x01, End - Jumpback), sizeof(uint32_t));
	Writer.Seek(End, BinaryVectorWriter::Position::Begin);
}

void PhiveShape2::WriteTagBody(BinaryVectorWriter& Writer)
{
	uint32_t Jumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes("TBDY");
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_TagBody.data()), m_TagBody.size());
	Writer.Align(4);
	uint32_t End = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(WriteBitFieldFlagsSize(0x01, End - Jumpback), sizeof(uint32_t));
	Writer.Seek(End, BinaryVectorWriter::Position::Begin);
}

void PhiveShape2::WriteTagHash(BinaryVectorWriter& Writer)
{
	uint32_t Jumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes("THSH");
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(m_TagHash.data()), m_TagHash.size());
	Writer.Align(4);
	uint32_t End = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(WriteBitFieldFlagsSize(0x01, End - Jumpback), sizeof(uint32_t));
	Writer.Seek(End, BinaryVectorWriter::Position::Begin);
}

std::pair<uint32_t, uint32_t> PhiveShape2::WriteMaterials(BinaryVectorWriter& Writer)
{
	std::pair<uint32_t, uint32_t> Offsets;

	Offsets.first = Writer.GetPosition();

	for (size_t i = 0; i < m_Materials.size(); i++)
	{
		Writer.WriteInteger(m_Materials[i].mMaterialId, sizeof(uint32_t));
		Writer.WriteInteger(m_Materials[i].mUnknown0, sizeof(uint32_t));

		uint64_t Flags = 0;

		for (size_t j = 0; j < PhiveMaterialData::mMaterialFlagNames.size(); j++)
		{
			if (!m_Materials[i].mFlags[j])
				continue;

			Flags |= PhiveMaterialData::mMaterialFlagMasks[j];
		}

		Writer.WriteInteger(Flags, sizeof(uint64_t));
	}

	Offsets.second = Writer.GetPosition();

	for (size_t i = 0; i < m_Materials.size(); i++)
	{
		uint64_t Filters = 0;
		//Generating base
		for (size_t j = 0; j < PhiveMaterialData::mMaterialFilterNames.size(); j++)
		{
			if (!m_Materials[i].mCollisionFlags[j])
				continue;

			if (PhiveMaterialData::mMaterialFilterIsBase[j])
				Filters |= PhiveMaterialData::mMaterialFilterMasks[j];
		}
		//Applying filters
		for (size_t j = 0; j < PhiveMaterialData::mMaterialFilterNames.size(); j++)
		{
			if (!m_Materials[i].mCollisionFlags[j])
				continue;

			if (!PhiveMaterialData::mMaterialFilterIsBase[j])
				Filters &= PhiveMaterialData::mMaterialFilterMasks[j];
		}
		Writer.WriteInteger(0xFFFFFFFF, sizeof(uint32_t));
		Writer.WriteInteger(Filters, sizeof(uint32_t));
	}

	Writer.Align(16);

	return Offsets;
}

uint32_t PhiveShape2::WriteItemData(BinaryVectorWriter& Writer)
{
	auto WriteItem = [this, &Writer](uint8_t TypeIndex, uint8_t Flags, uint32_t DataOffset, uint32_t Count)
		{
			Writer.WriteByte(TypeIndex);
			Writer.WriteByte(0x00);
			Writer.WriteByte(0x00);
			Writer.WriteByte(Flags);
			Writer.WriteInteger(DataOffset, sizeof(uint32_t));
			Writer.WriteInteger(Count, sizeof(uint32_t));
		};

	uint32_t IndxJumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes("INDX");
	uint32_t ItemJumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes("ITEM");
	
	//Root
	Writer.WriteInteger(0, sizeof(uint64_t));
	Writer.WriteInteger(0, sizeof(uint32_t));

	//hknpMeshShape
	WriteItem(0x01, 0x10, 0, 1);

	//ShapeTagTableEntry
	WriteItem(0x05, 0x20, m_HavokMeshShape.m_ShapeTagTable.m_Elements[0].m_Offset - 0x50, m_HavokMeshShape.m_ShapeTagTable.m_Elements.size());

	//TopLevelTree
	WriteItem(0x10, 0x20, m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[0].m_Offset - 0x50, m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements.size());

	//GeometrySections
	WriteItem(0x09, 0x20, m_HavokMeshShape.m_GeometrySections.m_Elements[0].m_Offset - 0x50, m_HavokMeshShape.m_GeometrySections.m_Elements.size());

	//SectionBvh
	for (hknpMeshShapeGeometrySection& GeometrySection : m_HavokMeshShape.m_GeometrySections.m_Elements)
	{
		if (GeometrySection.m_SectionBvh.m_Elements.empty())
			continue;

		WriteItem(0x2D, 0x20, GeometrySection.m_SectionBvh.m_Elements[0].m_Offset - 0x50, GeometrySection.m_SectionBvh.m_Elements.size());
	}

	//Primitives
	for (hknpMeshShapeGeometrySection& GeometrySection : m_HavokMeshShape.m_GeometrySections.m_Elements)
	{
		WriteItem(0x2F, 0x20, GeometrySection.m_Primitives.m_Elements[0].m_Offset - 0x50, GeometrySection.m_Primitives.m_Elements.size());
	}

	//VertexBuffer
	for (hknpMeshShapeGeometrySection& GeometrySection : m_HavokMeshShape.m_GeometrySections.m_Elements)
	{
		WriteItem(0x16, 0x20, GeometrySection.m_VertexBuffer.m_Elements[0].m_Offset - 0x50, GeometrySection.m_VertexBuffer.m_Elements.size() * 6);
	}

	//InteriorPrimitiveBitField
	for (hknpMeshShapeGeometrySection& GeometrySection : m_HavokMeshShape.m_GeometrySections.m_Elements)
	{
		if (GeometrySection.m_InteriorPrimitiveBitField.m_Elements.empty())
			continue;

		WriteItem(0x16, 0x20, GeometrySection.m_InteriorPrimitiveBitField.m_Elements[0].m_Offset - 0x50, GeometrySection.m_InteriorPrimitiveBitField.m_Elements.size());
	}

	uint32_t End = Writer.GetPosition();
	Writer.Seek(IndxJumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(WriteBitFieldFlagsSize(0x00, End - IndxJumpback), sizeof(uint32_t));
	Writer.Seek(ItemJumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(WriteBitFieldFlagsSize(0x01, End - ItemJumpback), sizeof(uint32_t));
	Writer.Seek(End, BinaryVectorWriter::Position::Begin);
	Writer.Align(16);

	return End - 0x30;
}

uint32_t PhiveShape2::FindSection(BinaryVectorReader Reader, std::string Section)
{
	Reader.Seek(0, BinaryVectorReader::Position::Begin);
	char Data[4] = { 0 };
	while (!(Data[0] == Section.at(0) && Data[1] == Section.at(1) && Data[2] == Section.at(2) && Data[3] == Section.at(3)))
	{
		Reader.ReadStruct(Data, 4);
	}
	return Reader.GetPosition() - 8; //-8 = Flags(1) - Size(3) - Magic(4)
}

PhiveShape2::hknpMeshShape& PhiveShape2::GetMeshShape()
{
	return m_HavokMeshShape;
}

void PhiveShape2::ReadHavokTagFileHeader(BinaryVectorReader& Reader)
{
	std::pair<uint8_t, uint32_t> TagData = ReadBitFieldFlagsSize(Reader.ReadUInt32());
	Reader.Seek(4, BinaryVectorReader::Position::Current);

	std::pair<uint8_t, uint32_t> SDKVData = ReadBitFieldFlagsSize(Reader.ReadUInt32());
	Reader.Seek(12, BinaryVectorReader::Position::Current);

	std::pair<uint8_t, uint32_t> DataSectionData = ReadBitFieldFlagsSize(Reader.ReadUInt32());
	Reader.Seek(4, BinaryVectorReader::Position::Current);
	
	m_HavokTagFileHeader.m_TagFlags = TagData.first;
	m_HavokTagFileHeader.m_TagSize = TagData.second;

	m_HavokTagFileHeader.m_SDKVFlags = SDKVData.first;
	m_HavokTagFileHeader.m_SDKVSize = SDKVData.second;

	m_HavokTagFileHeader.m_DataFlags = DataSectionData.first;
	m_HavokTagFileHeader.m_DataSize = DataSectionData.second;
}

std::vector<unsigned char> PhiveShape2::ToBinary()
{
	BinaryVectorWriter Writer;

	Writer.Seek(0x50, BinaryVectorWriter::Position::Begin); //Phive + Havok Header

	m_HavokMeshShape.Write(Writer);

	uint32_t TypeSizeJumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes("TYPE");

	//392 byte padding (fixed)
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x01);
	Writer.WriteByte(0x88);
	Writer.WriteBytes("TPTR");
	Writer.Seek(384, BinaryVectorWriter::Position::Current); //Padding

	//Tag String Table
	WriteStringTable(Writer, m_TagStringTable, "TST1");

	//Name mapping
	WriteTagNameMapping(Writer);

	//Field String Table
	WriteStringTable(Writer, m_FieldStringTable, "FST1");

	//Tag Body
	WriteTagBody(Writer);

	//Tag Hash
	WriteTagHash(Writer);

	//Padding
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x08);
	Writer.WriteBytes("TPAD");

	uint32_t TypeEnd = Writer.GetPosition();

	//Item
	uint32_t ItemEnd = WriteItemData(Writer);

	//Materials
	std::pair<uint32_t, uint32_t> MaterialOffsets = WriteMaterials(Writer);

	//Type Size
	Writer.Seek(TypeSizeJumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(WriteBitFieldFlagsSize(0x00, TypeEnd - TypeSizeJumpback), sizeof(uint32_t));

	//Header
	Writer.Seek(0, BinaryVectorWriter::Position::Begin);
	Writer.WriteBytes("Phive");
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x01);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0xFF);
	Writer.WriteByte(0xFE);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x04);
	Writer.WriteInteger(0x30, sizeof(uint32_t));

	Writer.WriteInteger(MaterialOffsets.first, sizeof(uint32_t));
	Writer.WriteInteger(MaterialOffsets.second, sizeof(uint32_t));
	Writer.WriteInteger(MaterialOffsets.second + std::ceil(m_Materials.size() / 2.0f) * 16, sizeof(uint32_t));
	Writer.WriteInteger(MaterialOffsets.second + std::ceil(m_Materials.size() / 2.0f) * 16 - 0x50, sizeof(uint32_t));
	Writer.WriteInteger(m_Materials.size() * 16, sizeof(uint32_t));
	Writer.WriteInteger(std::ceil(m_Materials.size() / 2.0f) * 16, sizeof(uint32_t));
	Writer.WriteInteger(0, sizeof(uint64_t));
	Writer.WriteInteger(_byteswap_ulong(ItemEnd), sizeof(uint32_t));
	Writer.WriteBytes("TAG0");
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x10);
	Writer.WriteBytes("SDKV");
	Writer.WriteBytes("20220100");

	uint32_t HavokTagFileSize = WriteBitFieldFlagsSize(0x01, TypeSizeJumpback - 0x48);
	Writer.WriteInteger(HavokTagFileSize, sizeof(uint32_t));

	Writer.WriteBytes("DATA");

	std::vector<unsigned char> Data = Writer.GetData();
	Data.resize(MaterialOffsets.second + std::ceil(m_Materials.size() / 2.0f) * 16);

	return Data;
}

void PhiveShape2::WriteToFile(std::string Path)
{
	std::ofstream File(Path, std::ios::binary);
	std::vector<unsigned char> Binary = ToBinary();
	std::copy(Binary.cbegin(), Binary.cend(),
		std::ostream_iterator<unsigned char>(File));
	File.close();
}

std::vector<float> PhiveShape2::ToVertices()
{
	std::vector<float> Vertices;

	for (hknpMeshShapeGeometrySection& GeometrySection : m_HavokMeshShape.m_GeometrySections.m_Elements)
	{
		for (hknpMeshShapeGeometrySectionVertex16_3& Vertex : GeometrySection.m_VertexBuffer.m_Elements)
		{
			Vertices.push_back(Vertex.m_X * m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16Inv[0] + GeometrySection.m_BitOffset[0] * GeometrySection.m_BitScale8Inv[0]);
			Vertices.push_back(Vertex.m_Y * m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16Inv[1] + GeometrySection.m_BitOffset[1] * GeometrySection.m_BitScale8Inv[1]);
			Vertices.push_back(Vertex.m_Z * m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16Inv[2] + GeometrySection.m_BitOffset[2] * GeometrySection.m_BitScale8Inv[2]);
		}
	}

	return Vertices;
}

std::vector<unsigned int> PhiveShape2::ToIndices()
{
	std::vector<unsigned int> Indices;

	uint32_t IndiceOffset = 0;

	for (hknpMeshShapeGeometrySection& GeometrySection : m_HavokMeshShape.m_GeometrySections.m_Elements)
	{
		for (hknpMeshShapeGeometrySectionPrimitive& Primitive : GeometrySection.m_Primitives.m_Elements)
		{
			Indices.push_back(Primitive.m_IdA + IndiceOffset);
			Indices.push_back(Primitive.m_IdB + IndiceOffset);
			Indices.push_back(Primitive.m_IdC + IndiceOffset);

			Indices.push_back(Primitive.m_IdA + IndiceOffset);
			Indices.push_back(Primitive.m_IdC + IndiceOffset);
			Indices.push_back(Primitive.m_IdD + IndiceOffset);
		}
		IndiceOffset += GeometrySection.m_VertexBuffer.m_Elements.size();
	}

	return Indices;
}

void PhiveShape2::DecodeTreeNode(hknpMeshShapeGeometrySection& GeometrySection, hknpAabb8TreeNode& TreeNode, glm::vec3 ParentMin, glm::vec3 ParentMax)
{
	bool IsLeaf = TreeNode.m_Data[2] > TreeNode.m_Data[3];

	glm::vec3 Min[4];
	glm::vec3 Max[4];

	for (int i = 0; i < 4; i++)
	{
		//std::cout << "Reverse MinX: " << ((BVHTreeNodeMinX + (float)GeometrySection.m_BitOffset[0]) / BitScale.x) << ", MaxX:" << ((BVHTreeNodeMaxX + (float)GeometrySection.m_BitOffset[0]) / BitScale.x) << std::endl;;

		Min[i] = glm::vec3(ParentMin.x + (TreeNode.m_TransposedFourAabbs8.m_MinX[i] / 255.0f) * (ParentMax.x - ParentMin.x),
			ParentMin.y + (TreeNode.m_TransposedFourAabbs8.m_MinY[i] / 255.0f) * (ParentMax.y - ParentMin.y),
			ParentMin.z + (TreeNode.m_TransposedFourAabbs8.m_MinZ[i] / 255.0f) * (ParentMax.z - ParentMin.z));

		Max[i] = glm::vec3(ParentMin.x + (TreeNode.m_TransposedFourAabbs8.m_MaxX[i] / 255.0f) * (ParentMax.x - ParentMin.x),
			ParentMin.y + (TreeNode.m_TransposedFourAabbs8.m_MaxY[i] / 255.0f) * (ParentMax.y - ParentMin.y),
			ParentMin.z + (TreeNode.m_TransposedFourAabbs8.m_MaxZ[i] / 255.0f) * (ParentMax.z - ParentMin.z));

		if (IsLeaf)
		{
			std::vector<float> Vertices = {
				Min[i].x, Min[i].y, Min[i].z, // 0: bottom-front-left
				Max[i].x, Min[i].y, Min[i].z, // 1: bottom-front-right
				Max[i].x, Min[i].y, Max[i].z, // 2: bottom-back-right
				Min[i].x, Min[i].y, Max[i].z, // 3: bottom-back-left
				Min[i].x, Max[i].y, Min[i].z, // 4: top-front-left
				Max[i].x, Max[i].y, Min[i].z, // 5: top-front-right
				Max[i].x, Max[i].y, Max[i].z, // 6: top-back-right
				Min[i].x, Max[i].y, Max[i].z  // 7: top-back-left
			};
			std::vector<uint32_t> Indices = {
				0, 1, 2,
				0, 1, 3,

				0, 1, 5,
				0, 1, 4,

				2, 3, 6,
				2, 3, 7,

				0, 3, 4,
				0, 3, 7,

				1, 2, 5,
				1, 2, 6,

				4, 5, 6,
				4, 5, 7
			};

			UIMapView::mDebugMeshes.emplace_back(Vertices, Indices);
		}
	}

	if (!IsLeaf)
	{
		for (int i = 0; i < 4; i++)
		{
			if(Max[i].x > Min[i].x && Max[i].y > Min[i].y && Max[i].z > Min[i].z)
				DecodeTreeNode(GeometrySection, GeometrySection.m_SectionBvh.m_Elements[TreeNode.m_Data[i]], ParentMin, ParentMax);
		}
	}
}

//NOTE: Materials need to be set up BEFORE injecting a new model, otherwise the shape tag table will be invalid and the user will be stuck at the loading screen
void PhiveShape2::InjectModel(std::vector<glm::vec3> Vertices, std::vector<std::pair<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t>> Indices)
{
	float MaxVertexError = 1e-3f;

	//VertexConversionUtil Setup
	{
		const float Step = std::pow(2.0f, (int)(*reinterpret_cast<uint32_t*>(&MaxVertexError) >> 23) - 126);
		
		m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16Inv[0] = Step;
		m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16Inv[1] = Step;
		m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16Inv[2] = Step;
		m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16Inv[3] = Step;

		m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16[0] = 1.0f / Step;
		m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16[1] = 1.0f / Step;
		m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16[2] = 1.0f / Step;
		m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16[3] = 1.0f / Step;
	}

	//Material setup
	m_Materials.clear();
	PhiveMaterialData::Material Material;
	Material.mMaterialId = 0x07;
	Material.mCollisionFlags[0] = true;
	m_Materials.push_back(Material);

	//Each of those are their own models. Furthermore, the vertex count does not exceed 255
	uint32_t IndexOffset = 0;
	std::vector<std::vector<glm::vec3>> VertexGroups;
	std::vector<std::vector<std::tuple<uint32_t, uint32_t, uint32_t>>> IndexGroups;
	VertexGroups.resize(1);
	IndexGroups.resize(1);

	auto WriteVertex = [](std::vector<glm::vec3>& VertexGroup, glm::vec3 Vertex)
		{
			for (size_t i = 0; i < VertexGroup.size(); i++)
			{
				if (VertexGroup[i] == Vertex)
				{
					return i;
				}
			}

			VertexGroup.push_back(Vertex);
			return VertexGroup.size() - 1;
		};

	for (auto& Face : Indices)
	{
		if (VertexGroups.back().size() > 252)
		{
			VertexGroups.resize(VertexGroups.size() + 1);
			IndexGroups.resize(IndexGroups.size() + 1);
		}

		uint32_t IdA = WriteVertex(VertexGroups.back(), Vertices[std::get<0>(Face.first)]);
		uint32_t IdB = WriteVertex(VertexGroups.back(), Vertices[std::get<1>(Face.first)]);
		uint32_t IdC = WriteVertex(VertexGroups.back(), Vertices[std::get<2>(Face.first)]);

		IndexGroups.back().push_back(std::make_tuple(IdA, IdB, IdC));
	}

	//TopLevelTree setup
	uint32_t TopLevelTreeCount = 1 + std::ceil(VertexGroups.size() / 4.0f);
	m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements.clear();
	m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements.resize(TopLevelTreeCount);

	//Setup nodes
	{
		//First node
		const uint32_t DefaultNum = 0x7F7FFFEE;
		const float DefaultFloat = *((float*)&DefaultNum);

		auto PrepareNode = [this, DefaultFloat](uint32_t Index)
			{
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_Reserve0 = 0;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_Reserve1 = 0;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_Reserve2 = 0;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_IsLeaf = 0;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_IsActive = 0;

				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinX[0] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinX[1] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinX[2] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinX[3] = DefaultFloat;

				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxX[0] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxX[1] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxX[2] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxX[3] = DefaultFloat;


				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinY[0] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinY[1] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinY[2] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinY[3] = DefaultFloat;

				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxY[0] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxY[1] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxY[2] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxY[3] = DefaultFloat;


				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinZ[0] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinZ[1] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinZ[2] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MinZ[3] = DefaultFloat;

				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxZ[0] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxZ[1] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxZ[2] = DefaultFloat;
				m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[Index].m_FourAabb.m_MaxZ[3] = DefaultFloat;
			};
	
		for (int i = 0; i < TopLevelTreeCount; i++)
		{
			PrepareNode(i);
		}

		//PrepareNode(2);
		/*
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[1].m_Data[0] = 2;
		
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2].m_Data[0] = DefaultNum;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2].m_Data[1] = DefaultNum;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2].m_Data[2] = DefaultNum;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2].m_Data[3] = DefaultNum;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2].m_IsLeaf = 1;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2].m_IsActive = 10;
		*/

		for (int i = 1; i < TopLevelTreeCount; i++)
		{
			m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[i].m_IsLeaf = 1;
			m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[i].m_IsActive = 0;
		}

		m_HavokMeshShape.m_ShapeTagTable.m_Elements.resize(2);
		m_HavokMeshShape.m_ShapeTagTable.m_Elements[0].m_MeshPrimitiveKey = 0;
		m_HavokMeshShape.m_ShapeTagTable.m_Elements[0].m_ShapeTag = 0;
		m_HavokMeshShape.m_ShapeTagTable.m_Elements[0].m_Reserve0 = 0;
		
		m_HavokMeshShape.m_ShapeTagTable.m_Elements[1].m_MeshPrimitiveKey = (VertexGroups.size() << 9) | (0xFF << 1) | 0;
		m_HavokMeshShape.m_ShapeTagTable.m_Elements[1].m_ShapeTag = 0xFFFF;
		m_HavokMeshShape.m_ShapeTagTable.m_Elements[1].m_Reserve0 = 0;
	}

	//A shape has a max vertex count of 255. One material per geometry section
	m_HavokMeshShape.m_GeometrySections.m_Elements.clear();
	m_HavokMeshShape.m_GeometrySections.m_Elements.resize(VertexGroups.size());

	std::vector<std::pair<glm::vec3, glm::vec3>> VertexGroupDomains(VertexGroups.size());
	std::vector<std::pair<glm::vec3, glm::vec3>> VertexGroupOriginalDomains(VertexGroups.size());

	//GeometrySection setup
	for (size_t i = 0; i < VertexGroups.size(); i++)
	{
		hknpMeshShapeGeometrySection& GeometrySection = m_HavokMeshShape.m_GeometrySections.m_Elements[i];

		glm::vec3 MinVertex(std::numeric_limits<float>::max());
		glm::vec3 MaxVertex(std::numeric_limits<float>::lowest());

		for (const auto& Vertex : VertexGroups[i]) {
			// Update min and max for each component
			MinVertex.x = std::min(MinVertex.x, Vertex.x);
			MinVertex.y = std::min(MinVertex.y, Vertex.y);
			MinVertex.z = std::min(MinVertex.z, Vertex.z);

			MaxVertex.x = std::max(MaxVertex.x, Vertex.x);
			MaxVertex.y = std::max(MaxVertex.y, Vertex.y);
			MaxVertex.z = std::max(MaxVertex.z, Vertex.z);
		}

		VertexGroupOriginalDomains[i] = std::make_pair(MinVertex, MaxVertex);
		VertexGroupDomains[i] = std::make_pair(MinVertex, MaxVertex);
		VertexGroupDomains[i].first -= glm::vec3(MaxVertexError, MaxVertexError, MaxVertexError);
		VertexGroupDomains[i].second += glm::vec3(MaxVertexError, MaxVertexError, MaxVertexError);

		auto GetExpandedDomainSpan = [](std::pair<glm::vec3, glm::vec3>& Domain)
			{
				const float F32Epsilon = std::numeric_limits<float>::epsilon();

				if (Domain.second.x - Domain.first.x == 0.0f)
				{
					Domain.first.x -= F32Epsilon;
					Domain.second.x += F32Epsilon;
				}
				if (Domain.second.y - Domain.first.y == 0.0f)
				{
					Domain.first.y -= F32Epsilon;
					Domain.second.y += F32Epsilon;
				}
				if (Domain.second.z - Domain.first.z == 0.0f)
				{
					Domain.first.z -= F32Epsilon;
					Domain.second.z += F32Epsilon;
				}
				
				glm::vec3 AbsoluteMin = Domain.first / glm::vec3(std::numeric_limits<uint8_t>::max());
				AbsoluteMin.x = std::abs(AbsoluteMin.x);
				AbsoluteMin.y = std::abs(AbsoluteMin.y);
				AbsoluteMin.z = std::abs(AbsoluteMin.z);

				glm::vec3 Expand = (Domain.second - Domain.first) / glm::vec3(std::numeric_limits<uint8_t>::max());
				Expand.x = std::max(Expand.x, AbsoluteMin.x);
				Expand.y = std::max(Expand.y, AbsoluteMin.y);
				Expand.z = std::max(Expand.z, AbsoluteMin.z);

				Domain.second += Expand;
				Domain.first += Expand;

				glm::vec3 Span = (Domain.second - Domain.first);
				return glm::vec4(Span.x, Span.y, Span.z, 1.0f);
			};

		std::pair<glm::vec3, glm::vec3> ExpandedDomain = VertexGroupDomains[i];
		glm::vec4 Span = GetExpandedDomainSpan(ExpandedDomain);
		
		glm::vec4 BitScale = glm::vec4(std::numeric_limits<uint8_t>::max()) / Span;
		glm::vec4 RangeMin = glm::vec4(ExpandedDomain.first.x, ExpandedDomain.first.y, ExpandedDomain.first.z, 1.0f) * BitScale;
		RangeMin.x = (int)std::round(RangeMin.x);
		RangeMin.y = (int)std::round(RangeMin.y);
		RangeMin.z = (int)std::round(RangeMin.z);
		RangeMin.w = (int)std::round(RangeMin.w);

		GeometrySection.m_BitOffset[0] = RangeMin.x;
		GeometrySection.m_BitOffset[1] = RangeMin.y;
		GeometrySection.m_BitOffset[2] = RangeMin.z;

		GeometrySection.m_BitScale8Inv[0] = 1.0f / BitScale.x;
		GeometrySection.m_BitScale8Inv[1] = 1.0f / BitScale.y;
		GeometrySection.m_BitScale8Inv[2] = 1.0f / BitScale.z;

		RangeMin = glm::vec4(VertexGroupOriginalDomains[i].first.x * m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16[0],
			VertexGroupOriginalDomains[i].first.y * m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16[1],
			VertexGroupOriginalDomains[i].first.z * m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16[2],
			1.0f);
		RangeMin.x = (int)std::round(RangeMin.x);
		RangeMin.y = (int)std::round(RangeMin.y);
		RangeMin.z = (int)std::round(RangeMin.z);
		RangeMin.w = (int)std::round(RangeMin.w);

		GeometrySection.m_SectionOffset[0] = (uint32_t)RangeMin.x;
		GeometrySection.m_SectionOffset[1] = (uint32_t)RangeMin.y;
		GeometrySection.m_SectionOffset[2] = (uint32_t)RangeMin.z;

		GeometrySection.m_Reserve0 = 0;

		/*
		uint32_t BVHTreeNodeMinX = (uint32_t)((float)BitScale.x * MinVertex.x) - GeometrySection.m_BitOffset[0];
		uint32_t BVHTreeNodeMaxX = (uint32_t)((float)BitScale.x * MaxVertex.x) - GeometrySection.m_BitOffset[0];

		std::cout << "Reverse MinX: " << ((BVHTreeNodeMinX + (float)GeometrySection.m_BitOffset[0]) / BitScale.x) << ", MaxX:" << ((BVHTreeNodeMaxX + (float)GeometrySection.m_BitOffset[0]) / BitScale.x) << std::endl;;

		std::cout << "asd\n";
		*/
	}

	//GeometrySection model injection
	for (size_t i = 0; i < VertexGroups.size(); i++)
	{
		hknpMeshShapeGeometrySection& GeometrySection = m_HavokMeshShape.m_GeometrySections.m_Elements[i];

		//Quantize vertices
		{
			GeometrySection.m_VertexBuffer.m_Elements.reserve(VertexGroups[i].size() + 1);
			GeometrySection.m_VertexBuffer.m_Elements.resize(VertexGroups[i].size());
			for (size_t j = 0; j < VertexGroups[i].size(); j++)
			{
				glm::vec3 ScaledFloatValue = glm::vec3((VertexGroups[i][j].x - VertexGroupOriginalDomains[i].first.x) * m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16[0],
					(VertexGroups[i][j].y - VertexGroupOriginalDomains[i].first.y) * m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16[1],
					(VertexGroups[i][j].z - VertexGroupOriginalDomains[i].first.z) * m_HavokMeshShape.m_VertexConversionUtil.m_BitScale16[2]);

				uint32_t NewVertexX = ScaledFloatValue.x;
				uint32_t NewVertexY = ScaledFloatValue.y;
				uint32_t NewVertexZ = ScaledFloatValue.z;

				hknpMeshShapeGeometrySectionVertex16_3& Vertex = GeometrySection.m_VertexBuffer.m_Elements[j];
				Vertex.m_X = *reinterpret_cast<uint16_t*>(&NewVertexX);
				Vertex.m_Y = *reinterpret_cast<uint16_t*>(&NewVertexY);
				Vertex.m_Z = *reinterpret_cast<uint16_t*>(&NewVertexZ);
			}
		}

		//Writing triangles quadish
		for (auto& Face : IndexGroups[i])
		{
			hknpMeshShapeGeometrySectionPrimitive Primitive;
			Primitive.m_IdA = std::get<0>(Face);
			Primitive.m_IdB = std::get<1>(Face);
			Primitive.m_IdC = std::get<2>(Face);
			Primitive.m_IdD = std::get<2>(Face);
			GeometrySection.m_Primitives.m_Elements.push_back(Primitive);
		}

		//Setting up top level tree
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[1 + ((int)(i / 4))].m_FourAabb.m_MinX[i % 4] = VertexGroupOriginalDomains[i].first.x;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[1 + ((int)(i / 4))].m_FourAabb.m_MinY[i % 4] = VertexGroupOriginalDomains[i].first.y;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[1 + ((int)(i / 4))].m_FourAabb.m_MinZ[i % 4] = VertexGroupOriginalDomains[i].first.z;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[1 + ((int)(i / 4))].m_FourAabb.m_MaxX[i % 4] = VertexGroupOriginalDomains[i].second.x;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[1 + ((int)(i / 4))].m_FourAabb.m_MaxY[i % 4] = VertexGroupOriginalDomains[i].second.y;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[1 + ((int)(i / 4))].m_FourAabb.m_MaxZ[i % 4] = VertexGroupOriginalDomains[i].second.z;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[1 + ((int)(i / 4))].m_Data[i % 4] = i;
		/*
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2 + ((int)(i / 4))].m_FourAabb.m_MinX[i % 4] = VertexGroupOriginalDomains[i].first.x;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2 + ((int)(i / 4))].m_FourAabb.m_MinY[i % 4] = VertexGroupOriginalDomains[i].first.y;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2 + ((int)(i / 4))].m_FourAabb.m_MinZ[i % 4] = VertexGroupOriginalDomains[i].first.z;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2 + ((int)(i / 4))].m_FourAabb.m_MaxX[i % 4] = VertexGroupOriginalDomains[i].second.x;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2 + ((int)(i / 4))].m_FourAabb.m_MaxY[i % 4] = VertexGroupOriginalDomains[i].second.y;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2 + ((int)(i / 4))].m_FourAabb.m_MaxZ[i % 4] = VertexGroupOriginalDomains[i].second.z;
		m_HavokMeshShape.m_TopLevelTree.m_Nodes.m_Elements[2 + ((int)(i / 4))].m_Data[i % 4] = 0;
		*/

		//Generating BVH
		{
			std::vector<uint32_t> ConvertedIndices;
			for (auto& Face : IndexGroups[i])
			{
				ConvertedIndices.push_back(std::get<0>(Face));
				ConvertedIndices.push_back(std::get<1>(Face));
				ConvertedIndices.push_back(std::get<2>(Face));
			}
			std::vector<BVNode> BNodes = BVNode::BuildBVHForMesh(VertexGroups[i], ConvertedIndices, ConvertedIndices.size());

			std::vector<BVNode*> BNodeLeafs;
			for (BVNode& Node : BNodes)
			{
				if (Node.mIsLeaf)
				{
					BNodeLeafs.push_back(&Node);
				}
			}

			std::vector<QuadBVHNode> BVHNodes;

			//Adding leafs
			uint32_t PrimitiveIndex = 0;
			for (size_t j = 0; j < std::ceil(BNodeLeafs.size() / 4.0f); j++)
			{
				QuadBVHNode Node;
				Node.mIsLeaf = true;

				for (int k = 0; k < 4; k++)
				{
					if (BNodeLeafs.size() > PrimitiveIndex)
					{
						Node.mMin[k] = BNodeLeafs[PrimitiveIndex]->mMin;
						Node.mMax[k] = BNodeLeafs[PrimitiveIndex]->mMax;
						Node.mPrimitiveIndices[k] = BNodeLeafs[PrimitiveIndex]->mPrimitive;
						PrimitiveIndex++;
					}
					else
					{
						Node.mMax[k] = VertexGroupOriginalDomains[i].first;
						Node.mMin[k] = VertexGroupOriginalDomains[i].second;
						Node.mPrimitiveIndices[k] = 0;
					}
				}

				BVHNodes.push_back(Node);
			}

			//Adding connectors
			uint32_t NodesToConnect = BVHNodes.size();
			uint32_t NodesToConnectBaseIndex = 0;
			uint32_t ConnectedNodes = 0;
			while (true)
			{
				for (size_t j = 0; j < std::ceil(NodesToConnect / 4.0f); j++)
				{
					QuadBVHNode QuadNode;
					QuadNode.mIsLeaf = false;

					for (int k = 0; k < 4; k++)
					{
						if (NodesToConnectBaseIndex < (NodesToConnect + ConnectedNodes))
						{
							QuadNode.mChildrenIndices[k] = NodesToConnectBaseIndex++;

							glm::vec3 MinChild(std::numeric_limits<float>::max());
							glm::vec3 MaxChild(std::numeric_limits<float>::lowest());
							for (int x = 0; x < 4; x++)
							{
								if (BVHNodes[QuadNode.mChildrenIndices[k]].mMax[x].x > BVHNodes[QuadNode.mChildrenIndices[k]].mMin[x].x &&
									BVHNodes[QuadNode.mChildrenIndices[k]].mMax[x].y > BVHNodes[QuadNode.mChildrenIndices[k]].mMin[x].y &&
									BVHNodes[QuadNode.mChildrenIndices[k]].mMax[x].z > BVHNodes[QuadNode.mChildrenIndices[k]].mMin[x].z)
								{
									MinChild.x = std::min(MinChild.x, BVHNodes[QuadNode.mChildrenIndices[k]].mMin[x].x);
									MinChild.y = std::min(MinChild.y, BVHNodes[QuadNode.mChildrenIndices[k]].mMin[x].y);
									MinChild.z = std::min(MinChild.z, BVHNodes[QuadNode.mChildrenIndices[k]].mMin[x].z);

									MaxChild.x = std::max(MaxChild.x, BVHNodes[QuadNode.mChildrenIndices[k]].mMax[x].x);
									MaxChild.y = std::max(MaxChild.y, BVHNodes[QuadNode.mChildrenIndices[k]].mMax[x].y);
									MaxChild.z = std::max(MaxChild.z, BVHNodes[QuadNode.mChildrenIndices[k]].mMax[x].z);
								}
							}

							QuadNode.mMin[k] = MinChild;
							QuadNode.mMax[k] = MaxChild;
						}
						else
						{
							QuadNode.mMin[k] = VertexGroupOriginalDomains[i].second;
							QuadNode.mMax[k] = VertexGroupOriginalDomains[i].first;
						}
					}

					BVHNodes.push_back(QuadNode);
				}

				ConnectedNodes = NodesToConnectBaseIndex;
				NodesToConnect = BVHNodes.size() - NodesToConnectBaseIndex;

				if (NodesToConnect == 1)
				{
					break;
				}
			}

			std::vector<QuadBVHNode> OrderedNodes;
			for (std::vector<QuadBVHNode>::reverse_iterator RIter = BVHNodes.rbegin();
				RIter != BVHNodes.rend(); ++RIter)
			{
				QuadBVHNode QuadNode = *RIter;
				if (!QuadNode.mIsLeaf)
				{
					for (int j = 0; j < 4; j++)
					{
						if (QuadNode.mMax[j].x > QuadNode.mMin[j].x && QuadNode.mMax[j].y > QuadNode.mMin[j].y && QuadNode.mMax[j].z > QuadNode.mMin[j].z)
						{
							QuadNode.mChildrenIndices[j] = BVHNodes.size() - 1 - QuadNode.mChildrenIndices[j];
						}
					}
				}
				OrderedNodes.push_back(QuadNode);
			}

			if (OrderedNodes.size() > 255)
			{
				Logger::Error("PhiveShape2", "SectionBVH overflow, " + std::to_string(OrderedNodes.size()) + " nodes, max 255");
			}

			GeometrySection.InjectBVH(OrderedNodes, VertexGroupDomains[i].first, VertexGroupDomains[i].second);
		}

		//Clearing bvh - testing
		{
			GeometrySection.m_SectionBvh.m_Elements.resize(1);
		}

		//Clearing interior primitive bitfield
		GeometrySection.m_InteriorPrimitiveBitField.m_Elements.resize((int)std::ceil(IndexGroups[i].size() / 8));
		for (hknpMeshShapeGeometrySectionInteriorPrimitiveBitField& Byte : GeometrySection.m_InteriorPrimitiveBitField.m_Elements)
		{
			Byte.m_BitField = 0;
		}
	}
	
	/*
	std::vector<float> DebugVertices = ToVertices();
	std::vector<uint32_t> DebugFaces = ToIndices();

	UIMapView::mDebugMeshes.emplace_back(DebugVertices, DebugFaces);
	*/
	/*
	int DomainIndex = 0;
	for (PhiveShape2::hknpMeshShapeGeometrySection& GeometrySection : GetMeshShape().m_GeometrySections.m_Elements)
	{
		DecodeTreeNode(GeometrySection, GeometrySection.m_SectionBvh.m_Elements[0], VertexGroupOriginalDomains[DomainIndex].first, VertexGroupOriginalDomains[DomainIndex].second);
		DomainIndex++;
	}
	*/

	Logger::Info("PhiveShape2", "Model injected");
}

PhiveShape2::PhiveShape2(std::vector<unsigned char> Bytes)
{
	BinaryVectorReader Reader(Bytes);

	Reader.ReadStruct(&m_PhiveShapeHeader, sizeof(PhiveShapeHeader));
	ReadHavokTagFileHeader(Reader);

	//Havok
	m_HavokMeshShape.Read(Reader);

	//Phive
	ReadPhiveMaterials(Reader);
	Reader.Seek(FindSection(Reader, "TST1"), BinaryVectorReader::Position::Begin);
	ReadStringTable(Reader, m_TagStringTable);
	Reader.Seek(FindSection(Reader, "FST1"), BinaryVectorReader::Position::Begin);
	ReadStringTable(Reader, m_FieldStringTable);
	Reader.Seek(FindSection(Reader, "THSH"), BinaryVectorReader::Position::Begin);
	ReadTagHash(Reader);
	Reader.Seek(FindSection(Reader, "TNA1"), BinaryVectorReader::Position::Begin);
	ReadTagNameMapping(Reader);
	Reader.Seek(FindSection(Reader, "TBDY"), BinaryVectorReader::Position::Begin);
	ReadTagBody(Reader);
}