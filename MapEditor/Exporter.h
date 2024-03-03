#pragma once

#include <string>
#include "Byml.h"
#include "ActorMgr.h"

namespace Exporter
{
	enum class Operation : uint8_t
	{
		Export = 0,
		Save = 1
	};

	void CreateExportOnlyFiles(std::string Path);
	void CreateRSTBL(std::string Path);
	void Export(std::string Path, Exporter::Operation Op);
}