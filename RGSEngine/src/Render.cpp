#include "Application.h"
#include "Window.h"
#include "Render.h"
#include "Log.h"
#include "Shader.h"


Render::Render() : Module()
{
	name = "render";
	background.r = 0;
	background.g = 0;
	background.b = 0;
	background.a = 255;

    VAO = 0;
    VBO = 0;
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
	LOG("Render start - Creating triangle");

	//Create shader
	shader = std::make_unique<Shader>();

// Triangle vertices
	float vertices[] = {
		-0.5f, -0.5f, 0.0f,  // left
		 0.5f, -0.5f, 0.0f,  // right
		 0.0f,  0.5f, 0.0f   // up
	};

	// Create VAO and VBO
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Bind Vertex Array Object
	glBindVertexArray(VAO);

	// Copy vertices to buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Config vertices
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	LOG("Triangle created successfully");

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
	shader->Use();
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
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
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
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

