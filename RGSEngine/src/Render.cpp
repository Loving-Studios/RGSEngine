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

#include "imgui.h"
#include "imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

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
	background.r = 0;
	background.g = 0;
	background.b = 0;
	background.a = 255;

	defaultCheckerTexture = 0;

	drawVertexNormals = false;
	drawFaceNormals = false;

	// Initialize camera rotation
	mainCamera = nullptr;
	cameraYaw = -90.0f;
	cameraPitch = 0.0f;

	cameraSpeed = 2.5f;
	cameraSensitivity = 0.1f;

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

	return true;
}

void Render::SetMainCamera(ComponentCamera* cam)
{
	mainCamera = cam;
	LOG("Main Camera set in Renderer.");

	// Sinc the controller yaw/pitch with the initial rotation of the camera
	if (mainCamera)
	{
		ComponentTransform* transform = mainCamera->owner->GetComponent<ComponentTransform>();
		if (transform)
		{
			glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transform->rotation));
			LOG("Camera controller sync (Yaw: %.2f, Pitch: %.2f)", cameraYaw, cameraPitch);
		}
	}
}

void Render::UpdateCameraRotation(ComponentTransform* transform)
{
	// Convert the angles, the controller, to a quaternion in this order: Pitch (X), Yaw (Y), Roll (Z)
	glm::quat newRotation = glm::quat(glm::vec3(glm::radians(cameraPitch), glm::radians(cameraYaw), 0.0f));

	// Apply the rotation to the transform of the camera
	transform->SetRotation(newRotation);
}


void Render::ProcessKeyboardMovement(float dt, ComponentTransform* transform)
{
	Input* input = Application::GetInstance().input.get();

	float speed = cameraSpeed * dt;

	if (input->IsShiftPressed())
	{
		speed *= 2.0f;
	}

	// Calculate the direction vector from the transform
	glm::vec3 front = transform->rotation * glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 right = transform->rotation * glm::vec3(1.0f, 0.0f, 0.0f);

	// WASD movement
	if (input->GetKey(SDL_SCANCODE_W))
		transform->position += front * speed;
	if (input->GetKey(SDL_SCANCODE_S))
		transform->position -= front * speed;
	if (input->GetKey(SDL_SCANCODE_A))
		transform->position -= right * speed;
	if (input->GetKey(SDL_SCANCODE_D))
		transform->position += right * speed;
}

void Render::ProcessMouseFreeLook(float deltaX, float deltaY, ComponentTransform* transform)
{
	cameraYaw += deltaX * cameraSensitivity;
	cameraPitch -= deltaY * cameraSensitivity;

	// Limit pitch to avoid flip
	if (cameraPitch > 89.0f)
		cameraPitch = 89.0f;
	if (cameraPitch < -89.0f)
		cameraPitch = -89.0f;

	UpdateCameraRotation(transform); // Sinc the rotation with the ComponentTransform
}

void Render::ProcessMouseOrbit(float deltaX, float deltaY, ComponentTransform* transform)
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
	transform->SetPosition(orbitCenter - (offset * orbitDistance));

	// Aim for the center, calculating the vector front
	glm::vec3 front = glm::normalize(orbitCenter - transform->position);

	// Update the camera vectors calculating the rotation quaternion who looks to that position
	transform->SetRotation(glm::quatLookAt(front, glm::vec3(0.0f, 1.0f, 0.0f)));
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

	// If there is no camera
	if (mainCamera == nullptr)
	{
		return true;
	}

	ComponentTransform* camTransform = mainCamera->owner->GetComponent<ComponentTransform>();
	if (camTransform == nullptr)
	{
		return true;
	}

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse)
	{
		isRightDragging = false;
		isOrbiting = false;
		return true;
	}

	// Mouse control for rotation
	Input* input = Application::GetInstance().input.get();

	if (input->IsAltPressed() && orbitTarget != nullptr)
	{
		if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
		{
			isOrbiting = true;
			// Use the GLOBAL position of the object as centre
			orbitCenter = orbitTarget->GetGlobalPosition();
			// Use the transform position of the camera
			orbitDistance = glm::length(camTransform->position - orbitCenter);
		}

		if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_UP)
		{
			isOrbiting = false;
		}

		if (isOrbiting && input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT)
		{
			int deltaX = 0;
			int deltaY = 0;
			input->GetMouseMotion(deltaX, deltaY);

			float dampedDeltaX = (float)deltaX * 0.8f;
			float dampedDeltaY = (float)deltaY * 0.8f;

			ProcessMouseOrbit(dampedDeltaX, dampedDeltaY, camTransform);
		}
	}
	// If Alt is not pressed stop orbiting
	if (!input->IsAltPressed())
	{
		isOrbiting = false;
	}

	// Free look view
	if (input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN)
	{
		isRightDragging = true;
	}

	if (input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_UP)
	{
		isRightDragging = false;
	}

	// Only active the free look view if is draggong and not orbiting
	if (isRightDragging && !isOrbiting && input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_REPEAT)
	{
		int deltaX = 0;
		int deltaY = 0;
		input->GetMouseMotion(deltaX, deltaY);

		float dampedDeltaX = (float)deltaX * 0.8f;
		float dampedDeltaY = (float)deltaY * 0.8f;

		ProcessMouseFreeLook(dampedDeltaX, dampedDeltaY, camTransform);
	}

	// Zoom
	int mouseWheelY = input->GetMouseWheel();
	if (mouseWheelY != 0)
	{
		mainCamera->cameraFOV -= mouseWheelY * 2.0f;
		if (mainCamera->cameraFOV < 1.0f) mainCamera->cameraFOV = 1.0f;
		if (mainCamera->cameraFOV > 90.0f) mainCamera->cameraFOV = 90.0f;
	}

	return true;
}

bool Render::Update(float dt)
{
	// Obtain the camera and the transform
	if (mainCamera == nullptr) return true;
	ComponentTransform* camTransform = mainCamera->owner->GetComponent<ComponentTransform>();
	if (camTransform == nullptr) return true;

	Input* input = Application::GetInstance().input.get();
	ImGuiIO& io = ImGui::GetIO();

	if (isRightDragging && !isOrbiting && !io.WantCaptureKeyboard)
	{
		ProcessKeyboardMovement(dt, camTransform);
	}

	// Obtain the matrix from the camera component
	int width, height;
	Application::GetInstance().window->GetWindowSize(width, height);
	// The camera now calculates his owns matrix
	glm::mat4 viewMatrix = mainCamera->GetViewMatrix();
	glm::mat4 projectionMatrix = mainCamera->GetProjectionMatrix(width, height);

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
		LOG("Camera FOV: %.2f", mainCamera->cameraFOV);
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
		if (texture != nullptr)
		{
			texture->Bind();
		}
		else
		{
			// Use default checker texture
			glBindTexture(GL_TEXTURE_2D, defaultCheckerTexture);
		}
		shader->SetInt("tex1", 0);

		// Draw the mesh
		mesh->Draw();

		// Unlink the texture
		glBindTexture(GL_TEXTURE_2D, 0);

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

void Render::FocusOnGameObject(GameObject* go, ComponentTransform* transform)
{
	if (go == nullptr || transform == nullptr)
		return;

	ComponentTransform* targetTransform = go->GetComponent<ComponentTransform>();
	if (targetTransform == nullptr)
		return;

	// Get the GLOBAL object position
	glm::vec3 targetPos = go->GetGlobalPosition();

	//Calculate camera distance based on object size
	float objectSize = glm::max(glm::max(targetTransform->scale.x, targetTransform->scale.y), targetTransform->scale.z);
	float distance = objectSize * 3.0f;
	// Minimum distance
	if (distance < 2.0f) distance = 2.0f;

	// Obtain the actual direction from the angles of the controller
	glm::vec3 front;
	front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	front.y = sin(glm::radians(cameraPitch));
	front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
	front = glm::normalize(front);

	// Set the new camera position
	glm::vec3 newCamPos = targetPos - (front * distance);
	transform->SetPosition(newCamPos);

	// Calculate the controller angles
	glm::vec3 direction = glm::normalize(targetPos - newCamPos);

	// Update those angles
	cameraYaw = glm::degrees(atan2(direction.z, direction.x));
	cameraPitch = glm::degrees(asin(direction.y));

	// Sync with the transform
	UpdateCameraRotation(transform);

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

//void Render::SetViewPort(const SDL_Rect& rect)
//{
//	SDL_SetRenderViewport(renderer, &rect);
//}
//
//void Render::ResetViewPort()
//{
//	SDL_SetRenderViewport(renderer, &viewport);
//}

