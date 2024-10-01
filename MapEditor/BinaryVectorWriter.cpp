#include "BinaryVectorWriter.h"

#include <iostream>

int BinaryVectorWriter::GetPosition()
{
	return this->m_Offset;
}

void BinaryVectorWriter::WriteByte(char Byte)
{
	this->m_Data.resize(this->m_Data.size() + 1);
	this->m_Data[m_Offset] = Byte;
	this->m_Offset++;
}

void BinaryVectorWriter::Seek(int Offset, BinaryVectorWriter::Position Position)
{
	//this->Stream->seekp(Offset, Position);
	if (Position == BinaryVectorWriter::Position::Begin)
	{
		if (Offset > this->m_Data.size())
		{
			this->m_Data.resize(Offset + 1);
		}
		this->m_Offset = Offset;
	}
	else if (Position == BinaryVectorWriter::Position::Current)
	{
		if (this->m_Offset + Offset >= this->m_Data.size())
		{
			this->m_Data.resize(Offset + this->m_Offset + 1);
		}
		this->m_Offset += Offset;
	}
	else if (Position == BinaryVectorWriter::Position::End)
	{
		this->m_Offset = this->m_Data.size() - Offset - 1;
	}
}

void BinaryVectorWriter::WriteBytes(const char* Bytes)
{
	if (this->m_Offset + strlen(Bytes) > this->m_Data.size())
	{
		this->m_Data.resize(this->m_Data.size() + strlen(Bytes));
	}
	for (int i = 0; i < strlen(Bytes); i++)
	{
		this->m_Data[this->m_Offset] = Bytes[i];
		this->m_Offset++;
	}
}

void BinaryVectorWriter::WriteRawUnsafe(const char* Bytes, int Size)
{
	if (this->m_Offset + Size > this->m_Data.size())
	{
		this->m_Data.resize(this->m_Data.size() + Size);
	}
	for (int i = 0; i < strlen(Bytes); i++)
	{
		this->m_Data[this->m_Offset + i] = Bytes[i];
	}
	this->m_Offset += Size;
}

void BinaryVectorWriter::WriteRawUnsafeFixed(const char* Bytes, int Size, bool BigEndian)
{
	if (this->m_Offset + Size > this->m_Data.size())
	{
		this->m_Data.resize(this->m_Data.size() + Size);
	}
	for (int i = 0; i < Size; i++)
	{
		this->m_Data[this->m_Offset + i] = Bytes[BigEndian ? (Size - 1 - i) : i];
	}
	this->m_Offset += Size;
}

void BinaryVectorWriter::WriteInteger(int64_t Data, int Size, bool BigEndian)
{
	if (this->m_Offset + Size > this->m_Data.size())
	{
		this->m_Data.resize(this->m_Data.size() + Size);
	}

	char Bytes[8] = { 0 };

	if(BigEndian)
		Data = _byteswap_uint64(Data);

	std::memcpy(Bytes, &Data, sizeof(Bytes));

	for (int i = 0; i < Size; i++) {
		this->m_Data[this->m_Offset + i] = Bytes[BigEndian ? (8 - Size + i) : i];
	}

	this->m_Offset += Size;
}

std::vector<unsigned char>& BinaryVectorWriter::GetData()
{
	return this->m_Data;
}

void BinaryVectorWriter::Align(uint32_t Alignment, uint8_t Aligner)
{
	while (GetPosition() % Alignment != 0)
		WriteByte(Aligner);
}