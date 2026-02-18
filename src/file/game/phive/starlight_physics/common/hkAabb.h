#pragma once

#include <glm/vec4.hpp>
#include <glm/common.hpp>

namespace application::file::game::phive::starlight_physics::common
{
	class hkAabb
	{
	public:
		hkAabb() = default;

		void SetEmpty();
		void GetCenter(glm::vec4& c);

        void IncludePoint(glm::vec4 point);
        void IncludeAabb(const hkAabb& aabb);
        void IncludePoints(const glm::vec4* p, int numPoints);
        float SurfaceArea();
        bool Contains(const hkAabb& other) const;

        inline void SetFromPoint(const glm::vec4& x)
        {
            mMin = x;
            mMax = x;
        }

        inline void SetFromLine(const glm::vec4& x, const glm::vec4& y)
        {
            mMin = glm::min(x, y);
            mMax = glm::max(x, y);
        }

        inline void SetFromTriangle(const glm::vec4& x, const glm::vec4& y, const glm::vec4& z)
        {
            SetFromLine(x, y);
            IncludePoint(z);
        }

        inline void SetFromTetrahedron(const glm::vec4& x, const glm::vec4& y, const glm::vec4& z, const glm::vec4& w)
        {
            SetFromTriangle(x, y, z);
            IncludePoint(w);
        }

        inline void ExpandBy(float exp)
        {
            mMax += exp;
            mMin -= exp;
        }

		inline void Translate(const glm::vec4& t)
        {
            mMin += t;
            mMax += t;
        }

        void SetUnion(const hkAabb& aabb0, const hkAabb& aabb1);
        bool IsEmpty();

		bool Equals(const hkAabb& other) const;

		glm::vec4 mMin;
		glm::vec4 mMax;
	};
}