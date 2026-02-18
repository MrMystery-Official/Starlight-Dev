#pragma once

#include <game/actor_component/ActorComponentBase.h>
#include <game/ActorPack.h>
#include <optional>

namespace application::game::actor_component
{
	class ActorComponentShapeParam : public ActorComponentBase
	{
	public:
		struct PhiveShapeParam
		{
			struct Box
			{
				glm::vec3 mHalfExtents = glm::vec3(0.0f);
				glm::vec3 mCenter = glm::vec3(0.0f);
				glm::vec3 mOffsetTranslation = glm::vec3(0.0f);
				glm::vec3 mOffsetRotation = glm::vec3(0.0f);

				std::string GetHash()
				{
					return "Box_" + std::to_string(mHalfExtents.x) + "_" + std::to_string(mHalfExtents.y) + "_" + std::to_string(mHalfExtents.z) + "_" + std::to_string(mCenter.x) + "_" + std::to_string(mCenter.y) + "_" + std::to_string(mCenter.z) +
						std::to_string(mOffsetTranslation.x) + "_" + std::to_string(mOffsetTranslation.y) + "_" + std::to_string(mOffsetTranslation.z) + "_" + std::to_string(mOffsetRotation.x) + "_" + std::to_string(mOffsetRotation.y) + "_" + std::to_string(mOffsetRotation.z);
				}
			};

			struct Sphere
			{
				glm::vec3 mCenter = glm::vec3(0.0f);
				float mRadius = 0.0f;

				std::string GetHash()
				{
					return "Sphere_" + std::to_string(mCenter.x) + "_" + std::to_string(mCenter.y) + "_" + std::to_string(mCenter.z) + "_" + std::to_string(mRadius);
				}
			};

			struct Polytope
			{
				glm::vec3 mOffsetTranslation = glm::vec3(0.0f);
				glm::vec3 mOffsetRotation = glm::vec3(0.0f);
				std::vector<glm::vec3> mVertices;

				std::string GetHash()
				{
					if (mVertices.empty()) return 0;

					size_t hash = 14695981039346656037ULL;
					const size_t prime = 1099511628211ULL;

					const unsigned char* data = reinterpret_cast<const unsigned char*>(mVertices.data());
					size_t size = mVertices.size() * sizeof(glm::vec3);

					for (size_t i = 0; i < size; i++)
					{
						hash ^= data[i];
						hash *= prime;
					}

					return "Polytope_" + std::to_string(mOffsetTranslation.x) + "_" + std::to_string(mOffsetTranslation.y) + "_" + std::to_string(mOffsetTranslation.z) + "_" + std::to_string(mOffsetRotation.x) + "_" + std::to_string(mOffsetRotation.y) + "_" + std::to_string(mOffsetRotation.z) +
						"_" + std::to_string(hash);
				}
			};

			std::vector<std::string> mPhiveShapes;
			std::vector<Box> mBoxes;
			std::vector<Sphere> mSpheres;
			std::vector<Polytope> mPolytopes;

			bool IsEmpty();
			std::string GetHash();
		};

		application::game::ActorPack* mActorPack = nullptr;
		PhiveShapeParam mPhiveShapeParam;

		ActorComponentShapeParam(application::game::ActorPack& ActorPack);

		static const std::string GetDisplayNameImpl();
		static const std::string GetInternalNameImpl();
		static bool ContainsComponent(application::game::ActorPack& Pack);

		static const ComponentType GetComponentTypeImpl()
		{
			return ActorComponentBase::ComponentType::SHAPE_PARAM;
		}

		virtual const ComponentType GetComponentType() const
		{
			return ActorComponentShapeParam::GetComponentTypeImpl();
		}

		virtual const std::string GetDisplayName() const
		{
			return ActorComponentShapeParam::GetDisplayNameImpl();
		}

		virtual const std::string GetInternalName() const
		{
			return ActorComponentShapeParam::GetInternalNameImpl();
		}

		virtual void DrawEditingMenu() override;

	private:
		std::string mPhiveShapeBymlName = "";

		void GetShapeParamBymlName(const std::string& Path);
	};
}