#pragma once

#include "Module.h"
#include "imgui.h"
#include <iostream>
#include <sstream>
#include <vector>

class GameObject;

class ModuleEditor : public Module
{
public:
    ModuleEditor();
    ~ModuleEditor();

    bool Start() override;
    bool PreUpdate() override;
    bool Update(float dt) override;
    bool PostUpdate() override;
    bool CleanUp() override;

private:
    // Functions to draw the windows
    void DrawMainMenuBar();
    void DrawHierarchyWindow();
    void DrawInspectorWindow();
    void DrawConsoleWindow();
    void DrawConfigurationWindow();
    void DrawAboutWindow();

    // Funtion recursive for the Hierarchy Window
    void DrawHierarchyNode(GameObject* go);

    void ApplyDefaultDockingLayout();

    void UpdateMemoryStats();

    bool showDemoWindow = false;
    bool firstTimeLayout = true;

    bool showHierarchyWindow = true;
    bool showInspectorWindow = true;
    bool showConsoleWindow = true;
    bool showConfigurationWindow = true;
    bool showAboutWindow = false;

    // Buffer for the console
    std::streambuf* oldCerrStreamBuf;
    std::stringstream consoleStream;

    // FPS graph
    std::vector<float> fpsLog;

    // The GameObject selected
    GameObject* selectedGameObject = nullptr;

    bool isNVIDIA = false;     // So we don't need to call glGetString on each frame
    int vram_budget_mb = 0;    // VRAM Total
    int vram_available_mb = 0; // VRAM Available
    int ram_usage_mb = 0;      // RAM used on the process
};