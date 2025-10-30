#pragma once

#include "Module.h"
#include <SDL3/SDL.h>

class Window : public Module
{
public:

	static const int DEFAULT_WIDTH = 1280;
	static const int DEFAULT_HEIGHT = 720;

	Window();

	// Destructor
	virtual ~Window();

	// Called before render is available
	bool Awake();

	// Called before quitting
	bool CleanUp();

	// Changae title
	void SetTitle(const char* title);

	// Retrive window size
	void GetWindowSize(int& width, int& height) const;

	// Retrieve window scale
	int GetScale() const;

	void SetFullscreen(bool enable);
	void SetBorderless(bool enable);
	void SetResizable(bool enable);

	void OnResize(int newWidth, int newHeight);
	void ResetWindowSize();

public:
	// The window we'll be rendering to
	SDL_Window* window;
	SDL_GLContext glContext;
	std::string title;

	int width = DEFAULT_WIDTH;
	int height = DEFAULT_HEIGHT;
	int scale = 1;

	bool fullscreen = false;
	bool borderless = false;
	bool resizable = false;
};