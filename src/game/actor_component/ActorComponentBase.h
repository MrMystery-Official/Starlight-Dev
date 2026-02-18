#pragma once

#include <string>
#include <file/game/sarc/SarcFile.h>

namespace application::game::actor_component
{
	struct ActorComponentBase
	{
		enum class ComponentType : uint8_t
		{
			BASE = 0,
			MODEL_INFO = 1,
			SHAPE_PARAM = 2
		};

		virtual const ComponentType GetComponentType() const = 0;
		virtual const std::string GetDisplayName() const = 0;
		virtual const std::string GetInternalName() const = 0;
		virtual void DrawEditingMenu() {}
	};
}