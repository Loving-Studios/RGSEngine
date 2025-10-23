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
	IBO = 0;
	textureID = 0;
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
	// X, Y, Z, U, V
	float vertices[] = {
		// Position          // Coordinates of Texture UV
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,  // Vertex 0 (down-left)
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f,  // Vertex 1 (down-right)
		 0.0f,  0.5f, 0.0f,   0.5f, 1.0f   // Vertex 2 (up-centre)
	};

	unsigned int indices[] = {
		0, 1, 2
	};

	// Create VAO, VBO and IBO
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &IBO);

	// Bind Vertex Array Object
	glBindVertexArray(VAO);

	// Copy vertices to buffer VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Copy vertices to buffer IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Config vertices

	// Position Atribute layout = 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Texture Coordinates Atribute layout = 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	LOG("Triangle created successfully");

	LOG("Creating default checker texture");

	// Texture data 8x8 pixels with 4 components: RGBA
	const int texWidth = 8;
	const int texHeight = 8;
	GLubyte checkerTexture[texWidth * texHeight * 4];
	for (int y = 0; y < texHeight; y++) {
		for (int x = 0; x < texWidth; x++) {
			int i = (y * texWidth + x) * 4;
			// Patrón de ajedrez
			bool isBlack = ((x % 2) == 0) != ((y % 2) == 0);
			checkerTexture[i + 0] = isBlack ? 0 : 255;   // R
			checkerTexture[i + 1] = isBlack ? 0 : 255;   // G
			checkerTexture[i + 2] = isBlack ? 0 : 255;   // B
			checkerTexture[i + 3] = 255;                 // A
		}
	}

	// Generate and link the texture
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Configure texture parameters
	// GL_NEAREST is the checkered pattern
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// GL_REPEAT causes the texture to repeat if the UVs > 1
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Upload texture data to the GPU
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerTexture);

	// Disconnect the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	LOG("Default texture created successfully");

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
	// Before drawing, activate the texture 0
	glActiveTexture(GL_TEXTURE0);
	// Link the texture 
	glBindTexture(GL_TEXTURE_2D, textureID);
	// Inform shader to use the texture 0
	shader->SetInt("tex1", 0);

	glBindVertexArray(VAO);
	// Draw using the indices of IBO wich are linked to VAO
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0); // Unlink VAO
	glBindTexture(GL_TEXTURE_2D, 0);

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
	glDeleteBuffers(1, &IBO);
	glDeleteTextures(1, &textureID);
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

