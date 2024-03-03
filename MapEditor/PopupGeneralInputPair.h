#pragma once

#include <string>

namespace PopupGeneralInputPair
{
	extern bool IsOpen;
	extern std::string Key;
	extern std::string Value;
	extern void (*Func)(std::string, std::string);
	extern std::string PopupTitle;

	void Render();
	void Open(std::string Title, void (*Callback)(std::string, std::string));
};