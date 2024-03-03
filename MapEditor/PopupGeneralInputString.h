#pragma once

#include <string>

namespace PopupGeneralInputString
{
	extern bool IsOpen;
	extern std::string Key;
	extern void (*Func)(std::string);
	extern std::string PopupTitle;
	extern std::string Value;
	extern std::string ConfirmButtonText;

	void Render();
	void Open(std::string Title, std::string TextFieldName, std::string ButtonText, void (*Callback)(std::string));
};