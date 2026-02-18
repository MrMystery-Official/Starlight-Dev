#pragma once

#include <string>
#include <stdarg.h>
#include <iostream>

namespace application::util
{
	namespace Logger
	{
		extern char gBuffer[];

		void Error(const std::string& Sys, const std::string& Fmt, ...);
		void Warning(const std::string& Sys, const std::string& Fmt, ...);
		void Info(const std::string& Sys, const std::string& Fmt, ...);
	}
}