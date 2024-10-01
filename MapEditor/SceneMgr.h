#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "Bfres.h"
#include "PhiveNavMesh.h"
#include "PhiveStaticCompound.h"
#include <optional>
#include <glm/glm.hpp>

namespace SceneMgr
{
	enum class Type : int
	{
		SkyIslands = 0,
		MainField = 1,
		MinusField = 2,
		LargeDungeon = 3,
		SmallDungeon = 4,
		NormalStage = 5,
		Custom = 6
	};

	extern SceneMgr::Type SceneType;
	extern Mesh NavigationMeshModel;
	extern std::vector<PhiveStaticCompound> mStaticCompounds;

	void LoadScene(SceneMgr::Type Type, std::string Identifier);
	PhiveStaticCompound* GetStaticCompoundForPos(glm::vec3 Pos);
	Mesh LoadNavMeshBfres(PhiveNavMesh& NavMesh);
	void Reload();
};