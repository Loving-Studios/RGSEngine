#include "ResourceManager.h"
#include "Log.h"
#include "LoadFiles.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <glad/glad.h>

using json = nlohmann::json;

ResourceManager& ResourceManager::GetInstance()
{
    static ResourceManager instance;
    return instance;
}

ResourceManager::ResourceManager()
{
    supportedMeshFormats = { ".fbx", ".obj", ".gltf", ".glb" };
    supportedTextureFormats = { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds" };
}

bool ResourceManager::Initialize()
{
    LOG("ResourceManager Initialize");

    if (!fs::exists("Assets"))
        fs::create_directory("Assets");

    if (!fs::exists("Library"))
        fs::create_directory("Library");

    if (!fs::exists("Library/Meshes"))
        fs::create_directory("Library/Meshes");

    if (!fs::exists("Library/Textures"))
        fs::create_directory("Library/Textures");

    RefreshAssetTree();

    int totalAssets = (int)resources.size();
    LOG("Found %d assets in Assets/", totalAssets);

    int unprocessed = CountUnprocessedAssets();
    int processed = totalAssets - unprocessed;

    LOG("Assets already in Library/: %d", processed);
    LOG("Assets needing processing: %d", unprocessed);

    LOG("ResourceManager Initialized Successfully");
    return true;
}

void ResourceManager::RefreshAssetTree()
{
    assetRoot = std::make_shared<AssetNode>();
    assetRoot->name = "Assets";
    assetRoot->path = "Assets";
    assetRoot->isDirectory = true;

    if (fs::exists("Assets"))
    {
        BuildAssetTree(fs::path("Assets"), assetRoot);
    }
}

void ResourceManager::BuildAssetTree(const fs::path& directoryPath, std::shared_ptr<AssetNode> parentNode)
{
    try
    {
        for (const auto& entry : fs::directory_iterator(directoryPath))
        {
            std::string fileName = entry.path().filename().string();

            // Skip .meta files
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
                std::string resourceType = GetResourceType(entry.path());
                std::string resourceID = GenerateResourceID(node->path);

                ResourceInfo info;
                info.assetPath = node->path;
                info.resourceID = resourceID;
                info.resourceType = resourceType;
                info.libraryPath = GetLibraryPath(node->path, resourceType);
                info.lastModified = fs::last_write_time(entry.path()).time_since_epoch().count();
                info.referenceCount = 0;

                // Load or create .meta file
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
        return "Library/Meshes/" + filename + ".rgs";
    else if (resourceType == "texture")
        return "Library/Textures/" + filename + ".rgst";

    return "Library/" + filename;
}

void ResourceManager::AcquireResourceReference(const std::string& resourceID)
{
    auto it = resources.find(resourceID);
    if (it != resources.end())
    {
        it->second.referenceCount++;
        LOG("Resource reference acquired: %s (Total refs: %d)",
            resourceID.substr(0, 8).c_str(), it->second.referenceCount);
    }
}

void ResourceManager::ReleaseResourceReference(const std::string& resourceID)
{
    auto it = resources.find(resourceID);
    if (it != resources.end() && it->second.referenceCount > 0)
    {
        it->second.referenceCount--;
        LOG("Resource reference released: %s (Remaining refs: %d)",
            resourceID.substr(0, 8).c_str(), it->second.referenceCount);
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
    LOG("Attempting to delete asset: %s", assetPath.c_str());

    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("ERROR: Asset not found in registry: %s", assetPath.c_str());
        return false;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    if (info.referenceCount > 0)
    {
        LOG("ERROR: Cannot delete asset with active references (%d)", info.referenceCount);
        LOG("This asset is currently being used by %d GameObject(s) in the scene", info.referenceCount);
        return false;
    }

    try
    {
        bool allDeleted = true;

        // Delete asset file
        if (fs::exists(assetPath))
        {
            fs::remove(assetPath);
            LOG("Deleted asset file: %s", assetPath.c_str());
        }
        else
        {
            LOG("WARNING: Asset file not found: %s", assetPath.c_str());
            allDeleted = false;
        }

        // Delete library file
        if (fs::exists(info.libraryPath))
        {
            fs::remove(info.libraryPath);
            LOG("Deleted library file: %s", info.libraryPath.c_str());
        }
        else
        {
            LOG("WARNING: Library file not found: %s", info.libraryPath.c_str());
        }

        // Delete .meta file
        std::string metaPath = assetPath + ".meta";
        if (fs::exists(metaPath))
        {
            fs::remove(metaPath);
            LOG("Deleted meta file: %s", metaPath.c_str());
        }
        else
        {
            LOG("WARNING: Meta file not found: %s", metaPath.c_str());
        }

        // Remove from internal registry
        resources.erase(resourceID);
        assetPathToID.erase(assetPath);

        LOG("Removed from internal registry");

        // Refresh asset tree
        RefreshAssetTree();
        LOG("Asset tree refreshed");

        LOG("Asset deleted successfully");
        return allDeleted;
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
        std::string filename = fs::path(sourcePath).filename().string();
        std::string destPath = "Assets/" + filename;

        // Check if file already exists
        if (fs::exists(destPath))
        {
            LOG("WARNING: File already exists in Assets, overwriting: %s", destPath.c_str());
        }

        fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
        LOG("Copied asset to: %s", destPath.c_str());

        // Refresh asset tree to register the new asset
        RefreshAssetTree();

        LOG("Asset imported successfully");
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
        // Create new .meta file
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
        // Create new .meta file if corrupted
        SaveMetaFile(assetPath, outInfo);
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
        metaJson["libraryPath"] = info.libraryPath;

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

    auto& app = Application::GetInstance();
    if (!app.loadFiles)
    {
        LOG("ERROR: LoadFiles module not initialized");
        return;
    }

    int processed = 0;
    int failed = 0;
    int skipped = 0;

    for (auto& [resourceID, info] : resources)
    {
        // Check if library file exists
        if (!fs::exists(info.libraryPath))
        {
            LOG("Processing: %s", info.assetPath.c_str());
            LOG("  Type: %s", info.resourceType.c_str());
            LOG("  Target Library: %s", info.libraryPath.c_str());

            if (!fs::exists(info.assetPath))
            {
                LOG("  ERROR: Source file does not exist!");
                failed++;
                continue;
            }

            try
            {
                bool success = false;

                if (info.resourceType == "mesh")
                {
                    success = ProcessMeshAsset(info);
                }
                else if (info.resourceType == "texture")
                {
                    unsigned int textureID = app.loadFiles->LoadTexture(info.assetPath.c_str());

                    if (textureID != 0 && fs::exists(info.libraryPath))
                    {
                        success = true;
                       
                        glDeleteTextures(1, &textureID);
                    }
                }
                else
                {
                    LOG("  ERROR: Unknown resource type '%s'", info.resourceType.c_str());
                    skipped++;
                    continue;
                }

                if (success)
                {
                    LOG("  SUCCESS: Processed and saved to Library");
                    processed++;
                    // Update last modified time
                    info.lastModified = fs::last_write_time(info.assetPath).time_since_epoch().count();
                    SaveMetaFile(info.assetPath, info);
                }
                else
                {
                    LOG("  FAILED: Could not process asset");
                    failed++;
                }
            }
            catch (const std::exception& e)
            {
                LOG("  ERROR: Exception during processing: %s", e.what());
                failed++;
            }
        }
        else
        {
            skipped++;
        }
    }

   

    if (processed > 0)
    {
        RefreshAssetTree();
    }
}

bool ResourceManager::ProcessMeshAsset(const ResourceInfo& info)
{
    const aiScene* scene = aiImportFile(info.assetPath.c_str(),
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG("    ERROR: Failed to load FBX with Assimp");
        return false;
    }

    if (scene->mNumMeshes == 0)
    {
        LOG("    ERROR: FBX has no meshes");
        aiReleaseImport(scene);
        return false;
    }

    LOG("    FBX contains %d meshes", scene->mNumMeshes);

    bool allSuccess = true;

    // Process all meshes in the FBX
    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* aiMesh = scene->mMeshes[i];

        MeshData meshData;

        // Vertices
        meshData.num_vertices = aiMesh->mNumVertices;
        meshData.vertices = new float[meshData.num_vertices * 3];
        memcpy(meshData.vertices, aiMesh->mVertices, sizeof(float) * meshData.num_vertices * 3);

        // Indices
        if (aiMesh->HasFaces())
        {
            meshData.num_indices = aiMesh->mNumFaces * 3;
            meshData.indices = new unsigned int[meshData.num_indices];
            for (unsigned int j = 0; j < aiMesh->mNumFaces; j++)
            {
                memcpy(&meshData.indices[j * 3], aiMesh->mFaces[j].mIndices, 3 * sizeof(unsigned int));
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
            for (unsigned int j = 0; j < meshData.num_vertices; j++)
            {
                meshData.texCoords[j * 2] = aiMesh->mTextureCoords[0][j].x;
                meshData.texCoords[j * 2 + 1] = aiMesh->mTextureCoords[0][j].y;
            }
        }

        // Generate library path for this mesh
        std::string meshName = aiMesh->mName.C_Str();
        if (meshName.empty())
        {
            fs::path fbxPath(info.assetPath);
            meshName = fbxPath.stem().string();
            if (scene->mNumMeshes > 1)
                meshName += "_mesh_" + std::to_string(i);
        }

        std::string libraryPath = "Library/Meshes/" + meshName + ".rgs";
        meshData.libraryPath = libraryPath;

        // Save to library
        bool saved = Application::GetInstance().loadFiles->SaveMeshToCustomFormat(libraryPath.c_str(), meshData);

        if (saved)
        {
            LOG("      Saved mesh %d: %s", i, libraryPath.c_str());
        }
        else
        {
            LOG("      ERROR: Failed to save mesh %d", i);
            allSuccess = false;
        }

        // Cleanup
        delete[] meshData.vertices;
        delete[] meshData.indices;
        if (meshData.texCoords) delete[] meshData.texCoords;
        if (meshData.normals) delete[] meshData.normals;
    }

    // Also save main mesh to the expected library path for the asset
    if (scene->mNumMeshes > 0 && !fs::exists(info.libraryPath))
    {
        aiMesh* mainMesh = scene->mMeshes[0];
        MeshData meshData;

        meshData.num_vertices = mainMesh->mNumVertices;
        meshData.vertices = new float[meshData.num_vertices * 3];
        memcpy(meshData.vertices, mainMesh->mVertices, sizeof(float) * meshData.num_vertices * 3);

        if (mainMesh->HasFaces())
        {
            meshData.num_indices = mainMesh->mNumFaces * 3;
            meshData.indices = new unsigned int[meshData.num_indices];
            for (unsigned int j = 0; j < mainMesh->mNumFaces; j++)
            {
                memcpy(&meshData.indices[j * 3], mainMesh->mFaces[j].mIndices, 3 * sizeof(unsigned int));
            }
        }

        if (mainMesh->HasNormals())
        {
            meshData.hasNormals = true;
            meshData.normals = new float[meshData.num_vertices * 3];
            memcpy(meshData.normals, mainMesh->mNormals, sizeof(float) * meshData.num_vertices * 3);
        }

        if (mainMesh->HasTextureCoords(0))
        {
            meshData.hasTexCoords = true;
            meshData.texCoords = new float[meshData.num_vertices * 2];
            for (unsigned int j = 0; j < meshData.num_vertices; j++)
            {
                meshData.texCoords[j * 2] = mainMesh->mTextureCoords[0][j].x;
                meshData.texCoords[j * 2 + 1] = mainMesh->mTextureCoords[0][j].y;
            }
        }

        meshData.libraryPath = info.libraryPath;
        Application::GetInstance().loadFiles->SaveMeshToCustomFormat(info.libraryPath.c_str(), meshData);

        delete[] meshData.vertices;
        delete[] meshData.indices;
        if (meshData.texCoords) delete[] meshData.texCoords;
        if (meshData.normals) delete[] meshData.normals;
    }

    aiReleaseImport(scene);
    return allSuccess;
}

void ResourceManager::RegenerateLibrary()
{
  

    // Delete existing library folders
    if (fs::exists("Library/Meshes"))
    {
        fs::remove_all("Library/Meshes");
        LOG("Deleted Library/Meshes");
    }
    if (fs::exists("Library/Textures"))
    {
        fs::remove_all("Library/Textures");
        LOG("Deleted Library/Textures");
    }

    // Recreate folders
    fs::create_directory("Library/Meshes");
    fs::create_directory("Library/Textures");
    LOG("Recreated Library folders");

    // Process all assets
    ProcessUnprocessedAssets();

    LOG("Library regeneration complete");
}

void ResourceManager::PrintResourceStats() const
{
  
    LOG("Total resources registered: %d", (int)resources.size());

    int totalReferences = 0;
    int meshCount = 0, textureCount = 0;
    int referencedCount = 0;

    for (const auto& [id, info] : resources)
    {
        totalReferences += info.referenceCount;

        if (info.resourceType == "mesh")
            meshCount++;
        else if (info.resourceType == "texture")
            textureCount++;

        if (info.referenceCount > 0)
        {
            referencedCount++;
            LOG("  %s: %d refs (Type: %s, ID: %s)",
                fs::path(info.assetPath).filename().string().c_str(),
                info.referenceCount,
                info.resourceType.c_str(),
                info.resourceID.substr(0, 8).c_str());
        }
    }

    LOG("Resources in use: %d / %d", referencedCount, (int)resources.size());
    LOG("Total references: %d", totalReferences);
    LOG("Meshes: %d | Textures: %d", meshCount, textureCount);
   
}

std::shared_ptr<GameObject> ResourceManager::LoadFBXToScene(const std::string& assetPath)
{
    LOG("Loading FBX to scene: %s", assetPath.c_str());

    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("ERROR: Asset not found in registry: %s", assetPath.c_str());
        return nullptr;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    // Load the FBX
    auto gameObject = Application::GetInstance().loadFiles->LoadFBX(assetPath.c_str());

    if (gameObject)
    {
        // Acquire reference for the mesh(es)
        AcquireResourceReference(resourceID);
        LOG("FBX loaded successfully (References: %d)", info.referenceCount);
    }
    else
    {
        LOG("ERROR: Failed to load FBX");
    }

    return gameObject;
}

unsigned int ResourceManager::LoadTextureToGPU(const std::string& assetPath)
{
    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("ERROR: Asset not found in registry: %s", assetPath.c_str());
        return 0;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    unsigned int textureID = Application::GetInstance().loadFiles->LoadTexture(assetPath.c_str());

    if (textureID != 0)
    {
        AcquireResourceReference(resourceID);
        LOG("Texture loaded to GPU (ID: %d, References: %d)", textureID, info.referenceCount);
    }
    else
    {
        LOG("ERROR: Failed to load texture");
    }

    return textureID;
}

int ResourceManager::CountUnprocessedAssets() const
{
    int count = 0;
    for (const auto& [resourceID, info] : resources)
    {
        if (!fs::exists(info.libraryPath))
            count++;
    }
    return count;
}

bool ResourceManager::ForceProcessAsset(const std::string& assetPath)
{
    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("ERROR: Asset not found: %s", assetPath.c_str());
        return false;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    LOG("Force processing asset: %s", assetPath.c_str());

    // Delete existing library file
    if (fs::exists(info.libraryPath))
    {
        fs::remove(info.libraryPath);
        LOG("Removed existing library file: %s", info.libraryPath.c_str());
    }

    bool success = false;

    try
    {
        if (info.resourceType == "mesh")
        {
            success = ProcessMeshAsset(info);
        }
        else if (info.resourceType == "texture")
        {
            unsigned int textureID = Application::GetInstance().loadFiles->LoadTexture(assetPath.c_str());
            success = (textureID != 0);

            if (success)
                glDeleteTextures(1, &textureID);
        }
    }
    catch (const std::exception& e)
    {
        LOG("ERROR processing asset: %s", e.what());
        return false;
    }

    if (success)
    {
        info.lastModified = fs::last_write_time(assetPath).time_since_epoch().count();
        SaveMetaFile(assetPath, info);
        RefreshAssetTree();
        LOG("Asset processed successfully");
    }

    return success;
}

void ResourceManager::UpdateReferenceCounts()
{
    // Reset all reference counts
    for (auto& [id, info] : resources)
    {
        info.referenceCount = 0;
    }

    // Count references from scene
    auto scene = Application::GetInstance().scene;
    if (scene && scene->rootObject)
    {
        CountReferencesInGameObject(scene->rootObject.get());
    }
}

void ResourceManager::CountReferencesInGameObject(GameObject* go)
{
    if (!go) return;

    // Check mesh component
    ComponentMesh* mesh = go->GetComponent<ComponentMesh>();
    if (mesh && !mesh->path.empty() && mesh->path.find("Primitive_") == std::string::npos)
    {
        auto it = assetPathToID.find(mesh->path);
        if (it != assetPathToID.end())
        {
            resources[it->second].referenceCount++;
        }
    }

    // Check texture component  
    ComponentTexture* texture = go->GetComponent<ComponentTexture>();
    if (texture && !texture->path.empty() &&
        texture->path != "default_checker" &&
        !texture->useDefaultTexture)
    {
        auto it = assetPathToID.find(texture->path);
        if (it != assetPathToID.end())
        {
            resources[it->second].referenceCount++;
        }
    }

    // Recursively check children
    for (const auto& child : go->GetChildren())
    {
        CountReferencesInGameObject(child.get());
    }
}