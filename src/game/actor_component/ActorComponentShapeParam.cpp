#include "ActorComponentShapeParam.h"

#include <file/game/byml/BymlFile.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <util/General.h>
#include <util/Math.h>

namespace application::game::actor_component
{
	bool ActorComponentShapeParam::PhiveShapeParam::IsEmpty()
	{
		return mPhiveShapes.empty() && mBoxes.empty() && mSpheres.empty() && mPolytopes.empty();
	}

	std::string ActorComponentShapeParam::PhiveShapeParam::GetHash()
	{
		std::string Str = "";
		for (const std::string& Path : mPhiveShapes)
		{
			Str += Path + "_";
		}

		for (Box& BBox : mBoxes)
		{
			Str += BBox.GetHash() + "_";
		}

		for (Sphere& Sphere : mSpheres)
		{
			Str += Sphere.GetHash() + "_";
		}

		for (Polytope& Hull : mPolytopes)
		{
			Str += Hull.GetHash() + "_";
		}

		return std::to_string(application::util::Math::StringHashMurMur3(Str));
	}

	void ActorComponentShapeParam::GetShapeParamBymlName(const std::string& Path)
	{
		if (mActorPack->mPack.HasEntry(Path))
		{
			application::file::game::byml::BymlFile PhiveControllerFile(mActorPack->mPack.GetEntry(Path).mBytes);

			if (PhiveControllerFile.HasChild("ShapeNamePathAry") && mPhiveShapeBymlName.empty())
			{
				for (auto& Child : PhiveControllerFile.GetNode("ShapeNamePathAry")->GetChildren())
				{
					std::string Name = Child.GetChild("Name")->GetValue<std::string>();
					if (Name.find("Physical") == std::string::npos && Name.find("Collision") == std::string::npos)
						continue;

					mPhiveShapeBymlName = Child.GetChild("FilePath")->GetValue<std::string>();
				}
			}

			if (PhiveControllerFile.HasChild("$parent"))
			{
				std::string ParentControllerPath = PhiveControllerFile.GetNode("$parent")->GetValue<std::string>();
				application::util::General::ReplaceString(ParentControllerPath, "Work/", "");
				application::util::General::ReplaceString(ParentControllerPath, ".gyml", ".bgyml");
				GetShapeParamBymlName(ParentControllerPath);
			}
		}
	}

	ActorComponentShapeParam::ActorComponentShapeParam(application::game::ActorPack& ActorPack) : mActorPack(&ActorPack)
	{
		std::string PhysicsComponentPath = "";
		for (application::file::game::SarcFile::Entry& Entry : ActorPack.mPack.GetEntries())
		{
			if (Entry.mName.starts_with("Component/Physics/"))
			{
				PhysicsComponentPath = Entry.mName;
			}
		}

		if (PhysicsComponentPath.empty())
			return;

		application::file::game::byml::BymlFile PhysicsParamFile(mActorPack->mPack.GetEntry(PhysicsComponentPath).mBytes);
		if (PhysicsParamFile.HasChild("ControllerSetPath"))
		{
			std::string PhiveControllerPath = PhysicsParamFile.GetNode("ControllerSetPath")->GetValue<std::string>();
			application::util::General::ReplaceString(PhiveControllerPath, "Work/", "");
			application::util::General::ReplaceString(PhiveControllerPath, ".gyml", ".bgyml");

			GetShapeParamBymlName(PhiveControllerPath);

			if (!mPhiveShapeBymlName.empty())
			{
				std::filesystem::path p(mPhiveShapeBymlName);
				mPhiveShapeBymlName = p.stem().string();

				std::vector<unsigned char>& Bytes = mActorPack->mPack.GetEntry("Phive/ShapeParam/" + mPhiveShapeBymlName + ".bgyml").mBytes;

				if (!Bytes.empty())
				{
					application::file::game::byml::BymlFile ShapeParamFile(Bytes);

					if (ShapeParamFile.HasChild("PhshMesh"))
					{
						for (auto& Child : ShapeParamFile.GetNode("PhshMesh")->GetChildren())
						{
							std::string PhshMeshPath = Child.GetChild("PhshMeshPath")->GetValue<std::string>();

							std::filesystem::path p(PhshMeshPath);
							PhshMeshPath = p.stem().string();

							mPhiveShapeParam.mPhiveShapes.push_back(PhshMeshPath);
						}
					}

					if (ShapeParamFile.HasChild("Box"))
					{
						for (auto& Child : ShapeParamFile.GetNode("Box")->GetChildren())
						{
							PhiveShapeParam::Box Box;
							if (Child.HasChild("Center"))
							{
								Box.mCenter.x = Child.GetChild("Center")->GetChild("X")->GetValue<float>();
								Box.mCenter.y = Child.GetChild("Center")->GetChild("Y")->GetValue<float>();
								Box.mCenter.z = Child.GetChild("Center")->GetChild("Z")->GetValue<float>();
							}
							if (Child.HasChild("HalfExtents"))
							{
								Box.mHalfExtents.x = Child.GetChild("HalfExtents")->GetChild("X")->GetValue<float>();
								Box.mHalfExtents.y = Child.GetChild("HalfExtents")->GetChild("Y")->GetValue<float>();
								Box.mHalfExtents.z = Child.GetChild("HalfExtents")->GetChild("Z")->GetValue<float>();
							}
							if (Child.HasChild("OffsetTranslation"))
							{
								Box.mOffsetTranslation.x = Child.GetChild("OffsetTranslation")->GetChild("X")->GetValue<float>();
								Box.mOffsetTranslation.y = Child.GetChild("OffsetTranslation")->GetChild("Y")->GetValue<float>();
								Box.mOffsetTranslation.z = Child.GetChild("OffsetTranslation")->GetChild("Z")->GetValue<float>();
							}
							if (Child.HasChild("OffsetRotation"))
							{
								Box.mOffsetRotation.x = Child.GetChild("OffsetRotation")->GetChild("X")->GetValue<float>();
								Box.mOffsetRotation.y = Child.GetChild("OffsetRotation")->GetChild("Y")->GetValue<float>();
								Box.mOffsetRotation.z = Child.GetChild("OffsetRotation")->GetChild("Z")->GetValue<float>();
							}

							mPhiveShapeParam.mBoxes.push_back(Box);
						}
					}

					if (ShapeParamFile.HasChild("Sphere"))
					{
						for (auto& Child : ShapeParamFile.GetNode("Sphere")->GetChildren())
						{
							PhiveShapeParam::Sphere Sphere;
							if (Child.HasChild("Center"))
							{
								Sphere.mCenter.x = Child.GetChild("Center")->GetChild("X")->GetValue<float>();
								Sphere.mCenter.y = Child.GetChild("Center")->GetChild("Y")->GetValue<float>();
								Sphere.mCenter.z = Child.GetChild("Center")->GetChild("Z")->GetValue<float>();
							}
							if (Child.HasChild("Radius"))
							{
								Sphere.mRadius = Child.GetChild("Radius")->GetValue<float>();
							}

							mPhiveShapeParam.mSpheres.push_back(Sphere);
						}
					}

					if (ShapeParamFile.HasChild("Polytope"))
					{
						for (auto& Child : ShapeParamFile.GetNode("Polytope")->GetChildren())
						{
							PhiveShapeParam::Polytope Polytope;
							if (Child.HasChild("OffsetTranslation"))
							{
								Polytope.mOffsetTranslation.x = Child.GetChild("OffsetTranslation")->GetChild("X")->GetValue<float>();
								Polytope.mOffsetTranslation.y = Child.GetChild("OffsetTranslation")->GetChild("Y")->GetValue<float>();
								Polytope.mOffsetTranslation.z = Child.GetChild("OffsetTranslation")->GetChild("Z")->GetValue<float>();
							}
							if (Child.HasChild("OffsetRotation"))
							{
								Polytope.mOffsetRotation.x = Child.GetChild("OffsetRotation")->GetChild("X")->GetValue<float>();
								Polytope.mOffsetRotation.y = Child.GetChild("OffsetRotation")->GetChild("Y")->GetValue<float>();
								Polytope.mOffsetRotation.z = Child.GetChild("OffsetRotation")->GetChild("Z")->GetValue<float>();
							}

							if (Child.HasChild("Vertices"))
							{
								for (auto& Vertex : Child.GetChild("Vertices")->GetChildren())
								{
									glm::vec3 Vec;
									Vec.x = Vertex.GetChild("X")->GetValue<float>();
									Vec.y = Vertex.GetChild("Y")->GetValue<float>();
									Vec.z = Vertex.GetChild("Z")->GetValue<float>();
									Polytope.mVertices.push_back(Vec);
								}
							}

							mPhiveShapeParam.mPolytopes.push_back(Polytope);
						}
					}
				}
				else
				{
					application::util::Logger::Warning("ActorComponentShapeParam", "Could not get phive shape of actor");
				}

			}
		}
	}

	const std::string ActorComponentShapeParam::GetDisplayNameImpl()
	{
		return "ShapeParam";
	}

	const std::string ActorComponentShapeParam::GetInternalNameImpl()
	{
		return "phive__ShapeParam";
	}

	bool ActorComponentShapeParam::ContainsComponent(application::game::ActorPack& Pack)
	{
		for (application::file::game::SarcFile::Entry& Entry : Pack.mPack.GetEntries())
		{
			if (Entry.mName.rfind("Component/ModelInfo/", 0) == 0)
			{
				return true;
			}
		}

		return false;
	}

	void ActorComponentShapeParam::DrawEditingMenu()
	{
		ImGui::Text("Not implemented yet.");
	}
}