#pragma once
#include "Module.h"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

class GameObject;
class ComponentMesh;

struct MeshData
{
    unsigned int num_vertices = 0;
    unsigned int num_indices = 0;
    float* vertices = nullptr;
    unsigned int* indices = nullptr;
    float* normals = nullptr;
    float* texCoords = nullptr;
    float* colors = nullptr;

    bool hasNormals = false;
    bool hasTexCoords = false;
    bool hasColors = false;
};

// Own format file header .rgs
struct MeshFileHeader
{
    unsigned int numVertices = 0;
    unsigned int numIndices = 0;

    // Flags with the data included
    bool hasNormals = false;
    bool hasTexCoords = false;
    bool hasColors = false;

    // Bounding box, material index, etc...
};

struct TextureHeader {
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int format = 0;
    unsigned int dataSize = 0;
};

class GameObject;

class LoadFiles : public Module
{
public:
    LoadFiles();
    virtual ~LoadFiles();

    bool Awake();
    bool Start();
    bool PreUpdate();
    bool Update(float dt);
    bool PostUpdate();
    bool CleanUp();

    std::shared_ptr<GameObject> LoadFBX(const char* file_path);
    bool LoadTexture(const char* file_path, GameObject* target);
    unsigned int LoadTexture(const char* file_path);
    bool LoadMeshFromFile(const char* file_path, GameObject* target);
    void HandleDropFile(const char* file_path);

    bool SaveMeshToCustomFormat(const char* path, const MeshData& meshData);
    bool LoadMeshFromCustomFormat(const char* path, MeshData& meshData);

private:

    void ProcessMesh(aiMesh* aiMesh, MeshData& meshData);
    std::shared_ptr<GameObject> CreateGameObjectFromMesh(const MeshData& meshData, const char* name);
    std::shared_ptr<GameObject> ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<GameObject> parent, const std::string& fbxDirectory, glm::mat4 accumulatedTransform = glm::mat4(1.0f));

    void LoadMaterialTextures(const aiScene* scene, aiMesh* mesh, std::shared_ptr<GameObject> gameObject, const std::string& fbxDirectory);

    void ApplyTextureToAllChildren(std::shared_ptr<GameObject> go, unsigned int textureID, const char* path);

    // Scale the root object so its largest dimension is the targetSize
    void NormalizeModelScale(std::shared_ptr<GameObject> rootObject, float targetSize = 5.0f);

    // Recursively calculates bounds (AABB) in world space
    void CalculateBoundingBox(std::shared_ptr<GameObject> obj,
        glm::vec3& minBounds, glm::vec3& maxBounds,
        const glm::mat4& parentTransform);

    aiLogStream stream;

    bool ImportTextureWithDevIL(const char* path, char*& buffer, TextureHeader& header);
    bool SaveTextureToCustomFormat(const char* path, const TextureHeader& header, const char* buffer);
    bool LoadTextureFromCustomFormat(const char* path, TextureHeader& header, char*& buffer);
    unsigned int CreateTextureFromBuffer(const TextureHeader& header, const char* buffer);
};