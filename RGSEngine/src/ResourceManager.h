#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;

// Forward declarations
class GameObject;
class ComponentMesh;
class ComponentTexture;

struct ResourceInfo
{
    std::string assetPath;      // Original path in Assets/
    std::string libraryPath;    // Path to Library/
    std::string resourceID;     // Unique ID generated from asset path
    int referenceCount = 0;     // Number of GameObjects using this resource
    uint64_t lastModified = 0;  // Last modification timestamp
    std::string resourceType;   // "mesh" or "texture"
};

// Structure for Assets folder/file tree visualization
struct AssetNode
{
    std::string name;                               // Display name
    std::string path;                               // Full path
    bool isDirectory;                               // Is folder or file
    std::vector<std::shared_ptr<AssetNode>> children;  // Child nodes
    std::shared_ptr<ResourceInfo> resourceInfo;     // Associated resource info (only for files)
};

class ResourceManager
{
public:
   
    static ResourceManager& GetInstance();

    bool Initialize();

   
    std::shared_ptr<AssetNode> GetAssetTree() const { return assetRoot; }

    void RefreshAssetTree();

    std::shared_ptr<GameObject> LoadFBXToScene(const std::string& assetPath);

   
    unsigned int LoadTextureToGPU(const std::string& assetPath);

    void AcquireResourceReference(const std::string& resourceID);

   
    void ReleaseResourceReference(const std::string& resourceID);


    void UpdateReferenceCounts();

    const ResourceInfo* GetResourceInfo(const std::string& resourceID) const;

   
    int GetReferenceCount(const std::string& resourceID) const;

   
    const std::unordered_map<std::string, ResourceInfo>& GetAllResources() const
    {
        return resources;
    }

  
      bool DeleteAsset(const std::string& assetPath);

    // Import external file into Assets/ folder
    bool ImportAsset(const std::string& sourcePath);

    void RegenerateLibrary();

  
    void ProcessUnprocessedAssets();

   
    int CountUnprocessedAssets() const;

    bool ForceProcessAsset(const std::string& assetPath);

 
    bool LoadMetaFile(const std::string& assetPath, ResourceInfo& outInfo);

   
    bool SaveMetaFile(const std::string& assetPath, const ResourceInfo& info);


    void PrintResourceStats() const;

  
    bool ProcessMeshAsset(const ResourceInfo& info);


    std::unordered_map<std::string, std::string> assetPathToID;

private:
   
    ResourceManager();
    ~ResourceManager() = default;

   
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    void BuildAssetTree(const fs::path& directoryPath, std::shared_ptr<AssetNode> parentNode);

   
    std::string GenerateResourceID(const std::string& assetPath);

   
    std::string GetLibraryPath(const std::string& assetPath, const std::string& resourceType);

  
    bool IsAssetFile(const fs::path& path) const;

   
    std::string GetResourceType(const fs::path& path) const;

   
    void CountReferencesInGameObject(GameObject* go);

private:
  
    std::shared_ptr<AssetNode> assetRoot;

  
    std::unordered_map<std::string, ResourceInfo> resources;

    // Supported file formats
    std::vector<std::string> supportedMeshFormats;
    std::vector<std::string> supportedTextureFormats;
};