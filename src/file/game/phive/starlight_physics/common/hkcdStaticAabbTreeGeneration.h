#pragma once

#include <file/game/phive/classes/HavokClasses.h>
#include <glm/vec3.hpp>

namespace application::file::game::phive::starlight_physics::common::hkcdStaticAabbTreeGeneration
{
	struct InternalNode
	{
		bool mIsLeaf = false;
		InternalNode* mLeft = nullptr;
		InternalNode* mRight = nullptr;
		glm::vec3 mMax;
		glm::vec3 mMin;
		uint32_t mPrimitive = 0;
		uint32_t mPrimitiveCount = 0;

		std::vector<classes::HavokClasses::hkcdCompressedAabbCodecs__Aabb6BytesCodec> buildAxis6ByteTree();
	};

	void buildFromAabbs(classes::HavokClasses::hkcdStaticAabbTree* dst, const std::vector<common::hkAabb>& aabbs);
}