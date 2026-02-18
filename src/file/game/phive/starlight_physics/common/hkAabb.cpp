#include <file/game/phive/starlight_physics/common/hkAabb.h>

#include <glm/vec3.hpp>
#include <glm/glm.hpp>

namespace application::file::game::phive::starlight_physics::common
{
	void hkAabb::SetEmpty()
	{
		mMin.x = std::numeric_limits<float>::max();
		mMin.y = std::numeric_limits<float>::max();
		mMin.z = std::numeric_limits<float>::max();
		mMin.w = std::numeric_limits<float>::max();

		mMax.x = std::numeric_limits<float>::lowest();
		mMax.y = std::numeric_limits<float>::lowest();
		mMax.z = std::numeric_limits<float>::lowest();
		mMax.w = std::numeric_limits<float>::lowest();
	}

	void hkAabb::GetCenter(glm::vec4& c)
	{
		c = (mMin + mMax) * 0.5f;
	}

	void hkAabb::IncludePoint(glm::vec4 point)
	{
		mMin = glm::min(mMin, point);
		mMax = glm::max(mMax, point);
	}

	void hkAabb::IncludeAabb(const hkAabb& aabb)
	{
		mMin = glm::min(mMin, aabb.mMin);
		mMax = glm::max(mMax, aabb.mMax);
	}

	void hkAabb::IncludePoints(const glm::vec4* p, int numPoints)
	{
		for (int i = 0; i < numPoints; i++)
		{
			IncludePoint(p[i]);
		}
	}

	float hkAabb::SurfaceArea()
	{
		glm::vec4 l = mMax - mMin;
		glm::vec4 u = glm::vec4(l.y, l.z, l.x, l.z);
		return 2.0f * glm::dot(l, u);
	}

	bool hkAabb::Contains(const hkAabb& other) const
	{
		glm::bvec4 mincomp = glm::lessThanEqual(mMin, other.mMin);
		glm::bvec4 maxcomp = glm::lessThanEqual(other.mMax, mMax);
		glm::bvec4 both = mincomp && maxcomp;
		return glm::all(glm::bvec3(both));
	}

	void hkAabb::SetUnion(const hkAabb& aabb0, const hkAabb& aabb1)
	{
		mMin = glm::min(aabb0.mMin, aabb1.mMin);
		mMax = glm::max(aabb0.mMax, aabb1.mMax);
	}

	bool hkAabb::IsEmpty()
	{
		glm::bvec4 res = glm::greaterThan(mMin, mMax);
		return !glm::any(res);
	}

	bool hkAabb::Equals(const hkAabb& other) const
	{
		return glm::all(glm::equal(mMin, other.mMin)) && glm::all(glm::equal(mMax, other.mMax));
	}
}