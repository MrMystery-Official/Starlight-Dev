#include "Logger.h"

#include <iostream>

namespace application::util
{
	char Logger::gBuffer[1024] = { 0 }; //Max msg length: 1024 chars

	void Logger::Error(const std::string& Sys, const std::string& Fmt, ...)
	{
		const char* fmt = Fmt.c_str();
		va_list args;
		va_start(args, fmt);
		vsnprintf(gBuffer, sizeof(gBuffer), fmt, args);
		va_end(args);

		std::cout << "[" << Sys << "] Error: " << gBuffer << "!\n";
	}

	void Logger::Warning(const std::string& Sys, const std::string& Fmt, ...)
	{
		const char* fmt = Fmt.c_str();
		va_list args;
		va_start(args, fmt);
		vsnprintf(gBuffer, sizeof(gBuffer), fmt, args);
		va_end(args);

		std::cout << "[" << Sys << "] Warning: " << gBuffer << ".\n";
	}

	void Logger::Info(const std::string& Sys, const std::string& Fmt, ...)
	{
		const char* fmt = Fmt.c_str();
		va_list args;
		va_start(args, fmt);
		vsnprintf(gBuffer, sizeof(gBuffer), fmt, args);
		va_end(args);

		std::cout << "[" << Sys << "] " << gBuffer << ".\n";
	}
}