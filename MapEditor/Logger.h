#pragma once

#include <string>

namespace Logger {
	
void Error(std::string System, std::string Description);
void Warning(std::string System, std::string Description);
void Info(std::string System, std::string Description);

}
