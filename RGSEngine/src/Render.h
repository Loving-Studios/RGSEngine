#pragma once

#include "Module.h" 
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <memory>
#include "ComponentCamera.h"

class Shader;
class GameObject;
class ComponentTransform;

class Render : public Module
{
public:

	Render();

	// Destructor
	virtual ~Render();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called before each loop iteration
	bool PreUpdate();

	// Called each loop iteration
	bool Update(float dt);

	// Called after each loop iteration
	bool PostUpdate();

	// Called before quitting
	bool CleanUp();

	// Set background color
	void SetBackgroundColor(SDL_Color color);

	void SetMainCamera(ComponentCamera* cam);
	ComponentCamera* GetMainCamera() const { return mainCamera; }

public:
	SDL_Color background;

	unsigned int defaultCheckerTexture;

	//Camera movement speed
	float cameraSpeed;
	float cameraSensitivity;

	bool drawVertexNormals;
	bool drawFaceNormals;

	void ProcessKeyboardMovement(float dt, ComponentTransform* transform);
	void FocusOnGameObject(GameObject* go, ComponentTransform* transform);

	// Orbit mode
	void SetOrbitTarget(GameObject* go);
	GameObject* GetOrbitTarget() const { return orbitTarget; }

	const glm::mat4& GetViewMatrix() const { return viewMatrix; }
	const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }

private:

	// Shader
	std::unique_ptr<Shader> shader;

	std::unique_ptr<Shader> normalsShader;

	float cameraYaw;
	float cameraPitch;

	// Orbit mode
	bool isOrbiting;
	GameObject* orbitTarget;
	glm::vec3 orbitCenter;
	float orbitDistance;
	int orbitLastMouseX;
	int orbitLastMouseY;

	// Mouse control 
	bool isRightDragging;
	int lastMouseX;
	int lastMouseY;

	ComponentCamera* mainCamera;

	void UpdateCameraRotation(ComponentTransform* transform);
	void ProcessMouseFreeLook(float deltaX, float deltaY, ComponentTransform* transform);
	void ProcessMouseOrbit(float deltaX, float deltaY, ComponentTransform* transform);

	void DrawGameObject(GameObject* go, const glm::mat4& parentTransform);
	void CreateDefaultCheckerTexture();

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
};

