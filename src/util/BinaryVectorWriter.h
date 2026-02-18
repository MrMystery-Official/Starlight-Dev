#pragma once

#include <vector>

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
	class BinaryVectorWriter
	{
	public:

		enum class Position : uint8_t
		{
			Begin = 0,
			Current = 1,
			End = 2
		};

		int GetPosition();
		void WriteByte(char Byte);
		void WriteBytes(const char* Bytes);
		void WriteInteger(int64_t Data, int Size, bool BigEndian = false);
		void WriteRawUnsafe(const char* Bytes, int Size);
		void WriteRawUnsafeFixed(const char* Bytes, int Size, bool BigEndian = false);
		void Seek(int Offset, BinaryVectorWriter::Position Start);
		void Align(uint32_t Alignment, uint8_t Aligner = 0x00);
		std::vector<unsigned char>& GetData();
	private:
		std::vector<unsigned char> mData;
		int mOffset = 0;
	};
}