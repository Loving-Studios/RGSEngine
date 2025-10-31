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
#include <cfloat> 

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

            LOG("========================================");
            LOG("FBX LOADING SUMMARY:");
            LOG("Name: %s", newObject->name.c_str());
            LOG("Active: %s", newObject->active ? "YES" : "NO");
            LOG("Components:");

            if (newObject->GetComponent<ComponentTransform>())
            {
                auto t = newObject->GetComponent<ComponentTransform>();
                LOG("  - Transform: pos(%.2f,%.2f,%.2f) scale(%.2f,%.2f,%.2f)",
                    t->position.x, t->position.y, t->position.z,
                    t->scale.x, t->scale.y, t->scale.z);
            }

            if (newObject->GetComponent<ComponentMesh>())
            {
                auto m = newObject->GetComponent<ComponentMesh>();
                LOG("  - Mesh: VAO=%d, VBO=%d, IBO=%d, Indices=%d",
                    m->VAO, m->VBO, m->IBO, m->indexCount);
            }

            if (newObject->GetComponent<ComponentTexture>())
            {
                auto tex = newObject->GetComponent<ComponentTexture>();
                LOG("  - Texture: ID=%d, Path='%s'",
                    tex->textureID, tex->path.c_str());
            }
            else
            {
                LOG("  - Texture: NONE (will use default checkers)");
            }

            LOG("Children: %d", (int)newObject->GetChildren().size());
            LOG("========================================"); if (newObject->GetComponent<ComponentTransform>())
            {
                auto t = newObject->GetComponent<ComponentTransform>();
                LOG("  - Transform: pos(%.2f,%.2f,%.2f) scale(%.2f,%.2f,%.2f)",
                    t->position.x, t->position.y, t->position.z,
                    t->scale.x, t->scale.y, t->scale.z);
            }

            if (newObject->GetComponent<ComponentMesh>())
            {
                auto m = newObject->GetComponent<ComponentMesh>();
                LOG("  - Mesh: VAO=%d, VBO=%d, IBO=%d, Indices=%d",
                    m->VAO, m->VBO, m->IBO, m->indexCount);
            }

            if (newObject->GetComponent<ComponentTexture>())
            {
                auto tex = newObject->GetComponent<ComponentTexture>();
                LOG("  - Texture: ID=%d, Path='%s'",
                    tex->textureID, tex->path.c_str());
            }
            else
            {
                LOG("  - Texture: NONE (will use default checkers)");
            }

            LOG("Children: %d", (int)newObject->GetChildren().size());
            LOG("========================================");

            //Application::GetInstance().render->FocusOnGameObject(newObject.get());
        }
    }
    else if (extension == "dds" || extension == "png" || extension == "jpg" || extension == "jpeg")
    {
        LOG("Detected texture file (%s), loading...", extension.c_str());
         LoadTexture(file_path);
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

    if (rootObject != nullptr)
    {
        LOG("=== FBX LOADED SUCCESSFULLY ===");
        LOG("GameObject name: %s", rootObject->name.c_str());
        LOG("Has Transform: %s", rootObject->GetComponent<ComponentTransform>() ? "YES" : "NO");
        LOG("Has Mesh: %s", rootObject->GetComponent<ComponentMesh>() ? "YES" : "NO");
        LOG("Has Texture: %s", rootObject->GetComponent<ComponentTexture>() ? "YES" : "NO");
        LOG("Number of children: %d", (int)rootObject->GetChildren().size());

        // Verificar que el mesh tenga datos
        ComponentMesh* mesh = rootObject->GetComponent<ComponentMesh>();
        if (mesh)
        {
            LOG("Mesh VAO: %d, VBO: %d, IBO: %d, IndexCount: %d",
                mesh->VAO, mesh->VBO, mesh->IBO, mesh->indexCount);
        }
    }

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

        AutoScaleGameObject(meshObject, meshData);

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
        // AÑADIR: Verificar rango de UVs
        float minU = FLT_MAX, maxU = -FLT_MAX;
        float minV = FLT_MAX, maxV = -FLT_MAX;

        for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i)
        {
            float u = meshData.texCoords[i * 2];
            float v = meshData.texCoords[i * 2 + 1];
            minU = std::min(minU, u);
            maxU = std::max(maxU, u);
            minV = std::min(minV, v);
            maxV = std::max(maxV, v);
        }

        LOG("  - UV range: U[%.2f, %.2f] V[%.2f, %.2f]", minU, maxU, minV, maxV);
    }
    else{ LOG("  - NO texture coordinates"); }

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

    AutoScaleGameObject(gameObject, meshData);

    return gameObject;
}

void LoadFiles::LoadMaterialTextures(const aiScene* scene, aiMesh* mesh, std::shared_ptr<GameObject> gameObject, const std::string& fbxDirectory)
{
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        LOG("=== MATERIAL INFO ===");
        LOG("Material index: %d", mesh->mMaterialIndex);

        // Información del material
        aiString materialName;
        if (material->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS)
        {
            LOG("Material name: %s", materialName.C_Str());
        }

        // Contar texturas de cada tipo
        int diffuseCount = material->GetTextureCount(aiTextureType_DIFFUSE);
        int specularCount = material->GetTextureCount(aiTextureType_SPECULAR);
        int normalCount = material->GetTextureCount(aiTextureType_NORMALS);

        LOG("Texture counts - Diffuse: %d, Specular: %d, Normal: %d",
            diffuseCount, specularCount, normalCount);

        // Buscar textura difusa
        if (diffuseCount > 0)
        {
            aiString texturePath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
            {
                std::string textureFile(texturePath.C_Str());
                LOG("Texture path from FBX: '%s'", textureFile.c_str());

                // Limpiar path
                if (textureFile.find("./") == 0)
                    textureFile = textureFile.substr(2);

                std::replace(textureFile.begin(), textureFile.end(), '\\', '/');

                // Intentar múltiples rutas
                std::vector<std::string> possiblePaths;
                possiblePaths.push_back(fbxDirectory + textureFile); // Relativa al FBX
                possiblePaths.push_back(textureFile); // Absoluta

                // Extraer solo el nombre del archivo
                size_t lastSlash = textureFile.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                {
                    std::string fileName = textureFile.substr(lastSlash + 1);
                    possiblePaths.push_back(fbxDirectory + fileName);
                }

                unsigned int textureID = 0;
                std::string loadedPath;

                LOG("Trying to load texture from possible paths:");
                for (const auto& path : possiblePaths)
                {
                    LOG("  - Trying: %s", path.c_str());
                    textureID = LoadTextureFromFile(path.c_str());
                    if (textureID != 0)
                    {
                        loadedPath = path;
                        LOG("SUCCESS!");
                        break;
                    }
                    else
                    {
                        LOG("Failed");
                    }
                }

                if (textureID != 0)
                {
                    auto texComponent = std::make_shared<ComponentTexture>(gameObject.get());
                    texComponent->textureID = textureID;
                    texComponent->path = loadedPath;
                    gameObject->AddComponent(texComponent);

                    LOG("TEXTURE LOADED AND APPLIED: %s (OpenGL ID: %d)",
                        loadedPath.c_str(), textureID);
                }
                else{ LOG("FAILED TO LOAD TEXTURE - Will use default checkers"); }
            }
            else{ LOG("Failed to get texture path from material"); }
        }
        else{ LOG("Material has NO diffuse texture"); }
    }
    else{ LOG("Mesh has no material assigned"); }
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
    LOG("=== DRAG & DROP TEXTURE ===");
    LOG("Loading texture: %s", file_path);

    unsigned int textureID = LoadTextureFromFile(file_path);

    if (textureID == 0)
    {
        LOG("Failed to load texture");
        return false;
    }

    LOG("Texture loaded successfully (ID: %d)", textureID);

    // TODO: Aplicar a GameObjects seleccionados
    // Por ahora, aplicar a TODOS los GameObjects de la escena como prueba

    std::shared_ptr<GameObject> root = Application::GetInstance().scene->rootObject;
    if (root)
    {
        ApplyTextureToAllChildren(root, textureID, file_path);
    }

    return true;
}

void LoadFiles::ApplyTextureToAllChildren(std::shared_ptr<GameObject> go, unsigned int textureID, const char* path)
{
    if (go == nullptr)
        return;

    // Si el GameObject tiene mesh, aplicar textura
    if (go->GetComponent<ComponentMesh>())
    {
        // Eliminar textura anterior si existe
        auto oldTex = go->GetComponent<ComponentTexture>();
        if (oldTex)
        {
            // No podemos eliminar componentes fácilmente, así que reemplazamos
            oldTex->textureID = textureID;
            oldTex->path = path;
            LOG("Texture updated on: %s", go->name.c_str());
        }
        else
        {
            // Crear nueva textura
            auto newTex = std::make_shared<ComponentTexture>(go.get());
            newTex->textureID = textureID;
            newTex->path = path;
            go->AddComponent(newTex);
            LOG("Texture applied to: %s", go->name.c_str());
        }
    }

    // Recursivo a hijos
    for (const auto& child : go->GetChildren())
    {
        ApplyTextureToAllChildren(child, textureID, path);
    }
}

void LoadFiles::AutoScaleGameObject(std::shared_ptr<GameObject> gameObject, const MeshData& meshData)
{
    if (meshData.num_vertices == 0 || meshData.vertices == nullptr)
        return;

    // Calcular bounding box
    float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

    for (unsigned int i = 0; i < meshData.num_vertices; ++i)
    {
        float x = meshData.vertices[i * 3 + 0];
        float y = meshData.vertices[i * 3 + 1];
        float z = meshData.vertices[i * 3 + 2];

        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
        minZ = std::min(minZ, z);
        maxZ = std::max(maxZ, z);
    }

    // Calcular tamaño
    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    float maxSize = std::max({ sizeX, sizeY, sizeZ });

    LOG("Object size: %.2f x %.2f x %.2f (max: %.2f)", sizeX, sizeY, sizeZ, maxSize);

    // Si el objeto es muy grande, escalarlo
    float targetSize = 2.0f; // Tamaño objetivo (2 unidades)

    if (maxSize > targetSize)
    {
        float scale = targetSize / maxSize;

        ComponentTransform* transform = gameObject->GetComponent<ComponentTransform>();
        if (transform)
        {
            transform->scale = glm::vec3(scale, scale, scale);
            LOG("Auto-scaled object by factor: %.4f (from %.2f to %.2f)", scale, maxSize, targetSize);
        }
    }
    else
    {
        LOG("Object size is fine, no scaling needed");
    }
}