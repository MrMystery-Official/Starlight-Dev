#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include "Bfres.h"
#include "Shader.h"

class BHTMPFile
{
public:
	
	std::vector<float> Tiles;
	uint32_t Width = 0;
	uint32_t Height = 0;
	BfresFile BfresModel;

	BHTMPFile() {}
	BHTMPFile(std::string Path);
	BHTMPFile(std::vector<unsigned char> Bytes);

	void Draw(Shader* Shader);
};