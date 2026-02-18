#pragma once

#include <string>
#include <vector>

namespace application::file::game::restbl
{
	class ResTableFile
	{
	public:
		uint32_t GetFileSize(uint32_t Hash);
		uint32_t GetFileSize(const std::string& Key);
		void SetFileSize(uint32_t Hash, uint32_t Value);
		void SetFileSize(const std::string& Key, uint32_t Value);

		uint32_t GenerateCrc32Hash(const std::string& Key);
		std::vector<unsigned char> ToBinary();
		void WriteToFile(const std::string& Path);

		void Initialize(std::vector<unsigned char> Data);

		ResTableFile(std::vector<unsigned char> Data);
		ResTableFile(const std::string& Path);
		ResTableFile() = default;
	private:
		struct Header
		{
			uint32_t mVersion;
			uint32_t mStringBlockSize;
			uint32_t mCrcMapCount;
			uint32_t mNameMapCount;
		};

		struct CrcEntry
		{
			uint32_t mHash;
			uint32_t mSize;
		};

		struct NameEntry
		{
			std::string mName;
			uint32_t mSize;
		};

		void GenerateCrcTable();

		std::vector<uint32_t> mCrcTable;
		std::vector<ResTableFile::CrcEntry> mCrcEntries;
		std::vector<ResTableFile::NameEntry> mNameEntries;
		ResTableFile::Header mHeader;
	};
}