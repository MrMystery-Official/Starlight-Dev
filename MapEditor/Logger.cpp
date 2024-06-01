#include "Logger.h"

#include "UIConsole.h"
#include <iostream>

#ifdef _MSC_VER
#include <windows.h> // Windows specific
HANDLE WinConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

void Logger::Error(std::string System, std::string Description)
{
#ifdef _MSC_VER
    SetConsoleTextAttribute(WinConsole, 12); // Red
#else
    std::cout << "\033[1;31m";
#endif

    std::cout << "Error in " << System << ": " << Description << "!\n";
    UIConsole::AddMessage("Error in " + System + ": " + Description + "!", UIConsole::MessageType::Error);
}

void Logger::Warning(std::string System, std::string Description)
{
#ifdef _MSC_VER
    SetConsoleTextAttribute(WinConsole, 14); // Yellow
#else
    std::cout << "\033[1;33m";
#endif

    std::cout << "Warning from " << System << ": " << Description << "!\n";
    UIConsole::AddMessage("Warning from " + System + ": " + Description + "!", UIConsole::MessageType::Warning);
}

void Logger::Info(std::string System, std::string Description)
{
#ifdef _MSC_VER
    SetConsoleTextAttribute(WinConsole, 15); // White
#else
    std::cout << "\033[1;37m";
#endif

    std::cout << System << ": " << Description << ".\n";
    UIConsole::AddMessage(System + ": " + Description + ".", UIConsole::MessageType::Info);
}