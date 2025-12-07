#include "ResourceStatsWindow.h"
#include "ResourceManager.h"
#include "Log.h"
#include <algorithm>

void ResourceStatsWindow::Draw(bool* pOpen)
{
    if (!ImGui::Begin("Resource Statistics", pOpen))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Refresh"))
    {
        Refresh();
    }

    ImGui::Separator();

    // global statistics
    const auto& resources = ResourceManager::GetInstance().GetAllResources();

    int totalResources = resources.size();
    int totalReferences = 0;
    int meshCount = 0, textureCount = 0;
    int referencedResources = 0;

    for (const auto& [id, info] : resources)
    {
        totalReferences += info.referenceCount;
        if (info.referenceCount > 0)
            referencedResources++;

        if (info.resourceType == "mesh")
            meshCount++;
        else if (info.resourceType == "texture")
            textureCount++;
    }

    ImGui::Text("Total Resources: %d", totalResources);
    ImGui::Text("Total References: %d", totalReferences);
    ImGui::Text("Resources in Use: %d / %d", referencedResources, totalResources);

    ImGui::Columns(4, "ResourceTable", true);
    ImGui::Text("Name"); ImGui::NextColumn();
    ImGui::Text("Type"); ImGui::NextColumn();
    ImGui::Text("References"); ImGui::NextColumn();
    ImGui::Text("Path"); ImGui::NextColumn();
    ImGui::Separator();

    for (const auto& [id, info] : resources)
    {
        std::string name = std::filesystem::path(info.assetPath).filename().string();

        ImGui::Text("%s", name.c_str()); ImGui::NextColumn();

        if (info.resourceType == "mesh")
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Mesh");
        else if (info.resourceType == "texture")
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Texture");
        else
            ImGui::Text("Unknown");
        ImGui::NextColumn();

      
        if (info.referenceCount == 0)
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "0");
        else if (info.referenceCount == 1)
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%d", info.referenceCount);
        else
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "%d", info.referenceCount);
        ImGui::NextColumn();

        ImGui::Text("%s", info.assetPath.c_str()); ImGui::NextColumn();
    }

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::Text("Meshes: %d | Textures: %d", meshCount, textureCount);
    ImGui::PopStyleColor();

    ImGui::End();
}

void ResourceStatsWindow::Refresh()
{
    ResourceManager::GetInstance().PrintResourceStats();
}