#pragma once

#include "Byml.h"
#include <string>

namespace PopupEditorAINBActorLinks
{
	extern bool IsOpen;
	extern bool FirstFrame;
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void DisplayInputText(std::string Key, BymlFile::Node* Node);
	void DisplayBymlAinbActorLinks(BymlFile& File);

	void Render();
	void Open();
};