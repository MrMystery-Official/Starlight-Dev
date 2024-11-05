#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <variant>
#include <cstdint>
#include <unordered_map>
#include "Byml.h"

namespace RailMgr
{
	struct Rail
	{
		struct Point
		{
			struct Dynamic
			{
				std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, glm::vec3> mData;
				BymlFile::Type mType;
			};

			std::string mGyml = "";
			uint64_t mHash = 0;
			float mNextDistance = FLT_MAX;
			float mPrevDistance = FLT_MAX;
			glm::vec3 mTranslate;
			std::unordered_map<std::string, Dynamic> mDynamic;
		};

		std::string mGyml = "";
		std::string mName = "";
		uint64_t mHash = 0;
		bool mIsClosed = false;
		std::vector<Point> mPoints;
	};

	extern std::vector<Rail> mRails;

	void LoadRailsForCurrentScene();
	BymlFile::Node RailToNode(RailMgr::Rail& Rail);
}