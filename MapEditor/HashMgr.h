#pragma once

#include "ActorMgr.h"

namespace HashMgr
{
	struct Hash
	{
		uint64_t Hash;
		uint32_t SRTHash;
	};

	extern uint64_t BiggestHash;
	extern uint32_t BiggestSRTHash;

	void Initialize();
	Hash GetHash(bool Physics);
};