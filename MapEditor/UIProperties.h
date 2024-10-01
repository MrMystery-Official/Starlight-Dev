#pragma once

#include <vector>

namespace UIProperties
{
	extern bool Open;
	extern bool FirstFrame;
	extern std::vector<const char*> mFluidShapes;
	extern std::vector<const char*> mFluidTypes;

	void DrawPropertiesWindow();
};