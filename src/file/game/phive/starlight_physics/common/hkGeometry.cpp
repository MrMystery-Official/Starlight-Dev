#include <file/game/phive/starlight_physics/common/hkGeometry.h>

namespace application::file::game::phive::starlight_physics::common
{
    void hkGeometry::clear()
    {
        mTriangles.clear();
        mVertices.clear();
    }

    bool hkGeometry::isValid() const
    {
        const unsigned int numVertices = mVertices.size();

        for (int i = 0; i < mTriangles.size(); ++i)
        {
            const Triangle& triangle = mTriangles[i];
            if (static_cast<unsigned int>(triangle.mA) >= numVertices ||
                static_cast<unsigned int>(triangle.mB) >= numVertices ||
                static_cast<unsigned int>(triangle.mC) >= numVertices)
            {
                return false;
            }
        }
        return true;
    }

    void hkGeometry::appendGeometry(const hkGeometry& geometry, const glm::mat4* transform)
    {
        const int baseVertex = mVertices.size();
        const int baseTriangle = mTriangles.size();

        mVertices.reserve(baseVertex + geometry.mVertices.size());
        mTriangles.reserve(baseTriangle + geometry.mTriangles.size());

		mVertices.insert(mVertices.end(), geometry.mVertices.begin(), geometry.mVertices.end());
		mTriangles.insert(mTriangles.end(), geometry.mTriangles.begin(), geometry.mTriangles.end());

        if (transform)
        {
            const glm::mat4& matrix = *transform;

            for (int i = baseVertex; i < mVertices.size(); ++i)
            {
                mVertices[i] = matrix * mVertices[i];
            }
        }

        if (baseVertex > 0)
        {
            for (int i = baseTriangle; i < mTriangles.size(); ++i)
            {
                mTriangles[i].mA += baseVertex;
                mTriangles[i].mB += baseVertex;
                mTriangles[i].mC += baseVertex;
            }
        }
    }

    int hkGeometry::appendVertex(const glm::vec4& v)
    {
        mVertices.push_back(v);
        return mVertices.size() - 1;
    }

    int hkGeometry::appendTriangle(const glm::vec4& a, const glm::vec4& b, const glm::vec4& c, int material)
    {
        Triangle& t = mTriangles.emplace_back();
        t.mA = appendVertex(a);
        t.mB = appendVertex(b);
        t.mC = appendVertex(c);
        t.mMaterial = material;

        return mTriangles.size() - 1;
    }
}