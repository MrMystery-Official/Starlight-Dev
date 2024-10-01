#pragma once

#include <vector>
#include <string>
#include "Mesh.h"
#include "Shader.h"

class HGHTFile
{
public:
	std::vector<float> mHeights;
	Mesh mMesh;

	HGHTFile() = default;
	HGHTFile(std::string Path);
	HGHTFile(std::vector<unsigned char> Bytes);
	void InitMesh();
};