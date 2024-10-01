#pragma once

#include "Actor.h"
#include "Byml.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace ActorMgr
{
	struct ActorInformation
	{
		GLBfres* Model = nullptr;
		bool NeedsPhysics = false;
		bool IsUMii = false;
		UMii UMiiData;
	};

	extern std::vector<Actor> Actors;
	extern std::vector<Actor*> UMiiActors;
	extern std::unordered_map<GLBfres*, std::vector<Actor*>> OpaqueActors;
	extern std::unordered_map<GLBfres*, std::vector<Actor*>> TransparentActorsDiscard;
	extern std::unordered_map<std::string, ActorInformation> ActorInfo; //Gyml -> ActorInfo

	Actor* AddActorFromByml(BymlFile::Node& Node, Actor* ModifyActor = nullptr);
	std::string GetDefaultModelKey(Actor& Actor);
	BymlFile::Node ActorToByml(Actor& Actor);
	Actor& AddActor(std::string Gyml, bool UpdateOrder = true, bool UseCachedData = true);
	Actor& AddActorWithoutHash(std::string Gyml, bool UpdateOrder = true, bool UseCachedData = true);
	Actor CreateBasicActor(std::string Gyml, bool SearchForPhysicsHash = true);
	bool IsPhysicsActor(const std::string& Gyml);
	void PostProcessActor(Actor& SceneActor, bool Physics);
	void UpdateModel(Actor* ModelActor);
	void UpdateMergedActorContent(Actor* Parent, Actor OldActor);
	void ProcessActorPack(std::string Gyml, Actor& NewActor);
	bool IsArea(const std::string& Gyml);
	bool IsSingleRenderPassActor(const std::string& Gyml);
	Actor& AddActor(Actor Template, bool UpdateOrder = true);
	Actor* GetActor(uint64_t Hash, uint32_t SRTHash);
	Actor* GetActor(std::string Gyml);
	void UpdateModelOrder();
	void DeleteActor(uint64_t Hash, uint32_t SRTHash);
	std::vector<Actor>& GetActors();
}