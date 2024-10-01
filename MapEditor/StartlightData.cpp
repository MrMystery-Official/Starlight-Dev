#include "StarlightData.h"

StarlightData::Level StarlightData::mLevel = StarlightData::Level::PERSONAL;

std::string StarlightData::GetVersion()
{
	return "1_0_3";
}

std::string StarlightData::GetStringTableSignature()
{
	return "Star_" + GetLevel() + "_" + GetVersion();
}

uint32_t StarlightData::GetU32Signature()
{
	return *(reinterpret_cast<uint32_t*>((char*)GetLevel().c_str()));
}

uint64_t StarlightData::GetU64Signature()
{
	std::string Result = "Star" + GetLevel();
	return *(reinterpret_cast<uint64_t*>((char*)Result.c_str()));
}

constexpr std::string StarlightData::GetLevel()
{
	switch (mLevel)
	{
	case Level::PATREON:
		return "PATR";
	case Level::PERSONAL:
		return "PERS";
	case Level::SHARED:
		return "SHAR";
	default:
		return "UNK1";
	}
}