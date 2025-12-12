#include "AssetWindow.h"
#include "ResourceManager.h"
#include "Log.h"
#include "Application.h"          
#include "ModuleScene.h"   

AssetWindow::AssetWindow()
{
}

AssetWindow::~AssetWindow()
{
}

void AssetWindow::Draw(bool* pOpen)
{
    if (!ImGui::Begin("Assets", pOpen, ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }

    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter >= 100)
    {
        ResourceManager::GetInstance().UpdateReferenceCounts();
        frameCounter = 0;
    }

    ImGui::SetNextItemWidth(-1);

    // Button: Refresh Assets
    if (ImGui::Button("Refresh Assets"))
    {
        ResourceManager::GetInstance().RefreshAssetTree();
        LOG("Assets refreshed");
    }
    ImGui::SameLine();

    // Button: Regenerate Library
    if (ImGui::Button("Regenerate Library"))
    {
        ResourceManager::GetInstance().RegenerateLibrary();
        LOG("Library regenerated");
    }
    ImGui::SameLine();

    // Button: Update Reference Counts
    if (ImGui::Button("Update References"))
    {
        ResourceManager::GetInstance().UpdateReferenceCounts();
        LOG("Reference counts updated from scene");
    }
    ImGui::SameLine();

    // Button: Show Stats
    if (ImGui::Button("Show Stats"))
    {
        ResourceManager::GetInstance().PrintResourceStats();
    }

    ImGui::Separator();

    // Check for unprocessed assets
    int unprocessed = ResourceManager::GetInstance().CountUnprocessedAssets();
    if (unprocessed > 0)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.2f, 1.0f));
        ImGui::Text("WARNING: %d unprocessed assets in Library!", unprocessed);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("Process Now"))
        {
            ResourceManager::GetInstance().ProcessUnprocessedAssets();
        }
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
        ImGui::Text("All assets processed correctly");
        ImGui::PopStyleColor();
    }

    ImGui::Separator();

    // Filters (for future implementation)
    ImGui::Checkbox("Meshes", &filterMeshes);
    ImGui::SameLine();
    ImGui::Checkbox("Textures", &filterTextures);

    ImGui::Separator();

    // Drag & Drop target area
    DrawDragDropTarget();

    ImGui::Separator();

    // Asset tree view
    if (ImGui::BeginChild("AssetTree", ImVec2(0, -100), true))
    {
        auto root = ResourceManager::GetInstance().GetAssetTree();
        if (root)
        {
            DrawAssetTree(root);
        }
    }
    ImGui::EndChild();

    ImGui::Separator();

    // Asset details panel
    DrawAssetDetails();

    // Delete confirmation popup
    if (showDeleteConfirm)
    {
        ImGui::OpenPopup("Delete Confirmation");
        showDeleteConfirm = false;
    }

    if (ImGui::BeginPopupModal("Delete Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure you want to delete this asset?");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "%s", deleteConfirmPath.c_str());
        ImGui::Spacing();

        // Find reference count for this asset
        int refCount = 0;
        for (const auto& [id, info] : ResourceManager::GetInstance().GetAllResources())
        {
            if (info.assetPath == deleteConfirmPath)
            {
                refCount = info.referenceCount;
                break;
            }
        }

        if (refCount > 0)
        {
            // Asset is in use - cannot delete
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            ImGui::Text("WARNING: This asset has %d active references!", refCount);
            ImGui::Text("Cannot delete while in use by GameObjects.");
            ImGui::Text("Remove the asset from all GameObjects first.");
            ImGui::PopStyleColor();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            // Asset not in use - can delete
            ImGui::Text("This will delete:");
            ImGui::BulletText("Asset file in Assets/");
            ImGui::BulletText("Processed file in Library/");
            ImGui::BulletText(".meta file");
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(120, 0)))
            {
                bool success = ResourceManager::GetInstance().DeleteAsset(deleteConfirmPath);

                if (success)
                {
                    LOG("Asset deleted successfully: %s", deleteConfirmPath.c_str());
                    selectedPath = "";
                    selectedNode = nullptr;
                }
                else
                {
                    LOG("Failed to delete asset: %s", deleteConfirmPath.c_str());
                }

                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor();

            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void AssetWindow::DrawAssetTree(std::shared_ptr<AssetNode> node)
{
    if (!node) return;

    ImGui::PushID(node.get());

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    // Select current node
    if (node == selectedNode)
        flags |= ImGuiTreeNodeFlags_Selected;

    // Leaf node (file)
    if (!node->isDirectory)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    // Icon for directory vs file
    std::string icon = node->isDirectory ? "[DIR]" : "[FILE]";

    // Show reference count for files
    std::string label = icon + " " + node->name;
    if (!node->isDirectory && node->resourceInfo && node->resourceInfo->referenceCount > 0)
    {
        label += " (" + std::to_string(node->resourceInfo->referenceCount) + " refs)";
    }

    bool nodeOpen = ImGui::TreeNodeEx(label.c_str(), flags);

    // Handle selection
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        selectedNode = node;
        selectedPath = node->path;
    }

    // Context menu
    if (ImGui::BeginPopupContextItem())
    {
        if (!node->isDirectory && ImGui::MenuItem("Delete"))
        {
            deleteConfirmPath = node->path;
            showDeleteConfirm = true;
        }

        if (!node->isDirectory && ImGui::MenuItem("Reimport"))
        {
            ResourceManager::GetInstance().ForceProcessAsset(node->path);
            LOG("Asset reimported: %s", node->path.c_str());
        }

        ImGui::EndPopup();
    }

    // Drag source (only for files)
    if (!node->isDirectory && ImGui::BeginDragDropSource())
    {
        std::string pathStr = node->path;
        ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
        ImGui::Text("Dragging: %s", node->name.c_str());
        ImGui::EndDragDropSource();
    }

    // Draw children if directory is open
    if (nodeOpen && node->isDirectory)
    {
        for (const auto& child : node->children)
        {
            DrawAssetTree(child);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void AssetWindow::DrawAssetDetails()
{
    if (!selectedNode || selectedNode->isDirectory)
    {
        ImGui::Text("Select an asset to view details");
        return;
    }

    ImGui::Text("Asset: %s", selectedNode->name.c_str());
    ImGui::Text("Path: %s", selectedPath.c_str());

    if (selectedNode->resourceInfo)
    {
        const auto& info = *selectedNode->resourceInfo;

        ImGui::Separator();
        ImGui::Text("Type: %s", info.resourceType.c_str());
        ImGui::Text("Library: %s", info.libraryPath.c_str());
        ImGui::Text("ID: %s...", info.resourceID.substr(0, 12).c_str());

        // Processing status
        bool existsInLibrary = std::filesystem::exists(info.libraryPath);
        ImGui::Text("Status: ");
        ImGui::SameLine();
        if (existsInLibrary)
        {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "PROCESSED");
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "NOT PROCESSED");

            if (ImGui::Button("Process Asset", ImVec2(150, 0)))
            {
                ResourceManager::GetInstance().ForceProcessAsset(selectedPath);
                LOG("Forced processing of: %s", selectedPath.c_str());
            }
        }

        ImGui::Separator();

        // Action buttons
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        if (ImGui::Button("Load to Scene", ImVec2(150, 0)))
        {
            if (info.resourceType == "mesh")
            {
                auto gameObject = ResourceManager::GetInstance().LoadFBXToScene(selectedPath);
                if (gameObject)
                {
                    Application::GetInstance().scene->AddGameObject(gameObject);
                    LOG("FBX loaded to scene: %s", selectedPath.c_str());

                    // Update reference counts
                    ResourceManager::GetInstance().UpdateReferenceCounts();
                }
                else
                {
                    LOG("ERROR: Failed to load FBX: %s", selectedPath.c_str());
                }
            }
            else if (info.resourceType == "texture")
            {
                LOG("Texture selection: Drag to GameObject instead.");
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // Disable delete button if has references
        if (info.referenceCount > 0)
        {
            ImGui::BeginDisabled();
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Delete Asset", ImVec2(150, 0)))
        {
            deleteConfirmPath = selectedPath;
            showDeleteConfirm = true;
        }
        ImGui::PopStyleColor();

        if (info.referenceCount > 0)
        {
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("Cannot delete: Asset is in use (%d references)", info.referenceCount);
            }
        }
    }
}

void AssetWindow::DrawDragDropTarget()
{
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.8f, 1.0f, 0.5f));
    ImGui::BeginChild("DropTarget", ImVec2(0, 60), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::Text("Drag files here to import them");
    ImGui::Text("(FBX, PNG, JPG, etc.)");

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_DROP"))
        {
            const char* filePath = (const char*)payload->Data;
            HandleDragDrop(std::string(filePath));
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void AssetWindow::HandleDragDrop(const std::string& filePath)
{
    LOG("Importing dropped file: %s", filePath.c_str());

    if (ResourceManager::GetInstance().ImportAsset(filePath))
    {
        LOG("Asset imported successfully");

        // Process immediately if needed
        int unprocessed = ResourceManager::GetInstance().CountUnprocessedAssets();
        if (unprocessed > 0)
        {
            LOG("Processing newly imported asset...");
            ResourceManager::GetInstance().ProcessUnprocessedAssets();
        }
    }
    else
    {
        LOG("ERROR: Failed to import asset");
    }
}