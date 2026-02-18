#include "PhiveNavMesh.h"

#include <file/game/phive/classes/HavokClassBinaryVectorReader.h>
#include <util/General.h>
#include <util/Math.h>

#include <glm/gtc/matrix_transform.hpp>

namespace application::file::game::phive::navmesh
{
	std::pair<std::vector<glm::vec3>, std::vector<uint32_t>> PhiveNavMesh::ToVerticesIndices()
	{
		if (!mModelOverride.first.empty() && !mModelOverride.second.empty())
			return mModelOverride;

		std::vector<uint32_t> Indices;
		std::vector<glm::vec3> Vertices;

		for (application::file::game::phive::classes::HavokClasses::hkaiNavMesh__Face& Face : mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mFaces.mElements)
		{
			if (Face.mNumEdges.mParent == 3)
			{
				Indices.push_back(mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mEdges.mElements[Face.mStartEdgeIndex.mParent.mParent].mA.mParent.mParent);
				Indices.push_back(mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mEdges.mElements[Face.mStartEdgeIndex.mParent.mParent].mB.mParent.mParent);
				Indices.push_back(mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mEdges.mElements[Face.mStartEdgeIndex.mParent.mParent + 1].mB.mParent.mParent);
				continue;
			}

			std::vector<std::pair<glm::vec3, int32_t>> FaceVertices;
			std::vector<application::file::game::phive::classes::HavokClasses::hkaiNavMesh__Edge> FaceEdges(Face.mNumEdges.mParent);
			for (size_t i = 0; i < FaceEdges.size(); i++)
			{
				FaceEdges[i] = mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mEdges.mElements[Face.mStartEdgeIndex.mParent.mParent + i];
			}

			int32_t EdgeIndex = 0;
			for (size_t i = 0; i < FaceEdges.size(); i++)
			{
				application::file::game::phive::classes::HavokClasses::hkVector4& Vertex = mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mVertices.mElements[FaceEdges[EdgeIndex].mB.mParent.mParent];
				FaceVertices.push_back(std::make_pair(glm::vec3(Vertex.mParent.mData[0], Vertex.mParent.mData[1], Vertex.mParent.mData[2]), FaceEdges[EdgeIndex].mB.mParent.mParent));

				for (size_t j = 0; j < FaceEdges.size(); j++)
				{
					if (FaceEdges[j].mA.mParent.mParent == FaceEdges[EdgeIndex].mB.mParent.mParent)
					{
						EdgeIndex = j;
						break;
					}
				}
			}

			for (uint32_t i = 0; i < FaceVertices.size() - 2; i++)
			{
				Indices.push_back(FaceVertices[0].second);
				Indices.push_back(FaceVertices[i + 1].second);
				Indices.push_back(FaceVertices[i + 2].second);
			}
		}

		for (application::file::game::phive::classes::HavokClasses::hkVector4& Vertex : mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mVertices.mElements)
		{
			Vertices.emplace_back(Vertex.mParent.mData[0], Vertex.mParent.mData[1], Vertex.mParent.mData[2]);
		}

		return std::make_pair(Vertices, Indices);
	}

	uint8_t PhiveNavMesh::GetTypeNameFlag(std::string Name)
	{
		application::util::General::ReplaceString(Name, "__", "::");

		if (Name == "hkRootLevelContainer" ||
			Name == "hkaiNavMesh" ||
			Name == "hkaiClusterGraph" ||
			Name == "hkaiNavMeshStaticTreeFaceIterator" ||
			Name == "hkcdStaticAabbTree" ||
			Name == "hkcdStaticAabbTree::Impl" ||
			Name == "hkaiStreamingSet")
			return 16;

		if (Name == "hkRootLevelContainer::NamedVariant" ||
			Name == "char" ||
			Name == "hkaiNavMesh::Face" ||
			Name == "hkaiNavMesh::Edge" ||
			Name == "hkVector4" ||
			Name == "hkInt32" ||
			Name == "hkaiClusterGraph::Node" ||
			Name == "hkaiIndex" ||
			Name == "hkcdCompressedAabbCodecs::Aabb6BytesCodec" ||
			Name == "hkaiClusterGraph::Edge" ||
			Name == "hkaiAnnotatedStreamingSet" ||
			Name == "hkaiStreamingSet::NavMeshConnection" ||
			Name == "hkAabb" ||
			Name == "hkPair" ||
			Name == "hkaiStreamingSet::ClusterGraphConnection")
			return 32;

		application::util::Logger::Error("PhiveNavMesh", "Could not calculate flag for class %s", Name.c_str());

		return 0;
	}

	std::vector<unsigned char> PhiveNavMesh::ToBinary()
	{
		application::file::game::phive::classes::HavokClassBinaryVectorWriter Writer;

		Writer.Seek(48, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

		mTagFile.InitializeItemRegistration();
		mTagFile.mDataHolder.GetPatches().clear();

		mTagFile.mDataHolder.GetItems().push_back(application::file::game::phive::classes::HavokDataHolder::Item());

		Writer.InstallWriteItemCallback([this](std::string Type, std::string ChildName, uint32_t DataOffset, uint32_t Count)
			{
				if (Type == "hkStringPtr" && ChildName == "char")
				{
					Type = "char";
					ChildName = "";
				}

				uint16_t TypeIndex = 10000;
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

				if (Type == "hkArray" && ChildName == "hkAabb")
				{
					TypeIndex = 24;
				}

				return mTagFile.RegisterItem(TypeIndex + 1, GetTypeNameFlag(mTagFile.mTypes[TypeIndex].mName), DataOffset - 0x50, TypeIndex == 10000 ? 0 : Count);
			});

		auto SearchHkTemplateTypeIndex = [this](const std::string& ParentName, const std::string& ValueName)
			{
				for (size_t i = 0; i < mTagFile.mTypes.size(); i++)
				{
					if (mTagFile.mTypes[i].mName == ParentName)
					{
						bool Found = false;
						for (auto& Template : mTagFile.mTypes[i].mTemplates)
						{
							if (mTagFile.mTypes[Template.mValue - 1].mName == ValueName)
							{
								Found = true;
								break;
							}
						}

						if (Found)
						{
							return (int32_t)i;
						}
					}
				}

				return (int32_t)-1;
			};

		auto SearchRawTypeIndex = [this](const std::string& Name)
			{
				for (size_t i = 0; i < mTagFile.mTypes.size(); i++)
				{
					if (mTagFile.mTypes[i].mName == Name)
					{
						return (int32_t)i;
					}
				}

				return (int32_t)-1;
			};

		Writer.InstallWritePatchCallback([this, &SearchHkTemplateTypeIndex, &SearchRawTypeIndex](const std::string& Name, unsigned int PatchOffset)
			{
				int16_t TypeIndex = 0;
				
				if (Name == "hkArray<hkRootLevelContainer::NamedVariant>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkRootLevelContainer::NamedVariant");

				if (Name == "hkStringPtr") TypeIndex = SearchRawTypeIndex("hkStringPtr");
				if (Name == "hkRefVariant") TypeIndex = SearchRawTypeIndex("hkRefVariant");
				if (Name == "hkArray<hkaiNavMesh::Face>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkaiNavMesh::Face");
				if (Name == "hkArray<hkaiNavMesh::Edge>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkaiNavMesh::Edge");
				if (Name == "hkArray<hkaiAnnotatedStreamingSet>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkaiAnnotatedStreamingSet");
				if (Name == "hkArray<hkVector4>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkVector4");
				if (Name == "hkArray<hkInt32>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkInt32");
				if (Name == "hkRefPtr<hkaiNavMeshStaticTreeFaceIterator>") TypeIndex = SearchHkTemplateTypeIndex("hkRefPtr", "hkaiNavMeshStaticTreeFaceIterator");
				if (Name == "hkRefPtr<hkaiStreamingSet>") TypeIndex = SearchHkTemplateTypeIndex("hkRefPtr", "hkaiStreamingSet");
				if (Name == "hkArray<hkaiClusterGraph::Node>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkaiClusterGraph::Node"); //52
				if (Name == "hkArray<hkaiClusterGraph::Edge>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkaiClusterGraph::Edge"); //54
				if (Name == "hkArray<hkaiIndex>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkaiIndex"); //58
				if (Name == "hkRefPtr<hkcdStaticAabbTree>") TypeIndex = SearchHkTemplateTypeIndex("hkRefPtr", "hkcdStaticAabbTree"); //65
				if (Name == "hkArray<hkaiStreamingSet::NavMeshConnection>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkaiStreamingSet::NavMeshConnection");
				if (Name == "hkArray<hkaiStreamingSet::ClusterGraphConnection>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkaiStreamingSet::ClusterGraphConnection");
				if (Name == "hkArray<hkAabb>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkAabb");
				if (Name == "hkArray<hkPair>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkPair");
				if (Name == "hkArray<hkHashMapDetail::Index>") TypeIndex = SearchHkTemplateTypeIndex("hkArray", "hkHashMapDetail::Index");
				if (Name == "hkRefPtr<hkcdStaticAabbTree::Impl>") TypeIndex = SearchHkTemplateTypeIndex("hkRefPtr", "hkcdStaticAabbTree::Impl"); //80

				if (Name == "hkRefPtr<hkaiNavMeshClearanceCacheSeeding::CacheDataSet>") TypeIndex = SearchHkTemplateTypeIndex("hkRefPtr", "hkaiNavMeshClearanceCacheSeeding::CacheDataSet");

				if (TypeIndex == -1)
				{
					application::util::Logger::Error("PhiveNavMesh", "Could not calculate patch type index for class %s", Name.c_str());
				}

				mTagFile.mDataHolder.AddPatch(TypeIndex + 1, PatchOffset - 0x50);
			});

		mTagFile.mRootClass.MarkAsItem(); //hkRootLevelContainer
		mTagFile.mRootClass.mNavMeshVariant.mVariant.MarkAsItem(); //hkaiNavMesh
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.MarkAsItem(); //hkaiClusterGraph
		mTagFile.mRootClass.mNavMeshVariant.mName.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mClassName.MarkAsItem();
		mTagFile.mRootClass.mClusterGraphVariant.mName.MarkAsItem();
		mTagFile.mRootClass.mClusterGraphVariant.mClassName.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mFaces.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mEdges.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mVertices.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mStreamingSets.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mFaceData.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mEdgeData.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mCachedFaceIterator.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mStreamingSets.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hkaiAnnotatedStreamingSet& Set) { Set.mStreamingSet.MarkAsItem(); });
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mStreamingSets.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hkaiAnnotatedStreamingSet& Set) { Set.mStreamingSet.mObj.mMeshConnections.MarkAsItem(); });
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mStreamingSets.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hkaiAnnotatedStreamingSet& Set) { Set.mStreamingSet.mObj.mAConnectionAabbs.MarkAsItem(); });
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mStreamingSets.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hkaiAnnotatedStreamingSet& Set) { Set.mStreamingSet.mObj.mBConnectionAabbs.MarkAsItem(); });
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mStreamingSets.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hkaiAnnotatedStreamingSet& Set) { Set.mStreamingSet.mObj.mGraphConnections.mParent.mItems.MarkAsItem(); });
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mCachedFaceIterator.mObj.mTree.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mCachedFaceIterator.mObj.mTree.mObj.mTreePtr.MarkAsItem();
		mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj.mCachedFaceIterator.mObj.mTree.mObj.mTreePtr.mObj.mTree.mParent.mNodes.MarkAsItem();

		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mPositions.MarkAsItem();
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mNodes.MarkAsItem();
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mEdges.MarkAsItem();
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mNodeData.MarkAsItem();
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mEdgeData.MarkAsItem();
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mFeatureToNodeIndex.MarkAsItem();
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mStreamingSets.MarkAsItem();
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mStreamingSets.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hkaiAnnotatedStreamingSet& Set) { Set.mStreamingSet.MarkAsItem(); });
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mStreamingSets.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hkaiAnnotatedStreamingSet& Set) { Set.mStreamingSet.mObj.mMeshConnections.MarkAsItem(); });
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mStreamingSets.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hkaiAnnotatedStreamingSet& Set) { Set.mStreamingSet.mObj.mAConnectionAabbs.MarkAsItem(); });
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mStreamingSets.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hkaiAnnotatedStreamingSet& Set) { Set.mStreamingSet.mObj.mBConnectionAabbs.MarkAsItem(); });
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mStreamingSets.ForEveryElement([](application::file::game::phive::classes::HavokClasses::hkaiAnnotatedStreamingSet& Set) { Set.mStreamingSet.mObj.mGraphConnections.mParent.mItems.MarkAsItem(); });
		mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj.mStreamingSets.MarkAsItem();

		mTagFile.ToBinary(Writer);

		bool IsDynamicNavMesh = mTagFile.mUnkBlock1.empty();
		
		mHeader.mTagFileSize = Writer.GetPosition() - 0x30;
		mHeader._mUnkOffset1 = mTagFile.mLastDataOffset - (IsDynamicNavMesh ? 4 : 0);
		mHeader._mUnkOffset2 = (IsDynamicNavMesh ? mHeader._mUnkOffset1 + 56 : mTagFile.mUnkBlock1AbsoluteOffset);
		mHeader._mUnkSize2 = mTagFile.mUnkBlock1.size();
		mHeader._mUnkSize1 = 52;

		Writer.Seek(mHeader._mUnkOffset1, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mReferenceRotation), sizeof(ReferenceRotation));

		Writer.Seek(0, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mHeader), 48);

		std::vector<unsigned char> Bytes = Writer.GetData();
		Bytes.resize(mHeader._mUnkOffset2 + mHeader._mUnkSize2 - (IsDynamicNavMesh ? 4 : 0));

		/*
		for (size_t i = 0; i < mTagFile.mTypes.size(); i++)
		{
			application::util::Logger::Info("PhiveNavMesh", "%u: %s", (uint32_t)i, mTagFile.mTypes[i].mName.c_str());
		}
		*/

		return Bytes;
	}

	void PhiveNavMesh::Initialize(const std::vector<unsigned char>& Bytes)
	{
		application::file::game::phive::classes::HavokClassBinaryVectorReader Reader(Bytes);

		Reader.ReadStruct(&mHeader, sizeof(PhiveNavMeshHeader));

		mTagFile.mUnkBlock1AbsoluteOffset = mHeader._mUnkOffset2 - 0x30;
		mTagFile.mUnkBlock1AbsoluteSize = mHeader._mUnkSize2;

		std::vector<unsigned char> TagFileData(mHeader._mUnkOffset2 + mHeader._mUnkSize2 - 0x30);
		Reader.ReadStruct(TagFileData.data(), TagFileData.size());
		mTagFile.LoadFromBytes(TagFileData, true);

		Reader.Seek(mHeader._mUnkOffset1, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
		Reader.ReadStruct(&mReferenceRotation, sizeof(ReferenceRotation));

		//mTagFile.PrintItems();
	}

	application::file::game::phive::classes::HavokTagFile<application::file::game::phive::classes::HavokClasses::hkNavMeshRootLevelContainer>& PhiveNavMesh::GetTagFile()
	{
		return mTagFile;
	}

	void PhiveNavMesh::InjectNavMeshModel(GeneratorData& GeneratorData)
	{
		if(GeneratorData.mVertices.empty() || GeneratorData.mIndices.empty())
		{
			application::util::Logger::Error("PhiveNavMesh", "Cannot inject empty NavMesh model");
			return;
		}

		//Make NavMesh compatible with grid
		/*
		std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> ClippedMesh = application::util::Math::ClipMeshToBox(std::make_pair(GeneratorData.mVertices, GeneratorData.mIndices), std::make_pair(glm::vec3(-124.0f, -1000.0f, -124.0f), glm::vec3(124.0f, 1000.0f, 124.0f)));

		GeneratorData.mVertices = ClippedMesh.first;
		GeneratorData.mIndices = ClippedMesh.second;

		for (int i = 0; i < 4; i++)
		{
			Mesh CenterMesh;
			CenterMesh.vertices = GeneratorData.mVertices;
			CenterMesh.indices = GeneratorData.mIndices;
			glm::vec3 CenterMeshCenter(0.0f);
			for (const glm::vec3& v : CenterMesh.vertices)
			{
				CenterMeshCenter += v;
			}
			CenterMeshCenter /= static_cast<float>(CenterMesh.vertices.size());

			std::pair<std::vector<glm::vec3>, std::vector<unsigned int>> RawOuterMesh = GeneratorData.mNeighbourNavMeshObjs[i]->ToVerticesIndices();
			Mesh OuterMesh;
			OuterMesh.vertices = RawOuterMesh.first;
			OuterMesh.indices = RawOuterMesh.second;

			// Determine offset based on index
			glm::vec3 offset(0.0f);
			if (i == 0) offset = glm::vec3(-252.0f, 0.0f, 0.0f);
			if (i == 1) offset = glm::vec3(252.0f, 0.0f, 0.0f);
			if (i == 2) offset = glm::vec3(0.0f, 0.0f, -252.0f);
			if (i == 3) offset = glm::vec3(0.0f, 0.0f, 252.0f);

			// Apply offset to outer mesh vertices
			for (glm::vec3& v : OuterMesh.vertices)
			{
				v += offset;
			}

			glm::vec3 OuterMeshCenter(0.0f);
			for (const glm::vec3& v : OuterMesh.vertices)
			{
				OuterMeshCenter += v;
			}
			OuterMeshCenter /= static_cast<float>(OuterMesh.vertices.size());

			// Connect meshes, passing the offset so vertices can be transformed to center mesh space
			connectMeshes(CenterMesh, OuterMesh, glm::vec3(0, CenterMeshCenter.y, 0), OuterMeshCenter, offset);

			GeneratorData.mVertices = CenterMesh.vertices;
			GeneratorData.mIndices = CenterMesh.indices;
		}
		*/

		/*
		starlight_physics::common::hkGeometry Geometry;

		//Injecting mesh
		{
			auto& Vertices = Geometry.mVertices;
			auto& Triangles = Geometry.mTriangles;

			for (int i = 0; i < GeneratorData.mVertices.size(); i++)
			{
				Vertices.push_back(glm::vec4(GeneratorData.mVertices[i], 1.0f));
			}

			Triangles.reserve(GeneratorData.mIndices.size() / 3);
			for (size_t i = 0; i < GeneratorData.mIndices.size() / 3; i++)
			{
				Triangles.emplace_back(GeneratorData.mIndices[i * 3], GeneratorData.mIndices[i * 3 + 1], GeneratorData.mIndices[i * 3 + 2]);
			}
		}
		*/

		starlight_physics::ai::hkaiNavMeshBuilder Builder;
		Builder.initialize(GeneratorData.mGeometryConfig);
		Builder.buildNavMesh(&mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj, GeneratorData.mVertices, GeneratorData.mIndices);
		Builder.buildClusterGraph(mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj, mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj);

		/*
					LoadNavMeshNeighbour("X" + std::to_string(NavMeshPair.first.mXIndex - 1) + "_Z" + std::to_string(NavMeshPair.first.mZIndex), 0);
		LoadNavMeshNeighbour("X" + std::to_string(NavMeshPair.first.mXIndex + 1) + "_Z" + std::to_string(NavMeshPair.first.mZIndex), 1);
		LoadNavMeshNeighbour("X" + std::to_string(NavMeshPair.first.mXIndex) + "_Z" + std::to_string(NavMeshPair.first.mZIndex - 1), 2);
		LoadNavMeshNeighbour("X" + std::to_string(NavMeshPair.first.mXIndex) + "_Z" + std::to_string(NavMeshPair.first.mZIndex + 1), 3);
		*/

		/*
		if (!GeneratorData.mIsSingleSceneNavMesh)
		{
			if (GeneratorData.mNeighbourNavMeshObjs[0] != nullptr) Builder.addPrebuiltTile(GeneratorData.mNeighbourNavMeshObjs[0]->mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj, 0, 1, glm::vec3(0.0f, 0.0f, 250.0f));
			if (GeneratorData.mNeighbourNavMeshObjs[1] != nullptr) Builder.addPrebuiltTile(GeneratorData.mNeighbourNavMeshObjs[1]->mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj, 2, 1, glm::vec3(500.0f, 0.0f, 250.0f));
			if (GeneratorData.mNeighbourNavMeshObjs[2] != nullptr) Builder.addPrebuiltTile(GeneratorData.mNeighbourNavMeshObjs[2]->mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj, 1, 0, glm::vec3(250.0f, 0.0f, 0.0f));
			if (GeneratorData.mNeighbourNavMeshObjs[3] != nullptr) Builder.addPrebuiltTile(GeneratorData.mNeighbourNavMeshObjs[3]->mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj, 1, 2, glm::vec3(250.0f, 0.0f, 500.0f));
		}

		Builder.buildTileForMesh(GeneratorData.mVertices, GeneratorData.mIndices, 1, 1, glm::vec3(250.0f, 0.0f, 250.0f), !GeneratorData.mIsSingleSceneNavMesh);

		Builder.buildNavMesh(&mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj, 1, 1);

		starlight_physics::ai::hkaiHierarchyUtils::ClusteringSetup Setup;
		starlight_physics::ai::hkaiHierarchyUtils::clusterNavMesh(mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj, mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj, Setup);
		*/

		//mModelOverride.first = GeneratorData.mVertices;
		//mModelOverride.second = GeneratorData.mIndices;

		/*
		{
			const int nverts = Builder.geometryGenerator.mCurrentMesh->nverts;
			const int npolys = Builder.geometryGenerator.mCurrentMesh->npolys;
			const int nvp = Builder.geometryGenerator.mCurrentMesh->nvp;

			const unsigned short* verts = Builder.geometryGenerator.mCurrentMesh->verts;
			const unsigned short* polys = Builder.geometryGenerator.mCurrentMesh->polys;

			// --------------------------------------------
			// 1️⃣ Decompress vertices
			// --------------------------------------------
			mModelOverride.first = Builder.geometryGenerator.getMeshVertices();

			// --------------------------------------------
			// 2️⃣ Triangulate each polygon (fan method)
			// --------------------------------------------
			for (int i = 0; i < npolys; ++i)
			{
				const unsigned short* p = &polys[i * nvp * 2];

				// Count valid vertices
				int vertCount = 0;
				for (int j = 0; j < nvp; ++j)
				{
					if (p[j] == 0xFFFF) // RC_MESH_NULL_IDX
						break;

					vertCount++;
				}

				if (vertCount < 3)
					continue;

				// Triangulate polygon as fan:
				// (v0, v1, v2), (v0, v2, v3), ...
				for (int j = 1; j < vertCount - 1; ++j)
				{
					uint32_t i0 = p[0];
					uint32_t i1 = p[j];
					uint32_t i2 = p[j + 1];

					mModelOverride.second.push_back(i0);
					mModelOverride.second.push_back(i1);
					mModelOverride.second.push_back(i2);
				}
			}
		}
		*/

		if (!GeneratorData.mIsSingleSceneNavMesh)
		{
			//Generate Streaming Sets
		}

		auto& Mesh = mTagFile.mRootClass.mNavMeshVariant.mVariant.mObj;
		auto& Graph = mTagFile.mRootClass.mClusterGraphVariant.mVariant.mObj;

		application::util::Logger::Info("PhiveNavMesh", "Successfully injected a new navigation mesh model.");
		application::util::Logger::Info("PhiveNavMesh", "Vertices: %u, Faces: %u, Edges: %u, Graph Nodes: %u, Graph Positions: %u, Graph Edges: %u",
			Mesh.mVertices.mElements.size(),
			Mesh.mFaces.mElements.size(),
			Mesh.mEdges.mElements.size(),
			Graph.mNodes.mElements.size(),
			Graph.mPositions.mElements.size(), 
			Graph.mEdges.mElements.size());

		application::util::Logger::Info("PhiveNavMesh", "AABB Nodes: %u", Mesh.mCachedFaceIterator.mObj.mTree.mObj.mTreePtr.mObj.mTree.mParent.mNodes.mElements.size());

		application::util::Logger::Info("PhiveNavMesh", "Streaming Sets: %u", Mesh.mStreamingSets.mElements.size());
		for (size_t i = 0; i < Mesh.mStreamingSets.mElements.size(); i++)
		{
			application::util::Logger::Info("PhiveNavMesh", "Streaming Set %u: %u Mesh Connections, %u A Connection AABBs, %u B Connection AABBs, %u Graph Connections",
				i,
				Mesh.mStreamingSets.mElements[i].mStreamingSet.mObj.mMeshConnections.mElements.size(),
				Mesh.mStreamingSets.mElements[i].mStreamingSet.mObj.mAConnectionAabbs.mElements.size(),
				Mesh.mStreamingSets.mElements[i].mStreamingSet.mObj.mBConnectionAabbs.mElements.size(),
				Mesh.mStreamingSets.mElements[i].mStreamingSet.mObj.mGraphConnections.mParent.mItems.mElements.size());
		}
	}

	PhiveNavMesh::PhiveNavMesh(const std::vector<unsigned char>& Bytes)
	{
		Initialize(Bytes);
	}
}