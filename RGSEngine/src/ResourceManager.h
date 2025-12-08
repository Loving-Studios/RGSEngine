#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;


class GameObject;
class ComponentMesh;
class ComponentTexture;


struct ResourceInfo
{
    std::string assetPath;      // Original path in Assets/
    std::string libraryPath;    // Path to Library/
    std::string resourceID;
    int referenceCount = 0;
    uint64_t lastModified = 0;
    std::string resourceType;
};

// Structure for Assets folder/file tree
struct AssetNode
{
    std::string name;
    std::string path;
    bool isDirectory;
    std::vector<std::shared_ptr<AssetNode>> children;
    std::shared_ptr<ResourceInfo> resourceInfo;
};

class ResourceManager
{
public:
    static ResourceManager& GetInstance();


    bool Initialize();


    std::shared_ptr<AssetNode> GetAssetTree() const { return assetRoot; }
    void RefreshAssetTree();


    std::shared_ptr<ComponentMesh> LoadMesh(const std::string& assetPath);
    std::shared_ptr<ComponentTexture> LoadTexture(const std::string& assetPath);

    std::shared_ptr<GameObject> LoadFBXToScene(const std::string& assetPath);
    unsigned int LoadTextureToGPU(const std::string& assetPath);


    void UnloadResource(const std::string& resourceID);
    void ReleaseResourceReference(const std::string& resourceID);
    void AcquireResourceReference(const std::string& resourceID);


    const ResourceInfo* GetResourceInfo(const std::string& resourceID) const;
    int GetReferenceCount(const std::string& resourceID) const;


    bool DeleteAsset(const std::string& assetPath);


    bool ImportAsset(const std::string& sourcePath);


    void RegenerateLibrary();

    // Meta files 
    bool LoadMetaFile(const std::string& assetPath, ResourceInfo& outInfo);
    bool SaveMetaFile(const std::string& assetPath, const ResourceInfo& info);


    void PrintResourceStats() const;
    const std::unordered_map<std::string, ResourceInfo>& GetAllResources() const
    {
        return resources;
    }
  
    int CountUnprocessedAssets() const;

    bool ForceProcessAsset(const std::string& assetPath);
    bool NeedsReimport(const std::string& assetPath) const;
    bool CheckAndReimportIfModified(const std::string& assetPath);
    bool ProcessMeshAsset(const ResourceInfo& info);

    void ProcessUnprocessedAssets();
    void CleanupOrphanedAssets();
    void CleanupUnusedLibraryFiles();
    void MonitorAssetChanges();
   
  
    std::string GetAssetInfo(const std::string& assetPath) const;
  


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


    std::shared_ptr<AssetNode> assetRoot;
    std::unordered_map<std::string, ResourceInfo> resources;        // resourceID -> info
    std::unordered_map<std::string, std::string> assetPathToID;     // assetPath -> resourceID


    std::unordered_map<std::string, std::shared_ptr<void>> loadedResources;


    std::vector<std::string> supportedMeshFormats;
    std::vector<std::string> supportedTextureFormats;
};