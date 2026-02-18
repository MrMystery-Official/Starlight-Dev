#pragma once

#include <game/actor_component/ActorComponentBase.h>
#include <game/ActorPack.h>
#include <memory>
#include <functional>
#include <util/Logger.h>

namespace application::game::actor_component
{
	namespace ActorComponentFactory
	{
		extern std::unordered_map<ActorComponentBase::ComponentType, std::function<std::unique_ptr<ActorComponentBase>(application::game::ActorPack&)>> gFactories;

        void Initialize();

		std::unique_ptr<application::game::actor_component::ActorComponentBase> MakeActorComponent(application::game::actor_component::ActorComponentBase::ComponentType Type, application::game::ActorPack& Pack);
		
        template<typename T>
        void RegisterComponent(ActorComponentBase::ComponentType Type)
        {
            application::util::Logger::Info("ActorComponentFactory", "Registered component \"%s\"", T::GetInternalNameImpl().c_str());
            gFactories[Type] = [](application::game::ActorPack& Pack) -> std::unique_ptr<ActorComponentBase> 
                {
                    if constexpr (requires { T::ContainsComponent(Pack); })
                    {
                        if (T::ContainsComponent(Pack))
                        {
                            return std::make_unique<T>(Pack);
                        }
                        return nullptr;
                    }
                    else 
                    {
                        return std::make_unique<T>(Pack);
                    }
                };
        }
	}
}