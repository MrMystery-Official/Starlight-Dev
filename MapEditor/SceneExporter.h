#pragma once

#include <vector>
#include <string>

namespace SceneExporter
{
	struct Settings
	{
		bool mAlignToZero = true;
		bool mExportFarActors = false;
		std::vector<std::string> mFilterActors;
	};

	extern Settings mSettings;

	void ExportScene();
}