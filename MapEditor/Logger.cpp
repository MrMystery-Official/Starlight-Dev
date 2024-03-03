#include "Logger.h"

#include <iostream>
#include <windows.h> // Windows specific
#include "UIConsole.h"

HANDLE WinConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void Logger::Error(std::string System, std::string Description)
{
	SetConsoleTextAttribute(WinConsole, 12); //Red
	std::cout << "Error in " << System << ": " << Description << "!\n";
	UIConsole::AddMessage("Error in " + System + ": " + Description + "!", UIConsole::MessageType::Error);
}

void Logger::Warning(std::string System, std::string Description)
{
	SetConsoleTextAttribute(WinConsole, 14); //Yellow
	std::cout << "Warning from " << System << ": " << Description << "!\n";
	UIConsole::AddMessage("Warning from " + System + ": " + Description + "!", UIConsole::MessageType::Warning);
}

void Logger::Info(std::string System, std::string Description)
{
	SetConsoleTextAttribute(WinConsole, 15); //White
	std::cout << System << ": " << Description << ".\n";
	UIConsole::AddMessage(System + ": " + Description + ".", UIConsole::MessageType::Info);
}