#include "ModuleScene.h"
#include "Application.h"
#include "Log.h"

#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"

#include <glad/glad.h>

ModuleScene::ModuleScene() : Module()
{
    name = "scene";
}

ModuleScene::~ModuleScene()
{
}

bool ModuleScene::Start()
{
    LOG("ModuleScene Start");

    // Creation of the GameObject root of the scene, the SceneRoot
    rootObject = std::make_shared<GameObject>("SceneRoot");

    return true;
}

bool ModuleScene::Update(float dt)
{
    // Update the rootObject wich will be updating all his childrens and components
    if (rootObject)
    {
        rootObject->Update();
    }

    return true;
}

bool ModuleScene::CleanUp()
{
    LOG("ModuleScene CleanUp");

    // As it is a shared_ptr, the 'rootObject' will be cleaning auto at the exit, calling the destroyers of all the GameObjects and Components
    rootObject.reset();

    return true;
}

void ModuleScene::AddGameObject(std::shared_ptr<GameObject> gameObject)
{
    if (gameObject != nullptr && rootObject != nullptr)
    {
        rootObject->AddChild(gameObject);
        LOG("GameObject '%s' added to scene", gameObject->GetName().c_str());
    }
}

void ModuleScene::CreatePyramid()
{
    LOG("Creating Test Pyramid GameObject");

    // Creation of the GameObject
    // Used the std::make_shared to create a smart pointer
    auto pyramidGO = std::make_shared<GameObject>("TestPyramid");

    // Adding component Transform, always the first one
    auto transform = std::make_shared<ComponentTransform>(pyramidGO.get());
    pyramidGO->AddComponent(transform);

    // Adding Mesh component
    auto mesh = std::make_shared<ComponentMesh>(pyramidGO.get());

    // --- Logic of VBO/IBO ---
    float vertices[] = {
        // Base vertex (y = -0.5) with UV
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  // front-left
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  // front-right
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  // back-right
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  // back-left
        // Apex (top point) with UV centered
         0.0f,  0.5f,  0.0f,  0.5f, 0.5f   // top
    };

    unsigned int indices[] = {
        0, 1, 2,  0, 2, 3, // Base
        0, 1, 4,          // Front
        1, 2, 4,          // Right
        2, 3, 4,          // Back
        3, 0, 4           // Left
    };
    mesh->indexCount = 18; // 6 faces * 3 index

    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->IBO);

    glBindVertexArray(mesh->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // --- End logic VBO/IBO ---

    pyramidGO->AddComponent(mesh);

    // Adding Texture component
    auto texture = std::make_shared<ComponentTexture>(pyramidGO.get());

    // --- Texture Logic ---
    const int texWidth = 8, texHeight = 8;
    GLubyte checkerTexture[texWidth * texHeight * 4];
    for (int y = 0; y < texHeight; y++) {
        for (int x = 0; x < texWidth; x++) {
            int i = (y * texWidth + x) * 4;
            bool isBlack = ((x % 2) == 0) != ((y % 2) == 0);
            checkerTexture[i + 0] = isBlack ? 0 : 255;
            checkerTexture[i + 1] = isBlack ? 0 : 255;
            checkerTexture[i + 2] = isBlack ? 0 : 255;
            checkerTexture[i + 3] = 255;
        }
    }

    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_2D, texture->textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerTexture);
    glBindTexture(GL_TEXTURE_2D, 0); // Unlink texture

    texture->width = texWidth;
    texture->height = texHeight;
    texture->path = "default_checker";
    // --- End Texture Logic ---

    pyramidGO->AddComponent(texture);

    // Adding the new GameObject as son of the rootObject
    rootObject->AddChild(pyramidGO);
}

static void CreateDefaultCheckerTexture(std::shared_ptr<ComponentTexture> texture)
{
    const int texWidth = 8, texHeight = 8;
    GLubyte checkerTexture[texWidth * texHeight * 4];
    for (int y = 0; y < texHeight; y++) {
        for (int x = 0; x < texWidth; x++) {
            int i = (y * texWidth + x) * 4;
            bool isBlack = ((x % 2) == 0) != ((y % 2) == 0);
            checkerTexture[i + 0] = isBlack ? 0 : 255;
            checkerTexture[i + 1] = isBlack ? 0 : 255;
            checkerTexture[i + 2] = isBlack ? 0 : 255;
            checkerTexture[i + 3] = 255;
        }
    }
    glGenTextures(1, &texture->textureID);
    glBindTexture(GL_TEXTURE_2D, texture->textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerTexture);
    glBindTexture(GL_TEXTURE_2D, 0);
    texture->width = texWidth;
    texture->height = texHeight;
    texture->path = "default_checker";
}

void ModuleScene::CreateTriangle()
{
    LOG("Creating Test Triangle");
    auto go = std::make_shared<GameObject>("Triangle");
    go->AddComponent(std::make_shared<ComponentTransform>(go.get()));
    auto mesh = std::make_shared<ComponentMesh>(go.get());

    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
         0.0f,  0.5f, 0.0f, 0.5f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2 };
    mesh->indexCount = 3;

    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->IBO);
    glBindVertexArray(mesh->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    go->AddComponent(mesh);
    auto texture = std::make_shared<ComponentTexture>(go.get());
    CreateDefaultCheckerTexture(texture);
    go->AddComponent(texture);
    rootObject->AddChild(go);
}

void ModuleScene::CreateSquare()
{
    LOG("Creating Test Square");
    auto go = std::make_shared<GameObject>("Square");
    go->AddComponent(std::make_shared<ComponentTransform>(go.get()));
    auto mesh = std::make_shared<ComponentMesh>(go.get());

    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2,  0, 2, 3 };
    mesh->indexCount = 6;

    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->IBO);
    glBindVertexArray(mesh->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    go->AddComponent(mesh);
    auto texture = std::make_shared<ComponentTexture>(go.get());
    CreateDefaultCheckerTexture(texture);
    go->AddComponent(texture);
    rootObject->AddChild(go);
}

void ModuleScene::CreateRectangle()
{
    LOG("Creating Test Rectangle");
    auto go = std::make_shared<GameObject>("Rectangle");
    go->AddComponent(std::make_shared<ComponentTransform>(go.get()));
    auto mesh = std::make_shared<ComponentMesh>(go.get());

    float vertices[] = {
        -1.0f, -0.5f, 0.0f, 0.0f, 0.0f, // Width 2
         1.0f, -0.5f, 0.0f, 1.0f, 0.0f,
         1.0f,  0.5f, 0.0f, 1.0f, 1.0f, // Height 1
        -1.0f,  0.5f, 0.0f, 0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2,  0, 2, 3 };
    mesh->indexCount = 6;

    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->IBO);
    glBindVertexArray(mesh->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    go->AddComponent(mesh);
    auto texture = std::make_shared<ComponentTexture>(go.get());
    CreateDefaultCheckerTexture(texture);
    go->AddComponent(texture);
    rootObject->AddChild(go);
}

void ModuleScene::CreateCube()
{
    LOG("Creating Test Cube");
    auto go = std::make_shared<GameObject>("Cube");
    go->AddComponent(std::make_shared<ComponentTransform>(go.get()));
    auto mesh = std::make_shared<ComponentMesh>(go.get());

    // 8 vertex, 5 floats (X,Y,Z,U,V)
    float vertices[] = {
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, // 0
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f, // 1
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f, // 2
        -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, // 3
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // 4
         0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // 5
         0.5f,  0.5f, -0.5f, 0.0f, 1.0f, // 6
        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f  // 7
    };
    unsigned int indices[] = {
        0, 1, 2,  0, 2, 3, // Front
        5, 4, 7,  5, 7, 6, // Back
        3, 2, 6,  3, 6, 7, // Up
        4, 5, 1,  4, 1, 0, // Down
        1, 5, 6,  1, 6, 2, // Right
        4, 0, 3,  4, 3, 7  // Left
    };
    mesh->indexCount = 36;

    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->IBO);
    glBindVertexArray(mesh->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    go->AddComponent(mesh);
    auto texture = std::make_shared<ComponentTexture>(go.get());
    CreateDefaultCheckerTexture(texture);
    go->AddComponent(texture);
    rootObject->AddChild(go);
}

void ModuleScene::CreateSphere()
{

}