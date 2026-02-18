#include "BinaryVectorWriter.h"

namespace application::util
{
	int BinaryVectorWriter::GetPosition()
	{
		return this->mOffset;
	}

	void BinaryVectorWriter::WriteByte(char Byte)
	{
		this->mData.resize(this->mData.size() + 1);
		this->mData[mOffset] = Byte;
		this->mOffset++;
	}

	void BinaryVectorWriter::Seek(int Offset, BinaryVectorWriter::Position Position)
	{
		//this->Stream->seekp(Offset, Position);
		if (Position == BinaryVectorWriter::Position::Begin)
		{
			if (Offset > this->mData.size())
			{
				this->mData.resize(Offset + 1);
			}
			this->mOffset = Offset;
		}
		else if (Position == BinaryVectorWriter::Position::Current)
		{
			if (this->mOffset + Offset >= this->mData.size())
			{
				this->mData.resize(Offset + this->mOffset + 1);
			}
			this->mOffset += Offset;
		}
		else if (Position == BinaryVectorWriter::Position::End)
		{
			this->mOffset = this->mData.size() - Offset - 1;
		}
	}

	void BinaryVectorWriter::WriteBytes(const char* Bytes)
	{
		if (this->mOffset + strlen(Bytes) > this->mData.size())
		{
			this->mData.resize(this->mData.size() + strlen(Bytes));
		}
		for (int i = 0; i < strlen(Bytes); i++)
		{
			this->mData[this->mOffset] = Bytes[i];
			this->mOffset++;
		}
	}

	void BinaryVectorWriter::WriteRawUnsafe(const char* Bytes, int Size)
	{
		if (this->mOffset + Size > this->mData.size())
		{
			this->mData.resize(this->mData.size() + Size);
		}
		for (int i = 0; i < strlen(Bytes); i++)
		{
			this->mData[this->mOffset + i] = Bytes[i];
		}
		this->mOffset += Size;
	}

	void BinaryVectorWriter::WriteRawUnsafeFixed(const char* Bytes, int Size, bool BigEndian)
	{
		if (this->mOffset + Size > this->mData.size())
		{
			this->mData.resize(this->mData.size() + Size);
		}
		for (int i = 0; i < Size; i++)
		{
			this->mData[this->mOffset + i] = Bytes[BigEndian ? (Size - 1 - i) : i];
		}
		this->mOffset += Size;
	}

	void BinaryVectorWriter::WriteInteger(int64_t Data, int Size, bool BigEndian)
	{
		if (this->mOffset + Size > this->mData.size())
		{
			this->mData.resize(this->mData.size() + Size);
		}

		char Bytes[8] = { 0 };

		if (BigEndian)
			Data = bswap_64(Data);

		std::memcpy(Bytes, &Data, sizeof(Bytes));

		for (int i = 0; i < Size; i++) {
			this->mData[this->mOffset + i] = Bytes[BigEndian ? (8 - Size + i) : i];
		}

		this->mOffset += Size;
	}

	std::vector<unsigned char>& BinaryVectorWriter::GetData()
	{
		return this->mData;
	}

	void BinaryVectorWriter::Align(uint32_t Alignment, uint8_t Aligner)
	{
		while (GetPosition() % Alignment != 0)
			WriteByte(Aligner);
	}
}