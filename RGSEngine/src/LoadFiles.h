#pragma once
#include "Module.h"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
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
    bool LoadTexture(const char* file_path);
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
    std::shared_ptr<GameObject> ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<GameObject> parent, const std::string& fbxDirectory);

    void LoadMaterialTextures(const aiScene* scene, aiMesh* mesh, std::shared_ptr<GameObject> gameObject, const std::string& fbxDirectory);
    void ApplyTextureToAllChildren(std::shared_ptr<GameObject> go, unsigned int textureID, const char* path);
    unsigned int LoadTextureFromFile(const char* file_path);

    void AutoScaleGameObject(std::shared_ptr<GameObject> gameObject, const MeshData& meshData);

    aiLogStream stream;
};