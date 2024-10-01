#pragma once

#include <string>
#include <functional>

namespace PopupGeneralInputPair
{
	extern bool IsOpen;
	extern std::string Key;
	extern std::string Value;
	extern std::function<void(std::string, std::string)> Func;
	extern std::string PopupTitle;
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open(std::string Title, std::function<void(std::string, std::string)> Callback);
};