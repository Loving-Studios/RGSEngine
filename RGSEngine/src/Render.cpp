#include "Application.h"
#include "Window.h"
#include "Render.h"
#include "Log.h"
#include "Shader.h"
#include "Input.h"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


Render::Render() : Module()
{
	name = "render";
	background.r = 0;
	background.g = 0;
	background.b = 0;
	background.a = 255;

	VAO = 0;
	VBO = 0;
	IBO = 0;
	textureID = 0;

	// Initialize camera rotation
	cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);

	cameraYaw = -90.0f;
	cameraPitch = 0.0f;

	cameraSpeed = 2.5f;
	cameraSensitivity = 0.1f;
	cameraFOV = 45.0f;

	isRightDragging = false;

	lastMouseX = 0;
	lastMouseY = 0;
}

// Destructor
Render::~Render()
{
}

// Called before render is available
bool Render::Awake()
{
	LOG("Create SDL rendering context");
	bool ret = true;

	SDL_Window* window = Application::GetInstance().window->window;

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	{
		LOG("Failed to initialize GLAD");
		return false;
	}

	// Config viewport
	int width, height;
	Application::GetInstance().window->GetWindowSize(width, height);
	glViewport(0, 0, width, height);

	glEnable(GL_DEPTH_TEST);

	return ret;
}

// Called before the first frame
bool Render::Start()
{
	LOG("render start");

	//Create shader
	shader = std::make_unique<Shader>();

	// Pyramid vertices with UV coordinates
	// X, Y, Z, U, V
	float vertices[] = {
		// Base vertices (y = -0.5) with UV
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  // front-left
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  // front-right
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  // back-right
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  // back-left

		// Apex (top point) with UV centered
		 0.0f,  0.5f,  0.0f,  0.5f, 0.5f   // top
	};

	// Indices for the pyramid
	unsigned int indices[] = {
		// Base (2 triangles)
		0, 1, 2,
		0, 2, 3,

		// Front face
		0, 1, 4,

		// Right face
		1, 2, 4,

		// Back face
		2, 3, 4,

		// Left face
		3, 0, 4
	};

	// Create VAO, VBO and IBO
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &IBO);

	// Bind Vertex Array Object
	glBindVertexArray(VAO);

	// Copy vertices to buffer VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Copy vertices to buffer IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Config vertices attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	LOG("Pyramid created successfully");

	LOG("Creating default checker texture");

	// Texture data 8x8 pixels with 4 components: RGBA
	const int texWidth = 8;
	const int texHeight = 8;
	GLubyte checkerTexture[texWidth * texHeight * 4];
	for (int y = 0; y < texHeight; y++) {
		for (int x = 0; x < texWidth; x++) {
			int i = (y * texWidth + x) * 4;
			// Patrón de ajedrez
			bool isBlack = ((x % 2) == 0) != ((y % 2) == 0);
			checkerTexture[i + 0] = isBlack ? 0 : 255;   // R
			checkerTexture[i + 1] = isBlack ? 0 : 255;   // G
			checkerTexture[i + 2] = isBlack ? 0 : 255;   // B
			checkerTexture[i + 3] = 255;                 // A
		}
	}

	// Generate and link the texture
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Configure texture parameters
	// GL_NEAREST is the checkered pattern
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// GL_REPEAT causes the texture to repeat if the UVs > 1
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Upload texture data to the GPU
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerTexture);

	// Disconnect the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	LOG("Default texture created successfully");

	return true;
}

void Render::UpdateCameraVectors()
{
	// Calcular nuevo vector 
	glm::vec3 front;
	front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	front.y = sin(glm::radians(cameraPitch));
	front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	cameraFront = glm::normalize(front);

	// Recalculate right y up
	cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
	cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
}


void Render::ProcessKeyboardMovement(float dt)
{
	Input* input = Application::GetInstance().input.get();

	float speed = cameraSpeed * dt;

	// WASD movement
	if (input->GetKey(SDL_SCANCODE_W))
		cameraPos += cameraFront * speed;
	if (input->GetKey(SDL_SCANCODE_S))
		cameraPos -= cameraFront * speed;
	if (input->GetKey(SDL_SCANCODE_A))
		cameraPos -= cameraRight * speed;
	if (input->GetKey(SDL_SCANCODE_D))
		cameraPos += cameraRight * speed;
}

void Render::ProcessMouseFreeLook(int deltaX, int deltaY)
{
	cameraYaw += deltaX * cameraSensitivity;
	cameraPitch -= deltaY * cameraSensitivity;

	// Limit pitch to avoid flip
	if (cameraPitch > 89.0f)
		cameraPitch = 89.0f;
	if (cameraPitch < -89.0f)
		cameraPitch = -89.0f;

	UpdateCameraVectors();
}
// Called each loop iteration
bool Render::PreUpdate()
{
	glClearColor(
		background.r / 255.0f,
		background.g / 255.0f,
		background.b / 255.0f,
		background.a / 255.0f
	);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Mouse control for rotation
	Input* input = Application::GetInstance().input.get();

	if (input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN)
	{
		isRightDragging = true;
		input->GetMousePosition(lastMouseX, lastMouseY);
	}

	if (input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_UP)
	{
		isRightDragging = false;
	}

	// Free look
	if (isRightDragging && input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_REPEAT)
	{
		int currentMouseX, currentMouseY;
		input->GetMousePosition(currentMouseX, currentMouseY);

		int deltaX = currentMouseX - lastMouseX;
		int deltaY = currentMouseY - lastMouseY;

		ProcessMouseFreeLook(deltaX, deltaY);

		lastMouseX = currentMouseX;
		lastMouseY = currentMouseY;
	}

	// Zoom
	int mouseWheelY = input->GetMouseWheel();
	if (mouseWheelY != 0)
	{
		cameraFOV -= mouseWheelY * 2.0f;
		// Limit FOV between 1 and 90 degrees
		if (cameraFOV < 1.0f)
			cameraFOV = 1.0f;
		if (cameraFOV > 90.0f)
			cameraFOV = 90.0f;
	}

	return true;
}

bool Render::Update(float dt)
{
	Input* input = Application::GetInstance().input.get();

	// Only allow WASD movement when the right button is pressed
	if (isRightDragging)
	{
		ProcessKeyboardMovement(dt);
	}
	shader->Use();


	glm::mat4 model = glm::mat4(1.0f);


	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	// Create perspective projection
	int width, height;
	Application::GetInstance().window->GetWindowSize(width, height);
	glm::mat4 projection = glm::perspective(glm::radians(cameraFOV), (float)width / (float)height, 0.1f, 100.0f);

	// Send matrices to shader
	shader->SetMat4("model", model);
	shader->SetMat4("view", view);
	shader->SetMat4("projection", projection);

	// Before drawing, activate the texture 0
	glActiveTexture(GL_TEXTURE0);
	// Link the texture 
	glBindTexture(GL_TEXTURE_2D, textureID);
	// Inform shader to use the texture 0
	shader->SetInt("tex1", 0);

	// Draw the pyramid
	glBindVertexArray(VAO);
	// Draw using the indices of IBO which are linked to VAO
	glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0); // Unlink VAO
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

bool Render::PostUpdate()
{
	SDL_GL_SwapWindow(Application::GetInstance().window->window);
	return true;
}

// Called before quitting
bool Render::CleanUp()
{
	LOG("Destroying SDL render");
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &IBO);
	glDeleteTextures(1, &textureID);
	return true;
}

void Render::SetBackgroundColor(SDL_Color color)
{
	background = color;
}

//void Render::SetViewPort(const SDL_Rect& rect)
//{
//	SDL_SetRenderViewport(renderer, &rect);
//}
//
//void Render::ResetViewPort()
//{
//	SDL_SetRenderViewport(renderer, &viewport);
//}

