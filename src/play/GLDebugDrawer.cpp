#include <play/GLDebugDrawer.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>
#include <vector>

GLDebugDrawer::GLDebugDrawer()
	: m_debugMode(0)
{
	// Setup OpenGL buffers
	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	glBindVertexArray(mVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	// Each line has 2 vertices with 3 floats each
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 2 * 1000, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);
}

GLDebugDrawer::~GLDebugDrawer()
{
	glDeleteBuffers(1, &mVBO);
	glDeleteVertexArrays(1, &mVAO);
}

void GLDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor)
{
	// Store lines in CPU buffer for batch draw
	mLines.push_back({ glm::vec3(from.x(), from.y(), from.z()), glm::vec3(to.x(), to.y(), to.z()) });
	mColors.push_back({ glm::vec3(fromColor.x(), fromColor.y(), fromColor.z()), glm::vec3(toColor.x(), toColor.y(), toColor.z()) });
}

void GLDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	drawLine(from, to, color, color);
}

// Call this at the end of your render loop, after setting camera matrices
void GLDebugDrawer::Render(application::gl::Shader* shader)
{
	if (mLines.empty())
		return;

	std::vector<float> vertexData;
	std::vector<float> colorData;

	for (size_t i = 0; i < mLines.size(); ++i)
	{
		// from
		vertexData.push_back(mLines[i].first.x);
		vertexData.push_back(mLines[i].first.y);
		vertexData.push_back(mLines[i].first.z);
		vertexData.push_back(mLines[i].second.x);
		vertexData.push_back(mLines[i].second.y);
		vertexData.push_back(mLines[i].second.z);
	}

	// Bind VAO/VBO
	glBindVertexArray(mVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);

	// Set shader uniforms
	shader->Bind();
	glUniformMatrix4fv(glGetUniformLocation(shader->mID, "camMatrix"), 1, GL_FALSE, &mCameraMatrix[0][0]);
	glm::mat4 model = glm::mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader->mID, "modelMatrix"), 1, GL_FALSE, &model[0][0]);

	// Draw lines
	glDrawArrays(GL_LINES, 0, (GLsizei)mLines.size() * 2);

	glBindVertexArray(0);

	mLines.clear();
	mColors.clear();
}

void GLDebugDrawer::setDebugMode(int debugMode)
{
	m_debugMode = debugMode;
}

int GLDebugDrawer::getDebugMode() const
{
	return m_debugMode;
}

void GLDebugDrawer::draw3dText(const btVector3& location, const char* textString) {}
void GLDebugDrawer::reportErrorWarning(const char* warningString) { printf("%s\n", warningString); }
void GLDebugDrawer::drawContactPoint(const btVector3& pointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
	drawLine(pointOnB, pointOnB + normalOnB * distance, color);
}
