#include "Model.h"
#include "Texture.h"
#include "Shader.h"
#include "Global.h"
#include <glm/gtc/type_ptr.hpp>

Model::Model(glm::vec3 position, glm::quat orientaton, Triangle* tris, GLsizei triCount, glm::vec3 color) : BaseObject(position, orientaton), triCount(triCount), m_color(color), m_colorMode(ColorOnly)
{
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, triCount * sizeof(Triangle), tris, GL_STATIC_DRAW);

	Vertex::ConfigVertexArrayObject(m_vao);
}

Model::~Model()
{
	// Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
}

void Model::Update(GLfloat deltaTime)
{
}

void Model::Render(Shader& shader) const
{
	GLint objectColorLoc = glGetUniformLocation(shader.Program, "objectColor");
	glUniform3f(objectColorLoc, m_color.r, m_color.g, m_color.b);
	glCheckError();

	GLint modelLoc = glGetUniformLocation(shader.Program, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(GetMatrix()));
	glCheckError();

	GLint colorModeLoc = glGetUniformLocation(shader.Program, "mode");
	glUniform1i(colorModeLoc, m_colorMode);
	glCheckError();

	glBindVertexArray(m_vao);
	glCheckError();
	glDrawArrays(GL_TRIANGLES, 0, triCount * 3);

	glCheckError();

	glBindVertexArray(0);
}

