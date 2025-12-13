#include "Module.h" 
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <memory>
#include "Frustum.h"

class Shader;
class GameObject;
class ComponentMesh;

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

	bool enableFrustumCulling;//=true;
	bool visualizeFrustumCulling;

	// Octree
	bool useOctreeForCulling = true;

	// Culling statistics
	int totalObjects = 0;
	int culledObjects = 0;
	int renderedObjects = 0;

	// Octree statistics
	int octreeQueriedObjects = 0;
	int octreeSkippedObjects = 0; 

public:
	SDL_Color background;

	unsigned int defaultCheckerTexture;

	//Camera movement speed
	float cameraSpeed;
	float cameraSensitivity;
	float cameraFOV;      // Field of view

	bool drawVertexNormals;
	bool drawFaceNormals;
	bool drawAABBs;

	void ProcessKeyboardMovement(float dt);
	void FocusOnGameObject(GameObject* go);

	// Orbit mode
	void SetOrbitTarget(GameObject* go);
	GameObject* GetOrbitTarget() const { return orbitTarget; }

	const glm::mat4& GetViewMatrix() const { return viewMatrix; }
	const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }
	const Frustum& GetCurrentFrustum() const { return currentFrustum; }

	GameObject* selectedObject = nullptr;

	// Debug colors for visualization
	glm::vec4 colorNormal;
	glm::vec4 colorCulled;
	glm::vec4 colorVisible;
	glm::vec4 colorSelected;

private:

	// Shader
	std::unique_ptr<Shader> shader;

	std::unique_ptr<Shader> normalsShader;

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

	Frustum currentFrustum;

	//Helper functions
	void UpdateCameraVectors();

	void ProcessMouseFreeLook(int deltaX, int deltaY);
	void ProcessMouseOrbit(int deltaX, int deltaY);

	void DrawGameObject(GameObject* go, const glm::mat4& parentTransform);

	void DrawNonMeshObjects(GameObject* go, const glm::mat4& parentTransform);

	void CreateDefaultCheckerTexture();

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	unsigned int gridVAO = 0;
	unsigned int gridVBO = 0;
	unsigned int gridVertexCount = 0;

	void CreateGrid();
	void DrawGrid();

	void DrawAABBWithColor(ComponentMesh* mesh, const glm::mat4& transform, const glm::vec4& color);
};

