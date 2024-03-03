#pragma once

#include "Byml.h"
#include <string>

namespace PopupEditorAINBActorLinks
{
	extern bool IsOpen;
	extern bool FirstFrame;

	void DisplayInputText(std::string Key, BymlFile::Node* Node);
	void DisplayBymlAinbActorLinks(BymlFile& File);

	void Render();
	void Open();
};