#pragma once

#include "imgui.h"
#pragma once

#include "imgui.h"
#include <memory>
#include <string>

struct AssetNode;

class AssetWindow
{
public:
    AssetWindow();
    ~AssetWindow();

    void Draw(bool* pOpen);

    bool IsOpen() const { return isOpen; }
    void SetOpen(bool open) { isOpen = open; }

private:
    bool isOpen = true;

    // Selection state
    std::shared_ptr<AssetNode> selectedNode;
    std::string selectedPath;

    // UI Methods
    void DrawAssetTree(std::shared_ptr<AssetNode> node);
    void DrawAssetDetails();
    void DrawDragDropTarget();

    // Interaction Methods
    void HandleDragDrop(const std::string& filePath);

    // Delete confirmation
    bool showDeleteConfirm = false;
    std::string deleteConfirmPath;

    // Filters
    bool filterMeshes = true;
    bool filterTextures = true;
};