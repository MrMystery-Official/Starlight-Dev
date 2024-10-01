#pragma once

#include <string>
#include "Byml.h"

namespace SceneCreator
{
	void ReplaceStringInBymlNodes(BymlFile::Node* Node, std::string Search, std::string Replace, std::string DoesNotContain = "");
	void CreateScene(std::string Identifier, std::string Template);
}