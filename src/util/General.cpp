#include "General.h"

namespace application::util
{
	void General::ReplaceString(std::string& Str, const std::string& From, const std::string& To)
	{
		size_t StartPos = 0;
		while ((StartPos = Str.find(From, StartPos)) != std::string::npos)
		{
			Str.replace(StartPos, From.length(), To);
			StartPos += To.length();
		}
	}
}