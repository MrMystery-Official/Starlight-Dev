#pragma once

#include <string>
#include <variant>
#include "Actor.h"

namespace PopupAddDynamicData
{
	extern bool IsOpen;
	extern std::string Key;
	extern std::string ActorName;
	extern bool ActorNameValid;
	extern bool ShowActorSpecific;
	extern int DataType;
	extern std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, Vector3F> Value;
	extern void (*Func)(std::string, Actor::DynamicData);
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open(std::string Name, void (*Callback)(std::string, Actor::DynamicData));
};