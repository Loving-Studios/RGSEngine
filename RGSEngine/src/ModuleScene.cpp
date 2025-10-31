#include "ModuleScene.h"
#include "Application.h"
#include "Log.h"
#include "LoadFiles.h"

#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"

#include <glad/glad.h>
#include <vector>

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

    //Create the fbx from the start of the engine
    std::shared_ptr<GameObject> baker_house =
        Application::GetInstance().loadFiles->LoadFBX("../Assets/BakerHouse.fbx");

    if (baker_house) {
        AddGameObject(baker_house);
    }
    else {
        LOG("Failed to load BakerHouse.fbx on start. Creating Pyramid as fallback.");
        CreatePyramid();
    }

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
    LOG("Creating Test Pyramid");
    // Creation of the GameObject
    // Used the std::make_shared to create a smart pointer
    auto go = std::make_shared<GameObject>("TestPyramid");
    // Adding component Transform, always the first one
    go->AddComponent(std::make_shared<ComponentTransform>(go.get()));
    // Adding Mesh component
    auto mesh = std::make_shared<ComponentMesh>(go.get());

    // 16 vertex for UV/Normals for each face
    float positions[] = {
        // Base 4 vertex
        -0.5f, -0.5f,  0.5f, // 0
         0.5f, -0.5f,  0.5f, // 1
         0.5f, -0.5f, -0.5f, // 2
        -0.5f, -0.5f, -0.5f, // 3
        // Front face 3 vertex
        -0.5f, -0.5f,  0.5f, // 4
         0.5f, -0.5f,  0.5f, // 5
         0.0f,  0.5f,  0.0f, // 6 (apex)
         // Right face 3 vertex
          0.5f, -0.5f,  0.5f, // 7
          0.5f, -0.5f, -0.5f, // 8
          0.0f,  0.5f,  0.0f, // 9 (apex)
          // Back face 3 vertex
           0.5f, -0.5f, -0.5f, // 10
          -0.5f, -0.5f, -0.5f, // 11
           0.0f,  0.5f,  0.0f, // 12 (apex)
           // Left face 3 vertex
           -0.5f, -0.5f, -0.5f, // 13
           -0.5f, -0.5f,  0.5f, // 14
            0.0f,  0.5f,  0.0f  // 15 (apex)
    };
    unsigned int num_vertices = 16;

    float uvs[] = {
        // Base
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        // Front
        0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f,
        // Right
        0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f,
        // Back
        0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f,
        // Left
        0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,  0, 2, 3, // Base
        4, 5, 6,          // Front
        7, 8, 9,          // Right
        10, 11, 12,       // Back
        13, 14, 15        // Left
    };
    unsigned int num_indices = 18; // 6 faces * 3 index

    mesh->LoadMesh(positions, num_vertices, indices, num_indices, uvs, nullptr);
    go->AddComponent(mesh);

    auto texture = std::make_shared<ComponentTexture>(go.get());
    CreateDefaultCheckerTexture(texture);
    go->AddComponent(texture);

    rootObject->AddChild(go);
}

void ModuleScene::CreateTriangle()
{
    LOG("Creating Test Triangle");
    auto go = std::make_shared<GameObject>("Triangle");
    go->AddComponent(std::make_shared<ComponentTransform>(go.get()));
    auto mesh = std::make_shared<ComponentMesh>(go.get());

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
    float uvs[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.5f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2 };

    mesh->LoadMesh(positions, 3, indices, 3, uvs, nullptr);
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

    float positions[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };
    float uvs[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2,  0, 2, 3 };

    mesh->LoadMesh(positions, 4, indices, 6, uvs, nullptr);
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

    float positions[] = {
        -1.0f, -0.5f, 0.0f, // Width 2
         1.0f, -0.5f, 0.0f,
         1.0f,  0.5f, 0.0f, // Height 1
        -1.0f,  0.5f, 0.0f
    };
    float uvs[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2,  0, 2, 3 };

    mesh->LoadMesh(positions, 4, indices, 6, uvs, nullptr);
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

    float positions[] = {
        // Front Face
        -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,   0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
        // Back Face
        -0.5f, -0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,   0.5f,  0.5f, -0.5f,   0.5f, -0.5f, -0.5f,
        // Top Face
        -0.5f,  0.5f,  0.5f,   0.5f,  0.5f,  0.5f,   0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
        // Bottom Face
        -0.5f, -0.5f,  0.5f,  -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,   0.5f, -0.5f,  0.5f,
        // Right Face
         0.5f, -0.5f,  0.5f,   0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,   0.5f,  0.5f,  0.5f,
         // Left Face
         -0.5f, -0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,  -0.5f,  0.5f, -0.5f,  -0.5f, -0.5f, -0.5f
    };
    unsigned int num_vertices = 24;

    float uvs[] = {
        // Front
        0.0f, 0.0f,   1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 1.0f,
        // Back
        0.0f, 0.0f,   1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 1.0f,
        // Top
        0.0f, 0.0f,   1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 1.0f,
        // Bottom
        0.0f, 0.0f,   1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 1.0f,
        // Right
        0.0f, 0.0f,   1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 1.0f,
        // Left
        0.0f, 0.0f,   1.0f, 0.0f,   1.0f, 1.0f,   0.0f, 1.0f
    };

    unsigned int indices[] = {
         0,  1,  2,    0,  2,  3, // Front
         4,  5,  6,    4,  6,  7, // Back
         8,  9, 10,    8, 10, 11, // Top
        12, 13, 14,   12, 14, 15, // Bottom
        16, 17, 18,   16, 18, 19, // Right
        20, 21, 22,   20, 22, 23  // Left
    };
    unsigned int num_indices = 36;

    mesh->LoadMesh(positions, num_vertices, indices, num_indices, uvs, nullptr);
    go->AddComponent(mesh);

    auto texture = std::make_shared<ComponentTexture>(go.get());
    CreateDefaultCheckerTexture(texture);
    go->AddComponent(texture);
    rootObject->AddChild(go);
}

void ModuleScene::CreateSphere()
{
    LOG("Creating Test Sphere");
    auto go = std::make_shared<GameObject>("Sphere");
    go->AddComponent(std::make_shared<ComponentTransform>(go.get()));
    auto mesh = std::make_shared<ComponentMesh>(go.get());

    std::vector<float> positions;
    std::vector<float> uvs;
    std::vector<unsigned int> indices;

    const int segments = 24;
    const int rings = 24;
    const float PI = 3.1415926f;
    float radius = 0.5f;

    for (int r = 0; r <= rings; ++r) {
        float phi = PI / 2.0f - float(r) * PI / float(rings); // 90 to -90
        float y = radius * sinf(phi);
        float r_cos_phi = radius * cosf(phi);

        for (int s = 0; s <= segments; ++s) {
            float theta = float(s) * 2.0f * PI / float(segments); // 0 to 360
            float x = r_cos_phi * cosf(theta);
            float z = r_cos_phi * sinf(theta);

            float u = (float)s / (float)segments;
            float v = (float)r / (float)rings;

            positions.push_back(x); positions.push_back(y); positions.push_back(z);
            uvs.push_back(u); uvs.push_back(v);
        }
    }

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            int first = (r * (segments + 1)) + s;
            int second = first + (segments + 1);

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    mesh->LoadMesh(positions.data(), positions.size() / 3, indices.data(), indices.size(), uvs.data(), nullptr);
    go->AddComponent(mesh);

    auto texture = std::make_shared<ComponentTexture>(go.get());
    CreateDefaultCheckerTexture(texture);
    go->AddComponent(texture);
    rootObject->AddChild(go);
}