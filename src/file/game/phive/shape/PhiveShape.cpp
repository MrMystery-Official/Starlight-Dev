#include "PhiveShape.h"

#include <file/game/phive/classes/HavokClassBinaryVectorReader.h>
#include <file/game/phive/shape/PhiveShapeBVH.h>
#include <file/game/phive/shape/PhiveShapeUtil.h>
#include <util/Logger.h>
#include <util/Math.h>
#include <glm/vec4.hpp>
#include <meshoptimizer.h>
#include <unordered_map>

#include <file/game/phive/starlight_physics/shape/hknpMeshShapeBuilder.h>

namespace application::file::game::phive::shape
{
	PhiveShape::PhiveShape(const std::vector<unsigned char>& Bytes)
	{
		application::file::game::phive::classes::HavokClassBinaryVectorReader Reader(Bytes);

		Reader.ReadStruct(&mHeader, sizeof(PhiveShapeHeader));

		std::vector<unsigned char> TagFileData(mHeader.mHktSize);
		Reader.ReadStruct(TagFileData.data(), TagFileData.size());
		mTagFile.LoadFromBytes(TagFileData, false);


		uint32_t MaterialCount = mHeader.mMaterialsSize / 16;
		mMaterials.resize(MaterialCount);

		for (uint32_t i = 0; i < MaterialCount; i++)
		{
			application::file::game::phive::util::PhiveMaterialData::PhiveMaterial& Material = mMaterials[i];

			Reader.Seek(mHeader.mMaterialsOffset + i * 16, application::util::BinaryVectorReader::Position::Begin);
			Material.mMaterialId = Reader.ReadUInt32();
			Material.mUnknown0 = Reader.ReadUInt32();
			uint64_t Flags = Reader.ReadUInt64();
			for (size_t j = 0; j < application::file::game::phive::util::PhiveMaterialData::gMaterialFlagMasks.size(); j++)
			{
				Material.mFlags[j] = (Flags & application::file::game::phive::util::PhiveMaterialData::gMaterialFlagMasks[j]) == application::file::game::phive::util::PhiveMaterialData::gMaterialFlagMasks[j];
			}

			Reader.Seek(mHeader.mCollisionFlagsOffset + i * 8, application::util::BinaryVectorReader::Position::Begin);
			uint64_t CollisionFlags = Reader.ReadUInt64();

			Material.mCollisionFlags[0] = true; //HitAll - 0xFFFFFFFF
		}
	}

	std::vector<unsigned char> PhiveShape::ToBinary()
	{
		application::file::game::phive::classes::HavokClassBinaryVectorWriter Writer;

		Writer.Seek(48, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

		mTagFile.InitializeItemRegistration();

		Writer.InstallWriteItemCallback([this](const std::string& Type, const std::string& ChildName, uint32_t DataOffset, uint32_t Count)
			{
				uint16_t TypeIndex = 0;
				for (size_t i = 0; i < mTagFile.mTypes.size(); i++)
				{
					if (mTagFile.mTypes[i].mName == Type)
					{
						if (ChildName.empty())
						{
							TypeIndex = i;
							break;
						}
					}

					if (mTagFile.mTypes[i].mName == ChildName)
					{
						TypeIndex = i + 1;
						break;
					}
				}

				uint8_t Flags = 0x20;
				if (Type == "hknpMeshShape") Flags = 0x0;
				if (Type == "hknpCompositeShape") Flags = 0x10;

				if (Type == "hkcdSimdTree::Node") TypeIndex++;

				return mTagFile.RegisterItem(TypeIndex, Flags, DataOffset - 0x50, TypeIndex == 0 ? 0 : Count);
			});

		mTagFile.mRootClass.MarkAsItem(); //hknpMeshShape
		mTagFile.mRootClass.mParent.MarkAsItem(); //hknpCompositeShape

		mTagFile.mRootClass.mShapeTagTable.MarkAsItem(); //hknpMeshShape::ShapeTagTableEntry
		mTagFile.mRootClass.mTopLevelTree.mNodes.MarkAsItem(); //hkcdSimdTree::Node
		mTagFile.mRootClass.mGeometrySections.MarkAsItem(); //hknpMeshShape::GeometrySection
		mTagFile.mRootClass.mGeometrySections.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection& GeometrySection) { GeometrySection.mSectionBvh.MarkAsItem(); });
		mTagFile.mRootClass.mGeometrySections.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection& GeometrySection) { GeometrySection.mPrimitives.MarkAsItem(); });
		mTagFile.mRootClass.mGeometrySections.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection& GeometrySection) { GeometrySection.mVertexBuffer.MarkAsItem(); });
		mTagFile.mRootClass.mGeometrySections.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection& GeometrySection) { GeometrySection.mInteriorPrimitiveBitField.MarkAsItem(); });

		mTagFile.ToBinary(Writer);

		Writer.Align(16);
		mHeader.mMaterialsOffset = Writer.GetPosition();
		mHeader.mMaterialsSize = mMaterials.size() * 16;

		for (size_t i = 0; i < mMaterials.size(); i++)
		{
			Writer.WriteInteger(mMaterials[i].mMaterialId, sizeof(uint32_t));
			Writer.WriteInteger(mMaterials[i].mUnknown0, sizeof(uint32_t));

			uint64_t Flags = 0;

			for (size_t j = 0; j < application::file::game::phive::util::PhiveMaterialData::gMaterialFlagNames.size(); j++)
			{
				if (!mMaterials[i].mFlags[j])
					continue;

				Flags |= application::file::game::phive::util::PhiveMaterialData::gMaterialFlagMasks[j];
			}

			Writer.WriteInteger(Flags, sizeof(uint64_t));
		}

		mHeader.mCollisionFlagsOffset = Writer.GetPosition();
		mHeader.mCollisionFlagsSize = mMaterials.size() * 8;

		for (size_t i = 0; i < mMaterials.size(); i++)
		{
			uint64_t Filters = 0;
			//Generating base
			for (size_t j = 0; j < application::file::game::phive::util::PhiveMaterialData::gMaterialFilterNames.size(); j++)
			{
				if (!mMaterials[i].mCollisionFlags[j])
					continue;

				if (application::file::game::phive::util::PhiveMaterialData::gMaterialFilterIsBase[j])
					Filters |= application::file::game::phive::util::PhiveMaterialData::gMaterialFilterMasks[j];
			}
			//Applying filters
			for (size_t j = 0; j < application::file::game::phive::util::PhiveMaterialData::gMaterialFilterNames.size(); j++)
			{
				if (!mMaterials[i].mCollisionFlags[j])
					continue;

				if (!application::file::game::phive::util::PhiveMaterialData::gMaterialFilterIsBase[j])
					Filters &= application::file::game::phive::util::PhiveMaterialData::gMaterialFilterMasks[j];
			}
			Writer.WriteInteger(0xFFFFFFFF, sizeof(uint32_t));
			Writer.WriteInteger(Filters, sizeof(uint32_t));
		}

		Writer.Align(16);
		mHeader.mFileSize = Writer.GetPosition();
		mHeader.mHktOffset = 48;
		mHeader.mHktSize = mHeader.mMaterialsOffset - 48;

		Writer.Seek(0, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mHeader), 48);

		return Writer.GetData();
	}

	std::vector<float> PhiveShape::ToVertices()
	{
		std::vector<float> Vertices;

		uint32_t MemSize = 0;

		for (application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection& GeometrySection : mTagFile.mRootClass.mGeometrySections.mElements)
		{
			MemSize += GeometrySection.GetVertexCount() * 3;
		}

		Vertices.reserve(MemSize);

		for (application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection& GeometrySection : mTagFile.mRootClass.mGeometrySections.mElements)
		{
			for (size_t i = 0; i < GeometrySection.GetVertexCount(); i++)
			{
				uint16_t X = (static_cast<uint16_t>(GeometrySection.mVertexBuffer.mElements[i * 6 + 1].mParent) << 8) | GeometrySection.mVertexBuffer.mElements[i * 6 + 0].mParent;
				uint16_t Y = (static_cast<uint16_t>(GeometrySection.mVertexBuffer.mElements[i * 6 + 3].mParent) << 8) | GeometrySection.mVertexBuffer.mElements[i * 6 + 2].mParent;
				uint16_t Z = (static_cast<uint16_t>(GeometrySection.mVertexBuffer.mElements[i * 6 + 5].mParent) << 8) | GeometrySection.mVertexBuffer.mElements[i * 6 + 4].mParent;

				Vertices.push_back(X * mTagFile.mRootClass.mVertexConversionUtil.mBitScale16Inv.mParent.mData[0] + GeometrySection.mBitOffset[0].mParent * GeometrySection.mBitScale8Inv.mX);
				Vertices.push_back(Y * mTagFile.mRootClass.mVertexConversionUtil.mBitScale16Inv.mParent.mData[1] + GeometrySection.mBitOffset[1].mParent * GeometrySection.mBitScale8Inv.mY);
				Vertices.push_back(Z * mTagFile.mRootClass.mVertexConversionUtil.mBitScale16Inv.mParent.mData[2] + GeometrySection.mBitOffset[2].mParent * GeometrySection.mBitScale8Inv.mZ);
			}
		}

		return Vertices;
	}

	std::vector<unsigned int> PhiveShape::ToIndices()
	{
		std::vector<unsigned int> Indices;

		uint32_t IndiceOffset = 0;
		uint32_t MemSize = 0;

		for (application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection& GeometrySection : mTagFile.mRootClass.mGeometrySections.mElements)
		{
			MemSize += GeometrySection.mPrimitives.mElements.size() * 6;
		}

		Indices.reserve(MemSize);

		for (application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection& GeometrySection : mTagFile.mRootClass.mGeometrySections.mElements)
		{
			for (application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection__Primitive& Primitive : GeometrySection.mPrimitives.mElements)
			{
				Indices.push_back(Primitive.mAId.mParent + IndiceOffset);
				Indices.push_back(Primitive.mBId.mParent + IndiceOffset);
				Indices.push_back(Primitive.mCId.mParent + IndiceOffset);

				Indices.push_back(Primitive.mAId.mParent + IndiceOffset);
				Indices.push_back(Primitive.mCId.mParent + IndiceOffset);
				Indices.push_back(Primitive.mDId.mParent + IndiceOffset);
			}
			IndiceOffset += GeometrySection.GetVertexCount();
		}

		return Indices;
	}

	// Custom hash function for glm::vec3
	struct Vec3Hash {
		std::size_t operator()(const glm::vec3& v) const {
			// Combine hashes of x, y, z components
			std::size_t h1 = std::hash<float>{}(v.x);
			std::size_t h2 = std::hash<float>{}(v.y);
			std::size_t h3 = std::hash<float>{}(v.z);

			// Use bit shifting and XOR for good hash distribution
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	// Custom equality function for glm::vec3 (handles floating point comparison)
	struct Vec3Equal {
		bool operator()(const glm::vec3& a, const glm::vec3& b) const {
			const float epsilon = 1e-6f; // Adjust tolerance as needed
			return std::abs(a.x - b.x) < epsilon &&
				std::abs(a.y - b.y) < epsilon &&
				std::abs(a.z - b.z) < epsilon;
		}
	};

	void PhiveShape::RemoveDuplicateVertices(GeneratorData& GeneratorData)
	{
		// Use unordered_map for O(1) average lookup time
		std::unordered_map<glm::vec3, uint32_t, Vec3Hash, Vec3Equal> vertexMap;
		std::vector<glm::vec3> newVertices;
		std::vector<std::pair<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t>> newIndices;

		// Reserve space to avoid reallocations
		newVertices.reserve(GeneratorData.mVertices.size());
		newIndices.reserve(GeneratorData.mIndices.size());

		// Lambda to get or create vertex index
		auto getVertexIndex = [&](const glm::vec3& vertex) -> uint32_t {
			auto it = vertexMap.find(vertex);
			if (it != vertexMap.end()) {
				return it->second; // Return existing index
			}

			// Add new vertex
			uint32_t newIndex = static_cast<uint32_t>(newVertices.size());
			newVertices.push_back(vertex);
			vertexMap[vertex] = newIndex;
			return newIndex;
			};

		// Process all triangles
		for (const auto& [face, material] : GeneratorData.mIndices) {
			uint32_t idA = getVertexIndex(GeneratorData.mVertices[std::get<0>(face)]);
			uint32_t idB = getVertexIndex(GeneratorData.mVertices[std::get<1>(face)]);
			uint32_t idC = getVertexIndex(GeneratorData.mVertices[std::get<2>(face)]);

			newIndices.emplace_back(std::make_tuple(idA, idB, idC), material);
		}

		// Update the original data
		newVertices.shrink_to_fit();
		GeneratorData.mVertices = std::move(newVertices);
		GeneratorData.mIndices = std::move(newIndices);
	}

	void PhiveShape::RunMeshOptimizer(GeneratorData& GeneratorData)
	{
		std::unordered_map<uint32_t, std::vector<uint32_t>> Indices;
		for (auto& [Face, Material] : GeneratorData.mIndices)
		{
			Indices[Material].push_back(std::get<0>(Face));
			Indices[Material].push_back(std::get<1>(Face));
			Indices[Material].push_back(std::get<2>(Face));
		}

		std::unordered_map<uint32_t, std::vector<uint32_t>> SimplifiedIndices;
		for (auto& [Material, Faces] : Indices)
		{
			SimplifiedIndices[Material].resize(Faces.size());
			SimplifiedIndices[Material].resize(meshopt_simplify(&SimplifiedIndices[Material][0], &Faces[0], Faces.size(), &GeneratorData.mVertices[0].x, GeneratorData.mVertices.size(), sizeof(glm::vec3), 0, GeneratorData.mMeshOptimizerTriangleReductionMaxError, meshopt_SimplifyLockBorder));
		}

		GeneratorData.mIndices.clear();
		for (auto& [Material, Faces] : SimplifiedIndices)
		{
			for (size_t i = 0; i < Faces.size(); i += 3)
			{
				GeneratorData.mIndices.push_back(std::make_pair(std::make_tuple(Faces[i], Faces[i + 1], Faces[i + 2]), Material));
			}
		}
	}

	void PhiveShape::InjectModel(GeneratorData& GeneratorData)
	{
		mMaterials = GeneratorData.mMaterials;

		application::util::Logger::Info("PhiveShape::InjectModel", "Preparing geometry...");

		RemoveDuplicateVertices(GeneratorData); //This is done to remove unreferenced and thus unnecessary vertices

		if (GeneratorData.mRunMeshOptimizer)
		{
			RunMeshOptimizer(GeneratorData); //Run the mesh optimizer
			RemoveDuplicateVertices(GeneratorData); //This is done to remove unreferenced and thus unnecessary vertices
		}

		application::util::Logger::Info("PhiveShape::InjectModel", "Constructing internal geometry...");
		starlight_physics::common::hkGeometry Geometry;

		//Injecting mesh
		{
			auto& Vertices = Geometry.mVertices;
			auto& Triangles = Geometry.mTriangles;

			for (int i = 0; i < GeneratorData.mVertices.size(); i++)
			{
				glm::vec4 Vertex(GeneratorData.mVertices[i].x, GeneratorData.mVertices[i].y, GeneratorData.mVertices[i].z, 0.0f);
				Vertices.push_back(Vertex);
			}

			//Triangles.m_data = reinterpret_cast<hkGeometry::Triangle*>(new char[sizeof(hkGeometry::Triangle) * Generator.mIndices.size()]);
			//Triangles.m_size = Generator.mIndices.size();
			for (size_t i = 0; i < GeneratorData.mIndices.size(); i++)
			{
				Triangles.emplace_back(std::get<0>(GeneratorData.mIndices[i].first), std::get<1>(GeneratorData.mIndices[i].first), std::get<2>(GeneratorData.mIndices[i].first), GeneratorData.mIndices[i].second);
			}
		}

		starlight_physics::shape::hknpMeshShapeBuilder Builder;

		application::util::Logger::Info("PhiveShape::InjectModel", "Starting collision mesh generation");
		Builder.compile(&mTagFile.mRootClass, Geometry);
		application::util::Logger::Info("PhiveShape::InjectModel", "Collision generation completed");

		//application::file::game::phive::shape::PhiveShapeUtil::GeometryUtil::ApplyGeometryOptimizer(GeneratorData);

		/*
		std::vector<std::pair<application::file::game::phive::shape::PhiveShapeUtil::TriangleToQuadConversionUtil::Quad, uint32_t>> QuadGeometry = application::file::game::phive::shape::PhiveShapeUtil::TriangleToQuadConversionUtil::ConvertTrianglesToQuads(GeneratorData);

		std::cout << "Geometry prepared.\n";

		application::file::game::phive::shape::PhiveShapeBVH::BVH BVH;

		std::vector<application::file::game::phive::shape::PhiveShapeBVH::Primitive> BVHPrimitives;
		BVHPrimitives.reserve(QuadGeometry.size());

		std::vector<uint32_t> QuadIndices;
		QuadIndices.reserve(QuadGeometry.size() * 4);
		std::vector<uint32_t> PrimitiveMaterialInfo; //1/4 of the size of QuadIndices
		PrimitiveMaterialInfo.reserve(QuadGeometry.size());

		for (size_t i = 0; i < QuadGeometry.size(); i++)
		{
			uint32_t IdA = QuadGeometry[i].first.mIndices[0];
			uint32_t IdB = QuadGeometry[i].first.mIndices[1];
			uint32_t IdC = QuadGeometry[i].first.mIndices[2];
			uint32_t IdD = QuadGeometry[i].first.mIndices[3];

			const glm::vec3& PointA = GeneratorData.mVertices[IdA];
			const glm::vec3& PointB = GeneratorData.mVertices[IdB];
			const glm::vec3& PointC = GeneratorData.mVertices[IdC];
			const glm::vec3& PointD = GeneratorData.mVertices[IdD];

			glm::vec3 MinPoint(std::min({ PointA.x, PointB.x, PointC.x, PointD.x }), std::min({ PointA.y, PointB.y, PointC.y, PointD.y }), std::min({ PointA.z, PointB.z, PointC.z, PointD.z }));
			glm::vec3 MaxPoint(std::max({ PointA.x, PointB.x, PointC.x, PointD.x }), std::max({ PointA.y, PointB.y, PointC.y, PointD.y }), std::max({ PointA.z, PointB.z, PointC.z, PointD.z }));

			//Making the bounding boxes of primitives bigger to ensure they are loaded when link steps on them
			MinPoint.x -= 1.0f;
			MinPoint.y -= 1.0f;
			MinPoint.z -= 1.0f;

			MaxPoint.x += 1.0f;
			MaxPoint.y += 1.0f;
			MaxPoint.z += 1.0f;

			BVHPrimitives.emplace_back(i, application::file::game::phive::shape::PhiveShapeBVH::BoundingBox(MinPoint.x, MinPoint.y, MinPoint.z, MaxPoint.x, MaxPoint.y, MaxPoint.z));
			
			QuadIndices.push_back(IdA);
			QuadIndices.push_back(IdB);
			QuadIndices.push_back(IdC);
			QuadIndices.push_back(IdD);

			PrimitiveMaterialInfo.push_back(QuadGeometry[i].second);
		}

		std::cout << "Building BVH...\n";
		BVH.Build(BVHPrimitives);

		BVH.GetRoot()->ComputePrimitiveCounts();
		BVH.GetRoot()->ComputeUniqueIndicesCounts(QuadIndices);
		BVH.GetRoot()->mIsSectionHead = true;
		BVH.GetRoot()->AttemptSectionSplit();

		std::cout << "BVH built.\n";

		//BVH.PrintTree();

		std::vector<application::file::game::phive::shape::PhiveShapeBVH::BVHNode*> SectionBVHs;
		std::function<void(application::file::game::phive::shape::PhiveShapeBVH::BVHNode* Node)> CollectSectionBVHs = [&SectionBVHs, &CollectSectionBVHs](application::file::game::phive::shape::PhiveShapeBVH::BVHNode* Node)
			{
				if (Node->mIsSectionHead)
				{
					SectionBVHs.push_back(Node);
				}
				else
				{
					for (int i = 0; i < 4; i++)
					{
						if (Node->mChildren[i])
						{
							CollectSectionBVHs(Node->mChildren[i].get());
						}
					}
				}
			};
		CollectSectionBVHs(BVH.GetRoot());

		auto WriteVertex = [](std::vector<glm::vec3>& VertexGroup, const glm::vec3& Vertex)
			{
				for (size_t i = 0; i < VertexGroup.size(); i++)
				{
					if (VertexGroup[i] == Vertex)
					{
						return i;
					}
				}

				VertexGroup.push_back(Vertex);
				return VertexGroup.size() - 1;
			};

		float MaxVertexError = 1e-3f;

		//VertexConversionUtil Setup
		{
			const float Step = std::pow(2.0f, (int)(*reinterpret_cast<uint32_t*>(&MaxVertexError) >> 23) - 126);

			mTagFile.mRootClass.mVertexConversionUtil.mBitScale16Inv.mParent.mData[0] = Step;
			mTagFile.mRootClass.mVertexConversionUtil.mBitScale16Inv.mParent.mData[1] = Step;
			mTagFile.mRootClass.mVertexConversionUtil.mBitScale16Inv.mParent.mData[2] = Step;
			mTagFile.mRootClass.mVertexConversionUtil.mBitScale16Inv.mParent.mData[3] = 0;

			mTagFile.mRootClass.mVertexConversionUtil.mBitScale16.mParent.mData[0] = 1.0f / Step;
			mTagFile.mRootClass.mVertexConversionUtil.mBitScale16.mParent.mData[1] = 1.0f / Step;
			mTagFile.mRootClass.mVertexConversionUtil.mBitScale16.mParent.mData[2] = 1.0f / Step;
			mTagFile.mRootClass.mVertexConversionUtil.mBitScale16.mParent.mData[3] = 0;
		}

		//GeometrySections Setup
		mTagFile.mRootClass.mGeometrySections.mElements.clear();
		mTagFile.mRootClass.mGeometrySections.mElements.resize(SectionBVHs.size());

		std::vector<std::vector<uint32_t>> SectionShapeTags;
		SectionShapeTags.resize(mTagFile.mRootClass.mGeometrySections.mElements.size());

		for (size_t i = 0; i < mTagFile.mRootClass.mGeometrySections.mElements.size(); i++)
		{
			application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection& GeometrySection = mTagFile.mRootClass.mGeometrySections.mElements[i];

			std::vector<glm::vec3> SectionVertices;
			std::vector<uint32_t> SectionIndices;

			std::function<void(application::file::game::phive::shape::PhiveShapeBVH::BVHNode* Node)> CollectSectionIndicesVertices = [&CollectSectionIndicesVertices, &SectionVertices, &SectionIndices, &QuadIndices, &GeneratorData, &WriteVertex, &i, &SectionShapeTags, &PrimitiveMaterialInfo](application::file::game::phive::shape::PhiveShapeBVH::BVHNode* Node)
				{
					for (int j = 0; j < 4; j++)
					{
						if (Node->mChildren[j])
						{
							if (Node->mChildren[j]->mIsLeaf)
							{
								for (int k = 0; k < 4; k++)
								{
									if (Node->mChildren[j]->mPrimitiveIndices[k] == -1)
										continue;

									const glm::vec3& PointA = GeneratorData.mVertices[QuadIndices[Node->mChildren[j]->mPrimitiveIndices[k] * 4]];
									const glm::vec3& PointB = GeneratorData.mVertices[QuadIndices[Node->mChildren[j]->mPrimitiveIndices[k] * 4 + 1]];
									const glm::vec3& PointC = GeneratorData.mVertices[QuadIndices[Node->mChildren[j]->mPrimitiveIndices[k] * 4 + 2]];
									const glm::vec3& PointD = GeneratorData.mVertices[QuadIndices[Node->mChildren[j]->mPrimitiveIndices[k] * 4 + 3]];

									SectionIndices.push_back(WriteVertex(SectionVertices, PointA));
									SectionIndices.push_back(WriteVertex(SectionVertices, PointB));
									SectionIndices.push_back(WriteVertex(SectionVertices, PointC));
									SectionIndices.push_back(WriteVertex(SectionVertices, PointD));

									SectionShapeTags[i].push_back(PrimitiveMaterialInfo[Node->mChildren[j]->mPrimitiveIndices[k]]);

									Node->mChildren[j]->mPrimitiveIndices[k] = SectionIndices.size() / 4 - 1;
								}
							}
							else
							{
								CollectSectionIndicesVertices(Node->mChildren[j].get());
							}
						}
					}
				};
			CollectSectionIndicesVertices(SectionBVHs[i]);


			glm::vec3 MinVertex(std::numeric_limits<float>::max());
			glm::vec3 MaxVertex(std::numeric_limits<float>::lowest());

			for (const auto& Vertex : SectionVertices) 
			{
				MinVertex = glm::min(MinVertex, Vertex);
				MaxVertex = glm::max(MaxVertex, Vertex);
			}

			application::file::game::phive::shape::PhiveShapeUtil::GeometrySectionConversionUtil ConversionUtil(MinVertex, MaxVertex);
			GeometrySection.mBitOffset[0].mParent = ConversionUtil.mBitOffset16.x;
			GeometrySection.mBitOffset[1].mParent = ConversionUtil.mBitOffset16.y;
			GeometrySection.mBitOffset[2].mParent = ConversionUtil.mBitOffset16.z;

			GeometrySection.mBitScale8Inv.mX = ConversionUtil.mBitScaleInv.x;
			GeometrySection.mBitScale8Inv.mY = ConversionUtil.mBitScaleInv.y;
			GeometrySection.mBitScale8Inv.mZ = ConversionUtil.mBitScaleInv.z;

			glm::i32vec3 SectionOffset = ConversionUtil.GetSectionOffset(mTagFile.mRootClass.mVertexConversionUtil);

			GeometrySection.mSectionOffset[0].mParent = SectionOffset.x;
			GeometrySection.mSectionOffset[1].mParent = SectionOffset.y;
			GeometrySection.mSectionOffset[2].mParent = SectionOffset.z;

			{
				GeometrySection.mVertexBuffer.mElements.resize(SectionVertices.size() * 6);
				for (size_t j = 0; j < SectionVertices.size(); j++)
				{
					glm::i16vec3 NewVertex = ConversionUtil.CompressVertex(mTagFile.mRootClass.mVertexConversionUtil, SectionOffset, SectionVertices[j]);

					GeometrySection.mVertexBuffer.mElements[j * 6].mParent = static_cast<uint8_t>(NewVertex.x & 0x00FF);
					GeometrySection.mVertexBuffer.mElements[j * 6 + 1].mParent = static_cast<uint8_t>((NewVertex.x >> 8) & 0x00FF);

					GeometrySection.mVertexBuffer.mElements[j * 6 + 2].mParent = static_cast<uint8_t>(NewVertex.y & 0x00FF);
					GeometrySection.mVertexBuffer.mElements[j * 6 + 3].mParent = static_cast<uint8_t>((NewVertex.y >> 8) & 0x00FF);

					GeometrySection.mVertexBuffer.mElements[j * 6 + 4].mParent = static_cast<uint8_t>(NewVertex.z & 0x00FF);
					GeometrySection.mVertexBuffer.mElements[j * 6 + 5].mParent = static_cast<uint8_t>((NewVertex.z >> 8) & 0x00FF);
				}

				//The last geometry section needs two extra bytes in the vertex buffer array (no idea why...)
				if (i == mTagFile.mRootClass.mGeometrySections.mElements.size() - 1)
				{
					application::file::game::phive::classes::HavokClasses::hkUint8 NullInt;
					NullInt.mParent = 0;

					GeometrySection.mVertexBuffer.mElements.push_back(NullInt);
					GeometrySection.mVertexBuffer.mElements.push_back(NullInt);
				}
			}

			for (size_t j = 0; j < SectionIndices.size() / 4; j++)
			{
				application::file::game::phive::classes::HavokClasses::hknpMeshShape__GeometrySection__Primitive Primitive;
				Primitive.mAId.mParent = SectionIndices[j * 4];
				Primitive.mBId.mParent = SectionIndices[j * 4 + 1];
				Primitive.mCId.mParent = SectionIndices[j * 4 + 2];
				Primitive.mDId.mParent = SectionIndices[j * 4 + 3];
				GeometrySection.mPrimitives.mElements.push_back(Primitive);
			}

			//Clearing m_InteriorPrimitiveBitField
			GeometrySection.mInteriorPrimitiveBitField.mElements.resize((int)std::ceil((float)(2 * (SectionIndices.size() / 4)) / 8));
			for (application::file::game::phive::classes::HavokClasses::hkUint8& Byte : GeometrySection.mInteriorPrimitiveBitField.mElements)
			{
				Byte.mParent = 0;
			}

			GeometrySection.mSectionBvh = BVH.BuildAabb8Tree(SectionBVHs[i], ConversionUtil);
		}

		
		std::cout << "Building TopLevelTree...\n";
		application::file::game::phive::shape::PhiveShapeBVH::BVH TopLevelTreeBVH;

		std::vector<application::file::game::phive::shape::PhiveShapeBVH::Primitive> TopLevelTreeBVHPrimitives;

		for (size_t i = 0; i < SectionBVHs.size(); i++)
		{
			application::file::game::phive::shape::PhiveShapeBVH::BoundingBox Box = SectionBVHs[i]->mChildBounds[0];
			if (SectionBVHs[i]->mChildBounds[1].IsValid()) Box.Expand(SectionBVHs[i]->mChildBounds[1]);
			if (SectionBVHs[i]->mChildBounds[2].IsValid()) Box.Expand(SectionBVHs[i]->mChildBounds[2]);
			if (SectionBVHs[i]->mChildBounds[3].IsValid()) Box.Expand(SectionBVHs[i]->mChildBounds[3]);

			TopLevelTreeBVHPrimitives.emplace_back(i, Box);

		}

		TopLevelTreeBVH.Build(TopLevelTreeBVHPrimitives);

		mTagFile.mRootClass.mTopLevelTree = TopLevelTreeBVH.BuildHkcdSimdTree(TopLevelTreeBVH.GetRoot(), true);

		//Setup shape tag table
		{
			mTagFile.mRootClass.mShapeTagTable.mElements.clear();

			application::file::game::phive::classes::HavokClasses::hknpMeshShape__ShapeTagTableEntry Entry;
			Entry.mMeshPrimitiveKey.mParent = 0;
			Entry.mShapeTag.mParent = !SectionShapeTags.empty() ? SectionShapeTags[0][0] : 0xFFFF;
			mTagFile.mRootClass.mShapeTagTable.mElements.push_back(Entry);

			int SectionIndex = 0;
			for (const auto& Section : mTagFile.mRootClass.mGeometrySections.mElements)
			{
				for (int PrimitiveIndex = 0; PrimitiveIndex < Section.mPrimitives.mElements.size(); PrimitiveIndex++)
				{
					const uint32_t& ShapeTag = SectionShapeTags[SectionIndex][PrimitiveIndex];
					if (mTagFile.mRootClass.mShapeTagTable.mElements.back().mShapeTag.mParent != ShapeTag)
					{
						Entry.mMeshPrimitiveKey.mParent = (SectionIndex << 9) | (PrimitiveIndex << 1) | 0;
						Entry.mShapeTag.mParent = ShapeTag;
						mTagFile.mRootClass.mShapeTagTable.mElements.push_back(Entry);
					}
				}

				SectionIndex++;
			}

			Entry.mMeshPrimitiveKey.mParent = (mTagFile.mRootClass.mGeometrySections.mElements.size() << 9) | (0xFF << 1) | 0;
			Entry.mShapeTag.mParent = 0xFFFF;
			mTagFile.mRootClass.mShapeTagTable.mElements.push_back(Entry);
		}

		uint8_t SectionBitCount = 32 - static_cast<uint8_t>(application::util::Math::CountLeadingZeros(mTagFile.mRootClass.mGeometrySections.mElements.size() - 1));
		mTagFile.mRootClass.mParent.mParent.mNumShapeKeyBits.mParent = SectionBitCount + 8 + 1;

		std::cout << "Model injection completed\n";
		*/
	}
}