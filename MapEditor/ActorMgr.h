#pragma once

#include "Actor.h"
#include "Byml.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include <unordered_map>
#include <vector>

namespace ActorMgr
{
	extern std::vector<Actor> Actors;
	extern std::unordered_map<BfresFile*, std::vector<Actor*>> OpaqueActors;
	extern std::unordered_map<BfresFile*, std::vector<Actor*>> TransparentActors;

	Actor* AddActorFromByml(BymlFile::Node& Node, Actor* ModifyActor = nullptr);
	BymlFile::Node ActorToByml(Actor& Actor);
	Actor& AddActor(std::string Gyml, bool UpdateOrder = true, bool UseCachedData = true);
	Actor CreateBasicActor(std::string Gyml);
	void UpdateModel(Actor* ModelActor);
	void UpdateMergedActorContent(Actor* Parent, Actor OldActor);
	Actor& AddActor(Actor Template, bool UpdateOrder = true);
	Actor* GetActor(uint64_t Hash, uint32_t SRTHash);
	Actor* GetActor(std::string Gyml);
	void UpdateModelOrder();
	void DeleteActor(uint64_t Hash, uint32_t SRTHash);
	std::vector<Actor>& GetActors();
}