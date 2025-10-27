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


Render::Render() : Module()
{
	name = "render";
	background.r = 0;
	background.g = 0;
	background.b = 0;
	background.a = 255;

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

	if (isRightDragging && !io.WantCaptureKeyboard)
	{
		ProcessKeyboardMovement(dt);
	}

	// Only allow WASD movement when the right button is pressed
	if (isRightDragging)
	{
		ProcessKeyboardMovement(dt);
	}

	//glm::mat4 model = glm::mat4(1.0f);
	// Creo que esta linea ya no hace falta
	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	// Create perspective projection
	int width, height;
	Application::GetInstance().window->GetWindowSize(width, height);
	glm::mat4 projection = glm::perspective(glm::radians(cameraFOV), (float)width / (float)height, 0.1f, 100.0f);

	shader->Use();

	// Send matrix to shader
	shader->Use();
	shader->SetMat4("view", view);
	shader->SetMat4("projection", projection);

	// Obtain the rootObject of the scene
	std::shared_ptr<GameObject> root = Application::GetInstance().scene->rootObject;

	// Start the process to draw recursive
	if (root != nullptr)
	{
		DrawGameObject(root.get());
	}

	return true;
}

void Render::DrawGameObject(GameObject* go)
{
	if (go == nullptr || !go->IsActive())
	{
		return;
	}

	// Obtain the needed components
	ComponentTransform* transform = go->GetComponent<ComponentTransform>();
	ComponentMesh* mesh = go->GetComponent<ComponentMesh>();
	ComponentTexture* texture = go->GetComponent<ComponentTexture>();

	// Only draw if the object has all the necesary
	if (mesh != nullptr && texture != nullptr && transform != nullptr)
	{
		// 1. Obtain the Model Matrix of the Component Transform
		glm::mat4 model = transform->GetModelMatrix();

		// 2. Sended to the shader
		shader->SetMat4("model", model);

		// 3. Link the texture
		glActiveTexture(GL_TEXTURE0);
		texture->Bind();
		shader->SetInt("tex1", 0);

		// 4. Draw the mesh
		mesh->Draw();

		// 5. Unlink the texture
		texture->Unbind();
	}

	// 6. Repeat the process for all the Childrens
	for (const auto& child : go->GetChildren())
	{
		DrawGameObject(child.get());
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
	// The shader is from this class so we have to CleanUp
	shader.reset();
	return true;
}

void Render::SetBackgroundColor(SDL_Color color)
{
	background = color;
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

//void Render::SetViewPort(const SDL_Rect& rect)
//{
//	SDL_SetRenderViewport(renderer, &rect);
//}
//
//void Render::ResetViewPort()
//{
//	SDL_SetRenderViewport(renderer, &viewport);
//}

