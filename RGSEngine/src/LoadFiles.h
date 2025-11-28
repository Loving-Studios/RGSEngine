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
    bool LoadTexture(const char* file_path, GameObject* target = nullptr);
    bool LoadMeshFromFile(const char* file_path, GameObject* target);
    void HandleDropFile(const char* file_path);

private:
    struct MeshData
    {
        unsigned int num_indices = 0;
        unsigned int* indices = nullptr;

        unsigned int num_vertices = 0;
        float* vertices = nullptr;

        float* texCoords = nullptr;
        bool hasTexCoords = false;

        float* normals = nullptr;
        bool hasNormals = false;

        float* colors = nullptr;
        bool hasColors = false;
    };

    void ProcessMesh(aiMesh* aiMesh, MeshData& meshData);
    std::shared_ptr<GameObject> CreateGameObjectFromMesh(const MeshData& meshData, const char* name);
    std::shared_ptr<GameObject> ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<GameObject> parent, const std::string& fbxDirectory, glm::mat4 accumulatedTransform = glm::mat4(1.0f));

    void LoadMaterialTextures(const aiScene* scene, aiMesh* mesh, std::shared_ptr<GameObject> gameObject, const std::string& fbxDirectory);
    unsigned int LoadTextureFromFile(const char* file_path);

    void ApplyTextureToAllChildren(std::shared_ptr<GameObject> go, unsigned int textureID, const char* path);

    // Escala el objeto raíz para que su mayor dimensión sea 'targetSize'
    void NormalizeModelScale(std::shared_ptr<GameObject> rootObject, float targetSize = 5.0f);

    // Calcula recursivamente los límites (AABB) en espacio mundial
    void CalculateBoundingBox(std::shared_ptr<GameObject> obj,
        glm::vec3& minBounds, glm::vec3& maxBounds,
        const glm::mat4& parentTransform);

    aiLogStream stream;
};