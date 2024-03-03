#pragma once

#include <string>

namespace UIConsole
{
	enum class MessageType : uint8_t
	{
		Info = 0,
		Warning = 1,
		Error = 2
	};

	extern bool Open;

	void DrawConsoleWindow();
	void AddMessage(std::string Message, UIConsole::MessageType Type);
};