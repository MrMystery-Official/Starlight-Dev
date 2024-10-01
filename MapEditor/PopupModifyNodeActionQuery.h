#pragma once

#include <string>
#include "BFEVFL.h"

namespace PopupModifyNodeActionQuery
{
	extern bool IsOpen;
	extern int16_t* mActorIndex;
	extern int16_t* mActionQueryIndex;
	extern std::vector<const char*> mActorNames;
	extern std::vector<const char*> mActionQueryNames;
	extern BFEVFLFile* mEventFile;
	extern BFEVFLFile::Container* mParameters;
	extern bool mIsAction;
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void CalculateActionQueryNames();
	void CalculateActorNames();
	void RebuildNodeParameters();
	void Open(int16_t& ActorIndex, int16_t& ActionQueryIndex, BFEVFLFile::Container& Parameters, bool IsAction, BFEVFLFile& EventFile);
};