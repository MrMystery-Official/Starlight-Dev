#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <file/game/phive/classes/HavokTagFile.h>
#include <file/game/phive/classes/HavokClasses.h>
#include <file/game/phive/starlight_physics/ai/hkaiNavMeshBuilder.h>
#include <glm/vec3.hpp>

namespace application::file::game::phive::navmesh
{
	class PhiveNavMesh
	{
	public:
		struct PhiveNavMeshHeader
		{
			char mMagic[6]; //Includes null terminator
			uint16_t mReserve1 = 0;
			uint16_t mBOM = 0;
			uint8_t mFileType = 0; //1 for NavMesh
			uint8_t mSectionCount = 0; //3 for NavMesh

			uint32_t mTagFileOffset = 0; //Usually 0x30(48) for NavMesh (Section 0)
			uint32_t _mUnkOffset1 = 0; //Usually 0x30(48) for NavMesh (Section 1)
			uint32_t _mUnkOffset2 = 0; //Usually 0x30(48) for NavMesh (Section 2)

			uint32_t mTagFileSize = 0;
			uint32_t _mUnkSize1 = 0;
			uint32_t _mUnkSize2 = 0;

			uint32_t _mPadding[3] = { 0 };
		};

		//Thanks to dt12345 who figured this struct out
		struct hkRotation
		{
			float mCol0[4];
			float mCol1[4];
			float mCol2[4];
		};

		// no idea wtf to call this struct but sure
		struct ReferenceRotation
		{
			uint32_t mSectionUid; // ignored if this is a dynamic NavMesh
			hkRotation mRotation;
		};

		struct GeneratorData
		{
			std::vector<glm::vec3> mVertices;
			std::vector<uint32_t> mIndices;

			bool mIsSingleSceneNavMesh = true;
			std::array<std::vector<unsigned char>, 4> mNeighbours;
			std::array<std::unique_ptr<PhiveNavMesh>, 4> mNeighbourNavMeshObjs = { };

			starlight_physics::ai::hkaiNavMeshGeometryGenerator::Config mGeometryConfig;
		};

		void Initialize(const std::vector<unsigned char>& Bytes);
		std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> ToVerticesIndices();
		uint8_t GetTypeNameFlag(std::string Name);
		std::vector<unsigned char> ToBinary();
		application::file::game::phive::classes::HavokTagFile<application::file::game::phive::classes::HavokClasses::hkNavMeshRootLevelContainer>& GetTagFile();

		void InjectNavMeshModel(GeneratorData& GeneratorData);

		PhiveNavMesh(const std::vector<unsigned char>& Bytes);
		PhiveNavMesh() = default;

		ReferenceRotation mReferenceRotation;

	private:
		PhiveNavMeshHeader mHeader;
		application::file::game::phive::classes::HavokTagFile<application::file::game::phive::classes::HavokClasses::hkNavMeshRootLevelContainer> mTagFile;

		std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> mModelOverride;
	};
}