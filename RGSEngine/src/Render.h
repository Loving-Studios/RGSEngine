#pragma once
#include "Module.h"
#include <SDL3/SDL.h>

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
	SDL_Renderer* renderer;
	SDL_Color background;

private:

};

