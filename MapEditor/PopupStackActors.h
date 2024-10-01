#pragma once

#include <cinttypes>

namespace PopupStackActors
{
	extern bool IsOpen;
	extern uint64_t SrcActor;
	extern float OffsetX;
	extern float OffsetY;
	extern float OffsetZ;
	extern uint16_t Count;
	extern void (*Func)(uint64_t, float, float, float, uint16_t);
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open(void (*Callback)(uint64_t, float, float, float, uint16_t), uint64_t Src = 0);
};