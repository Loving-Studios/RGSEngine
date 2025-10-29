#include "LoadFiles.h"
#include "Application.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "ComponentTexture.h"
#include "ModuleScene.h"
#include "Log.h"

#include <IL/il.h>
#include <IL/ilu.h>
#include <glad/glad.h>

#include <string>
#include <algorithm>

LoadFiles::LoadFiles()
{
    name = "loadFiles";
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_DEBUGGER, nullptr);
    aiAttachLogStream(&stream);
}

LoadFiles::~LoadFiles()
{
    aiDetachAllLogStreams();
}

bool LoadFiles::Awake()
{
    LOG("Loading LoadFiles module");

    // Inicializar DevIL
    ilInit();
    iluInit();

    // Verificar errores
    ILenum error = ilGetError();
    if (error != IL_NO_ERROR)
    {
        LOG("Error initializing DevIL: 0x%x", error);
        // No retornar false, solo advertir
    }
    else
    {
        LOG("DevIL initialized successfully");
    }

    return true;
}

bool LoadFiles::Start()
{
    LOG("Starting LoadFiles module");

    return true;
}

bool LoadFiles::PreUpdate()
{
    return true;
}

bool LoadFiles::Update(float dt)
{
    return true;
}

bool LoadFiles::PostUpdate()
{
    return true;
}

bool LoadFiles::CleanUp()
{
    LOG("Cleaning up LoadFiles module");
    return true;
}

void LoadFiles::HandleDropFile(const char* file_path)
{
    if (file_path == nullptr)
        return;

    std::string path(file_path);
    size_t dotPos = path.find_last_of(".");

    if (dotPos == std::string::npos)
    {
        LOG("File has no extension: %s", file_path);
        return;
    }

    std::string extension = path.substr(dotPos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == "fbx")
    {
        LOG("Detected FBX file, loading...");
        std::shared_ptr<GameObject> newObject = LoadFBX(file_path);
        if (newObject != nullptr)
        {
            LOG("FBX loaded successfully and added to scene");
            Application::GetInstance().scene->AddGameObject(newObject);
        }
    }
    else if (extension == "dds" || extension == "png" || extension == "jpg" || extension == "jpeg")
    {
        LOG("Detected texture file (%s), loading...", extension.c_str());
       /* LoadTexture(file_path);*/
    }
    else
    {
        LOG("Unsupported file format: %s", extension.c_str());
    }
}

std::shared_ptr<GameObject> LoadFiles::LoadFBX(const char* file_path)
{
    const aiScene* scene = aiImportFile(file_path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace);

    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        const char* error = aiGetErrorString();
        LOG("Error loading FBX %s: %s", file_path, error);
        return nullptr;
    }

    LOG("Successfully loaded FBX: %s", file_path);
    LOG("Number of meshes: %d", scene->mNumMeshes);
    LOG("Number of materials: %d", scene->mNumMaterials);

    // Obtener directorio del FBX para texturas relativas
    std::string fbxPath(file_path);
    size_t lastSlash = fbxPath.find_last_of("/\\");
    std::string fbxDirectory = (lastSlash != std::string::npos) ? fbxPath.substr(0, lastSlash + 1) : "";

    // Obtener nombre del archivo
    std::string fileName = (lastSlash != std::string::npos) ? fbxPath.substr(lastSlash + 1) : fbxPath;
    size_t lastDot = fileName.find_last_of(".");
    if (lastDot != std::string::npos)
        fileName = fileName.substr(0, lastDot);

    std::shared_ptr<GameObject> rootObject = nullptr;

    if (scene->mNumMeshes == 1)
    {
        MeshData meshData;
        ProcessMesh(scene->mMeshes[0], meshData);
        rootObject = CreateGameObjectFromMesh(meshData, fileName.c_str());

        // Cargar texturas del material
        LoadMaterialTextures(scene, scene->mMeshes[0], rootObject, fbxDirectory);

        // Liberar datos temporales
        delete[] meshData.vertices;
        delete[] meshData.indices;
        if (meshData.texCoords) delete[] meshData.texCoords;
        if (meshData.normals) delete[] meshData.normals;
        if (meshData.colors) delete[] meshData.colors;
    }
    else
    {
        rootObject = ProcessNode(scene->mRootNode, scene, nullptr, fbxDirectory);
        if (rootObject)
            rootObject->name = fileName;
    }

    aiReleaseImport(scene);

    return rootObject;
}

std::shared_ptr<GameObject> LoadFiles::ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<GameObject> parent, const std::string& fbxDirectory)
{
    std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>(node->mName.C_Str());

    auto transform = std::make_shared<ComponentTransform>(gameObject.get());
    gameObject->AddComponent(transform);

    if (parent)
    {
        parent->AddChild(gameObject);
    }

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        MeshData meshData;
        ProcessMesh(mesh, meshData);

        std::shared_ptr<GameObject> meshObject = gameObject;

        if (node->mNumMeshes > 1 && i > 0)
        {
            std::string meshName = std::string(node->mName.C_Str()) + "_Mesh" + std::to_string(i);
            meshObject = std::make_shared<GameObject>(meshName);

            auto meshTransform = std::make_shared<ComponentTransform>(meshObject.get());
            meshObject->AddComponent(meshTransform);

            gameObject->AddChild(meshObject);
        }

        auto compMesh = std::make_shared<ComponentMesh>(meshObject.get());
        compMesh->LoadMesh(meshData.vertices, meshData.num_vertices,
            meshData.indices, meshData.num_indices,
            meshData.texCoords, meshData.normals);
        meshObject->AddComponent(compMesh);

        // Cargar texturas del material
        LoadMaterialTextures(scene, mesh, meshObject, fbxDirectory);

        delete[] meshData.vertices;
        delete[] meshData.indices;
        if (meshData.texCoords) delete[] meshData.texCoords;
        if (meshData.normals) delete[] meshData.normals;
        if (meshData.colors) delete[] meshData.colors;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, gameObject, fbxDirectory);
    }

    return gameObject;
}

void LoadFiles::ProcessMesh(aiMesh* aiMesh, MeshData& meshData)
{
    // Copiar vértices
    meshData.num_vertices = aiMesh->mNumVertices;
    meshData.vertices = new float[meshData.num_vertices * 3];
    memcpy(meshData.vertices, aiMesh->mVertices, sizeof(float) * meshData.num_vertices * 3);
    LOG("Processing mesh '%s' with %d vertices", aiMesh->mName.C_Str(), meshData.num_vertices);

    // Copiar normales
    if (aiMesh->HasNormals())
    {
        meshData.hasNormals = true;
        meshData.normals = new float[meshData.num_vertices * 3];
        memcpy(meshData.normals, aiMesh->mNormals, sizeof(float) * meshData.num_vertices * 3);
        LOG("  - Has normals");
    }

    // Copiar coordenadas de textura
    if (aiMesh->HasTextureCoords(0))
    {
        meshData.hasTexCoords = true;
        meshData.texCoords = new float[meshData.num_vertices * 2];

        for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i)
        {
            meshData.texCoords[i * 2] = aiMesh->mTextureCoords[0][i].x;
            meshData.texCoords[i * 2 + 1] = aiMesh->mTextureCoords[0][i].y;
        }
        LOG("  - Has texture coordinates");
    }

    // Copiar colores de vértice
    if (aiMesh->HasVertexColors(0))
    {
        meshData.hasColors = true;
        meshData.colors = new float[meshData.num_vertices * 4]; // RGBA

        for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i)
        {
            meshData.colors[i * 4 + 0] = aiMesh->mColors[0][i].r;
            meshData.colors[i * 4 + 1] = aiMesh->mColors[0][i].g;
            meshData.colors[i * 4 + 2] = aiMesh->mColors[0][i].b;
            meshData.colors[i * 4 + 3] = aiMesh->mColors[0][i].a;
        }
        LOG("  - Has vertex colors");
    }

    // Copiar índices
    if (aiMesh->HasFaces())
    {
        meshData.num_indices = aiMesh->mNumFaces * 3;
        meshData.indices = new unsigned int[meshData.num_indices];

        for (unsigned int i = 0; i < aiMesh->mNumFaces; ++i)
        {
            if (aiMesh->mFaces[i].mNumIndices != 3)
            {
                LOG("  - WARNING: Face with != 3 indices!");
            }
            else
            {
                memcpy(&meshData.indices[i * 3], aiMesh->mFaces[i].mIndices, 3 * sizeof(unsigned int));
            }
        }

        LOG("  - %d indices (%d triangles)", meshData.num_indices, aiMesh->mNumFaces);
    }
}

std::shared_ptr<GameObject> LoadFiles::CreateGameObjectFromMesh(const MeshData& meshData, const char* name)
{
    auto gameObject = std::make_shared<GameObject>(name);

    auto transform = std::make_shared<ComponentTransform>(gameObject.get());
    gameObject->AddComponent(transform);

    auto compMesh = std::make_shared<ComponentMesh>(gameObject.get());
    compMesh->LoadMesh(meshData.vertices, meshData.num_vertices,
        meshData.indices, meshData.num_indices,
        meshData.texCoords, meshData.normals);
    gameObject->AddComponent(compMesh);

    return gameObject;
}

void LoadFiles::LoadMaterialTextures(const aiScene* scene, aiMesh* mesh, std::shared_ptr<GameObject> gameObject, const std::string& fbxDirectory)
{
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Buscar textura difusa
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString texturePath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
            {
                std::string textureFile(texturePath.C_Str());

                // Construir ruta completa
                std::string fullPath = fbxDirectory + textureFile;

                LOG("Loading texture: %s", fullPath.c_str());

                unsigned int textureID = LoadTextureFromFile(fullPath.c_str());

                if (textureID != 0)
                {
                    auto texComponent = std::make_shared<ComponentTexture>(gameObject.get());
                    texComponent->textureID = textureID;
                    texComponent->path = fullPath;
                    gameObject->AddComponent(texComponent);

                    LOG("Texture loaded successfully: %s (ID: %d)", fullPath.c_str(), textureID);
                }
                else
                {
                    LOG("Failed to load texture: %s", fullPath.c_str());
                }
            }
        }
    }
}

unsigned int LoadFiles::LoadTextureFromFile(const char* file_path)
{
    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    // Cargar imagen
    if (!ilLoadImage(file_path))
    {
        ILenum error = ilGetError();
        LOG("DevIL Error loading image: %d", error);
        ilDeleteImages(1, &imageID);
        return 0;
    }

    // Convertir a formato común (RGBA)
    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE))
    {
        LOG("DevIL Error converting image");
        ilDeleteImages(1, &imageID);
        return 0;
    }

    // Obtener datos de la imagen
    ILubyte* data = ilGetData();
    ILint width = ilGetInteger(IL_IMAGE_WIDTH);
    ILint height = ilGetInteger(IL_IMAGE_HEIGHT);

    // Crear textura OpenGL
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Limpiar DevIL
    ilDeleteImages(1, &imageID);

    LOG("Texture created: %dx%d, OpenGL ID: %d", width, height, textureID);

    return textureID;
}

bool LoadFiles::LoadTexture(const char* file_path)
{
    LOG("Loading texture for selected GameObjects: %s", file_path);

    unsigned int textureID = LoadTextureFromFile(file_path);

    if (textureID == 0)
    {
        LOG("Failed to load texture");
        return false;
    }

    // TODO: Aplicar a GameObjects seleccionados
    // Necesitas implementar un sistema de selección en tu editor

    LOG("Texture loaded but no selection system implemented yet");
    return true;
}