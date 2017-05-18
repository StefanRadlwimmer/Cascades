
#pragma once
#include "Shader.h"
#include "Engine.h"
#include "Camera.h"
#include "Global.h"
#include "Hud.h"
#include "PointLight.h"
#include "DirectionalLight.h"
#include "LinearPath.h"

std::vector<ControlPoint>* GetGreenLightPath()
{
	std::vector<ControlPoint>* path = new std::vector<ControlPoint>();
	path->push_back(ControlPoint(glm::vec3(0, -10, 0), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(0, 10, 0), glm::radians(glm::vec3(0, 0, 0))));
	return path;
}

std::vector<ControlPoint>* GetRedLightPath()
{
	std::vector<ControlPoint>* path = new std::vector<ControlPoint>();
	path->push_back(ControlPoint(glm::vec3(5, 10, 0), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(0, 7.5f, 5), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(-5, 5, 0), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(0, 2.5f, -5), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(5, 0, 0), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(0, -2.5f, 5), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(-5, -5, 0), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(0, -7.5f, -5), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(5, -10, 0), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(0, -7.5f, 5), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(-5, -5, 0), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(0, -2.5f, -5), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(5, 0, 0), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(0, 2.5f, 5), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(-5, 5, 0), glm::radians(glm::vec3(0, 0, 0))));
	path->push_back(ControlPoint(glm::vec3(0, 7.5f, -5), glm::radians(glm::vec3(0, 0, 0))));
	return path;
}

void TestCSAA()
{
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(10, 10, "Test", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	glewInit();

	PrintCSAA();

	glfwSetWindowShouldClose(window, GL_TRUE);
	glfwTerminate();

	std::cin.ignore();
}

int main(int argc, char** argv)
{
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--test") == 0)
		{
			TestCSAA();
			return 0;
		}
	}

	try {
#ifdef _DEBUG
		Engine::Init("Camera", 1900, 1000);
#else
		Engine::Init("Camera", 1900, 1000);
#endif
		Engine* engine = Engine::Instance();

		Shader* particleUpdate = new Shader("./shaders/ParticleUpdate.vert", "./shaders/ParticleUpdate.geom", nullptr);
		particleUpdate->Test("particleUpdate");

		Shader* pointLightShader = new Shader("./shaders/PointLight.vert", "./shaders/PointLight.geom", "./shaders/PointLight.frag");
		pointLightShader->Test("pointLightShader");

		Shader* directionalLightShader = new Shader("./shaders/DirectionalLight.vert", nullptr, "./shaders/DirectionalLight.frag");
		directionalLightShader->Test("directionalLightShader");
				
		PointLight* greenLight = new PointLight(glm::vec3(0, 0, 0), glm::vec3(0, 0.5f, 0), *pointLightShader, 15);
		greenLight->IsEnabled(true);
		greenLight->CastsShadows(false);
		greenLight->FollowPath(new LinearPath(*GetGreenLightPath(), 10, true));
		engine->AddLight(*greenLight);

		PointLight* redLight = new PointLight(glm::vec3(0, 0, 0), glm::vec3(0.5f, 0, 0), *pointLightShader, 15);
		redLight->IsEnabled(true);
		redLight->CastsShadows(false);
		redLight->FollowPath(new CircularPath(*GetRedLightPath(), 20, true));
		engine->AddLight(*redLight);

		DirectionalLight* mainLight1 = new DirectionalLight(glm::vec3(10), glm::vec3(0.5f), *directionalLightShader, 50, -10);
		mainLight1->IsEnabled(false);
		mainLight1->CastsShadows(false);
		engine->AddLight(*mainLight1);

		DirectionalLight* antiLight1 = new DirectionalLight(glm::vec3(-10), glm::vec3(0, 0, 0.05f), *directionalLightShader, 50, -10);
		antiLight1->IsEnabled(false);
		engine->AddLight(*antiLight1);

		engine->Start();

		return 0;
	}
	catch (std::exception e)
	{
		std::cout << "ERROR::" << e.what() << " | " << __FILE__ << " (" << __LINE__ << ")" << std::endl;
		std::cin.ignore();
		return -1;
	}
}