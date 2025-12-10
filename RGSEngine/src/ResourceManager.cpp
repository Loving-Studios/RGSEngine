#include "ResourceManager.h"
#include "Log.h"
#include "LoadFiles.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "Application.h"

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
        LOG("Asset not found in registry: %s", assetPath.c_str());
        return false;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    if (info.referenceCount > 0)
    {
        LOG("Cannot delete asset with active references (%d)", info.referenceCount);
        LOG("This asset is currently being used by GameObjects in the scene");
        return false;
    }

    try
    {
        bool allDeleted = true;

        if (fs::exists(assetPath))
        {
            fs::remove(assetPath);
            LOG("Deleted asset file: %s", assetPath.c_str());
        }
        else
        {
            LOG("Asset file not found: %s", assetPath.c_str());
            allDeleted = false;
        }

        if (fs::exists(info.libraryPath))
        {
            fs::remove(info.libraryPath);
            LOG("Deleted library file: %s", info.libraryPath.c_str());
        }
        else
        {
            LOG("Library file not found: %s", info.libraryPath.c_str());
        }

        std::string metaPath = assetPath + ".meta";
        if (fs::exists(metaPath))
        {
            fs::remove(metaPath);
            LOG("Deleted meta file: %s", metaPath.c_str());
        }
        else
        {
            LOG("Meta file not found: %s", metaPath.c_str());
        }

        resources.erase(resourceID);
        assetPathToID.erase(assetPath);

        LOG("Removed from internal registry");

        RefreshAssetTree();
        LOG("Asset tree refreshed");

        LOG("Asset deleted successfully");
        return allDeleted;
    }
    catch (const std::exception& e)
    {
        LOG("Error deleting asset: %s", e.what());
        return false;
    }
}

bool ResourceManager::ImportAsset(const std::string& sourcePath)
{
    LOG("Importing asset: %s", sourcePath.c_str());

    if (!fs::exists(sourcePath))
    {
        LOG("Source file does not exist: %s", sourcePath.c_str());
        return false;
    }

    try
    {
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
    LOG("Processing Unprocessed Assets");

    auto& app = Application::GetInstance();
    if (!app.loadFiles)
    {
        LOG("LoadFiles module not initialized");
        return;
    }

    int processed = 0;
    int failed = 0;
    int skipped = 0;

    for (auto& [resourceID, info] : resources)
    {
        if (!fs::exists(info.libraryPath))
        {
            LOG("Processing unprocessed asset: %s", info.assetPath.c_str());
            LOG("  Type: %s", info.resourceType.c_str());
            LOG("  Library path: %s", info.libraryPath.c_str());

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

                    if (success)
                        LOG("Mesh processed and saved to Library");
                    else
                        LOG("Failed to process mesh");
                }
                else if (info.resourceType == "texture")
                {
                    unsigned int textureID = app.loadFiles->LoadTexture(info.assetPath.c_str());

                    if (textureID != 0 && fs::exists(info.libraryPath))
                    {
                        LOG("Texture processed and saved to Library");
                        success = true;
                        glDeleteTextures(1, &textureID);
                    }
                    else
                    {
                        LOG("Failed to process texture");
                    }
                }
                else
                {
                    LOG("Unknown resource type '%s'", info.resourceType.c_str());
                    skipped++;
                    continue;
                }

                if (success)
                {
                    processed++;
                    info.lastModified = fs::last_write_time(info.assetPath).time_since_epoch().count();
                    SaveMetaFile(info.assetPath, info);
                }
                else
                {
                    failed++;
                }
            }
            catch (const std::exception& e)
            {
                LOG("Processing asset: %s", e.what());
                failed++;
            }
        }
        else
        {
            skipped++;
        }
    }

    LOG("Asset Processing Complete");
    LOG("Processed: %d | Failed: %d | Skipped: %d", processed, failed, skipped);

    if (failed > 0)
        LOG("Some assets failed to process.");

    if (processed > 0)
    {
        LOG("%d new assets added to Library/", processed);
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
        return false;

    if (scene->mNumMeshes == 0)
    {
        LOG("FBX has no meshes");
        aiReleaseImport(scene);
        return false;
    }

    LOG("  FBX contains %d meshes", scene->mNumMeshes);

    bool allSuccess = true;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* aiMesh = scene->mMeshes[i];

        LOG("    Processing mesh %d/%d: %s", i + 1, scene->mNumMeshes, aiMesh->mName.C_Str());

        MeshData meshData;

        meshData.num_vertices = aiMesh->mNumVertices;
        meshData.vertices = new float[meshData.num_vertices * 3];
        memcpy(meshData.vertices, aiMesh->mVertices, sizeof(float) * meshData.num_vertices * 3);

        if (aiMesh->HasFaces())
        {
            meshData.num_indices = aiMesh->mNumFaces * 3;
            meshData.indices = new unsigned int[meshData.num_indices];
            for (unsigned int j = 0; j < aiMesh->mNumFaces; j++)
            {
                memcpy(&meshData.indices[j * 3], aiMesh->mFaces[j].mIndices, 3 * sizeof(unsigned int));
            }
        }

        if (aiMesh->HasNormals())
        {
            meshData.hasNormals = true;
            meshData.normals = new float[meshData.num_vertices * 3];
            memcpy(meshData.normals, aiMesh->mNormals, sizeof(float) * meshData.num_vertices * 3);
        }

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

        bool saved = Application::GetInstance().loadFiles->SaveMeshToCustomFormat(libraryPath.c_str(), meshData);

        if (saved)
            LOG("      Saved: %s", libraryPath.c_str());
        else
        {
            LOG("      ERROR: Failed to save %s", libraryPath.c_str());
            allSuccess = false;
        }

        delete[] meshData.vertices;
        delete[] meshData.indices;
        if (meshData.texCoords) delete[] meshData.texCoords;
        if (meshData.normals) delete[] meshData.normals;
    }

    aiReleaseImport(scene);

    if (scene->mNumMeshes > 0 && !fs::exists(info.libraryPath))
    {
        const aiScene* scene2 = aiImportFile(info.assetPath.c_str(),
            aiProcess_Triangulate | aiProcess_FlipUVs |
            aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);

        if (scene2 && scene2->mNumMeshes > 0)
        {
            aiMesh* mainMesh = scene2->mMeshes[0];
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

            aiReleaseImport(scene2);
        }
    }

    return allSuccess;
}

void ResourceManager::RegenerateLibrary()
{
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
}

std::shared_ptr<GameObject> ResourceManager::LoadFBXToScene(const std::string& assetPath)
{
    LOG("Loading FBX to scene: %s", assetPath.c_str());

    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("Error Asset not found: %s", assetPath.c_str());
        return nullptr;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    auto gameObject = Application::GetInstance().loadFiles->LoadFBX(assetPath.c_str());

    if (gameObject)
    {
        AcquireResourceReference(resourceID);
        LOG("FBX loaded to scene: %s (References: %d)", assetPath.c_str(), info.referenceCount);
    }

    return gameObject;
}

unsigned int ResourceManager::LoadTextureToGPU(const std::string& assetPath)
{
    auto it = assetPathToID.find(assetPath);
    if (it == assetPathToID.end())
    {
        LOG("Error Asset not found: %s", assetPath.c_str());
        return 0;
    }

    std::string resourceID = it->second;
    ResourceInfo& info = resources[resourceID];

    unsigned int textureID = Application::GetInstance().loadFiles->LoadTexture(assetPath.c_str());

    if (textureID != 0)
    {
        AcquireResourceReference(resourceID);
        LOG("Texture loaded to GPU: %s (ID: %d, References: %d)",
            assetPath.c_str(), textureID, info.referenceCount);
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

    if (fs::exists(info.libraryPath))
    {
        fs::remove(info.libraryPath);
        LOG("Removed existing library file for reimport: %s", info.libraryPath.c_str());
    }

    if (!Application::GetInstance().loadFiles)
    {
        LOG("Error: LoadFiles module not initialized");
        return false;
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
        LOG("Error processing asset: %s", e.what());
        return false;
    }

    if (success)
    {
        info.lastModified = fs::last_write_time(assetPath).time_since_epoch().count();
        SaveMetaFile(assetPath, info);
        RefreshAssetTree();
        LOG("Asset processed successfully: %s", assetPath.c_str());
    }

    return success;
}