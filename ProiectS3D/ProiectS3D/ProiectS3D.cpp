// ViewOBJModel.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <Windows.h>
#include <locale>
#include <codecvt>

#include <stdlib.h> // necesare pentru citirea shader-elor
#include <stdio.h>
#include <math.h> 

#include <GL/glew.h>

#include <filesystem.h>
#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include<stb_image.h>

#include <glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "Shader.h"
#include "Model.h"

#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
const unsigned int SCREEN_WIDTH = 800;
const unsigned int SCREEN_HEIGHT = 600;

enum CameraMode {
	SPECTATOR,
	THIRD_PERSON
};

CameraMode cameraMode = SPECTATOR;


enum ECameraMovementType
{
	UNKNOWN,
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

float deltaYaw = 90.0f;
float deltaPitch = -10.0f;

class Camera
{
private:
	// Default camera values
	const float zNEAR = 0.1f;
	const float zFAR = 500.f;
	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float FOV = 45.0f;
	glm::vec3 startPosition;

public:
	Camera(const int width, const int height, const glm::vec3& position)
	{
		startPosition = position;
		Set(width, height, position);
	}



	void LockToTarget(glm::vec3 targetPosition, glm::vec3 targetRotation)
	{
		glm::vec3 offset = glm::vec3(0, 8, -15); // Offset vertical și înainte/înapoi
		float rotationRadians = glm::radians(targetRotation.y + 180.0f); // Rotație cu 180 grade pentru a privi în față
		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), rotationRadians, glm::vec3(0.0f, 1.0f, 0.0f));

	
		position = targetPosition;
		position.y += 3.3;

		yaw = targetRotation.y + 90.0f;
		//pitch = -targetRotation.x;

		UpdateCameraVectors();
	}

	void Unlock() {
		// Resetează camera la comportamentul standard
		Reset(SCR_WIDTH, SCR_HEIGHT);
	}

	void Set(const int width, const int height, const glm::vec3& position)
	{
		this->isPerspective = true;
		this->yaw = -90.0f;
		this->pitch = -10.0f;

		this->FoVy = FOV;
		this->width = width;
		this->height = height;
		this->zNear = zNEAR;
		this->zFar = zFAR;

		this->worldUp = glm::vec3(0, 1, 0);
		this->position = position + glm::vec3(0,50.0f,0);

		lastX = width / 2.0f;
		lastY = height / 2.0f;
		bFirstMouseMove = true;

		UpdateCameraVectors();
	}

	void Reset(const int width, const int height)
	{
		Set(width, height, startPosition);
	}

	void Reshape(int windowWidth, int windowHeight)
	{
		width = windowWidth;
		height = windowHeight;

		// define the viewport transformation
		glViewport(0, 0, windowWidth, windowHeight);
	}

	const glm::mat4 GetViewMatrix() const
	{
		// Returns the View Matrix
		return glm::lookAt(position, position + forward, up);
	}

	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(position, position + forward, up);
	}

	const glm::vec3 GetPosition() const
	{
		return position;
	}
	glm::mat4 GetProjMatrix(float aspect)
	{
		return glm::perspective(glm::radians(FoVy), aspect, zNear, zFar);
	}

	const glm::mat4 GetProjectionMatrix() const
	{
		glm::mat4 Proj = glm::mat4(1);
		if (isPerspective) {
			float aspectRatio = ((float)(width)) / height;
			Proj = glm::perspective(glm::radians(FoVy), aspectRatio, zNear, zFar);
		}
		else {
			float scaleFactor = 2000.f;
			Proj = glm::ortho<float>(
				-width / scaleFactor, width / scaleFactor,
				-height / scaleFactor, height / scaleFactor, -zFar, zFar);
		}
		return Proj;
	}

	void ProcessKeyboard(ECameraMovementType direction, float deltaTime)
	{
		float velocity = (float)(cameraSpeedFactor * deltaTime);
		switch (direction) {
		case ECameraMovementType::FORWARD:
			position += forward * velocity;
			break;
		case ECameraMovementType::BACKWARD:
			position -= forward * velocity;
			break;
		case ECameraMovementType::LEFT:
			position -= right * velocity;
			break;
		case ECameraMovementType::RIGHT:
			position += right * velocity;
			break;
		case ECameraMovementType::UP:
			position += up * velocity;
			break;
		case ECameraMovementType::DOWN:
			position -= up * velocity;
			break;
		}
	}

	void MouseControl(float xPos, float yPos)
	{
		if (bFirstMouseMove) {
			lastX = xPos;
			lastY = yPos;
			bFirstMouseMove = false;
		}

		float xChange = xPos - lastX;
		float yChange = lastY - yPos;
		lastX = xPos;
		lastY = yPos;

		if (fabs(xChange) <= 1e-6 && fabs(yChange) <= 1e-6) {
			return;
		}
		xChange *= mouseSensitivity;
		yChange *= mouseSensitivity;

		ProcessMouseMovement(xChange, yChange);
	}

	void ProcessMouseScroll(float yOffset)
	{
		if (FoVy >= 1.0f && FoVy <= 90.0f) {
			FoVy -= yOffset;
		}
		if (FoVy <= 1.0f)
			FoVy = 1.0f;
		if (FoVy >= 90.0f)
			FoVy = 90.0f;
	}

private:
	void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true)
	{
		yaw += xOffset;
		pitch += yOffset;

		//std::cout << "yaw = " << yaw << std::endl;
		//std::cout << "pitch = " << pitch << std::endl;

		// Avem grijă să nu ne dăm peste cap
		if (constrainPitch) {
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
		}

		// Se modifică vectorii camerei pe baza unghiurilor Euler
		UpdateCameraVectors();
	}

	void UpdateCameraVectors()
	{
		// Calculate the new forward vector
		this->forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->forward.y = sin(glm::radians(pitch));
		this->forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->forward = glm::normalize(this->forward);
		// Also re-calculate the Right and Up vector
		right = glm::normalize(glm::cross(forward, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		up = glm::normalize(glm::cross(right, forward));
	}

protected:
	const float cameraSpeedFactor = 15.5f;
	const float mouseSensitivity = 1.0f;

	// Perspective properties
	float zNear;
	float zFar;
	float FoVy;
	int width;
	int height;
	bool isPerspective;

	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 worldUp;

	// Euler Angles
	float yaw;
	float pitch;

	bool bFirstMouseMove = true;
	float lastX = 0.f, lastY = 0.f;
};

GLuint ProjMatrixLocation, ViewMatrixLocation, WorldMatrixLocation;
Model* masinaModel = nullptr;
Model* rotiModel = nullptr;
Model* raceTrackModel = nullptr;
Camera* pCamera = nullptr;

void Cleanup()
{
	delete pCamera;
	delete masinaModel;
	delete rotiModel;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// timing
double deltaTime = 0.0f;	// time between current frame and last frame
double lastFrame = 0.0f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_V && action == GLFW_PRESS) {
		if (cameraMode == SPECTATOR) {
			cameraMode = THIRD_PERSON;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			pCamera->LockToTarget(masinaModel->GetPosition(), glm::vec3(0, 2, -10));
		}
		else {
			cameraMode = SPECTATOR;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			pCamera->Unlock();
		}
	}
}

//skybox stuff
unsigned int cubemapTexture;
unsigned int skyboxVAO, skyboxVBO;

const float skyboxVertices[] = {
	// positions
	-1.0f, 1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f,

	-1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,

	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,

	-1.0f, 1.0f, -1.0f,
	1.0f, 1.0f, -1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, 1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, 1.0f,
	1.0f, -1.0f, 1.0f
};
const std::vector<std::string> faces{
		 FileSystem::getPath("Assets/skybox/day/right.jpg"),
		 FileSystem::getPath("Assets/skybox/day/left.jpg"),
		 FileSystem::getPath("Assets/skybox/day/top.jpg"),
		 FileSystem::getPath("Assets/skybox/day/bottom.jpg"),
		 FileSystem::getPath("Assets/skybox/day/front.jpg"),
		 FileSystem::getPath("Assets/skybox/day/back.jpg")
};
const std::vector<std::string> faces2{
		 FileSystem::getPath("Assets/skybox/night/right.jpg"),
		 FileSystem::getPath("Assets/skybox/night/left.jpg"),
		 FileSystem::getPath("Assets/skybox/night/top.jpg"),
		 FileSystem::getPath("Assets/skybox/night/bottom.jpg"),
		 FileSystem::getPath("Assets/skybox/night/front.jpg"),
		 FileSystem::getPath("Assets/skybox/night/back.jpg")
};

unsigned int loadCubemap(std::vector<std::string> faces)
{
	//initialize texture id and bind it
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			//stbi_image_free(data);
		}
		else {
			std::cout << "Could not load texture at path: " << faces[i] << std::endl;
			//stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;

}

//function to initialize sky box
void skyboxInit() {

	//skybox VAO
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	////load textures
	cubemapTexture = loadCubemap(faces);
}

void skyboxReload() {

	//skybox VAO
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	//load textures
	cubemapTexture = loadCubemap(faces2);
}

//function to render the skybox
void renderSkybox(Shader& shader)
{

	//view matrix constructed to remove the movement of the camera
	glm::mat4 viewMatrix = glm::mat4(glm::mat3(pCamera->GetViewMatrix()));
	// projection
	glm::mat4 projMatrix = pCamera->GetProjMatrix((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);

	shader.setMat4("view", viewMatrix);
	shader.setMat4("projection", projMatrix);

	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

}



GLuint loadTexture(char const* path)
{
	GLuint textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

glm::vec3 WORLD_UP(-1.0f, 0.0f, 0.0f);

void renderWheelsOnTrack(Model& model, Shader& shader)
{
	// View and projection matrices
	glm::mat4 viewMatrix = pCamera->GetViewMatrix();
	shader.setMat4("view", viewMatrix);

	glm::mat4 projMatrix = pCamera->GetProjMatrix((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);
	shader.setMat4("projection", projMatrix);

	// Define the positions for the three wheels
	std::vector<glm::vec3> wheelPositions = {
		glm::vec3(-10.0f, 0.0f, -0.5f),
		glm::vec3(10.0f, 0.0f, 5.0f),
		glm::vec3(15.0f, 0.0f, -0.5f)
	};

	// Define the scaling factor
	glm::vec3 scalingFactor = glm::vec3(10.0f); // Adjust this value to make the wheels bigger

	for (const auto& position : wheelPositions)
	{
		// Model transformation
		glm::mat4 modelMatrix = glm::mat4(1.0f);

		// Apply scaling
		modelMatrix = glm::scale(modelMatrix, scalingFactor);

		// Apply rotation (if necessary)
		modelMatrix = glm::rotate(modelMatrix, glm::radians(0.0f), WORLD_UP);

		// Apply translation
		modelMatrix = glm::translate(modelMatrix, position);

		// Set the model matrix in the shader
		shader.setMat4("model", modelMatrix);

		// Draw the model
		model.Draw(shader);
	}
}


float scaleFactor;
float aspectRatio;
float quadHeight;
float quadWidth;

struct SpriteInfo {
	glm::vec3 position;
	float rotation;
	glm::vec3 scale;
	float height;
};

struct SpriteCluster {
	glm::vec3 center;
	std::vector<SpriteInfo> sprites;
	int numSprites;
	float dispersionRadius;
};

int main()
{
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);



	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Karting", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);

	// tell GLFW to capture our mouse
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	glewInit();

	glEnable(GL_DEPTH_TEST);

	GLuint floorTexture = loadTexture("Models/Harta.jpg");

	GLuint spriteTexture = loadTexture("Models/spriteTexture.png");

	GLuint wallTexture = loadTexture("Models/wallTexture.jpg"); // Load a wall texture


	int width, height, nrChannels;
	unsigned char* data = stbi_load("Models/Harta.jpg", &width, &height, &nrChannels, 0);


	scaleFactor = 50.0f; // sau orice altă valoare pentru scalare
	aspectRatio = static_cast<float>(width) / height;
	quadHeight = 10.0f * scaleFactor;
	quadWidth = quadHeight * aspectRatio;

	float floorVertices[] = {
		// poziții                         // normale     // coordonate textura
		quadWidth / 2, 0.0f,  quadHeight / 2,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
	   -quadWidth / 2, 0.0f,  quadHeight / 2,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
	   -quadWidth / 2, 0.0f, -quadHeight / 2,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,

		quadWidth / 2, 0.0f,  quadHeight / 2,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
	   -quadWidth / 2, 0.0f, -quadHeight / 2,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
		quadWidth / 2, 0.0f, -quadHeight / 2,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f
	};

	GLuint floorVAO, floorVBO;
	glGenVertexArrays(1, &floorVAO);
	glBindVertexArray(floorVAO);

	glGenBuffers(1, &floorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

	// Poziție vertex
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	// Normala vertex
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	// Coordonatele texturii
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);



	skyboxInit();
	Shader skyboxShader("Shaders/skybox.vs", "Shaders/skybox.fs");
	skyboxShader.use();
	skyboxShader.setInt("skybox", 0);

	Shader spriteShader("Shaders/sprite.vs", "Shaders/sprite.fs");
	spriteShader.use();
	spriteShader.setInt("spriteTexture", 0);

	Shader wallShader("Shaders/Wall.vs", "Shaders/Wall.fs");
	wallShader.use();
	wallShader.setInt("wallTexture", 0);



	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};

		//sprite stuff
	float spriteVertices[] = {
		// Coordonate      // Coordonate textură
		0.5f,  0.5f,  1.0f, 1.0f, // top right
		0.5f, -0.5f,  1.0f, 0.0f, // bottom right
	   -0.5f, -0.5f,  0.0f, 0.0f, // bottom left
	   -0.5f,  0.5f,  0.0f, 1.0f  // top left 
	};

	unsigned int spriteIndices[] = {
		0, 1, 3,   // first Triangle
		1, 2, 3    // second Triangle
	};
	// Definirea unui vector pentru a stoca toate clusterele
	std::vector<SpriteCluster> grassClusters;

	// Inițializarea generatorului de numere aleatorii
	srand(static_cast<unsigned int>(time(nullptr)));

	// Crearea și adăugarea clusterurilor
	for (int i = 0; i < 15; ++i) {
		SpriteCluster cluster;
		// Generarea coordonatelor centrale în interiorul limitele quadului
		cluster.center = glm::vec3(
			((float)rand() / RAND_MAX) * quadWidth - quadWidth / 2, // Coordonată X aleatorie în interiorul quadului
			1.0f, // Înălțimea fixată la 1.0f
			((float)rand() / RAND_MAX) * quadHeight - quadHeight / 2 // Coordonată Z aleatorie în interiorul quadului
		);
		cluster.numSprites = 10 + rand() % 20; // Număr aleatoriu de sprite-uri între 10 și 30
		cluster.dispersionRadius = 0.5f + static_cast<float>(rand()) / RAND_MAX * 2.0f; // Raza de dispersie aleatorie între 0.5 și 2.5

		// Generarea sprite-urilor pentru acest cluster
		for (int j = 0; j < cluster.numSprites; ++j) {
			float angle = static_cast<float>(rand()) / RAND_MAX * 360.0f;
			float radius = static_cast<float>(rand()) / RAND_MAX * cluster.dispersionRadius;
			float x = cluster.center.x + radius * cos(glm::radians(angle));
			float z = cluster.center.z + radius * sin(glm::radians(angle));
			float y = 2.0f; // Înălțime fixă

			SpriteInfo sprite;
			sprite.position = glm::vec3(x, y, z);
			sprite.rotation = static_cast<float>(rand()) / RAND_MAX * 360.0f;
			sprite.scale = glm::vec3(5.0f); // Scală fixată
			sprite.height = 1.0f; // Înălțimea fixată

			cluster.sprites.push_back(sprite);
		}

		grassClusters.push_back(cluster);
	}


	GLuint spriteVAO, spriteVBO, spriteEBO;
	glGenVertexArrays(1, &spriteVAO);
	glGenBuffers(1, &spriteVBO);
	glGenBuffers(1, &spriteEBO);

	glBindVertexArray(spriteVAO);

	glBindBuffer(GL_ARRAY_BUFFER, spriteVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(spriteVertices), spriteVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spriteEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(spriteIndices), spriteIndices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// Texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//wall
	float borderWidth = -1.0f; // Width of the wall
	float wallHeight = 6.5f; // Height of the wall
	float zOffset = 0.0f;

	float wallVertices[] = {
		// positions              // texture coords
		// Line 1 - Top
		-quadWidth / 2 - borderWidth, 0.0f, quadHeight / 2 + borderWidth + zOffset, 0.0f, 0.0f,
		quadWidth / 2 + borderWidth, 0.0f, quadHeight / 2 + borderWidth + zOffset, 1.0f, 0.0f,
		-quadWidth / 2 - borderWidth, wallHeight, quadHeight / 2 + borderWidth + zOffset, 0.0f, 1.0f,
		quadWidth / 2 + borderWidth, wallHeight, quadHeight / 2 + borderWidth + zOffset, 1.0f, 1.0f,

		// Line 2 - Bottom
		-quadWidth / 2 - borderWidth, 0.0f, -quadHeight / 2 - borderWidth + zOffset, 0.0f, 0.0f,
		quadWidth / 2 + borderWidth, 0.0f, -quadHeight / 2 - borderWidth + zOffset, 1.0f, 0.0f,
		-quadWidth / 2 - borderWidth, wallHeight, -quadHeight / 2 - borderWidth + zOffset, 0.0f, 1.0f,
		quadWidth / 2 + borderWidth, wallHeight, -quadHeight / 2 - borderWidth + zOffset, 1.0f, 1.0f,

		// Line 3 - Left
		-quadWidth / 2 - borderWidth, 0.0f, -quadHeight / 2 - borderWidth + zOffset, 0.0f, 0.0f,
		-quadWidth / 2 - borderWidth, 0.0f, quadHeight / 2 + borderWidth + zOffset, 1.0f, 0.0f,
		-quadWidth / 2 - borderWidth, wallHeight, -quadHeight / 2 - borderWidth + zOffset, 0.0f, 1.0f,
		-quadWidth / 2 - borderWidth, wallHeight, quadHeight / 2 + borderWidth + zOffset, 1.0f, 1.0f,

		// Line 4 - Right
		quadWidth / 2 + borderWidth, 0.0f, -quadHeight / 2 - borderWidth + zOffset, 0.0f, 0.0f,
		quadWidth / 2 + borderWidth, 0.0f, quadHeight / 2 + borderWidth + zOffset, 1.0f, 0.0f,
		quadWidth / 2 + borderWidth, wallHeight, -quadHeight / 2 - borderWidth + zOffset, 0.0f, 1.0f,
		quadWidth / 2 + borderWidth, wallHeight, quadHeight / 2 + borderWidth + zOffset, 1.0f, 1.0f
	};

	unsigned int wallIndices[] = {
		// Indices for drawing the quads
		0, 1, 2, 1, 2, 3,       // Top wall
		4, 5, 6, 5, 6, 7,       // Bottom wall
		8, 9, 10, 9, 10, 11,    // Left wall
		12, 13, 14, 13, 14, 15  // Right wall
	};

	unsigned int wallVAO, wallVBO, wallEBO;
	glGenVertexArrays(1, &wallVAO);
	glGenBuffers(1, &wallVBO);
	glGenBuffers(1, &wallEBO);

	glBindVertexArray(wallVAO);

	glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), wallVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(wallIndices), wallIndices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// Texture coordinate attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	//wall

	// Create camera
	pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0, 0.0, 3.0));

	glm::vec3 lightPos(0.0f, 100.0f, 0.0f);

	wchar_t buffer[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, buffer);

	std::wstring executablePath(buffer);
	std::wstring wscurrentPath = executablePath.substr(0, executablePath.find_last_of(L"\\/"));

	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string currentPath = converter.to_bytes(wscurrentPath);



	Shader lightingShader((currentPath + "\\Shaders\\PhongLight.vs").c_str(), (currentPath + "\\Shaders\\PhongLight.fs").c_str());
	Shader lampShader((currentPath + "\\Shaders\\Lamp.vs").c_str(), (currentPath + "\\Shaders\\Lamp.fs").c_str());
	Shader shadowShader((currentPath + "\\Shaders\\ShadowMapping.vs").c_str(), (currentPath + " \\Shaders\\ShadowMapping.fs").c_str());
	Shader depthShader((currentPath + "\\Shaders\\ShadowMappingDepth.vs").c_str(), (currentPath + "\\Shaders\\ShadowMappingDepth.fs").c_str());

	GLuint depthMapFBO;
	GLuint depthMap;
	const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthMap);

	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// Render to depth map
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	depthShader.use();
	// Setează uniformele și matricile necesare pentru depthShader
	//RenderScene(depthShader);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Render scene with shadows
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shadowShader.use();
	shadowShader.setInt("shadowMap", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	// Setează alte uniforme necesare pentru shadowShader
	//RenderScene(shadowShader);

	Shader floorShader("Shaders/Floor.vs", "Shaders/Floor.fs");


	masinaModel = new Model(currentPath + "\\Models\\F1LowPoly.obj", false);
	rotiModel = new Model(currentPath + "\\Models\\roti.obj", false);

	// render loop
	while (!glfwWindowShouldClose(window)) {
		// per-frame time logic
		double currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		processInput(window);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightPos.x = 0.5 * cos(glfwGetTime());
		lightPos.z = 0.5 * sin(glfwGetTime());

		lightingShader.use();
		//lightingShader.SetVec3("objectColor", 0.5f, 1.0f, 0.31f);
		//lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
		//lightingShader.SetVec3("lightPos", lightPos);
		lightingShader.SetVec3("viewPos", pCamera->GetPosition());

		lightingShader.setMat4("projection", pCamera->GetProjectionMatrix());
		lightingShader.setMat4("view", pCamera->GetViewMatrix());

		renderWheelsOnTrack(*rotiModel, lightingShader);

		floorShader.use();
		floorShader.setInt("texture1", 0);

		// Setează transformările necesare
		glm::mat4 floorModel = glm::mat4(1.0f);
		floorShader.setMat4("model", floorModel);
		floorShader.setMat4("view", pCamera->GetViewMatrix());
		floorShader.setMat4("projection", pCamera->GetProjectionMatrix());

		glBindVertexArray(floorVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, floorTexture);  // Asigură-te că folosești GL_TEXTURE_2D aici
		floorShader.setInt("texture1", 0);  // Presupunând că uniforma în shader este numită 'texture1'
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);


		//wall
		wallShader.use();
		glm::mat4 wallModel = glm::mat4(1.0f);
		wallShader.setMat4("model", wallModel);
		wallShader.setMat4("view", pCamera->GetViewMatrix());
		wallShader.setMat4("projection", pCamera->GetProjectionMatrix());

		glBindVertexArray(wallVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, wallTexture);
		glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		

		//sprite
		spriteShader.use();
		spriteShader.setMat4("view", pCamera->GetViewMatrix());
		spriteShader.setMat4("projection", pCamera->GetProjectionMatrix());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, spriteTexture);
		glBindVertexArray(spriteVAO);


		for (const auto& cluster : grassClusters) 
		{
			for (const auto& sprite : cluster.sprites) {
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, sprite.position);
				model = glm::rotate(model, glm::radians(sprite.rotation), glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::scale(model, sprite.scale);

				spriteShader.setMat4("model", model);
				glBindVertexArray(spriteVAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
		}


		lightingShader.use();
		glm::mat4 masinaModelMatrix = masinaModel->GetTransformMatrix();
		lightingShader.setMat4("model", masinaModelMatrix);
		masinaModel->Draw(lightingShader);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glDepthFunc(GL_LEQUAL);
		skyboxShader.use();
		renderSkybox(skyboxShader);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	Cleanup();

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	glDeleteVertexArrays(1, &spriteVAO);
	glDeleteBuffers(1, &spriteVBO);
	glDeleteBuffers(1, &spriteEBO);
	glDeleteTextures(1, &spriteTexture);


	// glfw: terminate, clearing all previously allocated GLFW resources
	glfwTerminate();
	return 0;
}
bool isDay = true;

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
	{
		skyboxInit();
		isDay = true;
	}
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
	{
		skyboxReload();
		isDay = false;
	}

	// Diferențiem controlul camerei pe baza modului curent
	if (cameraMode == SPECTATOR) {
		// Controalele standard pentru modul Spectator
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			pCamera->ProcessKeyboard(FORWARD, (float)deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			pCamera->ProcessKeyboard(BACKWARD, (float)deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			pCamera->ProcessKeyboard(LEFT, (float)deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			pCamera->ProcessKeyboard(RIGHT, (float)deltaTime);
		if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
			pCamera->ProcessKeyboard(UP, (float)deltaTime);
		if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
			pCamera->ProcessKeyboard(DOWN, (float)deltaTime);

		glm::vec3 direction = glm::vec3(0.0f);
		float speed = 25.0f * deltaTime;

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
			direction.z += speed;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
			direction.z -= speed;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
			masinaModel->Rotate(90.0f * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
			masinaModel->Rotate(-90.0f * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));

		// Actualizează poziția
		masinaModel->UpdatePosition(direction);

		// Verifică și restricționează poziția
		glm::vec3 pos = masinaModel->GetPosition();
		float limitX = quadWidth / 2 - 8;
		float limitZ = quadHeight / 2 - 8;

		if (pos.x < -limitX) pos.x = -limitX;
		if (pos.x > limitX) pos.x = limitX;
		if (pos.z < -limitZ) pos.z = -limitZ;
		if (pos.z > limitZ) pos.z = limitZ;

		masinaModel->SetPosition(pos);


	}
	else if (cameraMode == THIRD_PERSON) {
		glm::vec3 direction = glm::vec3(0.0f);
		float speed = 25.0f * deltaTime;

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			direction.z += speed;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			direction.z -= speed;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			masinaModel->Rotate(90.0f * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			masinaModel->Rotate(-90.0f * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));

		// Actualizează poziția
		masinaModel->UpdatePosition(direction);

		// Verifică și restricționează poziția
		glm::vec3 pos = masinaModel->GetPosition();
		float limitX = quadWidth / 2 - 8;
		float limitZ = quadHeight / 2 - 8;

		if (pos.x < -limitX) pos.x = -limitX;
		if (pos.x > limitX) pos.x = limitX;
		if (pos.z < -limitZ) pos.z = -limitZ;
		if (pos.z > limitZ) pos.z = limitZ;

		masinaModel->SetPosition(pos);

		pCamera->LockToTarget(masinaModel->GetPosition(), -masinaModel->Rotation);
	}

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		pCamera->Reset(width, height);
	}
}



// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	pCamera->Reshape(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	pCamera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
	if (cameraMode == SPECTATOR)
		pCamera->ProcessMouseScroll((float)yOffset);
}
