#pragma once

#include "imgui.h"
#include <memory>
#include <string>
#include <vector>

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
    // UI
    bool isOpen = true;

    // selection state
    std::shared_ptr<AssetNode> selectedNode;
    std::string selectedPath;

    // Methods of UI
    void DrawAssetTree(std::shared_ptr<AssetNode> node);
    void DrawAssetDetails();
    void DrawContextMenu();
    void DrawDragDropTarget();

    // Methods of interaction
    bool HandleDelete();
    bool HandleImport();
    void HandleDragDrop(const std::string& filePath);


    void HandleAssetDragDrop();

    // State variables
    bool showDeleteConfirm = false;
    std::string deleteConfirmPath;

    // Filters
    bool filterMeshes = true;
    bool filterTextures = true;
};