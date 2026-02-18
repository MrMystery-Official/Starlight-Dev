#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <vector>
#include <gl/Shader.h>

class GLDebugDrawer : public btIDebugDraw
{
public:
    GLDebugDrawer();
    ~GLDebugDrawer();

    // btIDebugDraw overrides
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor) override;
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
    void drawContactPoint(const btVector3& pointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
    void reportErrorWarning(const char* warningString) override;
    void draw3dText(const btVector3& location, const char* textString) override;
    void setDebugMode(int debugMode) override;
    int getDebugMode() const override;

    // Modern OpenGL render call
    void Render(application::gl::Shader* shader);

    // Camera matrix for shader
    glm::mat4 mCameraMatrix;

private:
    int m_debugMode;

    struct Line {
        glm::vec3 first;
        glm::vec3 second;
    };

    std::vector<Line> mLines;
    std::vector<std::pair<glm::vec3, glm::vec3>> mColors;

    // OpenGL buffers
    unsigned int mVAO = 0;
    unsigned int mVBO = 0;
};