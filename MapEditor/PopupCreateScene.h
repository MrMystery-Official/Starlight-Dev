#pragma once

#include <string>

namespace PopupCreateScene
{
	extern bool IsOpen;
	extern std::string SceneIdentifier;
	extern std::string SceneTemplate;
	extern void (*Func)(std::string, std::string);
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open(void (*Callback)(std::string, std::string));
};