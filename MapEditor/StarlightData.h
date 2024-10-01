#pragma once

#include <string>

namespace StarlightData
{
	enum class Level : uint8_t
	{
		PATREON = 0,
		SHARED = 1,
		PERSONAL = 2
	};

	extern Level mLevel;

	std::string GetVersion();
	std::string GetStringTableSignature();
	uint32_t GetU32Signature();
	uint64_t GetU64Signature();
	constexpr std::string GetLevel();
}