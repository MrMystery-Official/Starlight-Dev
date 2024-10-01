#include "PhiveShape.h"

#include "Logger.h"
#include <iostream> //debug
#include "Vector3F.h"
#include <fstream>
#include "Editor.h"
#include "ZStdFile.h"

void PhiveShape::AlignWriter(BinaryVectorWriter& Writer, uint32_t Alignment, uint8_t Aligner)
{
	while (Writer.GetPosition() % Alignment != 0)
		Writer.WriteByte(Aligner);
}

//WritePackedInt taken from https://github.com/blueskythlikesclouds/TagTools/blob/havoc/Havoc/Extensions/BinaryWriterEx.cs
void PhiveShape::WritePackedInt(BinaryVectorWriter& Writer, int64_t Value)
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
int64_t PhiveShape::ReadPackedInt(BinaryVectorReader& Reader)
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


//Data
template<typename T>
void PhiveShape::hkRelArrayView<T>::Read(BinaryVectorReader& Reader)
{
	std::cout << "POS3: " << Reader.GetPosition() << std::endl;
	this->Offset = Reader.ReadInt32();
	this->Size = Reader.ReadInt32();
	uint32_t Jumback = Reader.GetPosition();
	if (this->Offset > 0)
	{
		Reader.Seek(this->Offset - 8, BinaryVectorReader::Position::Current);
		this->Elements.resize(this->Size);
		for (int i = 0; i < this->Size; i++)
		{
			T Object;
			if constexpr (std::is_integral_v<T>)
			{
				Reader.ReadStruct(&Object, sizeof(T));
				this->Elements[i] = Object;
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
			this->Elements[i] = Object;
		}
	}
	Reader.Seek(Jumback, BinaryVectorReader::Position::Begin);
}
template<typename T>
void PhiveShape::hkRelArrayView<T>::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	if (Offset == -1) std::cerr << "OFFSET -1\n";
	if (this->BaseOffset > 0) Writer.Seek(this->BaseOffset, BinaryVectorWriter::Position::Begin);

	Writer.WriteInteger(Offset - Writer.GetPosition(), sizeof(uint32_t));
	Writer.WriteInteger(this->Elements.size(), sizeof(uint32_t));

	uint32_t Jumpback = Writer.GetPosition();

	Writer.Seek(Offset, BinaryVectorWriter::Position::Begin);
	for (T& Element : this->Elements)
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
	this->LastElementOffset = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
}
template<typename T>
void PhiveShape::hkRelArrayView<T>::Skip(BinaryVectorWriter& Writer)
{
	this->BaseOffset = Writer.GetPosition();
	Writer.Seek(8, BinaryVectorWriter::Position::Current);
}

template<typename T>
void PhiveShape::hkRelArrayViewVertexBuffer<T>::Read(BinaryVectorReader& Reader)
{
	this->Offset = Reader.ReadInt32();
	this->Size = Reader.ReadInt32() / 6;
	uint32_t Jumback = Reader.GetPosition();
	if (this->Offset > 0)
	{
		Reader.Seek(this->Offset - 8, BinaryVectorReader::Position::Current);
		this->Elements.resize(this->Size);
		for (int i = 0; i < this->Size; i++)
		{
			T Object;
			if constexpr (std::is_integral_v<T>)
			{
				Reader.ReadStruct(&Object, sizeof(T));
				this->Elements[i] = Object;
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
			this->Elements[i] = Object;
		}
	}
	Reader.Seek(Jumback, BinaryVectorReader::Position::Begin);
}
template<typename T>
void PhiveShape::hkRelArrayViewVertexBuffer<T>::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	if (Offset == -1) std::cerr << "OFFSET -1\n";
	if (this->BaseOffset > 0) Writer.Seek(this->BaseOffset, BinaryVectorWriter::Position::Begin);

	Writer.WriteInteger(Offset - Writer.GetPosition(), sizeof(uint32_t));
	Writer.WriteInteger(this->Elements.size() * 6, sizeof(uint32_t));

	uint32_t Jumpback = Writer.GetPosition();

	Writer.Seek(Offset, BinaryVectorWriter::Position::Begin);
	for (T& Element : this->Elements)
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
	this->LastElementOffset = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
}
template<typename T>
void PhiveShape::hkRelArrayViewVertexBuffer<T>::Skip(BinaryVectorWriter& Writer)
{
	this->BaseOffset = Writer.GetPosition();
	Writer.Seek(8, BinaryVectorWriter::Position::Current);
}

template<typename T>
void PhiveShape::hkRelPtr<T>::Read(BinaryVectorReader& Reader)
{
	this->Offset = Reader.ReadInt64();
	uint32_t Jumback = Reader.GetPosition();
	if (this->Offset > 0)
	{
		Reader.Seek(this->Offset, BinaryVectorReader::Position::Current);
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
		this->Data = *reinterpret_cast<T*>(BaseObj);
	}
	Reader.Seek(Jumback, BinaryVectorReader::Position::Begin);
}
template<typename T>
void PhiveShape::hkRelPtr<T>::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	if (Offset == -1) std::cerr << "OFFSET -1\n";
	if (this->BaseOffset > 0) Writer.Seek(this->BaseOffset, BinaryVectorWriter::Position::Begin);

	Writer.WriteInteger((uint64_t)(Offset - Writer.GetPosition()), sizeof(uint64_t));

	uint32_t Jumpback = Writer.GetPosition();

	Writer.Seek(Offset, BinaryVectorWriter::Position::Begin);
	hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&this->Data);
	if (BaseObj)
	{
		BaseObj->Write(Writer);
	}
	else
	{
		std::cout << "Cannot cast to hkReadableWriteableObject" << std::endl;
	}
	this->LastElementOffset = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
}
template<typename T>
void PhiveShape::hkRelPtr<T>::Skip(BinaryVectorWriter& Writer)
{
	this->BaseOffset = Writer.GetPosition();
	Writer.Seek(8, BinaryVectorWriter::Position::Current);
}

template<typename T>
void PhiveShape::hkRelArray<T>::Read(BinaryVectorReader& Reader)
{
	std::cout << "POS: " << Reader.GetPosition() << std::endl;
	this->Offset = Reader.ReadUInt64();
	this->Size = Reader.ReadInt32();
	this->CapacityAndFlags = Reader.ReadInt32();
	uint32_t Jumback = Reader.GetPosition();
	std::cout << "JP: " << Jumback << std::endl;
	if (this->Offset > 0)
	{
		Reader.Seek(this->Offset - 16, BinaryVectorReader::Position::Current);
		this->Elements.resize(this->Size);
		for (int i = 0; i < this->Size; i++)
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
			this->Elements[i] = Object;
		}
	}
	Reader.Seek(Jumback, BinaryVectorReader::Position::Begin);
}
template<typename T>
void PhiveShape::hkRelArray<T>::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	if (Offset == -1) std::cerr << "OFFSET -1\n";
	if (this->BaseOffset > 0) Writer.Seek(this->BaseOffset, BinaryVectorWriter::Position::Begin);

	Writer.WriteInteger(!this->Elements.empty() ? (uint64_t)(Offset - Writer.GetPosition()) : 0, sizeof(uint64_t));
	Writer.WriteInteger((int32_t)this->Elements.size(), sizeof(int32_t));
	Writer.WriteInteger((int32_t)this->CapacityAndFlags, sizeof(int32_t));

	uint32_t Jumpback = Writer.GetPosition();

	Writer.Seek(Offset, BinaryVectorWriter::Position::Begin);
	for (T& Element : this->Elements)
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
	this->LastElementOffset = Writer.GetPosition();
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
}
template<typename T>
void PhiveShape::hkRelArray<T>::Skip(BinaryVectorWriter& Writer)
{
	this->BaseOffset = Writer.GetPosition();
	Writer.Seek(16, BinaryVectorWriter::Position::Current);
}

void PhiveShape::hkBaseObject::Read(BinaryVectorReader& Reader)
{
	this->VTFReserve = Reader.ReadUInt64();
}
void PhiveShape::hkBaseObject::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	Writer.WriteInteger(this->VTFReserve, sizeof(uint64_t));
}

void PhiveShape::hkReferencedObject::Read(BinaryVectorReader& Reader)
{
	this->BaseObject.Read(Reader);
	this->SizeAndFlags = Reader.ReadUInt64();
	this->RefCount = Reader.ReadUInt64();
}
void PhiveShape::hkReferencedObject::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->BaseObject.Write(Writer);
	Writer.WriteInteger(this->SizeAndFlags, sizeof(uint64_t));
	Writer.WriteInteger(this->RefCount, sizeof(uint64_t));
}

void PhiveShape::hknpShapePropertyBase::Read(BinaryVectorReader& Reader)
{
	this->ReferencedObject.Read(Reader);
	this->PropertyKey = Reader.ReadUInt16();
}
void PhiveShape::hknpShapePropertyBase::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->ReferencedObject.Write(Writer);
	Writer.WriteInteger(this->PropertyKey, sizeof(uint16_t));
}

void PhiveShape::hknpShapePropertiesEntry::Read(BinaryVectorReader& Reader)
{
	this->Object.Read(Reader);
}
void PhiveShape::hknpShapePropertiesEntry::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->Object.Write(Writer, Writer.GetPosition() + 8);
}

void PhiveShape::hknpShapeProperties::Read(BinaryVectorReader& Reader)
{
	this->Properties.Read(Reader);
}
void PhiveShape::hknpShapeProperties::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->Properties.Write(Writer, Writer.GetPosition() + 16);
}

void PhiveShape::hknpShape::Read(BinaryVectorReader& Reader)
{
	this->ReferencedObject.Read(Reader);
	this->Type = static_cast<PhiveShape::hknpShapeTypeEnum>(Reader.ReadUInt8());
	this->DispatchType = static_cast<PhiveShape::hknpCollisionDispatchTypeEnum>(Reader.ReadUInt8());
	this->Flags = static_cast<PhiveShape::hknpShapeFlagsEnum>(Reader.ReadUInt16());

	this->NumShapeKeyBits = Reader.ReadUInt8();
	std::cout << "POS2: " << Reader.GetPosition() << std::endl;
	Reader.ReadStruct(&this->Reserve0, 3);
	this->ConvexRadius = Reader.ReadFloat();
	this->Reserve1 = Reader.ReadUInt32();
	this->UserData = Reader.ReadUInt64();
	this->Properties.Read(Reader);
}
void PhiveShape::hknpShape::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->ReferencedObject.Write(Writer);
	Writer.WriteInteger((uint8_t)this->Type, sizeof(uint8_t));
	Writer.WriteInteger((uint8_t)this->DispatchType, sizeof(uint8_t));
	Writer.WriteInteger((uint16_t)this->Flags, sizeof(uint16_t));

	Writer.WriteInteger(this->NumShapeKeyBits, sizeof(uint8_t));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(this->Reserve0), 3);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&this->ConvexRadius), sizeof(float));
	Writer.WriteInteger(this->Reserve1, sizeof(uint32_t));
	Writer.WriteInteger(this->UserData, sizeof(uint64_t));
	this->Properties.Write(Writer);
}

void PhiveShape::hknpCompositeShape::Read(BinaryVectorReader& Reader)
{
	this->Shape.Read(Reader);
	this->ShapeTagCodecInfo = Reader.ReadUInt32();
	this->Reserve2 = Reader.ReadUInt32();
	this->Reserve3 = Reader.ReadUInt64();
}
void PhiveShape::hknpCompositeShape::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->Shape.Write(Writer);
	Writer.WriteInteger(this->ShapeTagCodecInfo, sizeof(uint32_t));
	Writer.WriteInteger(this->Reserve2, sizeof(uint32_t));
	Writer.WriteInteger(this->Reserve3, sizeof(uint64_t));
}

void PhiveShape::hknpMeshShapeVertexConversionUtil::Read(BinaryVectorReader& Reader)
{
	Reader.ReadStruct(this->BitScale16, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(this->BitScale16Inv, 4 * 4); //4 * sizeof(float)
}
void PhiveShape::hknpMeshShapeVertexConversionUtil::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&this->BitScale16[0]), 4 * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&this->BitScale16Inv[0]), 4 * 4);
}

void PhiveShape::hknpMeshShapeShapeTagTableEntry::Read(BinaryVectorReader& Reader)
{
	this->MeshPrimitiveKey = Reader.ReadUInt32();
	this->ShapeTag = Reader.ReadUInt16();
	this->Reserve0 = Reader.ReadUInt16();
}
void PhiveShape::hknpMeshShapeShapeTagTableEntry::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	Writer.WriteInteger(this->MeshPrimitiveKey, sizeof(uint32_t));
	Writer.WriteInteger(this->ShapeTag, sizeof(uint16_t));
	Writer.WriteInteger(this->Reserve0, sizeof(uint16_t));
}

void PhiveShape::hkcdFourAabb::Read(BinaryVectorReader& Reader)
{
	Reader.ReadStruct(this->MinX, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(this->MaxX, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(this->MinY, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(this->MaxY, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(this->MinZ, 4 * 4); //4 * sizeof(float)
	Reader.ReadStruct(this->MaxZ, 4 * 4); //4 * sizeof(float)
}
void PhiveShape::hkcdFourAabb::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	std::cout << "AABBPOS: " << Writer.GetPosition() << std::endl;

	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&this->MinX[0]), 4 * sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&this->MaxX[0]), 4 * sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&this->MinY[0]), 4 * sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&this->MaxY[0]), 4 * sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&this->MinZ[0]), 4 * sizeof(float));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&this->MaxZ[0]), 4 * sizeof(float));
}

void PhiveShape::hkcdSimdTreeNode::Read(BinaryVectorReader& Reader)
{
	this->FourAabb.Read(Reader);
	Reader.ReadStruct(this->Data, 4 * 4); //4 * sizeof(uint32_t)
	this->IsLeaf = Reader.ReadUInt8();
	this->IsActive = Reader.ReadUInt8();
	this->Reserve0 = Reader.ReadUInt16();
	this->Reserve1 = Reader.ReadUInt32();
	this->Reserve2 = Reader.ReadUInt64();
}
void PhiveShape::hkcdSimdTreeNode::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->FourAabb.Write(Writer);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&this->Data[0]), 4 * 4);
	Writer.WriteInteger(this->IsLeaf, sizeof(uint8_t));
	Writer.WriteInteger(this->IsActive, sizeof(uint8_t));
	Writer.WriteInteger(this->Reserve0, sizeof(uint16_t));
	Writer.WriteInteger(this->Reserve1, sizeof(uint32_t));
	Writer.WriteInteger(this->Reserve2, sizeof(uint64_t));
}

void PhiveShape::hkcdSimdTree::Read(BinaryVectorReader& Reader)
{
	std::cout << "RELARRAY: " << Reader.GetPosition() << std::endl;
	this->Nodes.Read(Reader);
	this->IsCompact = Reader.ReadUInt8();
	Reader.ReadStruct(this->Reserve0, 3); //3 * sizeof(uint8_t)
	this->Reserve1 = Reader.ReadUInt32();
}
void PhiveShape::hkcdSimdTree::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->Nodes.Skip(Writer);
	Writer.WriteInteger(this->IsCompact, sizeof(uint8_t));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(this->Reserve0), 3);
	Writer.WriteInteger(this->Reserve1, sizeof(uint32_t));
}

void PhiveShape::hknpTransposedFourAabbs8::Read(BinaryVectorReader& Reader)
{
	this->MinX = Reader.ReadUInt32();
	this->MaxX = Reader.ReadUInt32();
	this->MinY = Reader.ReadUInt32();
	this->MaxY = Reader.ReadUInt32();
	this->MinZ = Reader.ReadUInt32();
	this->MaxZ = Reader.ReadUInt32();
}
void PhiveShape::hknpTransposedFourAabbs8::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	Writer.WriteInteger(this->MinX, sizeof(uint32_t));
	Writer.WriteInteger(this->MaxX, sizeof(uint32_t));
	Writer.WriteInteger(this->MinY, sizeof(uint32_t));
	Writer.WriteInteger(this->MaxY, sizeof(uint32_t));
	Writer.WriteInteger(this->MinZ, sizeof(uint32_t));
	Writer.WriteInteger(this->MaxZ, sizeof(uint32_t));
}

void PhiveShape::hknpAabb8TreeNode::Read(BinaryVectorReader& Reader)
{
	this->TransposedFourAabbs8.Read(Reader);
	Reader.ReadStruct(Data, 4); //4 * sizeof(uint8_t)
}
void PhiveShape::hknpAabb8TreeNode::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->TransposedFourAabbs8.Write(Writer);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(this->Data), 4);
}

void PhiveShape::hknpMeshShapeGeometrySectionPrimitive::Read(BinaryVectorReader& Reader)
{
	this->IdA = Reader.ReadUInt8();
	this->IdB = Reader.ReadUInt8();
	this->IdC = Reader.ReadUInt8();
	this->IdD = Reader.ReadUInt8();
}
void PhiveShape::hknpMeshShapeGeometrySectionPrimitive::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	Writer.WriteInteger(this->IdA, sizeof(uint8_t));
	Writer.WriteInteger(this->IdB, sizeof(uint8_t));
	Writer.WriteInteger(this->IdC, sizeof(uint8_t));
	Writer.WriteInteger(this->IdD, sizeof(uint8_t));
}

void PhiveShape::hknpMeshShapeGeometrySectionVertex16_3::Read(BinaryVectorReader& Reader)
{
	this->X = Reader.ReadUInt16();
	this->Y = Reader.ReadUInt16();
	this->Z = Reader.ReadUInt16();
}
void PhiveShape::hknpMeshShapeGeometrySectionVertex16_3::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	Writer.WriteInteger(this->X, sizeof(uint16_t));
	Writer.WriteInteger(this->Y, sizeof(uint16_t));
	Writer.WriteInteger(this->Z, sizeof(uint16_t));
}

void PhiveShape::ParseTreeNode(std::vector<uint32_t>& TreeNodeEntryIndices, hknpMeshShapeGeometrySection& Section, uint32_t Index, bool BlockChild)
{
	hknpAabb8TreeNode Node = Section.SectionBvh.Elements[Index];
	EditorTreeNode EditorNode;

	Section.BVHTree.push_back(EditorNode);

	if (std::find(TreeNodeEntryIndices.begin(), TreeNodeEntryIndices.end(), Node.Data[0]) == TreeNodeEntryIndices.end() && !BlockChild && (Index + Node.Data[0] * 2) < Section.SectionBvh.Elements.size())
	{
		std::cout << "CHILD##########################################\n";
		TreeNodeEntryIndices.push_back(Node.Data[0]);
		ParseTreeNode(TreeNodeEntryIndices, Section, Index + 1);
		EditorNode.ChildA = Section.BVHTree.size() - 1;

		ParseTreeNode(TreeNodeEntryIndices, Section, Index + Node.Data[0] * 2);
		EditorNode.ChildB = Section.BVHTree.size() - 1;

		Section.BVHTree.pop_back();
		Section.BVHTree.push_back(EditorNode);
	}
}

void PhiveShape::hknpMeshShapeGeometrySection::Read(BinaryVectorReader& Reader)
{
	this->SectionBvh.Read(Reader);
	this->Primitives.Read(Reader);
	this->VertexBuffer.Read(Reader);
	this->InteriorPrimitiveBitField.Read(Reader);

	Reader.ReadStruct(this->SectionOffset, 3 * 4); //3 * sizeof(uint32_t)
	Reader.ReadStruct(this->BitScale8Inv, 3 * 4); //3 * sizeof(float)
	Reader.ReadStruct(this->BitOffset, 3 * 2); //3 * sizeof(int16_t)
	this->Reserve0 = Reader.ReadUInt16();

	//BVHTree
	std::vector<uint32_t> TreeNodeEntryIndices;
	PhiveShape::ParseTreeNode(TreeNodeEntryIndices, *this, 0);
}
void PhiveShape::hknpMeshShapeGeometrySection::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->SectionBvh.Skip(Writer);
	this->Primitives.Skip(Writer);
	this->VertexBuffer.Skip(Writer);
	this->InteriorPrimitiveBitField.Skip(Writer);

	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&this->SectionOffset[0]), 3 * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&this->BitScale8Inv[0]), 3 * 4);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&this->BitOffset[0]), 3 * 2);
	Writer.WriteInteger(this->Reserve0, sizeof(uint16_t));

	this->SectionBvhOffset = Writer.GetPosition();
	this->SectionBvh.Write(Writer, Writer.GetPosition());
	Writer.Seek(this->SectionBvh.LastElementOffset, BinaryVectorWriter::Position::Begin);
	AlignWriter(Writer, 16);

	this->PrimitivesOffset = Writer.GetPosition();
	this->Primitives.Write(Writer, Writer.GetPosition());
	Writer.Seek(this->Primitives.LastElementOffset, BinaryVectorWriter::Position::Begin);
	AlignWriter(Writer, 16);

	this->VertexBufferOffset = Writer.GetPosition();
	this->VertexBuffer.Write(Writer, Writer.GetPosition());
	Writer.Seek(this->VertexBuffer.LastElementOffset + 2, BinaryVectorWriter::Position::Begin);
	AlignWriter(Writer, 16);

	this->InteriorPrimitiveBitFieldOffset = Writer.GetPosition();
	this->InteriorPrimitiveBitField.Write(Writer, Writer.GetPosition());
}

void PhiveShape::hknpMeshShapePrimitiveMapping::Read(BinaryVectorReader& Reader)
{
	this->ReferencedObject.Read(Reader);
	this->SectionStart.Read(Reader);
	this->BitString.Read(Reader);
	this->BitsPerEntry = Reader.ReadUInt32();
	this->TriangleIndexBitMask = Reader.ReadUInt32();
}
void PhiveShape::hknpMeshShapePrimitiveMapping::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->ReferencedObject.Write(Writer);
	this->SectionStart.Skip(Writer);
	this->BitString.Skip(Writer);
	Writer.WriteInteger(this->BitsPerEntry, sizeof(uint32_t));
	Writer.WriteInteger(this->TriangleIndexBitMask, sizeof(uint32_t));

	this->SectionStart.Write(Writer, Writer.GetPosition());
	Writer.Seek(this->SectionStart.LastElementOffset, BinaryVectorWriter::Position::Begin);

	this->BitString.Write(Writer, Writer.GetPosition());
}

void PhiveShape::hknpMeshShape::Read(BinaryVectorReader& Reader)
{
	this->CompositeShape.Read(Reader);
	this->VertexConversionUtil.Read(Reader);
	this->ShapeTagTable.Read(Reader);
	this->TopLevelTree.Read(Reader);
	this->GeometrySections.Read(Reader);
	this->PrimitiveMapping.Read(Reader);
}
void PhiveShape::hknpMeshShape::Write(BinaryVectorWriter& Writer, int32_t Offset)
{
	this->CompositeShape.Write(Writer);
	this->VertexConversionUtil.Write(Writer);
	this->ShapeTagTable.Skip(Writer);
	this->TopLevelTree.Write(Writer);
	this->GeometrySections.Skip(Writer);
	this->PrimitiveMapping.Skip(Writer);

	this->ShapeTagTableOffset = Writer.GetPosition();
	this->ShapeTagTable.Write(Writer, Writer.GetPosition());
	Writer.Seek(this->ShapeTagTable.LastElementOffset, BinaryVectorWriter::Position::Begin);
	AlignWriter(Writer, 16);

	this->TopLevelTreeOffset = Writer.GetPosition();
	this->TopLevelTree.Nodes.Write(Writer, Writer.GetPosition());
	Writer.Seek(this->TopLevelTree.Nodes.LastElementOffset, BinaryVectorWriter::Position::Begin);
	AlignWriter(Writer, 16);

	/*

	this->GeometrySectionsOffset = Writer.GetPosition();
	this->GeometrySections.Write(Writer, Writer.GetPosition());
	Writer.Seek(this->GeometrySections.Elements.back().InteriorPrimitiveBitField.LastElementOffset, BinaryVectorWriter::Position::Begin);
	AlignWriter(Writer, 16);

	*/

	std::vector<uint32_t> GeometrySectionBaseOffsets;

	for (hknpMeshShapeGeometrySection& Section : GeometrySections.Elements)
	{
		GeometrySectionBaseOffsets.push_back(Writer.GetPosition());
		Writer.Seek(32, BinaryVectorWriter::Position::Current);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Section.SectionOffset[0]), 3 * 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Section.BitScale8Inv[0]), 3 * 4);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Section.BitOffset[0]), 3 * 2);
		Writer.WriteInteger(Section.Reserve0, sizeof(uint16_t));
	}

	for (int i = 0; i < GeometrySections.Elements.size(); i++)
	{
		uint32_t SectionOffset = Writer.GetPosition();
		for (hknpAabb8TreeNode& Node : GeometrySections.Elements[i].SectionBvh.Elements)
		{
			Node.Write(Writer);
		}
		AlignWriter(Writer, 16);
		uint32_t Jumpback = Writer.GetPosition();
		Writer.Seek(GeometrySectionBaseOffsets[i], BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(SectionOffset - Writer.GetPosition(), sizeof(uint32_t));
		Writer.WriteInteger(GeometrySections.Elements[i].SectionBvh.Elements.size(), sizeof(uint32_t));
		Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	}

	for (int i = 0; i < GeometrySections.Elements.size(); i++)
	{
		uint32_t SectionOffset = Writer.GetPosition();

		for (hknpMeshShapeGeometrySectionPrimitive& Primitive : GeometrySections.Elements[i].Primitives.Elements)
		{
			Primitive.Write(Writer);
		}

		AlignWriter(Writer, 16);
		uint32_t Jumpback = Writer.GetPosition();
		Writer.Seek(GeometrySectionBaseOffsets[i] + 8, BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(SectionOffset - Writer.GetPosition(), sizeof(uint32_t));
		Writer.WriteInteger(GeometrySections.Elements[i].Primitives.Elements.size(), sizeof(uint32_t));
		Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	}

	for (int i = 0; i < GeometrySections.Elements.size(); i++)
	{
		uint32_t SectionOffset = Writer.GetPosition();

		for (hknpMeshShapeGeometrySectionVertex16_3& Vertex : GeometrySections.Elements[i].VertexBuffer.Elements)
		{
			Vertex.Write(Writer);
		}

		AlignWriter(Writer, 16);
		uint32_t Jumpback = Writer.GetPosition();
		Writer.Seek(GeometrySectionBaseOffsets[i] + 16, BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(SectionOffset - Writer.GetPosition(), sizeof(uint32_t));
		Writer.WriteInteger(GeometrySections.Elements[i].VertexBuffer.Elements.size() * 6, sizeof(uint32_t));
		Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	}

	for (int i = 0; i < GeometrySections.Elements.size(); i++)
	{
		uint32_t SectionOffset = Writer.GetPosition();

		for (uint8_t BitField : GeometrySections.Elements[i].InteriorPrimitiveBitField.Elements)
		{
			Writer.WriteInteger(BitField, sizeof(uint8_t));
		}

		AlignWriter(Writer, 16);
		uint32_t Jumpback = Writer.GetPosition();
		Writer.Seek(GeometrySectionBaseOffsets[i] + 24, BinaryVectorWriter::Position::Begin);
		Writer.WriteInteger(SectionOffset - Writer.GetPosition(), sizeof(uint32_t));
		Writer.WriteInteger(GeometrySections.Elements[i].InteriorPrimitiveBitField.Elements.size(), sizeof(uint32_t));
		Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	}

	uint32_t Jumpback = Writer.GetPosition();
	Writer.Seek(0xE0, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(GeometrySectionBaseOffsets[0] - Writer.GetPosition(), sizeof(uint32_t));
	Writer.WriteInteger(GeometrySections.Elements.size(), sizeof(uint32_t));
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);

	//this->PrimitiveMapping.Write(Writer, Writer.GetPosition());
	//Writer.Seek(this->PrimitiveMapping.LastElementOffset, BinaryVectorWriter::Position::Begin);
}

void PhiveShape::WriteTNASection(BinaryVectorWriter& Writer)
{
	uint32_t Jumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current); //Size

	Writer.WriteBytes("TNA1");
	WritePackedInt(Writer, this->m_TNAItems.size());

	for (PhiveShape::TNAItem& TItem : this->m_TNAItems)
	{
		WritePackedInt(Writer, TItem.Index);
		WritePackedInt(Writer, TItem.TemplateCount);

		for (PhiveShape::TNAItem::ParameterStruct& Param : TItem.Parameters)
		{
			WritePackedInt(Writer, Param.Index);
			WritePackedInt(Writer, Param.Value);
		}
	}

	AlignWriter(Writer, 4);

	uint32_t End = Writer.GetPosition();

	uint32_t Size = Writer.GetPosition() - Jumpback;
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	Size = _byteswap_ulong(Size);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Size), 4);
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteByte(0x40); //Flag

	Writer.Seek(End, BinaryVectorWriter::Position::Begin);
}

void PhiveShape::ReadTNASection(BinaryVectorReader Reader, bool Splatoon)
{
	uint32_t Size = BitFieldToSize(Reader.ReadUInt32(true));
	uint32_t TargetAddress = Reader.GetPosition() + Size - 4; //-4 = Size(3) + Flags(1)

	Reader.Seek(4, BinaryVectorReader::Position::Current); //Magic

	uint32_t ElementCount = ReadPackedInt(Reader);

	m_TNAItems.resize(ElementCount);

	for (int i = 0; i < ElementCount; i++)
	{
		m_TNAItems[i].Index = ReadPackedInt(Reader);
		m_TNAItems[i].TemplateCount = ReadPackedInt(Reader);
		m_TNAItems[i].Tag = m_TagStringTable[m_TNAItems[i].Index];

		if (!Splatoon)
		{
			m_TNAItems[i].Parameters.resize(m_TNAItems[i].TemplateCount);
			for (int j = 0; j < m_TNAItems[i].TemplateCount; j++)
			{
				m_TNAItems[i].Parameters[j].Index = ReadPackedInt(Reader);
				m_TNAItems[i].Parameters[j].Value = ReadPackedInt(Reader);
				m_TNAItems[i].Parameters[j].Parameter = m_TagStringTable[m_TNAItems[i].Parameters[j].Index];

				if (m_TNAItems[i].Parameters[j].Parameter.at(0) == 't')
				{
					m_TNAItems[i].Parameters[j].ValueString = m_TagStringTable[m_TNAItems[i].Parameters[j].Value - 1];
				}
			}
		}
	}
}

uint32_t PhiveShape::FindSection(BinaryVectorReader Reader, std::string Section)
{
	Reader.Seek(0, BinaryVectorReader::Position::Begin);
	char Data[4] = { 0 };
	while (!(Data[0] == Section.at(0) && Data[1] == Section.at(1) && Data[2] == Section.at(2) && Data[3] == Section.at(3)))
	{
		Reader.ReadStruct(Data, 4);
	}
	return Reader.GetPosition();
}

inline uint32_t PhiveShape::BitFieldToSize(uint32_t Input)
{
	return Input & 0x3FFFFFFF;
}

void PhiveShape::ReadStringTable(BinaryVectorReader Reader, std::vector<std::string>* Dest)
{
	uint32_t Size = BitFieldToSize(Reader.ReadUInt32(true));
	uint32_t TargetAddress = Reader.GetPosition() + Size - 4; //-4 = Size(3) + Flags(1)

	Reader.Seek(4, BinaryVectorReader::Position::Current); //Magic

	std::string Text = "";
	while (Reader.GetPosition() != TargetAddress)
	{
		char Character = Reader.ReadUInt8();
		if (Character == 0x00)
		{
			Dest->push_back(Text);
			Text.clear();
			continue;
		}

		Text += Character;
	}
}

PhiveShape::hknpMeshShape& PhiveShape::GetMeshShape()
{
	return MeshShape;
}

uint32_t PhiveShape::WriteItemSection(BinaryVectorWriter& Writer)
{
	if (PhiveItemSection.empty())
	{
		uint32_t JumpbackIndx = Writer.GetPosition();
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x7C);
		Writer.WriteBytes("INDX");

		uint32_t JumbackItem = Writer.GetPosition();
		Writer.WriteByte(0x40);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x74);
		Writer.WriteBytes("ITEM");

		Writer.Seek(12, BinaryVectorWriter::Position::Current); //Default section

		//hknpMeshShape
		Writer.WriteByte(0x01);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x10);
		Writer.WriteInteger(0, sizeof(uint32_t));
		Writer.WriteInteger(1, sizeof(uint32_t));

		//ShapeTagTable
		Writer.WriteByte(0x05);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x20);
		Writer.WriteInteger(MeshShape.ShapeTagTableOffset - 0x50, sizeof(uint32_t));
		Writer.WriteInteger(MeshShape.ShapeTagTable.Elements.size(), sizeof(uint32_t));

		//NodeTree
		Writer.WriteByte(0x10);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x20);
		Writer.WriteInteger(MeshShape.TopLevelTreeOffset - 0x50, sizeof(uint32_t));
		Writer.WriteInteger(MeshShape.TopLevelTree.Nodes.Elements.size(), sizeof(uint32_t));

		//GeometrySection
		Writer.WriteByte(0x09);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x20);
		Writer.WriteInteger(MeshShape.GeometrySectionsOffset - 0x50, sizeof(uint32_t));
		Writer.WriteInteger(MeshShape.GeometrySections.Elements.size(), sizeof(uint32_t));

		//Aabb8TreeNode
		Writer.WriteByte(0x2D);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x20);
		Writer.WriteInteger(MeshShape.GeometrySections.Elements.back().SectionBvhOffset - 0x50, sizeof(uint32_t));
		Writer.WriteInteger(MeshShape.GeometrySections.Elements.back().SectionBvh.Elements.size(), sizeof(uint32_t));

		//Primitives
		Writer.WriteByte(0x2F);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x20);
		Writer.WriteInteger(MeshShape.GeometrySections.Elements.back().PrimitivesOffset - 0x50, sizeof(uint32_t));
		Writer.WriteInteger(MeshShape.GeometrySections.Elements.back().Primitives.Elements.size(), sizeof(uint32_t));

		//VertexBuffer (hkUint8)
		Writer.WriteByte(0x16);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x20);
		Writer.WriteInteger(MeshShape.GeometrySections.Elements.back().VertexBufferOffset - 0x50, sizeof(uint32_t));
		Writer.WriteInteger(MeshShape.GeometrySections.Elements.back().VertexBuffer.Elements.size() * 6 + 2, sizeof(uint32_t));

		//Aabb8TreeNode
		Writer.WriteByte(0x16);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x00);
		Writer.WriteByte(0x20);
		Writer.WriteInteger(MeshShape.GeometrySections.Elements.back().InteriorPrimitiveBitFieldOffset - 0x50, sizeof(uint32_t));
		Writer.WriteInteger(MeshShape.GeometrySections.Elements.back().InteriorPrimitiveBitField.Elements.size(), sizeof(uint32_t));
	}
	else
	{
		Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(PhiveItemSection.data()), PhiveItemSection.size());
	}

	uint32_t End = Writer.GetPosition();

	AlignWriter(Writer, 16);

	return End;
}

void PhiveShape::ReadItemSection(BinaryVectorReader Reader)
{
	uint32_t Size = BitFieldToSize(Reader.ReadUInt32(true));
	uint32_t TargetAddress = Reader.GetPosition() + Size - 4; //-4 = Size(3) + Flags(1)

	Reader.Seek(4, BinaryVectorReader::Position::Current); //Magic

	while (Reader.GetPosition() <= TargetAddress)
	{
		m_Items.resize(m_Items.size() + 1);
		Reader.ReadStruct(&m_Items.back(), 12); //Flags (4) + DataOffset (4) + Count (4)

		uint32_t TypeIndex = m_Items.back().Flags & 0xFFFFFF;
		if (TypeIndex > 0)
		{
			m_Items.back().TypeIndex = TypeIndex - 1;
		}
	}

	if (Reader.GetPosition() > TargetAddress)
		m_Items.pop_back();
}

void PhiveShape::ReadPhiveTables(BinaryVectorReader& Reader)
{
	this->m_PhiveTable1.resize(this->m_Header.TableSize0);
	Reader.ReadStruct(this->m_PhiveTable1.data(), this->m_Header.TableSize0);

	this->m_PhiveTable2.resize(this->m_Header.TableSize1);
	Reader.ReadStruct(this->m_PhiveTable2.data(), this->m_Header.TableSize1);
}

void PhiveShape::ReadTBDYSection(BinaryVectorReader Reader)
{
	uint32_t Jumpback = Reader.GetPosition();
	uint32_t Size = BitFieldToSize(Reader.ReadUInt32(true));

	Reader.Seek(Jumpback, BinaryVectorReader::Position::Begin);
	this->TagBody.resize(Size);
	Reader.ReadStruct(this->TagBody.data(), Size);
}

void PhiveShape::ReadTHSHSection(BinaryVectorReader Reader)
{
	uint32_t Jumpback = Reader.GetPosition();
	uint32_t Size = BitFieldToSize(Reader.ReadUInt32(true));

	Reader.Seek(Jumpback, BinaryVectorReader::Position::Begin);
	this->TagHashes.resize(Size);
	Reader.ReadStruct(this->TagHashes.data(), Size);
}

std::vector<float> PhiveShape::ToVertices()
{
	std::vector<float> Result;
	for (PhiveShape::hknpMeshShapeGeometrySection& Section : MeshShape.GeometrySections.Elements)
	{
		for (PhiveShape::hknpMeshShapeGeometrySectionVertex16_3& Vertex : Section.VertexBuffer.Elements)
		{
			Result.push_back(Vertex.X / MeshShape.VertexConversionUtil.BitScale16[0]);
			Result.push_back(Vertex.Y / MeshShape.VertexConversionUtil.BitScale16[1]);
			Result.push_back(Vertex.Z / MeshShape.VertexConversionUtil.BitScale16[2]);
		}
	}
	/*
	std::vector<float> Result(MeshShape.GeometrySections.Elements[0].VertexBuffer.Elements.size() * 3);

	float SmallestY = 100.0f;

	int Count = 0;
	for (PhiveShape::hknpMeshShapeGeometrySectionVertex16_3 Vertex : MeshShape.GeometrySections.Elements[0].VertexBuffer.Elements)
	{
		Result[Count * 3] = Vertex.X / MeshShape.VertexConversionUtil.BitScale16[0];
		Result[Count * 3 + 1] = Vertex.Y / MeshShape.VertexConversionUtil.BitScale16[1];
		Result[Count * 3 + 2] = Vertex.Z / MeshShape.VertexConversionUtil.BitScale16[2];

		SmallestY = std::fmin(SmallestY, Result[Count * 3 + 1]);

		Count++;
	}

	std::cout << "SMALLEST Y: " << SmallestY << std::endl;
	*/

	return Result;
}

std::vector<unsigned int> PhiveShape::ToIndices()
{
	std::vector<unsigned int> Result;

	uint32_t Base = 0;

	for (PhiveShape::hknpMeshShapeGeometrySection& Section : MeshShape.GeometrySections.Elements)
	{
		for (int i = 0; i < Section.Primitives.Elements.size(); i++)
		{
			hknpMeshShapeGeometrySectionPrimitive Primitive = Section.Primitives.Elements[i];

			Result.push_back(Primitive.IdA + Base);
			Result.push_back(Primitive.IdB + Base);
			Result.push_back(Primitive.IdC + Base);

			Result.push_back(Primitive.IdA + Base);
			Result.push_back(Primitive.IdC + Base);
			Result.push_back(Primitive.IdD + Base);
		}
		Base += Section.VertexBuffer.Elements.size();
	}

	/*
	for (int i = 0; i < MeshShape.GeometrySections.Elements[0].Primitives.Elements.size(); i++)
	{
		std::cout << "PLANE\n";
		hknpMeshShapeGeometrySectionPrimitive Primitive = MeshShape.GeometrySections.Elements[0].Primitives.Elements[i];

		Result.push_back(Primitive.IdA);
		Result.push_back(Primitive.IdB);
		Result.push_back(Primitive.IdC);

		Result.push_back(Primitive.IdA);
		Result.push_back(Primitive.IdC);
		Result.push_back(Primitive.IdD);
	}
	*/


	return Result;
}

void PhiveShape::WriteStringTable(BinaryVectorWriter& Writer, std::string Magic, std::vector<std::string>* Source)
{
	uint32_t Jumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current);

	Writer.WriteBytes(Magic.c_str());

	for (std::string& Str : *Source)
	{
		Writer.WriteBytes(Str.c_str());
		Writer.WriteByte(0x00); //null terminator
	}
	AlignWriter(Writer, 4, 0xFF);

	uint32_t End = Writer.GetPosition();

	uint32_t Size = Writer.GetPosition() - Jumpback;
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	Size = _byteswap_ulong(Size);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Size), 4);
	Writer.Seek(Jumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteByte(0x40); //Flag

	Writer.Seek(End, BinaryVectorWriter::Position::Begin);
}

std::vector<unsigned char> PhiveShape::ToBinary()
{
	BinaryVectorWriter Writer;

	Writer.WriteBytes("Phive");
	Writer.WriteByte(0x00);
	Writer.WriteInteger(0x01, sizeof(uint16_t));
	Writer.WriteByte(0xFF);
	Writer.WriteByte(0xFE);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x04);
	Writer.WriteInteger(0x30, sizeof(uint32_t));

	Writer.Seek(0x50, BinaryVectorWriter::Position::Begin);

	/*
	Vector3F SmallestVertex(100, 100, 100);
	Vector3F BiggestVertex(-100, -100, -100);
	for (int i = 0; i < Vertices.size() / 3; i++) //Import new vertices
	{
		SmallestVertex.SetX(std::fmin(SmallestVertex.GetX(), Vertices[i * 3]));
		SmallestVertex.SetY(std::fmin(SmallestVertex.GetY(), Vertices[i * 3 + 1]));
		SmallestVertex.SetZ(std::fmin(SmallestVertex.GetZ(), Vertices[i * 3 + 2]));

		BiggestVertex.SetX(std::fmax(BiggestVertex.GetX(), Vertices[i * 3]));
		BiggestVertex.SetY(std::fmax(BiggestVertex.GetY(), Vertices[i * 3 + 1]));
		BiggestVertex.SetZ(std::fmax(BiggestVertex.GetZ(), Vertices[i * 3 + 2]));
	}

	std::cout << "BOUNDING BOXES\n";
	std::cout << "MinX: " << SmallestVertex.GetX() << ", MinY: " << SmallestVertex.GetY() << ", MinZ: " << SmallestVertex.GetZ() << std::endl;
	std::cout << "MaxX: " << BiggestVertex.GetX() << ", MaxY: " << BiggestVertex.GetY() << ", MaxZ: " << BiggestVertex.GetZ() << std::endl;

	MeshShape.GeometrySections.Elements.back().VertexBuffer.Elements.clear();

	std::vector<float> NewVertices;
	std::vector<uint32_t> NewIndices = Indices;

	std::cout << "VANILLA\n";
	for (int i = 0; i < Indices.size(); i++)
	{
		std::cout << "Indice " << i << ": " << Vertices[Indices[i] * 3] << ", " << Vertices[Indices[i] * 3 + 1] << ", " << Vertices[Indices[i] * 3 + 2] << std::endl;
	}

	for (int i = 0; i < Vertices.size() / 3; i++) //Import new vertices
	{
		PhiveShape::hknpMeshShapeGeometrySectionVertex16_3 Vertex;
		Vertex.X = (Vertices[i * 3] + std::fabs(SmallestVertex.GetX())) * MeshShape.VertexConversionUtil.BitScale16[0];
		Vertex.Y = (Vertices[i * 3 + 1] + std::fabs(SmallestVertex.GetY())) * MeshShape.VertexConversionUtil.BitScale16[1];
		Vertex.Z = (Vertices[i * 3 + 2] + std::fabs(SmallestVertex.GetZ())) * MeshShape.VertexConversionUtil.BitScale16[2];

		std::cout << "Vertex " << i << ": " << Vertex.X << ", " << Vertex.Y << ", " << Vertex.Z << std::endl;

		bool Found = false;

		for (int j = 0; j < NewVertices.size() / 3; j++)
		{
			if (NewVertices[j * 3] == Vertices[i * 3] && NewVertices[j * 3 + 1] == Vertices[i * 3 + 1] && NewVertices[j * 3 + 2] == Vertices[i * 3 + 2])
			{
				Found = true;

				for (int k = 0; k < Indices.size(); k++)
				{
					if (Indices[k] == i)
						NewIndices[k] = j;
				}
			}
		}

		if (Found)
			continue;

		MeshShape.GeometrySections.Elements.back().VertexBuffer.Elements.push_back(Vertex);

		NewVertices.push_back(Vertices[i * 3]);
		NewVertices.push_back(Vertices[i * 3 + 1]);
		NewVertices.push_back(Vertices[i * 3 + 2]);

		for (int k = 0; k < Indices.size(); k++)
		{
			if (Indices[k] == i)
				NewIndices[k] = NewVertices.size() / 3 - 1;
		}
	}

	Indices = NewIndices;
	Vertices = NewVertices;

	std::cout << "OPTIMIZED\n";
	for (int i = 0; i < Indices.size(); i++)
	{
		std::cout << "Indice " << i << ": " << Vertices[Indices[i] * 3] << ", " << Vertices[Indices[i] * 3 + 1] << ", " << Vertices[Indices[i] * 3 + 2] << std::endl;
	}

	auto PointOnPlane = [](Vector3F PointA, Vector3F PointB, Vector3F PointC, Vector3F TargetPoint)
		{
			Vector3F DirectionalAB(PointB.GetX() - PointA.GetX(), PointB.GetY() - PointA.GetY(), PointB.GetZ() - PointA.GetZ());
			Vector3F DirectionalAC(PointC.GetX() - PointA.GetX(), PointC.GetY() - PointA.GetY(), PointC.GetZ() - PointA.GetZ());

			Vector3F Normal(
				DirectionalAB.GetY() * DirectionalAC.GetZ() - DirectionalAB.GetZ() * DirectionalAC.GetY(),
				DirectionalAB.GetZ() * DirectionalAC.GetX() - DirectionalAB.GetX() * DirectionalAC.GetZ(),
				DirectionalAB.GetX() * DirectionalAC.GetY() - DirectionalAB.GetY() * DirectionalAC.GetX()
			);

			float EquationSolution = Normal.GetX() * PointA.GetX() + Normal.GetY() * PointA.GetY() + Normal.GetZ() * PointA.GetZ();

			float TargetSolution = Normal.GetX() * TargetPoint.GetX() + Normal.GetY() * TargetPoint.GetY() + Normal.GetZ() * TargetPoint.GetZ();

			return std::fabs(TargetSolution - EquationSolution) < 0.001f;
		};

	MeshShape.GeometrySections.Elements.back().Primitives.Elements.clear();

	std::vector<std::vector<uint32_t>> WrittenIndices;
	for (int i = 0; i < Indices.size() / 3; i++)
	{
		int32_t MatchingIndex = -1;
		int32_t DifferentIndex = -1;

		PhiveShape::hknpMeshShapeGeometrySectionPrimitive Primitive;
		Primitive.IdA = Indices[i * 3];
		Primitive.IdB = Indices[i * 3 + 1];
		Primitive.IdC = Indices[i * 3 + 2];

		Vector3F PointA(Vertices[Primitive.IdA * 3], Vertices[Primitive.IdA * 3 + 1], Vertices[Primitive.IdA * 3 + 2]);
		Vector3F PointB(Vertices[Primitive.IdB * 3], Vertices[Primitive.IdB * 3 + 1], Vertices[Primitive.IdB * 3 + 2]);
		Vector3F PointC(Vertices[Primitive.IdC * 3], Vertices[Primitive.IdC * 3 + 1], Vertices[Primitive.IdC * 3 + 2]);

		for (int j = 0; j < Indices.size() / 3; j++)
		{
			Vector3F SubPointA(Vertices[Indices[j * 3] * 3], Vertices[Indices[j * 3] * 3 + 1], Vertices[Indices[j * 3] * 3 + 2]);
			Vector3F SubPointB(Vertices[Indices[j * 3 + 1] * 3], Vertices[Indices[j * 3 + 1] * 3 + 1], Vertices[Indices[j * 3 + 1] * 3 + 2]);
			Vector3F SubPointC(Vertices[Indices[j * 3 + 2] * 3], Vertices[Indices[j * 3 + 2] * 3 + 1], Vertices[Indices[j * 3 + 2] * 3 + 2]);

			uint32_t Matching = 0;
			if (SubPointA == PointA || SubPointA == PointB || SubPointA == PointC) Matching++;
			else DifferentIndex = Indices[j * 3];
			if (SubPointB == PointA || SubPointB == PointB || SubPointB == PointC) Matching++;
			else DifferentIndex = Indices[j * 3 + 1];
			if (SubPointC == PointA || SubPointC == PointB || SubPointC == PointC) Matching++;
			else DifferentIndex = Indices[j * 3 + 2];

			if (Matching == 2)
			{
				Vector3F TargetPoint(Vertices[DifferentIndex * 3], Vertices[DifferentIndex * 3 + 1], Vertices[DifferentIndex * 3 + 2]);
				if (PointOnPlane(PointA, PointB, PointC, TargetPoint))
				{

					if (SubPointA != TargetPoint)
					{
						if (SubPointA != PointA && SubPointA != PointC)
						{
							uint32_t PrimitiveBTemp = Primitive.IdB;

							Primitive.IdB = Primitive.IdA;
							Primitive.IdA = PrimitiveBTemp;
						}
					}

					if (SubPointB != TargetPoint)
					{
						if (SubPointB != PointA && SubPointB != PointC)
						{
							uint32_t PrimitiveBTemp = Primitive.IdB;

							Primitive.IdB = Primitive.IdA;
							Primitive.IdA = PrimitiveBTemp;
						}
					}

					if (SubPointC != TargetPoint)
					{
						if (SubPointC != PointA && SubPointC != PointC)
						{
							uint32_t PrimitiveBTemp = Primitive.IdB;

							Primitive.IdB = Primitive.IdA;
							Primitive.IdA = PrimitiveBTemp;
						}
					}

					MatchingIndex = DifferentIndex;
					break;
				}
			}
		}

		if (MatchingIndex == -1)
		{
			std::cout << "NO MATCH FOUND\n";
			Primitive.IdD = Primitive.IdA;
		}
		else
		{
			Primitive.IdD = DifferentIndex;
		}

		bool Found = false;

		for (std::vector<uint32_t> Indice : WrittenIndices)
		{
			if (std::find(Indice.begin(), Indice.end(), Primitive.IdA) != Indice.end() &&
				std::find(Indice.begin(), Indice.end(), Primitive.IdB) != Indice.end() &&
				std::find(Indice.begin(), Indice.end(), Primitive.IdC) != Indice.end() &&
				std::find(Indice.begin(), Indice.end(), Primitive.IdD) != Indice.end())
			{
				Found = true;
				std::cout << "FOUND\n";
				break;
			}
		}

		if (Found)
			continue;

		std::vector<uint32_t> Indice;
		Indice.push_back(Primitive.IdA);
		Indice.push_back(Primitive.IdB);
		Indice.push_back(Primitive.IdC);
		Indice.push_back(Primitive.IdD);
		WrittenIndices.push_back(Indice);

		MeshShape.GeometrySections.Elements.back().Primitives.Elements.push_back(Primitive);
	}

	//Sorting elements, lowest Y first, highest Y last
	std::vector<PhiveShape::hknpMeshShapeGeometrySectionPrimitive> NewPrimitives;
	std::vector<uint32_t> SortedPrimitives;


	for (int k = 0; k < MeshShape.GeometrySections.Elements.back().Primitives.Elements.size(); k++)
	{
		for (int i = 0; i < MeshShape.GeometrySections.Elements.back().Primitives.Elements.size(); i++)
		{
			if (std::find(SortedPrimitives.begin(), SortedPrimitives.end(), i) != SortedPrimitives.end()) continue;

			Vector3F PointA = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdA * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdA * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdA * 3 + 2]);
			Vector3F PointB = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdB * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdB * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdB * 3 + 2]);
			Vector3F PointC = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdC * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdC * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdC * 3 + 2]);
			Vector3F PointD = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdD * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdD * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdD * 3 + 2]);

			Vector3F LowestPoint = PointA;
			uint32_t LowestPointCount = 0;
			if (LowestPoint.GetY() > PointB.GetY()) LowestPoint = PointB;
			if (LowestPoint.GetY() > PointC.GetY()) LowestPoint = PointC;
			if (LowestPoint.GetY() > PointD.GetY()) LowestPoint = PointD;

			if (PointA.GetY() == LowestPoint.GetY()) LowestPointCount++;
			if (PointB.GetY() == LowestPoint.GetY()) LowestPointCount++;
			if (PointC.GetY() == LowestPoint.GetY()) LowestPointCount++;
			if (PointD.GetY() == LowestPoint.GetY()) LowestPointCount++;

			bool IsLowest = true;

			for (int j = 0; j < MeshShape.GeometrySections.Elements.back().Primitives.Elements.size(); j++)
			{
				if (j == i) continue;
				if (std::find(SortedPrimitives.begin(), SortedPrimitives.end(), j) != SortedPrimitives.end()) continue;

				Vector3F ChildPointA = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdA * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdA * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdA * 3 + 2]);
				Vector3F ChildPointB = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdB * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdB * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdB * 3 + 2]);
				Vector3F ChildPointC = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdC * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdC * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdC * 3 + 2]);
				Vector3F ChildPointD = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdD * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdD * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[j].IdD * 3 + 2]);

				Vector3F ChildLowestPoint = ChildPointA;
				uint32_t ChildLowestPointCount = 0;
				if (ChildLowestPoint.GetY() > ChildPointB.GetY()) ChildLowestPoint = ChildPointB;
				if (ChildLowestPoint.GetY() > ChildPointC.GetY()) ChildLowestPoint = ChildPointC;
				if (ChildLowestPoint.GetY() > ChildPointD.GetY()) ChildLowestPoint = ChildPointD;

				if (ChildPointA.GetY() == ChildLowestPoint.GetY()) ChildLowestPointCount++;
				if (ChildPointB.GetY() == ChildLowestPoint.GetY()) ChildLowestPointCount++;
				if (ChildPointC.GetY() == ChildLowestPoint.GetY()) ChildLowestPointCount++;
				if (ChildPointD.GetY() == ChildLowestPoint.GetY()) ChildLowestPointCount++;

				if (ChildLowestPoint.GetY() < LowestPoint.GetY())
				{
					IsLowest = false;
					break;
				}

				if (ChildLowestPoint.GetY() == LowestPoint.GetY())
				{
					if (ChildLowestPointCount > LowestPointCount)
					{
						IsLowest = false;
						break;
					}
				}
			}

			if (IsLowest)
			{
				NewPrimitives.push_back(MeshShape.GeometrySections.Elements.back().Primitives.Elements[i]);
				SortedPrimitives.push_back(i);
				break;
			}
		}
	}

	MeshShape.GeometrySections.Elements.back().Primitives.Elements = NewPrimitives;

	for (int i = 0; i < MeshShape.GeometrySections.Elements.back().Primitives.Elements.size(); i++)
	{
		Vector3F PointA = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdA * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdA * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdA * 3 + 2]);
		Vector3F PointB = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdB * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdB * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdB * 3 + 2]);
		Vector3F PointC = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdC * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdC * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdC * 3 + 2]);
		Vector3F PointD = Vector3F(Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdD * 3], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdD * 3 + 1], Vertices[MeshShape.GeometrySections.Elements.back().Primitives.Elements[i].IdD * 3 + 2]);
		std::cout << "Primitive " << i << ": (" << PointA.GetX() << ", " << PointA.GetY() << ", " << PointA.GetZ() << "); (" << PointB.GetX() << ", " << PointB.GetY() << ", " << PointB.GetZ() << "); (" << PointC.GetX() << ", " << PointC.GetY() << ", " << PointC.GetZ() << "); (" << PointD.GetX() << ", " << PointD.GetY() << ", " << PointD.GetZ() << ")" << std::endl;
	}

	SmallestVertex = Vector3F(-2, -1, -2);
	BiggestVertex = Vector3F(2, 0, 2);

	MeshShape.TopLevelTree.Nodes.Elements[1].FourAabb.MinX[0] = SmallestVertex.GetX();
	MeshShape.TopLevelTree.Nodes.Elements[1].FourAabb.MaxX[0] = BiggestVertex.GetX();
	MeshShape.TopLevelTree.Nodes.Elements[1].FourAabb.MinY[0] = SmallestVertex.GetY();
	MeshShape.TopLevelTree.Nodes.Elements[1].FourAabb.MaxY[0] = BiggestVertex.GetY();
	MeshShape.TopLevelTree.Nodes.Elements[1].FourAabb.MinZ[0] = SmallestVertex.GetZ();
	MeshShape.TopLevelTree.Nodes.Elements[1].FourAabb.MaxZ[0] = BiggestVertex.GetZ();

	MeshShape.TopLevelTree.Nodes.Elements[2].FourAabb.MinX[0] = SmallestVertex.GetX();
	MeshShape.TopLevelTree.Nodes.Elements[2].FourAabb.MaxX[0] = BiggestVertex.GetX();
	MeshShape.TopLevelTree.Nodes.Elements[2].FourAabb.MinY[0] = SmallestVertex.GetY();
	MeshShape.TopLevelTree.Nodes.Elements[2].FourAabb.MaxY[0] = BiggestVertex.GetY();
	MeshShape.TopLevelTree.Nodes.Elements[2].FourAabb.MinZ[0] = SmallestVertex.GetZ();
	MeshShape.TopLevelTree.Nodes.Elements[2].FourAabb.MaxZ[0] = BiggestVertex.GetZ();

	std::vector<hknpAabb8TreeNode> NodeTree;
	NodeTree.push_back(MeshShape.GeometrySections.Elements.back().SectionBvh.Elements[2]);
	NodeTree.push_back(MeshShape.GeometrySections.Elements.back().SectionBvh.Elements[0]);
	NodeTree.push_back(MeshShape.GeometrySections.Elements.back().SectionBvh.Elements[1]);
	MeshShape.GeometrySections.Elements.back().SectionBvh.Elements = NodeTree;

	MeshShape.GeometrySections.Elements.back().SectionBvh.Elements.pop_back();

	*/

	//Collision mesh
	MeshShape.Write(Writer);

	uint32_t DataEnd = Writer.GetPosition();

	//400 byte padding
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x09);
	Writer.WriteByte(0xD0);
	Writer.WriteBytes("TYPE");
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x01);
	Writer.WriteByte(0x88);
	Writer.WriteBytes("TPTR");
	Writer.Seek(384, BinaryVectorWriter::Position::Current);

	WriteStringTable(Writer, "TST1", &m_TagStringTable);

	WriteTNASection(Writer);

	WriteStringTable(Writer, "FST1", &m_FieldStringTable);

	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(this->TagBody.data()), this->TagBody.size());
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(this->TagHashes.data()), this->TagHashes.size());

	//8 byte padding
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x08);
	Writer.WriteBytes("TPAD");

	uint32_t HktEnd = WriteItemSection(Writer);

	uint32_t TableOffset1 = Writer.GetPosition();
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(this->m_PhiveTable1.data()), this->m_PhiveTable1.size());
	uint32_t TableOffset2 = Writer.GetPosition();
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(this->m_PhiveTable2.data()), this->m_PhiveTable2.size());

	uint32_t FileSize = Writer.GetPosition();

	Writer.Seek(0x10, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(TableOffset1, sizeof(uint32_t));
	Writer.WriteInteger(TableOffset2, sizeof(uint32_t));
	Writer.WriteInteger(FileSize, sizeof(uint32_t));
	Writer.WriteInteger(FileSize - this->m_PhiveTable1.size() - this->m_PhiveTable2.size() - 0x30, sizeof(uint32_t));
	Writer.WriteInteger(this->m_PhiveTable1.size(), sizeof(uint32_t));
	Writer.WriteInteger(this->m_PhiveTable2.size(), sizeof(uint32_t));

	Writer.WriteInteger(0, sizeof(uint64_t)); //Reserve
	uint32_t HktSize = HktEnd - 0x30;
	HktSize = _byteswap_ulong(HktSize);
	Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&HktSize), sizeof(uint32_t));
	Writer.WriteBytes("TAG0");
	Writer.WriteByte(0x40);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x10);
	Writer.WriteBytes("SDKV");
	Writer.WriteBytes("20220100");

	uint32_t DataSize = DataEnd - 0x48;
	Writer.WriteInteger(_byteswap_ulong(DataSize), sizeof(uint32_t));
	Writer.Seek(-4, BinaryVectorWriter::Position::Current);
	Writer.WriteByte(0x40);
	Writer.Seek(3, BinaryVectorWriter::Position::Current);
	Writer.WriteBytes("DATA");

	std::vector<unsigned char> Data = Writer.GetData();
	Data.resize(FileSize);

	return Data;
}

void PhiveShape::WriteToFile(std::string Path)
{
	std::ofstream File(Path, std::ios::binary);
	std::vector<unsigned char> Binary = ToBinary();
	std::copy(Binary.cbegin(), Binary.cend(),
		std::ostream_iterator<unsigned char>(File));
	File.close();
}

PhiveShape::PhiveShape(std::vector<unsigned char> Bytes, bool Splatoon)
{
	BinaryVectorReader Reader(Bytes, true);

	Reader.ReadStruct(&m_Header, sizeof(PhiveHeader));

	//TST1 -> tag names? (string table)
	Reader.Seek(FindSection(Reader, "TST1") - 8, BinaryVectorReader::Position::Begin); //-8 = Magic(4) + Size(3) + Flags(1)
	ReadStringTable(Reader, &m_TagStringTable);

	Reader.Seek(FindSection(Reader, "FST1") - 8, BinaryVectorReader::Position::Begin); //-8 = Magic(4) + Size(3) + Flags(1)
	ReadStringTable(Reader, &m_FieldStringTable);

	Reader.Seek(FindSection(Reader, "TNA1") - 8, BinaryVectorReader::Position::Begin); //-8 = Magic(4) + Size(3) + Flags(1)
	ReadTNASection(Reader, Splatoon);

	Reader.Seek(FindSection(Reader, "ITEM") - 8, BinaryVectorReader::Position::Begin); //-8 = Magic(4) + Size(3) + Flags(1)
	ReadItemSection(Reader);

	uint32_t DataSectionBaseOffset = FindSection(Reader, "DATA");

	for (Item& DataItem : m_Items)
	{
		if (DataItem.TypeIndex < 0) continue;

		if (m_TNAItems[DataItem.TypeIndex].Tag == "hknpMeshShape")
		{
			Reader.Seek(DataSectionBaseOffset + DataItem.DataOffset, BinaryVectorReader::Position::Begin);
			std::cout << "Decoding hknpMeshShape" << std::endl;
			MeshShape.Read(Reader);
		}
	}

	Reader.Seek(FindSection(Reader, "TBDY") - 8, BinaryVectorReader::Position::Begin); //-8 = Magic(4) + Size(3) + Flags(1)
	ReadTBDYSection(Reader);

	Reader.Seek(FindSection(Reader, "THSH") - 8, BinaryVectorReader::Position::Begin); //-8 = Magic(4) + Size(3) + Flags(1)
	ReadTHSHSection(Reader);

	Reader.Seek(this->m_Header.TableOffset0, BinaryVectorReader::Position::Begin);
	ReadPhiveTables(Reader);

	/*
	for (const PhiveShape::hknpAabb8TreeNode& Node : MeshShape.GeometrySections.Elements[0].SectionBvh.Elements)
	{
		std::cout << "Tree Node: " << (int)Node.Data[0] << ", " << (int)Node.Data[1] << ", " << (int)Node.Data[2] << ", " << (int)Node.Data[3] << "\n";
	}
	*/
}