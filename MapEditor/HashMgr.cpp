#include "HashMgr.h"

#include "Logger.h"
#include "ActorMgr.h"
#include "SceneMgr.h"
#include "Editor.h"
#include "Exporter.h"

#include "ZStdFile.h"
#include "PhiveStaticCompound.h"

#include "BinaryVectorReader.h"
#include "BinaryVectorWriter.h"
#include "Util.h"

uint64_t HashMgr::BiggestHash = 0;
uint32_t HashMgr::BiggestSRTHash = 0;
std::vector<uint64_t> HashMgr::mNewHashes;

void HashMgr::Initialize()
{
	BiggestHash = 0;
	BiggestSRTHash = 0;
	mNewHashes.clear();
	for (Actor& Actor : ActorMgr::GetActors())
	{
		BiggestHash = std::max(BiggestHash, Actor.Hash) + 1;
		BiggestSRTHash = std::max(BiggestSRTHash, Actor.SRTHash) + 1;
	}
}

uint64_t HashMgr::GenerateStaticCompoundHash(uint64_t BaseValue, uint64_t Modulus, uint64_t Output) {
	uint64_t Remainder = BaseValue % Modulus;
	uint64_t Difference = (Output >= Remainder) ? (Output - Remainder) : (Modulus - Remainder + Output);
	BaseValue += Difference;
	return BaseValue;
}

void HashMgr::FreeHash(uint64_t Hash)
{
	mNewHashes.erase(std::remove_if(mNewHashes.begin(), mNewHashes.end(),
		[Hash](uint64_t OldHash) { return OldHash == Hash; }), mNewHashes.end());

	if (SceneMgr::mStaticCompounds.empty())
		return;

	for (PhiveStaticCompound& StaticCompound : SceneMgr::mStaticCompounds)
	{
		StaticCompound.mActorHashMap.erase(std::remove_if(StaticCompound.mActorHashMap.begin(), StaticCompound.mActorHashMap.end(),
			[Hash](uint64_t OldHash) { return OldHash == Hash; }), StaticCompound.mActorHashMap.end());

		StaticCompound.mActorLinks.erase(std::remove_if(StaticCompound.mActorLinks.begin(), StaticCompound.mActorLinks.end(),
			[Hash](PhiveStaticCompound::PhiveStaticCompoundActorLink& Link) { return Link.mActorHash == Hash; }), StaticCompound.mActorLinks.end());
	}
}

HashMgr::Hash HashMgr::GetHash()
{
	/*
	if (Physics && SceneMgr::mStaticCompounds.empty())
	{
		PhiveStaticCompound* StaticCompound = SceneMgr::GetStaticCompoundForPos(glm::vec3(Position.GetX(), Position.GetY(), Position.GetZ()));

		uint32_t OptimalIndex = 0xFFFFFFFF;
		for (uint64_t Index : StaticCompound->mActorHashMap)
		{
			if (Index == 0)
			{
				OptimalIndex = Index;
				break;
			}
		}
		if (OptimalIndex == 0xFFFFFFFF)
		{
			OptimalIndex = StaticCompound->mActorLinks.size() * 2;
		}

		uint64_t NewHash = GenerateStaticCompoundHash(BiggestHash++,
			(StaticCompound->mActorLinks.size() + 1) * 2,
			OptimalIndex);

		StaticCompound->mActorHashMap[OptimalIndex] = NewHash;

		PhiveStaticCompound::PhiveStaticCompoundActorLink Link;
		Link.mActorHash = NewHash;
		Link.mBaseIndex = 0xFFFFFFFF;
		Link.mEndIndex = 0xFFFFFFFF;
		Link.mCompoundRigidBodyIndex = 0;
		Link.mIsValid = 0;
		Link.mReserve0 = BiggestSRTHash++;
		Link.mReserve1 = 0;
		Link.mReserve2 = 0;
		Link.mReserve3 = 0;

		StaticCompound->mActorLinks.push_back(Link);

		return HashMgr::Hash{
			.Hash = Link.mActorHash,
			.SRTHash = Link.mReserve0
		};
	}
	*/

	uint64_t Hash = BiggestHash++;

	mNewHashes.push_back(Hash);

	return HashMgr::Hash{ Hash, BiggestSRTHash++ };
}