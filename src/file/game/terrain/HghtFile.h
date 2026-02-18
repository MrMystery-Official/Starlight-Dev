#pragma once

#include <vector>

namespace application::file::game::terrain
{
	class HghtFile
	{
	public:
		void Initialize(std::vector<unsigned char> Data, uint16_t Width = 260, uint16_t Height = 260);
		std::vector<unsigned char> ToBinary();

		uint16_t GetHeightAtGridPos(const uint16_t& X, const uint16_t& Y);
		uint16_t& GetWidth();
		uint16_t& GetHeight();

		HghtFile(std::vector<unsigned char> Data, uint16_t Width = 260, uint16_t Height = 260);
		HghtFile() = default;

		std::vector<uint16_t> mHeightMap;
		bool mModified = false;

		uint16_t mWidth = 0;
		uint16_t mHeight = 0;
		uint64_t mDataSize = 222580; //Default data size for 260x260 hght file
	};
}