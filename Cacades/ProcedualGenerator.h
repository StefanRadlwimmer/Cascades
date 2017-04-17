#pragma once
#include <GL/glew.h>
#include <glm/detail/type_vec2.hpp>
#include <glm/detail/type_vec3.hpp>
#include <random>

class Shader;

class ProcedualGenerator
{
public:
	ProcedualGenerator(int seed);
	~ProcedualGenerator();
	void Generate3dTexture(int startLayer);
	void Generate3dTexture();

	void GenerateVBO(glm::vec3 cubesPerDimension);

	void SetUniforms(Shader& shader);

	GLuint GetTextureId() const;
	GLuint GetVboId() const;
	GLuint GetVaoId() const;
	GLuint GetVertexCount() const;
	float GetValue(int layer, int y, int x) const;

	static const int WIDTH = 96, DEPTH = 96, LAYERS = 256;

protected:
	void UpdateValues(int startLayer);
	static float AddPillar(glm::vec2 pos, glm::vec2 pillar);
	static float AddBounds(glm::vec2 pos);
	static float AddHelix(glm::vec2 pos, float sinLayer, float cosLayer);
	static float AddShelves(float cosLayer);
	void ApplyDataToTexture();

	static float NormalizeCoord(int coord, int dim);

	static const int LANE = WIDTH, LAYER = WIDTH * DEPTH;
	GLfloat m_values[LAYERS * DEPTH * WIDTH];

	glm::vec2 m_pillars[3]{
		glm::vec2(0.0f, 0.5f),
		glm::vec2(-0.4f, -0.25f),
		glm::vec2(0.4f, -0.25f) };

	GLuint m_textureId = 0;
	GLuint m_vao = 0, m_vbo = 0;
	GLuint m_vertexCount = 0;
	glm::vec3 m_resolution;

	std::default_random_engine m_random;
	int m_helixFrequence, m_shelveFrequence;
	int m_helixOffset, m_shelveOffset;
};

