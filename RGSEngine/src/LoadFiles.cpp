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
#include <fstream>

#include <filesystem>

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

    // Custom File Format, create them automatically on start if they are deleted
    if (!std::filesystem::exists("Library"))
        std::filesystem::create_directory("Library");

    if (!std::filesystem::exists("Library/Meshes"))
        std::filesystem::create_directory("Library/Meshes");

    if (!std::filesystem::exists("Library/Textures"))
        std::filesystem::create_directory("Library/Textures");

    // Inicializar DevIL
    ilInit();
    iluInit();

    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_UPPER_LEFT);

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
        rootObject = CreateGameObjectFromMesh(meshData, fileName.c_str(), file_path);

        
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
        rootObject = ProcessNode(scene->mRootNode, scene, nullptr, fbxDirectory, file_path, glm::mat4(1.0f));
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

std::shared_ptr<GameObject> LoadFiles::ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<GameObject> parent, const std::string& fbxDirectory, const char* assetPath, glm::mat4 accumulatedTransform)
{
    // Convert the transformation of the current node to GLM
    aiMatrix4x4 transformMatrix = node->mTransformation;
    glm::mat4 nodeTransform;
    // Assimp is Row-Major, GLM is Column-Major -> Transpose
    nodeTransform[0][0] = transformMatrix.a1; nodeTransform[0][1] = transformMatrix.b1; nodeTransform[0][2] = transformMatrix.c1; nodeTransform[0][3] = transformMatrix.d1;
    nodeTransform[1][0] = transformMatrix.a2; nodeTransform[1][1] = transformMatrix.b2; nodeTransform[1][2] = transformMatrix.c2; nodeTransform[1][3] = transformMatrix.d2;
    nodeTransform[2][0] = transformMatrix.a3; nodeTransform[2][1] = transformMatrix.b3; nodeTransform[2][2] = transformMatrix.c3; nodeTransform[2][3] = transformMatrix.d3;
    nodeTransform[3][0] = transformMatrix.a4; nodeTransform[3][1] = transformMatrix.b4; nodeTransform[3][2] = transformMatrix.c4; nodeTransform[3][3] = transformMatrix.d4;

    // Accumulate the transformation,matrix of the parent * matrix of this node
    glm::mat4 localTransform = accumulatedTransform * nodeTransform;

    // DETECT IF IT IS AN ASSIMP TRASH NODE $AssimpFbx$
    std::string nodeName = node->mName.C_Str();
    if (nodeName.find("$AssimpFbx$") != std::string::npos)
    {
        // Its a dummy node, skipped visually, dont create GameObject but process childrens giving the transformation accumulated so they dont lose the positiona and rotation
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            // Pass the information
            ProcessNode(node->mChildren[i], scene, parent, fbxDirectory, assetPath, localTransform);
        }
        // Dont return anything because this node does not exist in our hierarchy
        return nullptr;
    }

    // Its a normal node so we create a GameObject
    std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>(nodeName);
    auto transform = std::make_shared<ComponentTransform>(gameObject.get());

    // Decompose the accumulated matrix to apply it to the transform
    glm::vec3 position, scale, skew;
    glm::quat rotation;
    glm::vec4 perspective;

    // Break down the local matrix of the node
    if (glm::decompose(localTransform, scale, rotation, position, skew, perspective))
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

    // Add the parent, if exists
    if (parent)
    {
        parent->AddChild(gameObject);
    }

    // Process Mesh
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
            std::string meshName = nodeName + "_SubMesh_" + std::to_string(i);
            meshObject = std::make_shared<GameObject>(meshName);
            auto meshTransform = std::make_shared<ComponentTransform>(meshObject.get());
            meshObject->AddComponent(meshTransform);
            gameObject->AddChild(meshObject);
        }

        auto compMesh = std::make_shared<ComponentMesh>(meshObject.get());
        compMesh->path = assetPath;
        compMesh->libraryPath = meshData.libraryPath;
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
        // As a real node is created, needs to reset the accumulation for the childrens because the local transformation will already be relative to the GameObject
        ProcessNode(node->mChildren[i], scene, gameObject, fbxDirectory, assetPath, glm::mat4(1.0f));
    }

    return gameObject;
}

void LoadFiles::ProcessMesh(aiMesh* aiMesh, MeshData& meshData)
{
    // Generate file name in library
    std::string meshName = aiMesh->mName.C_Str();
    if (meshName.empty()) meshName = "generated_mesh_" + std::to_string(aiMesh->mNumVertices);

    std::string libraryPath = "Library/Meshes/" + meshName + ".rgs";

    meshData.libraryPath = libraryPath;

    // Check if the file exists, if exists, load the file and skip assimp
    std::ifstream checkFile(libraryPath);
    if (checkFile.good())
    {
        checkFile.close();

        if (LoadMeshFromCustomFormat(libraryPath.c_str(), meshData))
        {
            LOG("Resources: Loaded mesh from Library (FAST): %s", libraryPath.c_str());
            return;
        }
    }

    // If not, the file didnt exist on Library, so slow version with assimp
    LOG("Resources: Importing mesh from FBX (SLOW)...");

    meshData.num_vertices = aiMesh->mNumVertices;
    meshData.vertices = new float[meshData.num_vertices * 3];
    memcpy(meshData.vertices, aiMesh->mVertices, sizeof(float) * meshData.num_vertices * 3);

    if (aiMesh->HasFaces())
    {
        meshData.num_indices = aiMesh->mNumFaces * 3;
        meshData.indices = new unsigned int[meshData.num_indices];
        for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
        {
            memcpy(&meshData.indices[i * 3], aiMesh->mFaces[i].mIndices, 3 * sizeof(unsigned int));
        }
    }

    // Normals
    if (aiMesh->HasNormals())
    {
        meshData.hasNormals = true;
        meshData.normals = new float[meshData.num_vertices * 3];
        memcpy(meshData.normals, aiMesh->mNormals, sizeof(float) * meshData.num_vertices * 3);
    }

    // UVs
    if (aiMesh->HasTextureCoords(0))
    {
        meshData.hasTexCoords = true;
        meshData.texCoords = new float[meshData.num_vertices * 2];
        for (unsigned int i = 0; i < meshData.num_vertices; i++)
        {
            meshData.texCoords[i * 2] = aiMesh->mTextureCoords[0][i].x;
            meshData.texCoords[i * 2 + 1] = aiMesh->mTextureCoords[0][i].y;
        }
    }

    SaveMeshToCustomFormat(libraryPath.c_str(), meshData);
    LOG("Resources: Saved mesh to Library: %s", libraryPath.c_str());
}

std::shared_ptr<GameObject> LoadFiles::CreateGameObjectFromMesh(const MeshData& meshData, const char* name, const char* assetPath)
{
    auto gameObject = std::make_shared<GameObject>(name);

    auto transform = std::make_shared<ComponentTransform>(gameObject.get());
    gameObject->AddComponent(transform);

    auto compMesh = std::make_shared<ComponentMesh>(gameObject.get());
    compMesh->path = assetPath;
    compMesh->libraryPath = meshData.libraryPath;
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
                    textureID = LoadTexture(path.c_str());
                    if (textureID != 0)
                    {
                        loadedPath = path;
                        LOG("SUCCESS!");
                        break;
                    }
                }

                if (textureID != 0)
                {
                    auto texComponent = std::make_shared<ComponentTexture>(gameObject.get());
                    texComponent->textureID = textureID;
                    texComponent->path = loadedPath;

                    std::string pathString(loadedPath);
                    std::string filename = pathString.substr(pathString.find_last_of("/\\") + 1);
                    size_t lastDot = filename.find_last_of(".");
                    if (lastDot != std::string::npos) filename = filename.substr(0, lastDot);

                    texComponent->libraryPath = "Library/Textures/" + filename + ".rgst";

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

bool LoadFiles::LoadTexture(const char* file_path, GameObject* target)
{
    LOG("=== TEXTURE LOADING SYSTEM ===");

    if (target == nullptr)
    {
        LOG("Error: No target GameObject provided for texture loading.");
        return false;
    }

    std::string pathString(file_path);
    std::string filename = pathString.substr(pathString.find_last_of("/\\") + 1);
    size_t lastDot = filename.find_last_of(".");
    if (lastDot != std::string::npos) filename = filename.substr(0, lastDot);
    std::string internalPath = "Library/Textures/" + filename + ".rgst";

    unsigned int textureID = LoadTexture(file_path);

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
    if (textureComp)
    {
        // If has a component, update the data
        textureComp->textureID = textureID;
        // Save the original path by reference, Asset
        textureComp->path = file_path;
        textureComp->libraryPath = internalPath;
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
        newTex->libraryPath = internalPath;
        target->AddComponent(newTex);
        LOG("Texture component ADDED to GameObject: %s", target->GetName().c_str());
    }
    LOG("Texture applied to %s (Internal: %s)", target->GetName().c_str(), internalPath.c_str());
    return true;
}
return false;
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

    currentMesh->path = file_path;
    currentMesh->libraryPath = meshData.libraryPath;

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

bool LoadFiles::SaveMeshToCustomFormat(const char* path, const MeshData& meshData)
{
    // Open the file in binary mode and truncate, and overwrite
    std::ofstream file(path, std::ios::binary | std::ios::trunc);

    if (!file.is_open())
    {
        LOG("Error: Could not open file for writing: %s", path);
        return false;
    }

    // Prepare and write the header
    MeshFileHeader header;
    header.numVertices = meshData.num_vertices;
    header.numIndices = meshData.num_indices;
    header.hasNormals = meshData.hasNormals;
    header.hasTexCoords = meshData.hasTexCoords;
    header.hasColors = meshData.hasColors;

    // Write the header structure as it is in memory
    file.write((char*)&header, sizeof(MeshFileHeader));

    // Write the array data

    // Vertexs -> 3 floats each vertex
    file.write((char*)meshData.vertices, sizeof(float) * header.numVertices * 3);

    // Index
    file.write((char*)meshData.indices, sizeof(unsigned int) * header.numIndices);

    // Normals, optional
    if (header.hasNormals)
    {
        file.write((char*)meshData.normals, sizeof(float) * header.numVertices * 3);
    }

    // UVs, optional
    if (header.hasTexCoords)
    {
        file.write((char*)meshData.texCoords, sizeof(float) * header.numVertices * 2);
    }

    // Colors, optional
    if (header.hasColors)
    {
        file.write((char*)meshData.colors, sizeof(float) * header.numVertices * 4);
    }

    file.close();
    LOG("Success: Mesh saved to custom format: %s", path);
    return true;
}

bool LoadFiles::LoadMeshFromCustomFormat(const char* path, MeshData& meshData)
{
    // Open binary mode
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        LOG("Error: Could not open custom mesh file: %s", path);
        return nullptr;
    }

    // Read header
    MeshFileHeader header;
    file.read((char*)&header, sizeof(MeshFileHeader));

    // Reserve temporary memory to read the data
    // Creation of a local MeshData to send later to the ComponentMesh
    meshData.num_vertices = header.numVertices;
    meshData.num_indices = header.numIndices;
    meshData.hasNormals = header.hasNormals;
    meshData.hasTexCoords = header.hasTexCoords;
    meshData.hasColors = header.hasColors;

    // Vertex
    meshData.vertices = new float[header.numVertices * 3];
    file.read((char*)meshData.vertices, sizeof(float) * header.numVertices * 3);

    // Index
    meshData.indices = new unsigned int[header.numIndices];
    file.read((char*)meshData.indices, sizeof(unsigned int) * header.numIndices);

    // Normals
    if (header.hasNormals)
    {
        meshData.normals = new float[header.numVertices * 3];
        file.read((char*)meshData.normals, sizeof(float) * header.numVertices * 3);
    }

    // UVs
    if (header.hasTexCoords)
    {
        meshData.texCoords = new float[header.numVertices * 2];
        file.read((char*)meshData.texCoords, sizeof(float) * header.numVertices * 2);
    }

    // Colors
    if (header.hasColors)
    {
        meshData.colors = new float[header.numVertices * 4];
        file.read((char*)meshData.colors, sizeof(float) * header.numVertices * 4);
    }

    file.close();
    LOG("Success: Mesh loaded from custom format: %s", path);
    return true;
}

unsigned int LoadFiles::LoadTexture(const char* file_path)
{
    // Generate the destination path in Library
    std::string pathString(file_path);
    std::string filename = pathString.substr(pathString.find_last_of("/\\") + 1);

    // Remove the original extension and replace it with our own .rgst
    size_t lastDot = filename.find_last_of(".");
    if (lastDot != std::string::npos) filename = filename.substr(0, lastDot);
    std::string libraryPath = "Library/Textures/" + filename + ".rgst";

    TextureHeader header;
    char* buffer = nullptr;
    unsigned int textureID = 0;

    // Check if it already exists in Library
    std::ifstream f(libraryPath.c_str());
    if (f.good())
    {
        f.close();
        LOG("Texture found in Library, loading custom format: %s", libraryPath.c_str());
        if (LoadTextureFromCustomFormat(libraryPath.c_str(), header, buffer))
        {
            textureID = CreateTextureFromBuffer(header, buffer);
            // Clean RAM memory, is already in VRAM
            delete[] buffer;
            return textureID;
        }
    }

    // If it does not exist or failed to load, we import with DevIL slow path to load
    LOG("Texture NOT found in Library, importing with DevIL: %s", file_path);
    if (ImportTextureWithDevIL(file_path, buffer, header))
    {
        // Save it in Library for next time
        SaveTextureToCustomFormat(libraryPath.c_str(), header, buffer);

        // Create the texture in OpenGL
        textureID = CreateTextureFromBuffer(header, buffer);
        delete[] buffer;
    }

    return textureID;
}

bool LoadFiles::ImportTextureWithDevIL(const char* path, char*& buffer, TextureHeader& header)
{
    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    if (!ilLoadImage(path))
    {
        LOG("DevIL Error loading: %s", path);
        ilDeleteImages(1, &imageID);
        return false;
    }

    // Force conversion to RGBA to standardize the format
    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE))
    {
        LOG("DevIL Error converting image");
        ilDeleteImages(1, &imageID);
        return false;
    }

    // Fill the header
    header.width = ilGetInteger(IL_IMAGE_WIDTH);
    header.height = ilGetInteger(IL_IMAGE_HEIGHT);
    // Save in RGBA
    header.format = GL_RGBA;
    // 4 bytes per pixel, RBGA 
    header.dataSize = header.width * header.height * 4;

    // Copy the data from DevIL to our buffer
    buffer = new char[header.dataSize];
    memcpy(buffer, ilGetData(), header.dataSize);

    ilDeleteImages(1, &imageID);
    return true;
}

bool LoadFiles::SaveTextureToCustomFormat(const char* path, const TextureHeader& header, const char* buffer)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        LOG("Error saving custom texture: %s", path);
        return false;
    }

    // Write header
    file.write((const char*)&header, sizeof(TextureHeader));

    // Write Pixel Data
    file.write(buffer, header.dataSize);

    file.close();
    LOG("Texture saved to Library: %s", path);
    return true;
}

bool LoadFiles::LoadTextureFromCustomFormat(const char* path, TextureHeader& header, char*& buffer)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    // Read header
    file.read((char*)&header, sizeof(TextureHeader));

    // Reserve memory and read data
    buffer = new char[header.dataSize];
    file.read(buffer, header.dataSize);

    file.close();
    return true;
}

unsigned int LoadFiles::CreateTextureFromBuffer(const TextureHeader& header, const char* buffer)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Basic parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload data to the GPU
    glTexImage2D(GL_TEXTURE_2D, 0, header.format, header.width, header.height, 0, header.format, GL_UNSIGNED_BYTE, buffer);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    LOG("Texture created in OpenGL (ID: %d) from buffer", textureID);
    return textureID;
}