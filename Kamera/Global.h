#pragma once
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <iostream>

const int PATH_APPROXIMATION = 100;
const int VS_IN_POSITION = 0;
const int VS_IN_NORMAL = 1;
const int VS_IN_UV = 2;
const int VS_IN_TANGENT = 3;
const int KEY_COUNT = 512;
const float KEY_SENSITIVITY = 0.05f;
const float MOUSE_SENSITIVITY = 0.005f;
const int DIR_LIGHT = 0;
const int SPOT_LIGHT = 1;
const int POINT_LIGHT = 2;

static GLuint WIDTH = 1920;
static GLuint HEIGHT = 1080;

#ifdef _DEBUG
#define glCheckError() glCheckError_(__FILE__, __LINE__) 
#else
#define glCheckError() 
#endif

struct LightIndexer
{
	LightIndexer() {}
	LightIndexer(int textureOffset) : TextureIndex(textureOffset) {}

	int DirIndex = 0;
	int SpotIndex = 0;
	int PointIndex = 0;
	int TextureIndex = 0;
};

inline glm::quat MakeQuad(GLfloat pitch, GLfloat yaw, GLfloat roll)
{
	return glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
}

inline glm::quat MakeQuad(glm::vec3 v)
{
	return MakeQuad(v.x, v.y, v.z);
}

GLenum static glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}

inline float ClampAngles(const float orientation)
{
	if (orientation > glm::pi<float>())
		return orientation - glm::two_pi<float>();

	if (orientation < -glm::pi<float>())
		return  orientation + glm::two_pi<float>();

	return orientation;
}

inline glm::vec3 ClampAngles(const glm::vec3& v)
{
	return glm::vec3(ClampAngles(v.x), ClampAngles(v.y), ClampAngles(v.z));
}

