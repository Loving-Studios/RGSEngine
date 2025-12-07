#include "ResourceManager.h"
#include "Log.h"
#include "LoadFiles.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "Application.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>
#include <fstream>

using json = nlohmann::json;

ResourceManager& ResourceManager::GetInstance()
{
    static ResourceManager instance;
    return instance;
}

ResourceManager::ResourceManager()
{
    // supported extensions
    supportedMeshFormats = { ".fbx", ".obj", ".gltf", ".glb" };
    supportedTextureFormats = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds" };
}

bool ResourceManager::Initialize()
{
    LOG("=== ResourceManager Initialize ===");

    // Create necessary directories
    if (!fs::exists("Assets"))
        fs::create_directory("Assets");
    if (!fs::exists("Library"))
        fs::create_directory("Library");
    if (!fs::exists("Library/Meshes"))
        fs::create_directory("Library/Meshes");
    if (!fs::exists("Library/Textures"))
        fs::create_directory("Library/Textures");

    // Load existing resources into Library
    RefreshAssetTree();
    ProcessUnprocessedAssets();

    LOG("ResourceManager initialized successfully");
    LOG("Found %d assets", (int)resources.size());

    return true;
}

void ResourceManager::RefreshAssetTree()
{
    LOG("Refreshing Asset Tree...");

    assetRoot = std::make_shared<AssetNode>();
    assetRoot->name = "Assets";
    assetRoot->path = "Assets";
    assetRoot->isDirectory = true;

    if (fs::exists("Assets"))
    {
        BuildAssetTree(fs::path("Assets"), assetRoot);
    }

    LOG("Asset Tree refreshed");
}

void ResourceManager::BuildAssetTree(const fs::path& directoryPath, std::shared_ptr<AssetNode> parentNode)
{
    try
    {
        for (const auto& entry : fs::directory_iterator(directoryPath))
        {
            std::string fileName = entry.path().filename().string();

            // Ignore files . meta
            if (entry.path().extension().string() == ".meta")
                continue;

            auto node = std::make_shared<AssetNode>();
            node->name = fileName;
            node->path = entry.path().string();
            node->isDirectory = entry.is_directory();

            if (entry.is_directory())
            {
                
                BuildAssetTree(entry.path(), node);
            }
            else if (IsAssetFile(entry.path()))
            {
                // It is a resource file
                std::string resourceType = GetResourceType(entry.path());
                std::string resourceID = GenerateResourceID(node->path);

                ResourceInfo info;
                info.assetPath = node->path;
                info.resourceID = resourceID;
                info.resourceType = resourceType;
                info.libraryPath = GetLibraryPath(node->path, resourceType);
                info.lastModified = fs::last_write_time(entry.path()).time_since_epoch().count();
                info.referenceCount = 0;

                // Try to load metadata
                LoadMetaFile(node->path, info);

                resources[resourceID] = info;
                assetPathToID[node->path] = resourceID;
                node->resourceInfo = std::make_shared<ResourceInfo>(info);

                LOG("Asset registered: %s (ID: %s, Type: %s)",
                    fileName.c_str(), resourceID.substr(0, 8).c_str(), resourceType.c_str());
            }

            parentNode->children.push_back(node);
        }
    }
    catch (const std::exception& e)
    {
        LOG("Error building asset tree: %s", e.what());
    }
}

bool ResourceManager::IsAssetFile(const fs::path& path) const
{
    std::string ext = path.extension().string();

   
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto& fmt : supportedMeshFormats)
        if (ext == fmt) return true;

    for (const auto& fmt : supportedTextureFormats)
        if (ext == fmt) return true;

    return false;
}

std::string ResourceManager::GetResourceType(const fs::path& path) const
{
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto& fmt : supportedMeshFormats)
        if (ext == fmt) return "mesh";

    for (const auto& fmt : supportedTextureFormats)
        if (ext == fmt) return "texture";

    return "unknown";
}

std::string ResourceManager::GenerateResourceID(const std::string& assetPath)
{
    
    std::hash<std::string> hasher;
    uint64_t hashValue = hasher(assetPath);

    std::stringstream ss;
    ss << std::hex << hashValue;
    return ss.str();
}

std::string ResourceManager::GetLibraryPath(const std::string& assetPath, const std::string& resourceType)
{
    fs::path p(assetPath);
    std::string filename = p.stem().string(); 

    if (resourceType == "mesh")
    {
        return "Library/Meshes/" + filename + ".rgs";
    }
    else if (resourceType == "texture")
    {
        return "Library/Textures/" + filename + ".rgst";
    }

    return "Library/" + filename;
}

std::shared_ptr<ComponentMesh> ResourceManager::LoadMesh(const std::string& assetPath)
{
    LOG("Loading mesh: %s", assetPath.c_str());

    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("ERROR: Asset not found: %s", assetPath.c_str());
        return nullptr;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    
    auto mesh = std::make_shared<ComponentMesh>(nullptr);
    mesh->path = assetPath;
    mesh->libraryPath = info.libraryPath;

   
    AcquireResourceReference(resourceID);

    LOG("Mesh loaded: %s (References: %d)", assetPath.c_str(), info.referenceCount);

    return mesh;
}

std::shared_ptr<ComponentTexture> ResourceManager::LoadTexture(const std::string& assetPath)
{
    LOG("Loading texture: %s", assetPath.c_str());

    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("ERROR: Asset not found: %s", assetPath.c_str());
        return nullptr;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    // Create a texture component
    auto texture = std::make_shared<ComponentTexture>(nullptr);
    texture->path = assetPath;
    texture->libraryPath = info.libraryPath;

    // Increase reference
    AcquireResourceReference(resourceID);

    LOG("Texture loaded: %s (References: %d)", assetPath.c_str(), info.referenceCount);

    return texture;
}

void ResourceManager::AcquireResourceReference(const std::string& resourceID)
{
    auto it = resources.find(resourceID);
    if (it != resources.end())
    {
        it->second.referenceCount++;
    }
}

void ResourceManager::ReleaseResourceReference(const std::string& resourceID)
{
    auto it = resources.find(resourceID);
    if (it != resources.end() && it->second.referenceCount > 0)
    {
        it->second.referenceCount--;
    }
}

void ResourceManager::UnloadResource(const std::string& resourceID)
{
    auto it = resources.find(resourceID);
    if (it == resources.end()) return;

    ReleaseResourceReference(resourceID);

    if (it->second.referenceCount == 0)
    {
        LOG("Unloading unused resource: %s", resourceID.substr(0, 8).c_str());
        loadedResources.erase(resourceID);
    }
}

const ResourceInfo* ResourceManager::GetResourceInfo(const std::string& resourceID) const
{
    auto it = resources.find(resourceID);
    if (it != resources.end())
        return &it->second;
    return nullptr;
}

int ResourceManager::GetReferenceCount(const std::string& resourceID) const
{
    auto it = resources.find(resourceID);
    if (it != resources.end())
        return it->second.referenceCount;
    return -1;
}

bool ResourceManager::DeleteAsset(const std::string& assetPath)
{
    LOG("Deleting asset: %s", assetPath.c_str());

    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("ERROR: Asset not found: %s", assetPath.c_str());
        return false;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    if (info.referenceCount > 0)
    {
        LOG("ERROR: Cannot delete asset with active references (%d)", info.referenceCount);
        return false;
    }

    try
    {
        // Remove file from Asset
        if (fs::exists(assetPath))
        {
            fs::remove(assetPath);
            LOG("Deleted asset file: %s", assetPath.c_str());
        }

        // Delete file from Library
        if (fs::exists(info.libraryPath))
        {
            fs::remove(info.libraryPath);
            LOG("Deleted library file: %s", info.libraryPath.c_str());
        }

        // Delete file . meta
        std::string metaPath = assetPath + ".meta";
        if (fs::exists(metaPath))
        {
            fs::remove(metaPath);
            LOG("Deleted meta file: %s", metaPath.c_str());
        }

        
        resources.erase(resourceID);
        assetPathToID.erase(assetPath);

        RefreshAssetTree();
        return true;
    }
    catch (const std::exception& e)
    {
        LOG("ERROR deleting asset: %s", e.what());
        return false;
    }
}

bool ResourceManager::ImportAsset(const std::string& sourcePath)
{
    LOG("Importing asset: %s", sourcePath.c_str());

    if (!fs::exists(sourcePath))
    {
        LOG("ERROR: Source file does not exist: %s", sourcePath.c_str());
        return false;
    }

    try
    {
        // Determine destination in Assets/
        std::string filename = fs::path(sourcePath).filename().string();
        std::string destPath = "Assets/" + filename;

        
        fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
        LOG("Copied asset to: %s", destPath.c_str());

        
        RefreshAssetTree();

        return true;
    }
    catch (const std::exception& e)
    {
        LOG("ERROR importing asset: %s", e.what());
        return false;
    }
}

bool ResourceManager::LoadMetaFile(const std::string& assetPath, ResourceInfo& outInfo)
{
    std::string metaPath = assetPath + ".meta";

    if (!fs::exists(metaPath))
    {
       
        SaveMetaFile(assetPath, outInfo);
        return false;
    }

    try
    {
        std::ifstream file(metaPath);
        json metaJson;
        file >> metaJson;
        file.close();

        outInfo.resourceID = metaJson["resourceID"];
        outInfo.lastModified = metaJson["lastModified"];

        LOG("Loaded meta file: %s", metaPath.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        LOG("ERROR loading meta file: %s", e.what());
        return false;
    }
}

bool ResourceManager::SaveMetaFile(const std::string& assetPath, const ResourceInfo& info)
{
    std::string metaPath = assetPath + ".meta";

    try
    {
        json metaJson;
        metaJson["assetPath"] = info.assetPath;
        metaJson["resourceID"] = info.resourceID;
        metaJson["resourceType"] = info.resourceType;
        metaJson["lastModified"] = info.lastModified;

        std::ofstream file(metaPath);
        file << metaJson.dump(4);
        file.close();

        LOG("Saved meta file: %s", metaPath.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        LOG("ERROR saving meta file: %s", e.what());
        return false;
    }
}

void ResourceManager::ProcessUnprocessedAssets()
{
    LOG("Processing unprocessed assets...");

    for (auto& [resourceID, info] : resources)
    {
        // Only process if it does not exist in Library/
        if (!fs::exists(info.libraryPath))
        {
            LOG("Processing asset: %s", info.assetPath.c_str());

            //  Processing by type
            if (info.resourceType == "mesh")
            {
                
                MeshData meshData;

              

                LOG("Mesh asset registered: %s", info.assetPath.c_str());
            }
            else if (info.resourceType == "texture")
            {
                LOG("Texture asset registered: %s", info.assetPath.c_str());
            }
        }
    }

}

void ResourceManager::CleanupUnusedLibraryFiles()
{
    LOG("Cleaning up unused library files...");

    std::vector<std::string> validLibraryFiles;
    for (const auto& [id, info] : resources)
    {
        validLibraryFiles.push_back(info.libraryPath);
    }

    // Recorrer directorios de Library
    for (const auto& dir : { "Library/Meshes", "Library/Textures" })
    {
        if (fs::exists(dir))
        {
            for (const auto& entry : fs::directory_iterator(dir))
            {
                std::string path = entry.path().string();

                bool isValid = std::find(validLibraryFiles.begin(),
                    validLibraryFiles.end(),
                    path) != validLibraryFiles.end();

                if (!isValid && entry.is_regular_file())
                {
                    try
                    {
                        fs::remove(entry.path());
                        LOG("Removed orphaned library file: %s", path.c_str());
                    }
                    catch (const std::exception& e)
                    {
                        LOG("ERROR removing file: %s", e.what());
                    }
                }
            }
        }
    }
}

void ResourceManager::RegenerateLibrary()
{
    LOG("=== Regenerating Library ===");

    
    if (fs::exists("Library/Meshes"))
        fs::remove_all("Library/Meshes");
    if (fs::exists("Library/Textures"))
        fs::remove_all("Library/Textures");

    
    fs::create_directory("Library/Meshes");
    fs::create_directory("Library/Textures");

    
    ProcessUnprocessedAssets();

    LOG("Library regenerated");
}

void ResourceManager::PrintResourceStats() const
{
    LOG("=== Resource Statistics ===");
    LOG("Total resources: %d", (int)resources.size());

    int totalReferences = 0;
    int meshCount = 0, textureCount = 0;

    for (const auto& [id, info] : resources)
    {
        totalReferences += info.referenceCount;

        if (info.resourceType == "mesh")
            meshCount++;
        else if (info.resourceType == "texture")
            textureCount++;

        if (info.referenceCount > 0)
        {
            LOG("  %s: %d refs (Type: %s)",
                fs::path(info.assetPath).filename().string().c_str(),
                info.referenceCount,
                info.resourceType.c_str());
        }
    }

    LOG("Meshes: %d, Textures: %d", meshCount, textureCount);
    LOG("Total references: %d", totalReferences);
}

std::shared_ptr<GameObject> ResourceManager::LoadFBXToScene(const std::string& assetPath)
{
    LOG("Loading FBX to scene: %s", assetPath.c_str());

    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("ERROR: Asset not found: %s", assetPath.c_str());
        return nullptr;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    // Usar LoadFiles directamente
    auto gameObject = Application::GetInstance().loadFiles->LoadFBX(assetPath.c_str());

    if (gameObject)
    {
        AcquireResourceReference(resourceID);
        LOG("FBX loaded to scene: %s (References: %d)",
            assetPath.c_str(), info.referenceCount);
    }

    return gameObject;
}
unsigned int ResourceManager::LoadTextureToGPU(const std::string& assetPath)
{
    LOG("Loading texture to GPU: %s", assetPath.c_str());

    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("ERROR: Asset not found: %s", assetPath.c_str());
        return 0;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    // Usar LoadFiles para cargar textura
    unsigned int textureID = Application::GetInstance().loadFiles->LoadTexture(assetPath.c_str());

    if (textureID != 0)
    {
        AcquireResourceReference(resourceID);
        LOG("Texture loaded to GPU: %s (ID: %d, References: %d)",
            assetPath.c_str(), textureID, info.referenceCount);
    }

    return textureID;
}