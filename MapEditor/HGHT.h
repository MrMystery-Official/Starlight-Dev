#pragma once

#include <vector>
#include <string>

class HGHTFile
{
public:
	struct Tile
	{

	};

	HGHTFile() {}
	HGHTFile(std::string Path);
	HGHTFile(std::vector<unsigned char> Bytes);
};