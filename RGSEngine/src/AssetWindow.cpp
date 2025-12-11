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

    ImGui::SetNextItemWidth(-1);

    if (ImGui::Button("Refresh Assets"))
    {
        ResourceManager::GetInstance().RefreshAssetTree();
        LOG("Assets refreshed");
    }
    ImGui::SameLine();

    if (ImGui::Button("Regenerate Library"))
    {
        ResourceManager::GetInstance().RegenerateLibrary();
        LOG("Library regenerated");
    }
    ImGui::SameLine();

    if (ImGui::Button("Show Stats"))
    {
        ResourceManager::GetInstance().PrintResourceStats();
    }

    ImGui::Separator();

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

    ImGui::Checkbox("Meshes", &filterMeshes);
    ImGui::SameLine();
    ImGui::Checkbox("Textures", &filterTextures);

    ImGui::Separator();

    DrawDragDropTarget();

    ImGui::Separator();

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

    DrawAssetDetails();

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
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
            ImGui::Text("WARNING: This asset has %d active references!", refCount);
            ImGui::Text("Cannot delete while in use by GameObjects.");
            ImGui::PopStyleColor();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            ImGui::Text("This will delete:");
            ImGui::BulletText("Asset file");
            ImGui::BulletText("Library file");
            ImGui::BulletText(".meta file");
            ImGui::Spacing();

            if (ImGui::Button("Delete", ImVec2(120, 0)))
            {
                bool success = ResourceManager::GetInstance().DeleteAsset(deleteConfirmPath);

                if (success)
                {
                    selectedPath = "";
                    selectedNode = nullptr;
                }
                else
                {
                    LOG("Failed to delete asset: %s", deleteConfirmPath.c_str());
                }

                ImGui::CloseCurrentPopup();
            }
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

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (node == selectedNode)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (!node->isDirectory)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    std::string icon = node->isDirectory ? "[DIR]" : "[FIL]";
    std::string label = icon + " " + node->name;

    bool nodeOpen = ImGui::TreeNodeEx(node.get(), flags, "%s", label.c_str());

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

        ImGui::EndPopup();
    }

    // Drag source
    if (!node->isDirectory && ImGui::BeginDragDropSource())
    {
        std::string pathStr = node->path;
        ImGui::SetDragDropPayload("ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
        ImGui::Text("Dragging: %s", node->name.c_str());
        ImGui::EndDragDropSource();
    }

    if (nodeOpen && node->isDirectory)
    {
        for (const auto& child : node->children)
        {
            DrawAssetTree(child);
        }
        ImGui::TreePop();
    }
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
        if (existsInLibrary)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
            ImGui::Text("Status: PROCESSED");
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.2f, 1.0f));
            ImGui::Text("Status: NOT PROCESSED");
            ImGui::PopStyleColor();

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

        ImGui::SameLine();

     
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
    }
    else
    {
        LOG("ERROR: Failed to import asset");
    }
}