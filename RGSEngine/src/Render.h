#pragma once
#include "Module.h"
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <memory>

class Shader;

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


public:
	SDL_Color background;

private:

	unsigned int VAO; // Vertex Array Object
	unsigned int VBO; // Vertex Buffer Object
	unsigned int IBO; // Index Buffer Object
	unsigned int textureID;

	// Shader
	std::unique_ptr<Shader> shader;

	// Camera propieties 

	glm::vec3 cameraPos; // Camera position
	glm::vec3 cameraFront; // Direction the camera is looking at
	glm::vec3 cameraUp; // Vector up camera
	glm::vec3 cameraRight; // Vector right camera

	float cameraYaw;      // Rotation horizontal (Y)
	float cameraPitch;   // Rotation vertical (X)

	//Camera movement speed
	float cameraSpeed;
	float cameraSensitivity;

	// Mouse control 
	bool isRightDragging;
	int lastMouseX;
	int lastMouseY;

	//Helper functions
	void UpdateCameraVectors();
	void ProcessKeyboardMovement(float dt);
	void ProcessMouseFreeLook(int deltaX, int deltaY);


};

