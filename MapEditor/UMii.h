#pragma once

#include <vector>
#include <string>
#include "Vector3F.h"
#include "GLBfres.h"
#include "Shader.h"
#include "Camera.h"

class UMii
{
public:

	UMii(std::vector<unsigned char> BymlBytes);
	UMii() {}

	void Draw(Vector3F Translate, Vector3F Rotate, Vector3F Scale, Shader* Shader, bool Picking = false, Camera* CameraView = nullptr);

	std::string SexAge;
	std::string Type;
	std::string Number;
	std::string Race;

private:
	struct ModelElement
	{
		GLBfres* Model;
	};

	std::vector<ModelElement> m_ModelElements;
};