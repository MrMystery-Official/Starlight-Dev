#include "HashMgr.h"

#include "Logger.h"

uint64_t HashMgr::BiggestHash = 0;
uint32_t HashMgr::BiggestSRTHash = 0;

void HashMgr::Initialize()
{
	BiggestHash = 0;
	BiggestSRTHash = 0;
	for (Actor& Actor : ActorMgr::GetActors())
	{
		BiggestHash = std::max(BiggestHash, Actor.Hash);
		BiggestSRTHash = std::max(BiggestSRTHash, Actor.SRTHash);
	}
}

HashMgr::Hash HashMgr::GetHash(bool Physics)
{
	if (Physics) Logger::Warning("HashMgr", "A physics hash was requested, although this is not implemented yet. Returning non-physics hash");

	BiggestHash++;
	BiggestSRTHash++;
	return HashMgr::Hash{ BiggestHash, BiggestSRTHash };
}