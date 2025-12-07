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

   
    ImGui::Checkbox("Meshes", &filterMeshes);
    ImGui::SameLine();
    ImGui::Checkbox("Textures", &filterTextures);

    ImGui::Separator();

    // Drop area
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
        ImGui::Text("Are you sure you want to delete this asset?\n%s", deleteConfirmPath.c_str());
        ImGui::Spacing();

        if (ImGui::Button("Delete", ImVec2(120, 0)))
        {
            ResourceManager::GetInstance().DeleteAsset(deleteConfirmPath);
            selectedPath = "";
            selectedNode = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
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

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("References: %d", info.referenceCount);
        ImGui::PopStyleColor();

        ImGui::Separator();
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
                LOG("Texture selection not supported yet. Drag to GameObject instead.");
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();

        if (ImGui::Button("Delete Asset", ImVec2(150, 0)))
        {
            deleteConfirmPath = selectedPath;
            showDeleteConfirm = true;
        }

        ImGui::SameLine();

        if (ImGui::Button("Copy Path", ImVec2(150, 0)))
        {
            ImGui::SetClipboardText(selectedPath.c_str());
            LOG("Copied path to clipboard: %s", selectedPath.c_str());
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

bool AssetWindow::HandleDelete()
{
    if (selectedPath.empty()) return false;

    bool success = ResourceManager::GetInstance().DeleteAsset(selectedPath);
    if (success)
    {
        selectedPath = "";
        selectedNode = nullptr;
    }
    return success;
}

bool AssetWindow::HandleImport()
{
        return true;
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

void AssetWindow::HandleAssetDragDrop()
{
    
}