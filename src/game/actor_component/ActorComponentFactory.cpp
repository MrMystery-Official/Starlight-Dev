#include "ActorComponentFactory.h"

#include <game/actor_component/ActorComponentModelInfo.h>
#include <game/actor_component/ActorComponentShapeParam.h>

namespace application::game::actor_component
{
	std::unordered_map<ActorComponentBase::ComponentType, std::function<std::unique_ptr<ActorComponentBase>(application::game::ActorPack&)>> ActorComponentFactory::gFactories;

	std::unique_ptr<application::game::actor_component::ActorComponentBase> ActorComponentFactory::MakeActorComponent(application::game::actor_component::ActorComponentBase::ComponentType Type, application::game::ActorPack& Pack)
	{
		auto Iter = gFactories.find(Type);
		return Iter != gFactories.end() ? Iter->second(Pack) : nullptr;
	}

	void ActorComponentFactory::Initialize()
	{
		RegisterComponent<application::game::actor_component::ActorComponentModelInfo>(ActorComponentBase::ComponentType::MODEL_INFO);
		RegisterComponent<application::game::actor_component::ActorComponentShapeParam>(ActorComponentBase::ComponentType::SHAPE_PARAM);
	}
}