#pragma once

#include <string>

namespace PopupSettings
{
	extern bool IsOpen;
	extern std::string ColorActorName;

	void Render();
	void Open();
};