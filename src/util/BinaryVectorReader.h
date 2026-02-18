#pragma once

#include <vector>
#include <string>

#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_16(x) BSWAP_16(x)
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_16(x) bswap16(x)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_16(x) swap16(x)
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <sys/types.h>
#include <machine/bswap.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_16(x) bswap16(x)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#else

#include <byteswap.h>

#endif

namespace application::util
{
	class BinaryVectorReader
	{
	public:
		enum class Position : uint8_t
		{
			Begin = 0,
			Current = 1,
			End = 2
		};

		BinaryVectorReader() = default;
		BinaryVectorReader(std::vector<unsigned char> Bytes, bool BigEndian = false);

		void Seek(int Offset, BinaryVectorReader::Position Position);
		int GetPosition();
		uint32_t GetSize();
		uint8_t ReadUInt8();
		int8_t ReadInt8();
		uint16_t ReadUInt16(bool BigEndian = false);
		int16_t ReadInt16(bool BigEndian = false);
		uint32_t ReadUInt24(bool BigEndian = false);
		uint32_t ReadUInt32(bool BigEndian = false);
		int32_t ReadInt32(bool BigEndian = false);
		uint64_t ReadUInt64(bool BigEndian = false);
		int64_t ReadInt64(bool BigEndian = false);
		float ReadFloat(bool BigEndian = false);
		double ReadDouble(bool BigEndian = false);
		void ReadStruct(void* Dest, uint32_t Size, int Offset = -1);
		std::string ReadString(int Size = -1);
		std::wstring ReadWString(int Size);
		void Align(uint32_t Alignment);
	private:
		int mOffset = -1;
		std::vector<unsigned char> mBytes;
	};
}