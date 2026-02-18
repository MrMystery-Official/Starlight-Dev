#pragma once

#include <file/game/phive/classes/HavokTagFile.h>
#include <file/game/phive/classes/HavokClasses.h>
#include <file/game/phive/util/PhiveMaterialData.h>
#include <vector>
#include <glm/vec3.hpp>

namespace application::file::game::phive::shape
{
	class PhiveShape
	{
	public:
		struct PhiveShapeHeader
		{
			char mMagic[6]; //Includes null terminator
			uint16_t mReserve1 = 0;
			uint16_t mBOM = 0;
			uint8_t mMajorVersion = 0;
			uint8_t mMinorVersion = 0;
			uint32_t mHktOffset = 0;
			uint32_t mMaterialsOffset = 0;
			uint32_t mCollisionFlagsOffset = 0;
			uint32_t mFileSize = 0;
			uint32_t mHktSize = 0;
			uint32_t mMaterialsSize = 0;
			uint32_t mCollisionFlagsSize = 0;
			uint32_t mReserve2 = 0;
			uint32_t mReserve3 = 0;
		};

		struct GeneratorData
		{
			std::vector<application::file::game::phive::util::PhiveMaterialData::PhiveMaterial> mMaterials;

			std::vector<glm::vec3> mVertices;
			std::vector<std::pair<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t>> mIndices;

			float mMeshOptimizerTriangleReductionMaxError = 0.0001f;

			bool mRunMeshOptimizer = true;
		};

		PhiveShapeHeader mHeader;
		application::file::game::phive::classes::HavokTagFile<application::file::game::phive::classes::HavokClasses::hknpMeshShape> mTagFile;
		std::vector<application::file::game::phive::util::PhiveMaterialData::PhiveMaterial> mMaterials;

		std::vector<unsigned char> ToBinary();

		std::vector<float> ToVertices();
		std::vector<unsigned int> ToIndices();
		void InjectModel(GeneratorData& GeneratorData);

		PhiveShape(const std::vector<unsigned char>& Bytes);
		PhiveShape() = default;
	private:
		void RemoveDuplicateVertices(GeneratorData& GeneratorData);
		void RunMeshOptimizer(GeneratorData& GeneratorData);
	};
}