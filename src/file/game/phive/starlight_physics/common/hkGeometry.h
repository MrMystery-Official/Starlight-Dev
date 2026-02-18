#pragma once

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace application::file::game::phive::starlight_physics::common
{
    //https://github.com/nvnprogram/FiveX/blob/main/source/havok/hkGeometry.h
    struct hkGeometry
    {
        struct Triangle
        {
            Triangle() = default;
            Triangle(int a, int b, int c, int material = -1)
                : mA(a), mB(b), mC(c), mMaterial(material)
            {
            }

            void set(int a, int b, int c, int material = -1)
            {
                mA = a;
                mB = b;
                mC = c;
                mMaterial = material;
            }

            int getVertex(int i) const { return (&mA)[i]; }

            int mA = 0;
            int mB = 0;
            int mC = 0;
            int mMaterial = 0;
        };

        hkGeometry() = default;

        void clear();
        bool isValid() const;

        glm::vec4& getVertex(int triangleIndex, int vertexIndex)
        {
            assert(vertexIndex >= 0 && vertexIndex < 3 && "Index out-of-range");
            return mVertices[(&mTriangles[triangleIndex].mA)[vertexIndex]];
        }

        template <typename VERTEX_TYPE>
        void getTriangle(int triangleIndex, VERTEX_TYPE* vertices) const
        {
            assert(triangleIndex >= 0 && triangleIndex < mTriangles.size() && "Index out-of-range");
            vertices[0] = mVertices[mTriangles[triangleIndex].mA];
            vertices[1] = mVertices[mTriangles[triangleIndex].mB];
            vertices[2] = mVertices[mTriangles[triangleIndex].mC];
        }

        void appendGeometry(const hkGeometry& geometry, const glm::mat4* transform = nullptr);

        int appendVertex(const glm::vec4& v);
        int appendTriangle(const glm::vec4& a, const glm::vec4& b, const glm::vec4& c, int material = -1);

        std::vector<glm::vec4> mVertices;
        std::vector<Triangle> mTriangles;
    };
}