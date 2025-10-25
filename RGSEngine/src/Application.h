#pragma once

#include <memory>
#include <list>
#include <cstdint>
#include "Module.h"


// Modules
class Window;
class Input;
class Render;
class ModuleScene;
class LoadFiles;

class Application
{
public:

	// Public method to get the instance of the Singleton
	static Application& GetInstance();

	//	
	void AddModule(std::shared_ptr<Module> module);

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called each loop iteration
	bool Update();

	// Called before quitting
	bool CleanUp();

private:

	// Private constructor to prevent instantiation
	// Constructor
	Application();

	// Delete copy constructor and assignment operator to prevent copying
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	// Call modules before each loop iteration
	void PrepareUpdate();

	// Call modules before each loop iteration
	void FinishUpdate();

	// Call modules before each loop iteration
	bool PreUpdate();

	// Call modules on each loop iteration
	bool DoUpdate();

	// Call modules after each loop iteration
	bool PostUpdate();

	std::list<std::shared_ptr<Module>> moduleList;

public:

	enum EngineState
	{
		CREATE = 1,
		AWAKE,
		START,
		LOOP,
		CLEAN,
		FAIL,
		EXIT
	};

	// Modules
	std::shared_ptr<Window> window;
	std::shared_ptr<Input> input;
	std::shared_ptr<Render> render;
	std::shared_ptr<ModuleScene> scene;

private:
	float dt;
	uint64_t lastFrameTime = 0;
};