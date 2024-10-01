#pragma once

#include <string>

namespace PopupAddActor
{
	extern bool IsOpen;
	extern std::string Gyml;
	extern void (*Func)(std::string);
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open(void (*Callback)(std::string));
};