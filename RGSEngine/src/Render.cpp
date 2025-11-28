#include "Application.h"
#include "Window.h"
#include "Render.h"
#include "Log.h"
#include "Shader.h"
#include "Input.h"

#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "ComponentCamera.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace NormalShaders
{
	const char* vertex = R"(
    #version 460 core
    layout (location = 0) in vec3 aPos; // Positions

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
    )";

	const char* fragment = R"(
    #version 460 core
    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Color amarillo
    }
    )";
}

Render::Render() : Module()
{
	name = "render";
	background.r = 50;
	background.g = 50;
	background.b = 50;
	background.a = 255;
	// Grey Color
	defaultCheckerTexture = 0;

	drawVertexNormals = false;
	drawFaceNormals = false;

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

	// Initialize orbit
	isOrbiting = false;
	orbitTarget = nullptr;
	orbitCenter = glm::vec3(0.0f);
	orbitDistance = 0.0f;

	lastMouseX = 0;
	lastMouseY = 0;
	orbitLastMouseX = 0;
	orbitLastMouseY = 0;
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

	// Create shader
	shader = std::make_unique<Shader>();

	// Create shader for the normals
	normalsShader = std::make_unique<Shader>(NormalShaders::vertex, NormalShaders::fragment);
	
	CreateDefaultCheckerTexture();

	CreateGrid();

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

	if (input->IsShiftPressed())
	{
		speed *= 2.0f;
	}

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

void Render::ProcessMouseOrbit(int deltaX, int deltaY)
{
	// Update rotation angles based on mouse movement
	cameraYaw += deltaX * cameraSensitivity;
	cameraPitch -= deltaY * cameraSensitivity;

	// Limit pitch to avoid flip
	if (cameraPitch > 89.0f)
		cameraPitch = 89.0f;
	if (cameraPitch < -89.0f)
		cameraPitch = -89.0f;

	// Calculate the new position of the camera orbiting around the center
	glm::vec3 offset;
	offset.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	offset.y = sin(glm::radians(cameraPitch));
	offset.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));

	offset = glm::normalize(offset);

	// Position the camera at orbit distance from center
	cameraPos = orbitCenter - (offset * orbitDistance);

	// Aim for the center
	cameraFront = glm::normalize(orbitCenter - cameraPos);

	// Update the camera vectors
	cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
	cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
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

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse)
	{
		isRightDragging = false;
		return true;
	}

	// Mouse control for rotation
	Input* input = Application::GetInstance().input.get();

	if (input->IsAltPressed() && orbitTarget != nullptr)
	{
		if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
		{
			isOrbiting = true;
			input->GetMousePosition(orbitLastMouseX, orbitLastMouseY);

			// Configurar el centro de órbita y la distancia
			ComponentTransform* transform = orbitTarget->GetComponent<ComponentTransform>();
			if (transform != nullptr)
			{
				orbitCenter = transform->position;
				orbitDistance = glm::length(cameraPos - orbitCenter);
			}
		}

		if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_UP)
		{
			isOrbiting = false;
		}

		if (isOrbiting && input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT)
		{
			int currentMouseX, currentMouseY;
			input->GetMousePosition(currentMouseX, currentMouseY);

			int deltaX = currentMouseX - orbitLastMouseX;
			int deltaY = currentMouseY - orbitLastMouseY;

			ProcessMouseOrbit(deltaX, deltaY);

			orbitLastMouseX = currentMouseX;
			orbitLastMouseY = currentMouseY;
		}
	}

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
	ImGuiIO& io = ImGui::GetIO();

	if (isRightDragging && !isOrbiting && !io.WantCaptureKeyboard)
	{
		ProcessKeyboardMovement(dt);
	}

	// Only allow WASD movement when the right button is pressed
	if (isRightDragging)
	{
		ProcessKeyboardMovement(dt);
	}

	viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	// Create perspective projection
	int width, height;
	Application::GetInstance().window->GetWindowSize(width, height);
	projectionMatrix = glm::perspective(glm::radians(cameraFOV), (float)width / (float)height, 0.1f, 100.0f);

	DrawGrid();

	shader->Use();

	// Send matrix to shader
	shader->Use();
	shader->SetMat4("view", viewMatrix);
	shader->SetMat4("projection", projectionMatrix);

	normalsShader->Use();
	normalsShader->SetMat4("view", viewMatrix);
	normalsShader->SetMat4("projection", projectionMatrix);

	shader->Use();
	// Obtain the rootObject of the scene
	std::shared_ptr<GameObject> root = Application::GetInstance().scene->rootObject;
	// Start the process to draw recursive
	if (root != nullptr)
	{
		DrawGameObject(root.get(), glm::mat4(1.0f));
	}

	static bool loggedOnce = false;
	if (!loggedOnce)
	{
		LOG("=== CAMERA INFO ===");
		LOG("Camera position: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);
		LOG("Camera front: (%.2f, %.2f, %.2f)", cameraFront.x, cameraFront.y, cameraFront.z);
		LOG("Camera FOV: %.2f", cameraFOV);
		loggedOnce = true;
	}

	return true;
}

void Render::DrawGameObject(GameObject* go, const glm::mat4& parentTransform)
{
	if (go == nullptr || !go->IsActive())
	{
		return;
	}

	// Obtain the needed components
	ComponentTransform* transform = go->GetComponent<ComponentTransform>();
	ComponentMesh* mesh = go->GetComponent<ComponentMesh>();
	ComponentTexture* texture = go->GetComponent<ComponentTexture>();

	glm::mat4 localTransform = (transform != nullptr) ? transform->GetModelMatrix() : glm::mat4(1.0f);
	glm::mat4 globalTransform = parentTransform * localTransform;

	if (mesh != nullptr && transform != nullptr)
	{
		// Force the main shader is active
		shader->Use();

		// TEMPORARY LOG FOR DEBUG
		static bool loggedOnce = false;
		if (!loggedOnce && mesh->VBO_UV != 0)
		{
			LOG("Drawing mesh with UVs: VBO_UV = %d", mesh->VBO_UV);
			loggedOnce = true;
		}

		// Send to the shader
		shader->SetMat4("model", globalTransform);

		// Link the texture
		glActiveTexture(GL_TEXTURE0);

		bool alphaTest = false;
		float alphaCutoff = 0.0f;
		bool blending = false;

		if (texture != nullptr)
		{
			texture->Bind();

			alphaTest = texture->enableAlphaTest;
			alphaCutoff = texture->alphaThreshold;
			blending = texture->enableBlending;

			// BLENDING
			if (blending) {
				glEnable(GL_BLEND);
				glBlendFunc(texture->blendSrc, texture->blendDst);
			}
			else {
				glDisable(GL_BLEND);
			}
		}
		else
		{
			// Use default checker texture
			glBindTexture(GL_TEXTURE_2D, defaultCheckerTexture);
			glDisable(GL_BLEND);
		}

		// Send Uniforms of Alpha Test to the shader
		shader->SetBool("enableAlphaTest", alphaTest);
		shader->SetFloat("alphaThreshold", alphaCutoff);

		shader->SetInt("tex1", 0);

		// Draw the mesh
		mesh->Draw();

		// Unlink the texture
		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_BLEND);

		if (drawVertexNormals || drawFaceNormals)
		{
			// Use the shader of the normals
			normalsShader->Use();
			// Send the model matrix
			normalsShader->SetMat4("model", globalTransform);

			if (drawVertexNormals) mesh->DrawNormals();
			if (drawFaceNormals)   mesh->DrawFaceNormals();
		}
	}

	ComponentCamera* camera = go->GetComponent<ComponentCamera>();
	if (camera != nullptr && camera->active)
	{
		// Using the shader for the normals
		normalsShader->Use();

		// Send the indentity transform because GenerateFrustumGizmo already uses the world coords with transform->position
		glm::mat4 identity = glm::mat4(1.0f);
		normalsShader->SetMat4("model", identity);

		// Draw the lines
		camera->DrawFrustum();

		shader->Use();
	}

	for (const auto& child : go->GetChildren())
	{
		DrawGameObject(child.get(), globalTransform);
	}
}

bool Render::PostUpdate()
{
	//Draw the inferface of ImGui in screen
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	SDL_GL_SwapWindow(Application::GetInstance().window->window);
	return true;
}

// Called before quitting
bool Render::CleanUp()
{
	LOG("Destroying SDL render");

	if (defaultCheckerTexture != 0)
	{
		glDeleteTextures(1, &defaultCheckerTexture);
		defaultCheckerTexture = 0;
	}

	// Grid CleanUp
	if (gridVAO != 0) { glDeleteVertexArrays(1, &gridVAO); gridVAO = 0; }
	if (gridVBO != 0) { glDeleteBuffers(1, &gridVBO); gridVBO = 0; }

	// The shader is from this class so we have to CleanUp
	shader.reset();
	normalsShader.reset();
	return true;
}

void Render::SetBackgroundColor(SDL_Color color)
{
	background = color;
}

void Render::SetOrbitTarget(GameObject* go)
{
	orbitTarget = go;
}

void Render::FocusOnGameObject(GameObject* go)
{
	if (go == nullptr)
		return;

	ComponentTransform* transform = go->GetComponent<ComponentTransform>();
	if (transform == nullptr)
		return;

	// Get the object position
	glm::vec3 targetPos = transform->position;

	//Calculate camera distance based on object size
	float objectSize = glm::max(glm::max(transform->scale.x, transform->scale.y), transform->scale.z);
	float distance = objectSize * 3.0f; // Multiplicador para dar espacio visual

	// Minimum distance

	if (distance < 2.0f)
		distance = 2.0f;

	// Position the camera at a distance from the object in the current viewing direction
	cameraPos = targetPos - (cameraFront * distance);

	// Calculate the vector from camera to object
	glm::vec3 direction = glm::normalize(targetPos - cameraPos);

	// Calculate yaw and pitch from this direction vector
	cameraYaw = glm::degrees(atan2(direction.z, direction.x));
	cameraPitch = glm::degrees(asin(direction.y));


	UpdateCameraVectors();

	LOG("Camera focused on: %s at position (%.2f, %.2f, %.2f)",
		go->GetName().c_str(), targetPos.x, targetPos.y, targetPos.z);
}

void Render::CreateDefaultCheckerTexture()
{
	const int texWidth = 64, texHeight = 64;
	GLubyte* checkerTexture = new GLubyte[texWidth * texHeight * 4];

	for (int y = 0; y < texHeight; y++) {
		for (int x = 0; x < texWidth; x++) {
			int i = (y * texWidth + x) * 4;

			// 8x8 pixel squares
			bool isBlack = (((x / 8) % 2) == 0) != (((y / 8) % 2) == 0);

			GLubyte color = isBlack ? 50 : 200;
			checkerTexture[i + 0] = color;
			checkerTexture[i + 1] = color;
			checkerTexture[i + 2] = color;
			checkerTexture[i + 3] = 255;
		}
	}

	glGenTextures(1, &defaultCheckerTexture);
	glBindTexture(GL_TEXTURE_2D, defaultCheckerTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerTexture);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] checkerTexture;

	LOG("Default checker texture created: ID %d (%dx%d)", defaultCheckerTexture, texWidth, texHeight);

	if (glIsTexture(defaultCheckerTexture))
	{
		LOG("Checker texture verification: OK");
	}
	else
	{
		LOG("ERROR: Checker texture creation failed!");
	}
}

void Render::CreateGrid()
{
	float gridSize = 15.0f;
	int lines = 15;
	std::vector<float> vertices;

	for (int i = -lines; i <= lines; ++i) {
		float pos = i * (gridSize / lines);

		// Paralel lines to the Z axis (X-axis lines)
		vertices.push_back(pos); vertices.push_back(0.0f); vertices.push_back(-gridSize);
		vertices.push_back(pos); vertices.push_back(0.0f); vertices.push_back(gridSize);

		// Paralel lines to the X (Z-axis lines)
		vertices.push_back(-gridSize); vertices.push_back(0.0f); vertices.push_back(pos);
		vertices.push_back(gridSize); vertices.push_back(0.0f); vertices.push_back(pos);
	}

	gridVertexCount = vertices.size() / 3;

	glGenVertexArrays(1, &gridVAO);
	glGenBuffers(1, &gridVBO);

	glBindVertexArray(gridVAO);
	glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	// Position attribute (layout 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	LOG("Grid VAO created: %d, Lines: %d", gridVAO, gridVertexCount / 2);
}

void Render::DrawGrid()
{
	if (gridVAO == 0) return;

	// Using the same shader (color) as the normals
	normalsShader->Use();

	// Grid doesnt have model transformation, its the identity
	glm::mat4 model = glm::mat4(1.0f);
	normalsShader->SetMat4("model", model);

	glBindVertexArray(gridVAO);
	glDrawArrays(GL_LINES, 0, gridVertexCount);
	glBindVertexArray(0);
}