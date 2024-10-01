#pragma once

#include <string>

namespace PopupSettings
{
	extern bool IsOpen;
	extern std::string ColorActorName;
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open();
};