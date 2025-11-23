#include "LoadFiles.h"
#include "Application.h"
#include "ModuleEditor.h"
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
#include <glm/gtc/matrix_transform.hpp>
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
    ilShutDown();
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

        GameObject* selected = Application::GetInstance().editor->GetSelectedGameObject();

        if (selected != nullptr)
        {
            LoadTexture(file_path, selected);
        }
        else
        {
            LOG("WARNING: No GameObject selected. Please select an object in the Hierarchy to apply the texture.");
        }
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

    // Get FBX directory for relative textures
    std::string fbxPath(file_path);
    size_t lastSlash = fbxPath.find_last_of("/\\");
    std::string fbxDirectory = (lastSlash != std::string::npos) ? fbxPath.substr(0, lastSlash + 1) : "";

    // Get file name
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

        
        LoadMaterialTextures(scene, scene->mMeshes[0], rootObject, fbxDirectory);

        // Release temporary data
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
        NormalizeModelScale(rootObject, 5.0f);

        LOG("=== FBX LOADED SUCCESSFULLY ===");
        LOG("GameObject name: %s", rootObject->name.c_str());
        LOG("Has Transform: %s", rootObject->GetComponent<ComponentTransform>() ? "YES" : "NO");
        LOG("Has Mesh: %s", rootObject->GetComponent<ComponentMesh>() ? "YES" : "NO");
        LOG("Has Texture: %s", rootObject->GetComponent<ComponentTexture>() ? "YES" : "NO");
        LOG("Number of children: %d", (int)rootObject->GetChildren().size());

        // Verify that the mesh has data
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

    aiMatrix4x4 transformMatrix = node->mTransformation;

    // Assimp: Row-major. GLM/OpenGL: Column-major
    // We need to transpose the matrix when passing it to GLM
    glm::mat4 glmTransform;
    glmTransform[0][0] = transformMatrix.a1; glmTransform[0][1] = transformMatrix.b1; glmTransform[0][2] = transformMatrix.c1; glmTransform[0][3] = transformMatrix.d1;
    glmTransform[1][0] = transformMatrix.a2; glmTransform[1][1] = transformMatrix.b2; glmTransform[1][2] = transformMatrix.c2; glmTransform[1][3] = transformMatrix.d2;
    glmTransform[2][0] = transformMatrix.a3; glmTransform[2][1] = transformMatrix.b3; glmTransform[2][2] = transformMatrix.c3; glmTransform[2][3] = transformMatrix.d3;
    glmTransform[3][0] = transformMatrix.a4; glmTransform[3][1] = transformMatrix.b4; glmTransform[3][2] = transformMatrix.c4; glmTransform[3][3] = transformMatrix.d4;

    glm::vec3 position, scale, skew;
    glm::quat rotation;
    glm::vec4 perspective;

    // Break down the local matrix of the node
    if (glm::decompose(glmTransform, scale, rotation, position, skew, perspective))
    {
        transform->SetPosition(position);
        transform->SetRotation(rotation);
        transform->SetScale(scale);
    }
    else
    {
        LOG("Failed to decompose transformation matrix for node: %s", node->mName.C_Str());
        transform->SetPosition(glm::vec3(0, 0, 0));
        transform->SetRotation(glm::quat(1, 0, 0, 0));
        transform->SetScale(glm::vec3(1, 1, 1));
    }

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

        std::shared_ptr<GameObject> meshObject;

        if (node->mNumMeshes == 1)
        {
            // If 1 node = 1 mesh, use the same object
            meshObject = gameObject;
        }
        else
        {
            // Else 1 node = N meshes, create childrens for extra meshes
            std::string meshName = std::string(node->mName.C_Str()) + "_SubMesh_" + std::to_string(i);
            meshObject = std::make_shared<GameObject>(meshName);
            auto meshTransform = std::make_shared<ComponentTransform>(meshObject.get());
            // The childrens of the multiple meshes usually are on 0, 0, 0 of the local origin
            meshObject->AddComponent(meshTransform);
            gameObject->AddChild(meshObject);
        }

        auto compMesh = std::make_shared<ComponentMesh>(meshObject.get());
        compMesh->LoadMesh(meshData.vertices, meshData.num_vertices,
            meshData.indices, meshData.num_indices,
            meshData.texCoords, meshData.normals);
        meshObject->AddComponent(compMesh);

        LoadMaterialTextures(scene, mesh, meshObject, fbxDirectory);

        // CleanUP
        delete[] meshData.vertices;
        delete[] meshData.indices;
        if (meshData.texCoords) delete[] meshData.texCoords;
        if (meshData.normals) delete[] meshData.normals;
        if (meshData.colors) delete[] meshData.colors;
    }

    // Process all the childrens
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, gameObject, fbxDirectory);
    }

    return gameObject;
}

void LoadFiles::ProcessMesh(aiMesh* aiMesh, MeshData& meshData)
{
    // Copy vertices
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

    // Copy texture coordinates
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

    // Copy vertex colors
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

    // Copy indexes
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

        LOG("=== MATERIAL INFO ===");
        LOG("Material index: %d", mesh->mMaterialIndex);

        // Material information
        aiString materialName;
        if (material->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS)
        {
            LOG("Material name: %s", materialName.C_Str());
        }

        // Count textures of each type
        int diffuseCount = material->GetTextureCount(aiTextureType_DIFFUSE);
        int specularCount = material->GetTextureCount(aiTextureType_SPECULAR);
        int normalCount = material->GetTextureCount(aiTextureType_NORMALS);

        LOG("Texture counts - Diffuse: %d, Specular: %d, Normal: %d",
            diffuseCount, specularCount, normalCount);

        // Search for diffuse texture
        if (diffuseCount > 0)
        {
            aiString texturePath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
            {
                std::string textureFile(texturePath.C_Str());
                LOG("Texture path from FBX: '%s'", textureFile.c_str());

                // Clean path
                if (textureFile.find("./") == 0)
                    textureFile = textureFile.substr(2);

                std::replace(textureFile.begin(), textureFile.end(), '\\', '/');

                // Try multiple routes
                std::vector<std::string> possiblePaths;
                possiblePaths.push_back(fbxDirectory + textureFile); // Relative
                possiblePaths.push_back(textureFile); // Absolute

                // Extract the file name
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

    // Load image
    if (!ilLoadImage(file_path))
    {
        ILenum error = ilGetError();
        LOG("DevIL Error loading image: %d", error);
        ilDeleteImages(1, &imageID);
        return 0;
    }

    // Convert to RGBA format
    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE))
    {
        LOG("DevIL Error converting image");
        ilDeleteImages(1, &imageID);
        return 0;
    }

    // Get image data
    ILubyte* data = ilGetData();
    ILint width = ilGetInteger(IL_IMAGE_WIDTH);
    ILint height = ilGetInteger(IL_IMAGE_HEIGHT);

    // Create texture 
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

    // Clean DevIL
    ilDeleteImages(1, &imageID);

    LOG("Texture created: %dx%d, OpenGL ID: %d", width, height, textureID);

    return textureID;
}

bool LoadFiles::LoadTexture(const char* file_path, GameObject* target)
{
    LOG("=== DRAG & DROP TEXTURE ===");
    LOG("Loading texture: %s", file_path);

    if (target == nullptr)
    {
        LOG("Error: No target GameObject provided for texture loading.");
        return false;
    }

    unsigned int textureID = LoadTextureFromFile(file_path);

    if (textureID == 0)
    {
        LOG("Failed to load texture");
        return false;
    }

    LOG("Texture loaded successfully (ID: %d)", textureID);

    // Check if the Component has a Mesh to apply the texture
    if (target->GetComponent<ComponentMesh>() != nullptr)
    {
        auto textureComp = target->GetComponent<ComponentTexture>();

        if (textureComp != nullptr)
        {
            // If has a component, update the data
            textureComp->textureID = textureID;
            textureComp->path = file_path;
            // Remove the default flag to see the new one
            textureComp->useDefaultTexture = false;

            LOG("Texture component UPDATED on GameObject: %s", target->GetName().c_str());
        }
        else
        {
            // If doesn't have a component, assign a new one
            auto newTex = std::make_shared<ComponentTexture>(target);
            newTex->textureID = textureID;
            newTex->path = file_path;
            target->AddComponent(newTex);

            LOG("Texture component ADDED to GameObject: %s", target->GetName().c_str());
        }
    }
    else
    {
        LOG("WARNING: Selected object '%s' has no Mesh. Texture loaded but not applied.", target->GetName().c_str());
    }

    return true;
}

void LoadFiles::ApplyTextureToAllChildren(std::shared_ptr<GameObject> go, unsigned int textureID, const char* path)
{
    if (go == nullptr)
        return;

    // If the GameObject has a mesh, apply texture
    if (go->GetComponent<ComponentMesh>())
    {
        // Remove previous texture if it exists
        auto oldTex = go->GetComponent<ComponentTexture>();
        if (oldTex)
        {
            oldTex->textureID = textureID;
            oldTex->path = path;
            LOG("Texture updated on: %s", go->name.c_str());
        }
        else
        {
            // Create new texture
            auto newTex = std::make_shared<ComponentTexture>(go.get());
            newTex->textureID = textureID;
            newTex->path = path;
            go->AddComponent(newTex);
            LOG("Texture applied to: %s", go->name.c_str());
        }
    }

    // Recursive to children
    for (const auto& child : go->GetChildren())
    {
        ApplyTextureToAllChildren(child, textureID, path);
    }
}

void LoadFiles::NormalizeModelScale(std::shared_ptr<GameObject> rootObject, float targetSize)
{
    if (rootObject == nullptr) return;

    // Initialize the limits to the maximum/minimum possible
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    // Initial identity matrix for the root
    glm::mat4 identity(1.0f);

    CalculateBoundingBox(rootObject, minBounds, maxBounds, identity);

    // If no geometry was found, return
    if (minBounds.x == std::numeric_limits<float>::max()) return;

    // Calculate dimensions
    glm::vec3 size = maxBounds - minBounds;
    float maxDimension = std::max({ size.x, size.y, size.z });

    LOG("Model Dimensions: %.2f x %.2f x %.2f (Max: %.2f)", size.x, size.y, size.z, maxDimension);

    // Apply scale if necessary
    if (maxDimension > 0.0f)
    {
        // Calculate the factor so that the largest dimension is exactly targetSize
        float scale = targetSize / maxDimension;

        ComponentTransform* t = rootObject->GetComponent<ComponentTransform>();
        if (t != nullptr)
        {
            // We multiply the current scale by the new factor, to respect previous scales
            glm::vec3 currentScale = t->scale;
            t->SetScale(currentScale * scale);
        }

        LOG("Model normalized: %.2f -> scale=%.4f", maxDimension, scale);
    }
}

void LoadFiles::CalculateBoundingBox(std::shared_ptr<GameObject> obj,
    glm::vec3& minBounds, glm::vec3& maxBounds,
    const glm::mat4& parentTransform)
{
    if (obj == nullptr) return;

    // Calculate Local Transformation
    glm::mat4 localTransform(1.0f);
    ComponentTransform* t = obj->GetComponent<ComponentTransform>();

    if (t != nullptr)
    {
        localTransform = t->GetModelMatrix();
    }

    // Calculate Cumulative Global Transformation
    glm::mat4 worldTransform = parentTransform * localTransform;

    // Process the mesh
    ComponentMesh* meshComp = obj->GetComponent<ComponentMesh>();
    if (meshComp != nullptr && meshComp->VAO != 0 && meshComp->VBO != 0)
    {
        // Read the vertex of the GPU
        glBindBuffer(GL_ARRAY_BUFFER, meshComp->VBO);

        GLint bufferSize;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
        int numVertices = bufferSize / (3 * sizeof(float));

        if (numVertices > 0)
        {
            // Temporary buffer
            std::vector<float> vertices(numVertices * 3);
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, vertices.data());

            for (int i = 0; i < numVertices; ++i)
            {
                // Get local position
                glm::vec4 localVertex(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2], 1.0f);

                // Transforming the world with the accumulated matrix
                glm::vec4 worldVertex = worldTransform * localVertex;

                // Expand limits (Min/Max)
                minBounds.x = std::min(minBounds.x, worldVertex.x);
                minBounds.y = std::min(minBounds.y, worldVertex.y);
                minBounds.z = std::min(minBounds.z, worldVertex.z);

                maxBounds.x = std::max(maxBounds.x, worldVertex.x);
                maxBounds.y = std::max(maxBounds.y, worldVertex.y);
                maxBounds.z = std::max(maxBounds.z, worldVertex.z);
            }
        }
        // Unlink
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    for (const auto& child : obj->GetChildren())
    {
        CalculateBoundingBox(child, minBounds, maxBounds, worldTransform);
    }
}

bool LoadFiles::LoadMeshFromFile(const char* file_path, GameObject* target)
{
    if (!target) return false;

    ComponentMesh* currentMesh = target->GetComponent<ComponentMesh>();
    if (!currentMesh)
    {
        LOG("Error: Target GameObject does not have a Mesh Component.");
        return false;
    }

    const aiScene* scene = aiImportFile(file_path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || scene->mNumMeshes == 0)
    {
        LOG("Error loading mesh: %s", aiGetErrorString());
        return false;
    }

    // Get the first mesh from the file
    aiMesh* aiMesh = scene->mMeshes[0];

    MeshData meshData;
    // Reuse your ProcessMesh function
    ProcessMesh(aiMesh, meshData);

    // Load the data into the existing component, clearing the previous one
    currentMesh->LoadMesh(meshData.vertices, meshData.num_vertices,
        meshData.indices, meshData.num_indices,
        meshData.texCoords, meshData.normals);

    // CleanUp
    delete[] meshData.vertices;
    delete[] meshData.indices;
    if (meshData.texCoords) delete[] meshData.texCoords;
    if (meshData.normals) delete[] meshData.normals;
    if (meshData.colors) delete[] meshData.colors;

    aiReleaseImport(scene);
    LOG("Mesh replaced from: %s", file_path);
    return true;
}