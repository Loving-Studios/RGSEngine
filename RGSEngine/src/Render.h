#include "Module.h" 
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <memory>

class Shader;
class GameObject;

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

	//Camera movement speed
	float cameraSpeed;
	float cameraSensitivity;
	float cameraFOV;      // Field of view

	void ProcessKeyboardMovement(float dt);
	void FocusOnGameObject(GameObject* go);

	// Orbit mode
	void SetOrbitTarget(GameObject* go);
	GameObject* GetOrbitTarget() const { return orbitTarget; }

private:

	// Shader
	std::unique_ptr<Shader> shader;

	unsigned int defaultCheckerTexture;

	// Camera propieties 

	glm::vec3 cameraPos; // Camera position
	glm::vec3 cameraFront; // Direction the camera is looking at
	glm::vec3 cameraUp; // Vector up camera
	glm::vec3 cameraRight; // Vector right camera

	float cameraYaw;      // Rotation horizontal (Y)
	float cameraPitch;   // Rotation vertical (X)

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

	//Helper functions
	void UpdateCameraVectors();

	void ProcessMouseFreeLook(int deltaX, int deltaY);
	void ProcessMouseOrbit(int deltaX, int deltaY);

	void DrawGameObject(GameObject* go);

	void CreateDefaultCheckerTexture();
};

