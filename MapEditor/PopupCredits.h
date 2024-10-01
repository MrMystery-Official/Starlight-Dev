#pragma once

#include <string>

namespace PopupCredits
{
	extern bool IsOpen;
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open();
};