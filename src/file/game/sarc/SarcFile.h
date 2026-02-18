#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>

#include <util/BinaryVectorReader.h>
#include <util/BinaryVectorWriter.h>

namespace application::file::game
{
	class SarcFile
	{
	public:
		struct Entry
		{
			std::string mName;
			std::vector<unsigned char> mBytes;

			void WriteToFile(const std::string& Path);
		};

		bool mLoaded = false;

		std::vector<SarcFile::Entry>& GetEntries();
		SarcFile::Entry& GetEntry(const std::string& Name);
		bool HasEntry(const std::string& Name);
		bool HasDirectory(const std::string& Path);

		std::vector<unsigned char> ToBinary();
		void WriteToFile(const std::string& Path);
		void Initialize(std::vector<unsigned char> Bytes);

		SarcFile() = default;
		SarcFile(const std::string& Path);
		SarcFile(std::vector<unsigned char> Bytes);
	private:
		struct Node
		{
			uint32_t mHash;
			uint32_t mStringOffset;
			uint32_t mDataStart;
			uint32_t mDataEnd;
		};

		struct HashValue
		{
			uint32_t mHash = 0;
			SarcFile::Entry* mNode = nullptr;
		};

		std::vector<Entry> mEntries;

		int GCD(int a, int b);
		int LCM(int a, int b);
		int AlignUp(int Value, int Size);
		int GetBinaryFileAlignment(std::vector<unsigned char> Data);
		bool AlignWriter(application::util::BinaryVectorWriter& Writer, int Alignment);
	};
}