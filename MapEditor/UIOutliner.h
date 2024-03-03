#pragma once

#include "Actor.h"
#include "Camera.h"

namespace UIOutliner
{
	extern bool Open;
	extern Actor* SelectedActor;

	void Initalize(Camera& Cam);
	void SelectActor(Actor* pActor);
	void DrawOutlinerWindow();
};