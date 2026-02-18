#include "HardwareInfo.h"

#include <sstream>
#include <iomanip>

#ifdef _WIN32
    #include <windows.h>
    #include <intrin.h>
#elif __APPLE__
    #include <sys/sysctl.h>
    #include <sys/mount.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <dirent.h>
#elif __linux__
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fstream>
#endif

namespace application::util
{
    // Get CPU ID using platform-specific methods
    std::string HardwareInfo::GetProcessorID()
    {
        std::stringstream ss;

        #ifdef _WIN32
            int cpuInfo[4] = { -1 };
            __cpuid(cpuInfo, 1);

            ss << std::hex << std::uppercase
               << std::setfill('0') << std::setw(8) << cpuInfo[3]
               << std::setfill('0') << std::setw(8) << cpuInfo[0];
        
        #elif __APPLE__
            char buffer[256];
            size_t bufferlen = sizeof(buffer);
            if (sysctlbyname("machdep.cpu.brand_string", &buffer, &bufferlen, NULL, 0) == 0) {
                ss << std::hex << std::uppercase;
                for (size_t i = 0; i < bufferlen; ++i) {
                    ss << std::setfill('0') << std::setw(2) << static_cast<int>(buffer[i]);
                }
            }
        
        #elif __linux__
            std::ifstream cpuinfo("/proc/cpuinfo");
            std::string line;
            while (std::getline(cpuinfo, line)) {
                if (line.find("processor") != std::string::npos) {
                    ss << std::hex << std::uppercase << line;
                    break;
                }
            }
        
        #else
            ss << "Unknown";
        #endif

        return ss.str();
    }

    // Get Volume Serial Number using platform-specific methods
    std::string HardwareInfo::GetVolumeSerial()
    {
        std::stringstream ss;

        #ifdef _WIN32
            DWORD serialNum = 0;
            GetVolumeInformation((LPCSTR)L"C:\\", nullptr, 0, &serialNum, nullptr, nullptr, nullptr, 0);
            ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << serialNum;
        
        #elif __APPLE__
            struct statfs fs;
            if (statfs("/", &fs) == 0) {
                ss << std::hex << std::uppercase << fs.f_fsid.val[0];
            }
        
        #elif __linux__
            std::ifstream mountinfo("/proc/mounts");
            std::string line;
            while (std::getline(mountinfo, line)) {
                if (line.find(" / ") != std::string::npos) {
                    struct stat st;
                    if (stat("/", &st) == 0) {
                        ss << std::hex << std::uppercase << st.st_dev;
                        break;
                    }
                }
            }
        
        #else
            ss << "Unknown";
        #endif

        return ss.str();
    }

    // Get motherboard serial number using platform-specific methods
    std::string HardwareInfo::GetMotherboardSerial()
    {
        std::stringstream ss;

        #ifdef _WIN32
            char buf[256] = { 0 };
            std::string cmd = "wmic baseboard get serialnumber";
            FILE* pipe = _popen(cmd.c_str(), "r");

            if (pipe) {
                while (fgets(buf, sizeof(buf), pipe) != nullptr) {
                    if (strlen(buf) > 2 && strstr(buf, "SerialNumber") == nullptr) {
                        _pclose(pipe);
                        return std::string(buf);
                    }
                }
                _pclose(pipe);
            }
        
        #elif __APPLE__
            char buffer[256];
            size_t bufferlen = sizeof(buffer);
            if (sysctlbyname("hw.model", &buffer, &bufferlen, NULL, 0) == 0) {
                ss << buffer;
            }
        
        #elif __linux__
            std::ifstream dmi_id("/sys/class/dmi/id/board_serial");
            if (dmi_id.good()) {
                std::string serial;
                std::getline(dmi_id, serial);
                ss << serial;
            }
        
        #else
            ss << "Unknown";
        #endif

        return ss.str().empty() ? "Unknown" : ss.str();
    }

    // Combine all IDs to create a unique hardware ID
    std::string HardwareInfo::GetUniqueHardwareID()
    {
        std::string cpuId = GetProcessorID();
        std::string volumeSerial = GetVolumeSerial();
        std::string motherboardSerial = GetMotherboardSerial();

        std::stringstream ss;
        ss << cpuId << "-" << volumeSerial << "-" << motherboardSerial;
        return ss.str();
    }
}