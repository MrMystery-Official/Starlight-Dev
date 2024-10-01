#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include "Vector3F.h"
#include "Byml.h"

namespace HashMgr
{
	struct Hash
	{
		uint64_t Hash;
		uint32_t SRTHash;
	};

	extern uint64_t BiggestHash;
	extern uint32_t BiggestSRTHash;
	extern std::vector<uint64_t> mNewHashes;

	uint64_t GenerateStaticCompoundHash(uint64_t BaseValue, uint64_t Modulus, uint64_t Output);
	void FreeHash(uint64_t Hash);
	void Initialize();
	Hash GetHash();
};