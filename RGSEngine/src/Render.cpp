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

	// Initialize rotation and mouse control
	rotationX = 0.0f;
	rotationY = 0.0f;
	isDragging = false;
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

	// Detect when left mouse button is pressed
	if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
	{
		isDragging = true;
		input->GetMousePosition(lastMouseX, lastMouseY);
	}

	// Detect when button release
	if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_UP)
	{
		isDragging = false;
	}

	//  Update the rotation
	if (isDragging && input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT)
	{
		int currentMouseX, currentMouseY;
		input->GetMousePosition(currentMouseX, currentMouseY);

		
		int deltaX = currentMouseX - lastMouseX;
		int deltaY = currentMouseY - lastMouseY;

		// Update angles of rotation
		float sensitivity = 0.5f;
		rotationY += deltaX * sensitivity;  
		rotationX += deltaY * sensitivity;  

		// Limit rotation on X to avoid full vertical turns
		if (rotationX > 89.0f) rotationX = 89.0f;
		if (rotationX < -89.0f) rotationX = -89.0f;

		// Update last mouse position
		lastMouseX = currentMouseX;
		lastMouseY = currentMouseY;
	}

	return true;
}

bool Render::Update(float dt)
{
	shader->Use();

	// Create transformation matrices
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	// Apply rotations based on mouse input
	model = glm::rotate(model, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate on X
	model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate on Y

	// Move camera back to see the object
	view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

	// Create perspective projection
	int width, height;
	Application::GetInstance().window->GetWindowSize(width, height);
	projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

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

