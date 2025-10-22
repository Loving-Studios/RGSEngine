#include "Application.h"
#include "Window.h"
#include "Render.h"
#include "Log.h"


Render::Render() : Module()
{
	name = "render";
	background.r = 255;
	background.g = 150;
	background.b = 25;
	background.a = 255;
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

    /*
    renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == NULL)
    {
        LOG("Could not create the renderer! SDL_Error: %s\n", SDL_GetError());
        ret = false;
    }
    */
	/*else
	{
	camera.w = Engine::GetInstance().window->width * scale;
	camera.h = Engine::GetInstance().window->height * scale;
	camera.x = 0;
	camera.y = 0;

	}*/

    return ret;
}

// Called before the first frame
bool Render::Start()
{
	LOG("render start");
	// back background
	/*if (!SDL_GetRenderViewport(renderer, &viewport))
	{
		LOG("SDL_GetRenderViewport failed: %s", SDL_GetError());
	}*/
	return true;
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

    return true;
}

bool Render::Update(float dt)
{

	return true;
}

bool Render::PostUpdate()
{
    SDL_GL_SwapWindow(Application::GetInstance().window->window);
	return true;
}

// Called before quitting
bool Render::CleanUp()
{
	LOG("Destroying SDL render");
	//SDL_DestroyRenderer(renderer);
	return true;
}

void Render::SetBackgroundColor(SDL_Color color)
{
	background = color;
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

