#pragma once

#include "Actor.h"
#include "Byml.h"
#include <string>

namespace ActorCollisionCreator
{
	void ReplaceStringInBymlNodes(BymlFile::Node* Node, std::string Search, std::string Replace, std::string DoesNotContain = "");
	void AddCollisionActor(Actor& CollisionActorParent);
}