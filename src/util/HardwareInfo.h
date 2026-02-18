#pragma once

#include <string>

namespace application::util
{
	namespace HardwareInfo
	{
        std::string GetProcessorID();
        std::string GetVolumeSerial();
        std::string GetMotherboardSerial();
        std::string GetUniqueHardwareID();
	}
}