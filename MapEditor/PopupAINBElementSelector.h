#pragma once

#include "AINB.h"
#include <string>

namespace PopupAINBElementSelector {

	extern bool IsOpen;
	extern std::string Key;
	extern void* ThisPtr;
	extern void (*Func)(void*, std::string);
	extern std::string PopupTitle;
	extern std::string Value;
	extern std::string ConfirmButtonText;
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open(std::string Title, std::string TextFieldName, std::string ButtonText, void* ThisPtr, void (*Callback)(void*, std::string));

}