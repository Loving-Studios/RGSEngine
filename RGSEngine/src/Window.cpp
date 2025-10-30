#include "Window.h"
#include "Log.h"
#include "Application.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>

Window::Window() : Module()
{
	window = NULL;
	name = "window";
}

// Destructor
Window::~Window()
{
}

// Called before render is available
bool Window::Awake()
{
	LOG("Init SDL window & surface");
	bool ret = true;

	if (SDL_Init(SDL_INIT_VIDEO) != true)
	{
		LOG("SDL_VIDEO could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		// Create window
		Uint32 flags = 0;
		bool fullscreen_window = false;

		// Get the values from .h
		width = DEFAULT_WIDTH;
		height = DEFAULT_HEIGHT;
		scale = 1;

		fullscreen = false;
		borderless = false;
		resizable = true;

		if (fullscreen == true)        flags |= SDL_WINDOW_FULLSCREEN;
		if (borderless == true)        flags |= SDL_WINDOW_BORDERLESS;
		if (resizable == true)         flags |= SDL_WINDOW_RESIZABLE;
		flags |= SDL_WINDOW_OPENGL;
		

		// Request an OpenGL 4.5 context (should be core)
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
		// Also request a depth buffer
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);


		// SDL3: SDL_CreateWindow(title, w, h, flags). Set position separately.
		window = SDL_CreateWindow("RGSEngine Application", width, height, flags);

		if (window == NULL)
		{
			LOG("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			ret = false;
		}
		else
		{
			if (fullscreen_window == true)
			{
				SDL_SetWindowFullscreenMode(window, nullptr); // use desktop resolution
				SDL_SetWindowFullscreen(window, true);
			}
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			SDL_GL_CreateContext(window);
			glContext = SDL_GL_CreateContext(window);
			if (glContext == NULL)
			{
				ret = false;
			}
			else
			{
				SDL_GL_SetSwapInterval(1);
			}
		}
	}

	return ret;
}

// Called before quitting
bool Window::CleanUp()
{
	LOG("Destroying SDL window and quitting all SDL systems");

	//Destroy context
	if (glContext != NULL)
	{
		SDL_GL_DestroyContext(glContext);
	}
	// Destroy window
	if (window != NULL)
	{
		SDL_DestroyWindow(window);
	}

	// Quit SDL subsystems
	SDL_Quit();
	return true;
}

// Set new window title
void Window::SetTitle(const char* new_title)
{
	//title.create(new_title);
	SDL_SetWindowTitle(window, new_title);
}

void Window::GetWindowSize(int& width, int& height) const
{
	width = this->width;
	height = this->height;
}

int Window::GetScale() const
{
	return scale;
}

void Window::SetFullscreen(bool enable)
{
	if (fullscreen != enable)
	{
		fullscreen = enable;
		SDL_SetWindowFullscreen(window, fullscreen ? true : false);
		SDL_GetWindowSize(window, &width, &height);
	}
}

void Window::SetBorderless(bool enable)
{
	if (borderless != enable)
	{
		borderless = enable;
		SDL_SetWindowBordered(window, borderless ? true : false);
	}
}

void Window::SetResizable(bool enable)
{
	if (resizable != enable)
	{
		resizable = enable;
		SDL_SetWindowResizable(window, resizable ? true : false);
	}
}

void Window::OnResize(int newWidth, int newHeight)
{
	width = newWidth;
	height = newHeight;

	glViewport(0, 0, width, height);
}

void Window::ResetWindowSize()
{
	SDL_SetWindowSize(window, DEFAULT_WIDTH, DEFAULT_HEIGHT);
}