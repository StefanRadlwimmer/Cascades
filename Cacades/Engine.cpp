#include "Engine.h"
#include <GL/glew.h>
#include "Shader.h"
#include "Camera.h"
#include "Global.h"
#include <algorithm>
#include <sstream>
#include "Timer.h"
#include <glm/gtc/type_ptr.hpp>

Engine::Engine(GLFWwindow& window) : m_window(window), m_mcShader(nullptr), m_generator(1), m_activeLayer(0)
{
}

Engine::~Engine()
{
	glfwSetWindowShouldClose(&m_window, GL_TRUE);
}

void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

GLFWwindow* Engine::InitWindow(const char* windowTitle, bool fullscreen)
{
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	GLFWmonitor* monitor = nullptr;
	if (fullscreen)
	{
		monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode * mode = glfwGetVideoMode(monitor);
		SCREEN_WIDTH = mode->width;
		SCREEN_HEIGHT = mode->height;
	}

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, windowTitle, monitor, nullptr);
	glfwMakeContextCurrent(window);
	if (fullscreen)
		glfwMaximizeWindow(window);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// Set the required callback functions
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetWindowSizeCallback(window, ResizeCallback);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	glewInit();

	glGetError(); // Call it once to catch glewInit()

	glDebugMessageCallback(DebugCallback, NULL);

	// OpenGL configuration
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	//glEnable(GL_CULL_FACE);
	/*glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);

	return window;
}

void Engine::Init(char* windowTitle)
{
	m_instance = new Engine(*InitWindow(windowTitle, true));
}

void Engine::Init(char* windowTitle, GLuint width, GLuint height)
{
	SCREEN_WIDTH = width;
	SCREEN_HEIGHT = height;
	m_instance = new Engine(*InitWindow(windowTitle, false));
}

void Engine::Start(Camera* camera, Shader* mcShader, Hud* hud)
{
	glUseProgram(mcShader->Program);

	m_mcShader = mcShader;
	m_hud = hud;
	m_camera = camera;

#define TIME
#ifdef TIME
	Timer::TimeAndPrint("Generate Texture", Delegate(&ProcedualGenerator::GenerateVBO, &m_generator, glm::vec3(m_generator.WIDTH, m_generator.LAYERS, m_generator.DEPTH)));
#else
	m_generator.GenerateVBO(glm::vec3(m_generator.WIDTH, m_generator.LAYERS, m_generator.DEPTH));
#endif

	glCheckError();
	m_lookupTable.WriteLookupTablesToGpu();
	m_lookupTable.UpdateUniforms(*m_mcShader);

	Loop();
}

void Engine::RenderMcMesh(Shader& shader)
{
	glDeleteBuffers(1, &m_tbo);

	// Create transform feedback buffer
	glGenBuffers(1, &m_tbo);
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_tbo);
	glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, m_generator.GetVertexCount() * sizeof(glm::vec3) * 3, nullptr, GL_DYNAMIC_COPY);
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
	glCheckError();

	shader.Use();
	m_generator.SetUniformsMC(shader);

	glm::vec3 viewPos = m_camera->GetPosition();
	GLuint viewPosLocation = glGetUniformLocation(shader.Program, "viewPos");
	glUniform3fv(viewPosLocation, 1, glm::value_ptr(viewPos));
	glCheckError();

	glm::vec3 geomPos(0, 0, 0);
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, geomPos);
	GLuint modelLocation = glGetUniformLocation(shader.Program, "model");
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	glCheckError();

	glm::mat4 view = m_camera->GetViewMatrix();
	GLuint viewLocation = glGetUniformLocation(shader.Program, "view");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
	glCheckError();

	glm::mat4 proj = m_camera->GetProjectionMatrix();
	GLuint projLocation = glGetUniformLocation(shader.Program, "projection");
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(proj));
	glCheckError();

	// Create query object to collect info
	GLuint queryTF;
	glGenQueries(1, &queryTF);

	// Perform feedback transform
	//glEnable(GL_RASTERIZER_DISCARD);

	glBindVertexArray(m_generator.GetVaoId());
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_tbo);

	//glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, queryTF);
	//glBeginTransformFeedback(GL_TRIANGLES);
	glDrawArrays(GL_POINTS, 0, m_generator.GetVertexCount());
	//glEndTransformFeedback();
	//glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	glBindVertexArray(0);

	//glDisable(GL_RASTERIZER_DISCARD);

	//glFlush();

	//// Fetch and print results
	//glGetQueryObjectuiv(queryTF, GL_QUERY_RESULT, &m_triCount);
	//printf("%u primitives generated!\n\n", m_triCount);

	//glDeleteQueries(1, &queryTF);

	//glDeleteVertexArrays(1, &m_vao);
	//glDeleteBuffers(1, &m_vbo);

	//glGenVertexArrays(1, &m_vao);
	//glBindVertexArray(m_vao);

	//glGenBuffers(1, &m_vbo);
	//glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	//glBufferData(GL_ARRAY_BUFFER, 3 * m_triCount * sizeof(glm::vec3), nullptr, GL_STATIC_DRAW);
	//glCopyBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, GL_ARRAY_BUFFER, 0, 0, 3 * m_triCount * sizeof(glm::vec3));
	//glCheckError();
	//// Position attribute
	//glEnableVertexAttribArray(VS_IN_POSITION);
	//glVertexAttribPointer(VS_IN_POSITION, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glCheckError();
}

void Engine::Update(GLfloat deltaTime)
{
	m_camera->ProcessInput(m_window);
	m_camera->Update(deltaTime);
}

void Engine::Loop()
{
	// DeltaTime variables
	GLfloat deltaTime = 0.0f;
	GLfloat lastFrame = 0.0f;
	GLfloat second = 0.0f;
	int frame = 0;
	int fps = 0;

	// Game loop
	while (!glfwWindowShouldClose(&m_window))
	{
		// Calculate delta time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
#ifdef _DEBUG
		deltaTime = 1.0f / 60;
#endif
		second += deltaTime;
		lastFrame = currentFrame;
		glfwPollEvents();

		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glClearColor(.5f, .5f, .5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCheckError();

		Update(deltaTime);

		m_generator.Generate3dTexture(m_activeLayer);
		RenderMcMesh(*m_mcShader);

		glfwSwapBuffers(&m_window);
		glCheckError();

		++frame;

		if (second > 1)
		{
			fps = frame;
			frame = 0;
			second -= 1;
		}
	}
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

void Engine::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	Instance()->m_KeyCallback(window, key, scancode, action, mode);
}

// Is called whenever a key is pressed/released via GLFW
void Engine::m_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (action == GLFW_PRESS || action == GLFW_REPEAT)
		switch (key)
		{
		case GLFW_KEY_KP_1:
			m_activeLayer -= 5;
			break;
		case GLFW_KEY_KP_7:
			m_activeLayer += 5;
			break;
		}
}

void Engine::CursorPosCallback(GLFWwindow* window, double x, double y)
{
	Instance()->m_CursorPosCallback(window, x, y);
}

void Engine::m_CursorPosCallback(GLFWwindow* window, double x, double y)
{
	m_camera->CursorPosCallback(x, y);
}

void Engine::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	Instance()->m_MouseButtonCallback(window, button, action, mods);
}

void Engine::ResizeCallback(GLFWwindow* window, int width, int height)
{
	Instance()->m_ResizeCallback(window, width, height);
}

void Engine::m_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{

}

void Engine::m_ResizeCallback(GLFWwindow* window, int width, int height)
{
	SCREEN_WIDTH = width;
	SCREEN_HEIGHT = height;
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

Engine* Engine::m_instance = nullptr;

void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	char* sourceString = "", *typeString = "", *severityString = "";

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:			sourceString = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceString = "WINDOW_SYSTEM"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:	sourceString = "THIRD_PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION:	sourceString = "APPLICATION"; break;
	case GL_DEBUG_SOURCE_OTHER:			sourceString = "OTHER"; break;
	}

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:				typeString = "ERROR"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:	typeString = "DEPRECATED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	typeString = "UNDEFINED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_PORTABILITY:			typeString = "PORTABILITY"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:			typeString = "PERFORMANCE"; break;
	case GL_DEBUG_TYPE_MARKER:				typeString = "MARKER"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:			typeString = "PUSH_GROUP"; break;
	case GL_DEBUG_TYPE_POP_GROUP:			typeString = "POP_GROUP"; break;
	case GL_DEBUG_TYPE_OTHER:				typeString = "OTHER"; break;
	}

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:			severityString = "HIGH"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:			severityString = "MEDIUM"; break;
	case GL_DEBUG_SEVERITY_LOW:				severityString = "LOW"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:	severityString = "NOTIFICATION"; break;
	}

	printf("%s::%s[%s](%d): %s\n", sourceString, typeString, severityString, id, message);
}
