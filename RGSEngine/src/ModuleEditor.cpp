#include <SDL3/SDL.h>
#include <SDL3/SDL_version.h>
#include <glad/glad.h>

#include <windows.h>
#include <psapi.h>
#include <commdlg.h>

#include "ModuleEditor.h"

#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include "Application.h"
#include "Window.h"
#include "Log.h"
#include "Render.h"
#include "Input.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "ComponentCamera.h"
#include <ImGuizmo.h>
#include "LoadFiles.h"
#include "Time.h"
#include "ResourceManager.h"
#include "Ray.h"

#include <IL/il.h>
#include <glm/gtc/type_ptr.hpp>

static bool IsAncestor(GameObject* parent, GameObject* child)
{
    if (parent == nullptr || child == nullptr) return false;

    GameObject* iterator = child->GetParent();
    while (iterator != nullptr)
    {
        if (iterator == parent) return true;
        iterator = iterator->GetParent();
    }
    return false;
}

static bool CheckIfGameObjectExists(GameObject* target, GameObject* root);

ModuleEditor::ModuleEditor() : Module()
{
    name = "editor";
}

ModuleEditor::~ModuleEditor()
{
}

// Static function to open the explorer window of Windows
std::string OpenFileDialog(const char* filter)
{
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);

    // Get window and HWND
    Window* winModule = Application::GetInstance().window.get();
    SDL_Window* sdlWindow = winModule->window;
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlWindow), "SDL.window.win32.hwnd", NULL);

    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = "";

    bool wasFullscreen = winModule->fullscreen;

    if (wasFullscreen)
    {
        // Exit full screen mode
        winModule->SetFullscreen(false);

        for (int i = 0; i < 10; ++i)
        {
            SDL_PumpEvents();
            // Force a swap so that Windows redraws the window frame
            SDL_GL_SwapWindow(sdlWindow);
            SDL_Delay(10);
        }
    }

    // Open the dialogue window
    BOOL result = GetOpenFileNameA(&ofn);

    // Return to full screen if necessary
    if (wasFullscreen)
    {
        winModule->SetFullscreen(true);
        for (int i = 0; i < 5; ++i)
        {
            SDL_PumpEvents();
            SDL_Delay(10);
        }
    }

    if (result)
    {
        return std::string(fileName);
    }
    return std::string("");
}

std::string SaveFileDialog(const char* filter)
{
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);

    // Get window and HWND
    Window* winModule = Application::GetInstance().window.get();
    SDL_Window* sdlWindow = winModule->window;
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlWindow), "SDL.window.win32.hwnd", NULL);

    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    // OFN_OVERWRITEPROMPT
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "json";

    bool wasFullscreen = winModule->fullscreen;

    if (wasFullscreen)
    {
        winModule->SetFullscreen(false);
        for (int i = 0; i < 10; ++i)
        {
            SDL_PumpEvents();
            SDL_GL_SwapWindow(sdlWindow);
            SDL_Delay(10);
        }
    }

    BOOL result = GetSaveFileNameA(&ofn);

    if (wasFullscreen)
    {
        winModule->SetFullscreen(true);
        for (int i = 0; i < 5; ++i)
        {
            SDL_PumpEvents();
            SDL_Delay(10);
        }
    }

    if (result)
    {
        return std::string(fileName);
    }
    return std::string("");
}

bool ModuleEditor::Start()
{
    if (Application::GetInstance().isGameMode)
    {
        LOG("Skipping ModuleEditor (Game Mode)");
        return true;
    }

    LOG("ModuleEditor Start");

    // Create the ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Activation of the features que want
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Viewports, windows outside of the main

    // ImGui Style
    ImGui::StyleColorsDark();

    // Initialize the bindings for SDL3 and OpenGL3
    SDL_Window* window = Application::GetInstance().window->window;
    SDL_GLContext glContext = Application::GetInstance().window->glContext;

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 460"); // Force to use the same version of the shader

    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    mCurrentGizmoMode = ImGuizmo::WORLD;

    // Only check once if it's NVIDIA
    if (strstr((const char*)glGetString(GL_VENDOR), "NVIDIA"))
    {
        isNVIDIA = true;
    }

    assetWindow = std::make_unique<AssetWindow>();
    resourceStatsWindow = std::make_unique<ResourceStatsWindow>();

    return true;
}

bool ModuleEditor::PreUpdate()
{
    // New ImGui Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGuizmo::BeginFrame();

    return true;
}

bool ModuleEditor::Update(float dt)
{
    // --- HISTORIC OF FPS ---
    // Limited to 100 frames
    if (fpsLog.size() < 100)
    {
        fpsLog.push_back(1.0f / dt);
    }
    else
    {
        // Desplazar el vector
        fpsLog.erase(fpsLog.begin());
        fpsLog.push_back(1.0f / dt);
    }
    // Update stats of memory on each frame
    UpdateMemoryStats();

    ModuleScene* scene = Application::GetInstance().scene.get();
    ModuleScene::SimulationState state = scene->GetSimulationState();


    static ModuleScene::SimulationState previousState = ModuleScene::SimulationState::STOPPED;

    if (previousState != ModuleScene::SimulationState::STOPPED &&
        state == ModuleScene::SimulationState::STOPPED)
    {
        LOG("Editor detected STOP - clearing selection");
        selectedGameObject = nullptr;
        Application::GetInstance().render->selectedObject = nullptr;
    }

    

   
    if (previousState == ModuleScene::SimulationState::STOPPED &&  state == ModuleScene::SimulationState::PLAYING)
    {
        
        // Reset del timer
        simulationElapsedTime = 0.0f;
        simulationStartTime = Time::realTimeSinceStartup;
        LOG("Timer started");
    }

    if (state == ModuleScene::SimulationState::PLAYING)
    {
        simulationElapsedTime += Time::realDeltaTime;
    }

    else if (state == ModuleScene::SimulationState::STOPPED)
    {
        simulationElapsedTime = 0.0f;
        simulationStartTime = 0.0f;
    }

    previousState = state;

    // --- FOCUS ON SELECTED GAMEOBJECT ---

    // --- MOUSE PICKING ---
    Input* input = Application::GetInstance().input.get();
    ImGuiIO& io = ImGui::GetIO();

    // Only perform picking if we are NOT on the ImGui interface
    if (!io.WantCaptureMouse)
    {
        // Left click to select objects
        if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
        {
            // Get mouse position
            int mouseX, mouseY;
            input->GetMousePosition(mouseX, mouseY);

            // Obtain window dimensions
            int screenWidth, screenHeight;
            Application::GetInstance().window->GetWindowSize(screenWidth, screenHeight);

            // Obtain camera matrices
            const glm::mat4& viewMatrix = Application::GetInstance().render->GetViewMatrix();
            const glm::mat4& projectionMatrix = Application::GetInstance().render->GetProjectionMatrix();

            // Generate the ray from the camera to the mouse
            Ray pickRay = Raycast::ScreenPointToRay(
                mouseX, mouseY,
                screenWidth, screenHeight,
                viewMatrix, projectionMatrix
            );


            GameObject* hitObject = nullptr;
            float closestDistance = std::numeric_limits<float>::max();

            ModuleScene* scene = Application::GetInstance().scene.get();

            
            if (scene->useOctree && scene->octree && scene->octree->IsInitialized())
            {
                // Octree query - only objects close to the ray
                std::vector<GameObject*> candidates = scene->octree->QueryRay(pickRay);

                LOG("Octree mouse picking: %d candidates (from %d total objects)",
                    (int)candidates.size(), scene->octree->GetObjectCount());

                // Test each candidate
                for (GameObject* candidate : candidates)
                {
                    if (candidate == nullptr) continue;

                    ComponentMesh* mesh = candidate->GetComponent<ComponentMesh>();
                    if (mesh == nullptr) continue;

                    float distance = 0.0f;
                    if (Raycast::IntersectGameObject(pickRay, candidate, distance))
                    {
                        if (distance < closestDistance)
                        {
                            closestDistance = distance;
                            hitObject = candidate;
                        }
                    }
                }
            }
            else
            {
                // Traditional method without Octree
                GameObject* root = scene->rootObject.get();
                hitObject = Raycast::FindClosestIntersection(pickRay, root, closestDistance);
            }

            // Select object
            if (hitObject != nullptr && hitObject->GetName() != "SceneRoot")
            {
                selectedGameObject = hitObject;
                Application::GetInstance().render->selectedObject = hitObject;
                LOG("Selected: %s (distance: %.2f)", hitObject->GetName().c_str(), closestDistance);

                scrollToSelection = true;
            }
            else
            {
                selectedGameObject = nullptr;
                Application::GetInstance().render->selectedObject = nullptr;
                LOG("Deselected (no hit)");
            }
        }
    }

    // --- FOCUS ON SELECTED GAMEOBJECT ---
    //Input* input = Application::GetInstance().input.get();

    // Process F key if an object is selected
    if (selectedGameObject != nullptr)
    {
        if (input->GetKey(SDL_SCANCODE_F) == KEY_DOWN)
        {
            Application::GetInstance().render->FocusOnGameObject(selectedGameObject);
        }
        // Set object as orbit target when selected
        Application::GetInstance().render->SetOrbitTarget(selectedGameObject);
    }
    else
    {
        // If nothing is selected, clear the orbit target
        Application::GetInstance().render->SetOrbitTarget(nullptr);
    }

    // ImGuizmo
    if (!ImGui::IsAnyItemActive())
    {
        if (input->GetKey(SDL_SCANCODE_W) == KEY_DOWN)
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        if (input->GetKey(SDL_SCANCODE_E) == KEY_DOWN)
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        if (input->GetKey(SDL_SCANCODE_R) == KEY_DOWN)
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        if (input->GetKey(SDL_SCANCODE_Q) == KEY_DOWN && selectedGameObject != nullptr)
        {
            selectedGameObject = nullptr;
            // Disable the ImGuizmo so it's not floating with anything attached
            mCurrentGizmoOperation = (ImGuizmo::OPERATION)-1;
        }
    }

    // --- DRAW THE INTERFACE ---
    // Configuration of the window that occupies the entire main screen
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Removed all the borders, titlebars and everything
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    window_flags |= ImGuiWindowFlags_NoBackground;

    // No padding and Rounding
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    // Start the Container Window
    ImGui::Begin("DockSpace", nullptr, window_flags);

    ImGui::PopStyleVar(3);

    // ImGuizmo only used if there is a GameObject selected with transform
    ComponentTransform* targetTransform = (selectedGameObject) ? selectedGameObject->GetComponent<ComponentTransform>() : nullptr;
    if (targetTransform != nullptr)
    {
        // ImGuizmo area of drawing
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);

        // Obtain the camera matrix from render
        const glm::mat4& viewMatrix = Application::GetInstance().render->GetViewMatrix();
        const glm::mat4& projectionMatrix = Application::GetInstance().render->GetProjectionMatrix();

        // Obtain the global matrix of the object
        glm::mat4 modelMatrix = selectedGameObject->GetGlobalMatrix();

        // Call the funtion Manipulate() and glm::value_ptr converts GLM matrix to ImGuizmo float
        ImGuizmo::Manipulate(glm::value_ptr(viewMatrix),
            glm::value_ptr(projectionMatrix),
            mCurrentGizmoOperation,
            mCurrentGizmoMode,
            glm::value_ptr(modelMatrix));

        // Check if the user has used the ImGuizmo
        if (ImGuizmo::IsUsing())
        {
            // If is moved, update the transform of the object
            selectedGameObject->SetLocalFromGlobal(modelMatrix);
            selectedGameObject->UpdateAABBRecursive();

            ModuleScene* scene = Application::GetInstance().scene.get();
            if (scene->useOctree && scene->octree)
            {
                scene->octree->Update(selectedGameObject);
            }
        }
    }

    // --- WINDOWS ---
    // Main menu is drawed inside ImGui::Begin
    DrawMainMenuBar();

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    bool shouldFocusInspector = false;

    // If it's the first time opening applies the default view
    if (firstTimeLayout)
    {
        ApplyDefaultDockingLayout();
        firstTimeLayout = false;
        shouldFocusInspector = true;
    }

    // Show the demo window at the beginning
    if (showDemoWindow)
        ImGui::ShowDemoWindow(&showDemoWindow);

    if (showHierarchyWindow)
        DrawHierarchyWindow();

    if (showInspectorWindow)
        DrawInspectorWindow();

    if (showConsoleWindow)
        DrawConsoleWindow();

    if (showConfigurationWindow)
        DrawConfigurationWindow();

    if (showAssetWindow && assetWindow)
        assetWindow->Draw(&showAssetWindow);

    if (showResourceStatsWindow && resourceStatsWindow)
        resourceStatsWindow->Draw(&showResourceStatsWindow);

    if (showAboutWindow)
        DrawAboutWindow();

    if (shouldFocusInspector)
    {
        ImGui::SetWindowFocus("Inspector");
    }

    if (showTimeDebugWindow)
        DrawTimeDebugWindow();

    if (showTimerWindow)
        DrawTimerWindow();

    if (showPerformanceWindow)
        DrawPerformanceWindow();

    if (showOctreeDebugWindow)
        DrawOctreeDebugWindow();

    // Close the container window
    ImGui::End();

    return true;
}

bool ModuleEditor::PostUpdate()
{
    // Render all ImGui drawing commands
    ImGui::Render();
    // The real drawing is made on ModuleRender::PostUpdate

    // If the viewports are enabled, ImGui need to update the extra windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    return true;
}

bool ModuleEditor::CleanUp()
{
    LOG("ModuleEditor CleanUp");

   
    assetWindow.reset();
    resourceStatsWindow.reset();

    // Clean ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    return true;
}

void ModuleEditor::DrawMainMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        // --- File Menu ---
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene"))
            {
                std::string path = SaveFileDialog("JSON Files\0*.json\0All Files\0*.*\0");
                if (!path.empty())
                {
                    Application::GetInstance().scene->SaveScene(path.c_str());
                }
            }

            if (ImGui::MenuItem("Load Scene"))
            {
                std::string path = OpenFileDialog("JSON Files\0*.json\0All Files\0*.*\0");
                if (!path.empty())
                {
                    Application::GetInstance().scene->LoadScene(path.c_str());
                    selectedGameObject = nullptr;
                }
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
            {
                SDL_Event quit_event;
                quit_event.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quit_event);
            }
            ImGui::EndMenu();
        }

        // --- View Menu ---
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Hierarchy", NULL, &showHierarchyWindow);
            ImGui::MenuItem("Inspector", NULL, &showInspectorWindow);
            ImGui::MenuItem("Configuration", NULL, &showConfigurationWindow);
            ImGui::MenuItem("Console", NULL, &showConsoleWindow);
            ImGui::MenuItem("Time Debug", NULL, &showTimeDebugWindow);
            ImGui::MenuItem("Timer", NULL, &showTimerWindow);

            ImGui::MenuItem("Performance Stats", NULL, &showPerformanceWindow);

            ImGui::Separator();

            ImGui::MenuItem("Assets", NULL, &showAssetWindow);
            ImGui::MenuItem("Resource Statistics", NULL, &showResourceStatsWindow);

            ImGui::MenuItem("Octree Debug", NULL, &showOctreeDebugWindow);

            if (ImGui::MenuItem("Reset Layout"))
            {
                firstTimeLayout = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Create"))
        {
            if (ImGui::BeginMenu("2D Primitives"))
            {
                if (ImGui::MenuItem("Triangle")) { Application::GetInstance().scene->CreateTriangle(); }
                if (ImGui::MenuItem("Square")) { Application::GetInstance().scene->CreateSquare(); }
                if (ImGui::MenuItem("Rectangle")) { Application::GetInstance().scene->CreateRectangle(); }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("3D Primitives"))
            {
                if (ImGui::MenuItem("Pyramid")) { Application::GetInstance().scene->CreatePyramid(); }
                if (ImGui::MenuItem("Cube")) { Application::GetInstance().scene->CreateCube(); }
                if (ImGui::MenuItem("Sphere")) { Application::GetInstance().scene->CreateSphere(); }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        // --- Help Menu---
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("ImGui Demo", NULL, &showDemoWindow)) {}

            ImGui::Separator();

            if (ImGui::MenuItem("Documentation"))
            {
                //SDL_OpenURL("https://github.com/Loving-Studios/RGSEngine/tree/main/docs");
                SDL_OpenURL("https://github.com/Loving-Studios/RGSEngine/blob/main/README.md");
            }
            if (ImGui::MenuItem("Report a Bug"))
            {
                SDL_OpenURL("https://github.com/Loving-Studios/RGSEngine/issues");
            }
            if (ImGui::MenuItem("Download Latest Release"))
            {
                SDL_OpenURL("https://github.com/Loving-Studios/RGSEngine/releases");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("About RGSEngine", NULL, &showAboutWindow)) {}

            ImGui::EndMenu();
        }

        ImGui::Separator();
        ImGui::Spacing();

        ModuleScene* scene = Application::GetInstance().scene.get();
        ModuleScene::SimulationState state = scene->GetSimulationState();

        // Play/Pause/Stop buttons
        ImVec4 playColor = (state == ModuleScene::SimulationState::PLAYING)
            ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f)
            : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

        ImVec4 pauseColor = (state == ModuleScene::SimulationState::PAUSED)
            ? ImVec4(0.9f, 0.7f, 0.2f, 1.0f)
            : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

        ImVec4 stopColor = (state == ModuleScene::SimulationState::STOPPED)
            ? ImVec4(0.8f, 0.2f, 0.2f, 1.0f)
            : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, playColor);
        if (ImGui::Button("Play"))
        {
            scene->Play();
        }
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Start simulation (saves current state)");
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(state != ModuleScene::SimulationState::PLAYING);
        ImGui::PushStyleColor(ImGuiCol_Button, pauseColor);
        if (ImGui::Button("Pause"))
        {
            scene->Pause();
        }
        ImGui::PopStyleColor();
        ImGui::EndDisabled();

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Pause simulation");
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(state == ModuleScene::SimulationState::STOPPED);
        ImGui::PushStyleColor(ImGuiCol_Button, stopColor);
        if (ImGui::Button("Stop"))
        {
            scene->Stop();
        }
        ImGui::PopStyleColor();
        ImGui::EndDisabled();

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Stop simulation and restore original state");
        }

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::Separator();

        // Show current state
        const char* stateText = "STOPPED";
        ImVec4 stateColor = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);

        if (state == ModuleScene::SimulationState::PLAYING)
        {
            stateText = "PLAYING";
            stateColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
        }
        else if (state == ModuleScene::SimulationState::PAUSED)
        {
            stateText = "PAUSED";
            stateColor = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);
        }

        ImGui::SameLine();
        ImGui::TextColored(stateColor, "%s", stateText);

        // --- TIMER VISUAL ---
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::SameLine();

        // Calculate minutes, seconds, milliseconds usando simulationElapsedTime
        int minutes = (int)(simulationElapsedTime / 60.0f);
        int seconds = (int)(simulationElapsedTime) % 60;
        int milliseconds = (int)((simulationElapsedTime - (int)simulationElapsedTime) * 1000.0f);

        // Change color based on state
        ImVec4 timerColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // Default gray
        if (state == ModuleScene::SimulationState::PLAYING)
            timerColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Green
        else if (state == ModuleScene::SimulationState::PAUSED)
            timerColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow

        ImGui::TextColored(timerColor, "Timer: %02d:%02d.%03d", minutes, seconds, milliseconds);
        ImGui::EndMenuBar();
    }
}

void ModuleEditor::DrawHierarchyWindow()
{
    if (!ImGui::Begin("Hierarchy", &showHierarchyWindow))
    {
        // If it's closed, exit
        ImGui::End();
        return;
    }

    // Create Empty GameOject
    if (ImGui::Button("Create Empty"))
    {
        Application::GetInstance().scene->CreateEmptyGameObject();
    }

    // Delete GameOject
    if (selectedGameObject != nullptr && selectedGameObject->GetParent() != nullptr)
    {
        Input* input = Application::GetInstance().input.get();

        if (input->GetKey(SDL_SCANCODE_DELETE) == KEY_DOWN)
        {
            LOG("Deleting GameObject: %s (UID: %llu)",
                selectedGameObject->GetName().c_str(), selectedGameObject->uid);

            ModuleScene* scene = Application::GetInstance().scene.get();
            if (scene->octree && scene->useOctree)
            {
                scene->octree->Remove(selectedGameObject);
            }

            // Release resources BEFORE removing from parent
            selectedGameObject->ReleaseResourceReferences();

            // Inform the parent to delete the children selected
            selectedGameObject->GetParent()->RemoveChild(selectedGameObject);

            // Deselect the object
            selectedGameObject = nullptr;
            Application::GetInstance().render->selectedObject = nullptr;

            // Update reference counts in ResourceManager
            ResourceManager::GetInstance().UpdateReferenceCounts();


            LOG("GameObject deleted and references updated");
        }
    }

    // ImGuizmo controls
    if (ImGui::RadioButton("Translate - W", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::RadioButton("Rotate - E", mCurrentGizmoOperation == ImGuizmo::ROTATE))
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::RadioButton("Scale - R", mCurrentGizmoOperation == ImGuizmo::SCALE))
        mCurrentGizmoOperation = ImGuizmo::SCALE;

    if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        mCurrentGizmoMode = ImGuizmo::WORLD;
    ImGui::SameLine();
    if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        mCurrentGizmoMode = ImGuizmo::LOCAL;

    ImGui::Separator();

    // Obtain the rootObject of the scene
    GameObject* root = Application::GetInstance().scene->rootObject.get();
    if (root)
    {
        // Call to the recursive function to draw the root node
        DrawHierarchyNode(root);
    }

    // Create invisible space at the end of the window
    ImVec2 available = ImGui::GetContentRegionAvail();
    ImGui::Dummy(available);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_GO"))
        {
            GameObject* droppedGO = *(GameObject**)payload->Data;
            // Droped anywhere the SceneRoot
            if (droppedGO && root && droppedGO->GetParent() != root)
            {
                droppedGO->SetParent(root);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (scrollToSelection)
    {
        scrollToSelection = false;
    }

    ImGui::End();
}

void ModuleEditor::DrawHierarchyNode(GameObject* go)
{
    if (go == nullptr) return;

    if (scrollToSelection && selectedGameObject != nullptr)
    {
        //If this node is a ‘grandparent’ of the selected node, open it to view its contents.
        if (IsAncestor(go, selectedGameObject))
        {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        }

        //If this node IS the selected one, scroll down to here
        if (go == selectedGameObject)
        {
            ImGui::SetScrollHereY();
        }
    }

    ImGui::PushID(go);

    // SceneRoot cannot be desactivated
    if (go->GetParent() != nullptr)
    {
        // The checkbox is unique for this object
        ImGui::Checkbox("##active", &go->active);
        ImGui::SameLine();
    }

    // Save the node lead without childrens at the begining to be consistent later
    bool isLeaf = go->GetChildren().empty();

    // Configuration the flags for the TreeNode
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (go->GetName() == "SceneRoot")
    {
        nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    // If it's the selected node it's applied the flag selectedGameObject
    if (go == selectedGameObject)
    {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    // If it has no childs  it's a node marked as 'leaf'
    if (isLeaf)
    {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    // If the object is inactive is drawn grey
    if (!go->active)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    }

    // Draw the TreeNode
    bool nodeOpen = ImGui::TreeNodeEx(go->GetName().c_str(), nodeFlags);

    // Remove grey color
    if (!go->active)
    {
        ImGui::PopStyleColor();
    }

    // Check if the user has made click on the node
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        selectedGameObject = go; // Selected
        Application::GetInstance().render->selectedObject = go;
    }

    // Drag the object on the hierarchy
    if (go->GetParent() != nullptr && ImGui::BeginDragDropSource())
    {
        // send the pointer of the object as payload
        ImGui::SetDragDropPayload("HIERARCHY_GO", &go, sizeof(GameObject*));
        ImGui::Text("Moving %s", go->GetName().c_str());
        ImGui::EndDragDropSource();
    }

    // Drop object to make it parent
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_GO"))
        {
            GameObject* droppedGO = *(GameObject**)payload->Data;

            // Avoid reparenting to itself, or to its own childrens
            if (droppedGO != go && !droppedGO->IsAncestorOf(go))
            {
                droppedGO->SetParent(go);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // New menu right click
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Create Empty Child"))
        {
            // Create empty object
            auto child = std::make_shared<GameObject>("Empty Child");
            child->AddComponent(std::make_shared<ComponentTransform>(child.get()));

            // Add to the list of childs of this object
            go->AddChild(child);
        }

        if (ImGui::MenuItem("Delete"))
        {
            if (go->GetParent())
            {
                LOG("Context menu delete: %s", go->GetName().c_str());

                ModuleScene* scene = Application::GetInstance().scene.get();
                if (scene->octree && scene->useOctree)
                {
                    scene->octree->Remove(go);
                }

                // Release resources BEFORE removing
                go->ReleaseResourceReferences();

                go->GetParent()->RemoveChild(go);

                // Update reference counts
                ResourceManager::GetInstance().UpdateReferenceCounts();
            }

            if (selectedGameObject == go)
                selectedGameObject = nullptr;
        }

        ImGui::EndPopup();
    }

    // If the node is open and it's not marked as 'leaf' the childrens are drawn
    if (nodeOpen)
    {
        // Only if it WASNT a leaf at the begining, ImGui made Push, so it has to be made the TreePop
        if (!isLeaf)
        {
            // Draw the childrens, even if its empty the for will do nothing
            for (const auto& child : go->GetChildren())
            {
                DrawHierarchyNode(child.get());
            }
            ImGui::TreePop(); // Pop mandatory because the TreeNode made push
        }
        else
        {
            // If IT WAS a leaf, ImGui didnt made push cause the flag NoTreePushOnOpen
            // Even if the user adds a son inside, don't need to make the TreePop in this frame
            // On the next frame, isLeaf will be false and will enter the correct spot
        }
    }

    ImGui::PopID();
}

void ModuleEditor::DrawInspectorWindow()
{
    if (!ImGui::Begin("Inspector", &showInspectorWindow))
    {
        ImGui::End();
        return;
    }

    // If nothing is selected just show a text and exit
    if (selectedGameObject == nullptr)
    {
        ImGui::Text("No GameObject selected.");
        ImGui::End();
        return;
    }

    ModuleScene* scene = Application::GetInstance().scene.get();
    if (scene && scene->rootObject)
    {
        bool exists = CheckIfGameObjectExists(selectedGameObject, scene->rootObject.get());
        if (!exists)
        {
            LOG("Selected object no longer exists - clearing selection");
            selectedGameObject = nullptr;
            Application::GetInstance().render->selectedObject = nullptr;
            ImGui::Text("No GameObject selected.");
            ImGui::End();
            return;
        }
    }

   
    bool isPlaying = scene->IsPlaying() || scene->IsPaused();

    if (isPlaying)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
        ImGui::Text("WARNING: Editing disabled during simulation");
        ImGui::PopStyleColor();
        ImGui::Separator();
    }


    ImGui::BeginDisabled(isPlaying);

    // --- If there is something selected ---

    // Show the name
    ImGui::Text("GameObject: %s", selectedGameObject->GetName().c_str());
    ImGui::Text("UID: %llu", selectedGameObject->uid);
    ImGui::Separator();

    // Iterate all of the components
    for (auto& component : selectedGameObject->components)
    {
        // Show the UI for all the type of component
        switch (component->GetType())
        {
        case ComponentType::TRANSFORM:
        {
            ComponentTransform* transform = static_cast<ComponentTransform*>(component.get());
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool changed = false;
                if (ImGui::DragFloat3("Position", (float*)&transform->position, 0.1f))
                {
                    changed = true;
                }

                glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transform->rotation));
                if (ImGui::DragFloat3("Rotation", (float*)&eulerAngles, 1.0f))
                {
                    transform->SetRotation(glm::quat(glm::radians(eulerAngles)));
                    changed = true;
                }

                if (ImGui::DragFloat3("Scale", (float*)&transform->scale, 0.1f))
                {
                    changed = true;
                }

                // If the user tapped on any value, we update the boxes.
                if (changed)
                {
                    selectedGameObject->UpdateAABBRecursive();
                }

            }

            break;
        }

        case ComponentType::MESH:
        {
            ComponentMesh* mesh = static_cast<ComponentMesh*>(component.get());
            if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (!mesh->path.empty())
                    ImGui::Text("Path: %s", mesh->path.c_str());

                if (!mesh->libraryPath.empty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    ImGui::Text("Internal: %s", mesh->libraryPath.c_str());
                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    ImGui::Text("Internal: Generated / Primitive");
                    ImGui::PopStyleColor();
                }
                ImGui::Separator();

                ImGui::Text("Index Count: %d", mesh->indexCount);
                ImGui::Text("VAO: %d, VBO: %d, IBO: %d", mesh->VAO, mesh->VBO, mesh->IBO);

                ImGui::Separator();
                if (ImGui::Button("Select Mesh..."))
                {
                    std::string path = OpenFileDialog("Mesh Files\0*.fbx;*.rgs\0FBX Files\0*.fbx\0Custom Mesh (*.rgs)\0*.rgs\0All Files\0*.*\0");
                    if (!path.empty())
                    {
                        Application::GetInstance().loadFiles->LoadMeshFromFile(path.c_str(), selectedGameObject);
                    }
                }

                ImGui::Separator();

                if (mesh->normalsVAO != 0)
                {
                    ImGui::Checkbox("Show Vertex Normals", &Application::GetInstance().render->drawVertexNormals);
                }
                if (mesh->faceNormalsVAO != 0)
                {
                    ImGui::Checkbox("Show Face Normals", &Application::GetInstance().render->drawFaceNormals);
                }
                if (mesh->aabbVAO != 0)
                {
                    ImGui::Checkbox("Draw AABB", &Application::GetInstance().render->drawAABBs);
                }
            }
            break;
        }

        case ComponentType::TEXTURE:
        {
            ComponentTexture* texture = static_cast<ComponentTexture*>(component.get());
            if (ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Material Color");
                ImGui::ColorEdit4("##tintColor", (float*)&texture->color);
                ImGui::Separator();

                bool useDefault = texture->useDefaultTexture;
                if (ImGui::Checkbox("Use Default Checkered Texture", &useDefault))
                {
                    texture->useDefaultTexture = useDefault;
                    if (useDefault)
                    {
                        // SAve the older state if its not yet in default mode
                        if (texture->path != "default_checker")
                        {
                            texture->originalTextureID = texture->textureID;
                            texture->originalPath = texture->path;
                            texture->originalColor = texture->color;
                        }

                        unsigned int defaultTexID = Application::GetInstance().render->defaultCheckerTexture;
                        texture->textureID = defaultTexID;
                        texture->path = "default_checker";

                        texture->color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    }
                    else
                    {
                        // Restore original state of the color and texture
                        texture->textureID = texture->originalTextureID;
                        texture->path = texture->originalPath;
                        texture->color = texture->originalColor;

                        texture->originalTextureID = 0;
                        texture->originalPath = "";
                    }
                }

                ImGui::Text("Path: %s", texture->path.c_str());
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::Text("Internal: %s", texture->libraryPath.c_str());
                ImGui::PopStyleColor();
                ImGui::Text("Size: %d x %d", texture->width, texture->height);
                ImGui::Text("Texture ID: %d", texture->textureID);

                ImGui::SameLine();
                if (ImGui::Button("Select Texture..."))
                {
                    std::string path = OpenFileDialog("Texture Files\0*.png;*.jpg;*.dds;*.jpeg;*.tga;*.rgst\0Image Files\0*.png;*.jpg;*.dds;*.jpeg;*.tga\0Custom Texture (*.rgst)\0*.rgst\0All Files\0*.*\0");
                    if (!path.empty())
                    {
                        Application::GetInstance().loadFiles->LoadTexture(path.c_str(), selectedGameObject);
                    }
                }

                ImGui::Separator();
                ImGui::Text("Transparency Settings");

                ImGui::Checkbox("Enable Alpha Test", &texture->enableAlphaTest);
                if (texture->enableAlphaTest)
                {
                    ImGui::SliderFloat("Alpha Threshold", &texture->alphaThreshold, 0.0f, 1.0f);
                }

                ImGui::Checkbox("Enable Blending", &texture->enableBlending);
                if (texture->enableBlending)
                {
                    const char* items[] = { "GL_SRC_ALPHA", "GL_ONE", "GL_ZERO", "GL_ONE_MINUS_SRC_ALPHA" };
                    static int currentSrc = 0;
                    static int currentDst = 3;

                    if (ImGui::Combo("Source Factor", &currentSrc, items, IM_ARRAYSIZE(items))) {
                        if (currentSrc == 0) texture->blendSrc = GL_SRC_ALPHA;
                        if (currentSrc == 1) texture->blendSrc = GL_ONE;
                        if (currentSrc == 2) texture->blendSrc = GL_ZERO;
                        if (currentSrc == 3) texture->blendSrc = GL_ONE_MINUS_SRC_ALPHA;
                    }
                    if (ImGui::Combo("Dest Factor", &currentDst, items, IM_ARRAYSIZE(items))) {
                        if (currentDst == 0) texture->blendDst = GL_SRC_ALPHA;
                        if (currentDst == 1) texture->blendDst = GL_ONE;
                        if (currentDst == 2) texture->blendDst = GL_ZERO;
                        if (currentDst == 3) texture->blendDst = GL_ONE_MINUS_SRC_ALPHA;
                    }
                }

                ImGui::Separator();

                if (ImGui::Button("Apply Window Texture (Blending)"))
                {
                    Application::GetInstance().loadFiles->LoadTexture("Assets/Transparency/blending_transparent_window.png", selectedGameObject);
                    texture->enableBlending = true;
                    texture->blendSrc = GL_SRC_ALPHA;
                    texture->blendDst = GL_ONE_MINUS_SRC_ALPHA;
                    texture->enableAlphaTest = false;
                }

                if (ImGui::Button("Apply Grass Texture (Alpha Test)"))
                {
                    Application::GetInstance().loadFiles->LoadTexture("Assets/Transparency/grass.png", selectedGameObject);
                    texture->enableAlphaTest = true;
                    texture->alphaThreshold = 0.1f;
                    texture->enableBlending = false;
                }
            }
            break;
        }

        case ComponentType::CAMERA:
        {
            ComponentCamera* camera = static_cast<ComponentCamera*>(component.get());
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Active", &camera->active);

                if (ImGui::DragFloat("FOV", &camera->cameraFOV, 0.1f, 1.0f, 179.0f))
                {
                    camera->GenerateFrustumGizmo();
                }

                if (ImGui::DragFloat("Near Plane", &camera->nearPlane, 0.1f, 0.01f, 1000.0f))
                {
                    if (camera->nearPlane <= 0.0f) camera->nearPlane = 0.01f;
                    camera->GenerateFrustumGizmo();
                }

                if (ImGui::DragFloat("Far Plane", &camera->farPlane, 1.0f, 0.01f, 2000.0f))
                {
                    if (camera->farPlane <= camera->nearPlane) camera->farPlane = camera->nearPlane + 0.1f;
                    camera->GenerateFrustumGizmo();
                }
            }
            break;
        }
        }
    }

    ImGui::EndDisabled();
    ImGui::End();
}

void ModuleEditor::ApplyDefaultDockingLayout()
{
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Force reset any previous layout
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

    // Creation of the splits

    ImGuiID dock_main_id = dockspace_id;

    // Main divided so we can put the inspector at the right
    // The dock_main_id is updated so the espace resultant is the centre
    ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

    // Main divided, the new centre, so we can put the Hierarchy at the left
    ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);

    // Main divided, the new centre, so we can put the Console at the Bottom of the screen
    ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.20f, nullptr, &dock_main_id);


    ImGuiID dock_right_lower = ImGui::DockBuilderSplitNode(dock_right_id, ImGuiDir_Down, 0.5f, nullptr, &dock_right_id);
    // The space left after all the divisions on dock_main_id it's the Viewport 3D, the passthrough

    // Apply the Dock of our windows to the id's created
    ImGui::DockBuilderDockWindow("Hierarchy", dock_left_id);
    ImGui::DockBuilderDockWindow("Assets", dock_left_id);
    ImGui::DockBuilderDockWindow("Inspector", dock_right_id);
    ImGui::DockBuilderDockWindow("Configuration", dock_right_id); // Same Tab as the Inspector
    ImGui::DockBuilderDockWindow("Console", dock_bottom_id);
    ImGui::DockBuilderDockWindow("Resource Statistics", dock_right_lower);
    ImGui::DockBuilderDockWindow("Dear ImGui Demo", dock_main_id); // Centered just in case

    ImGui::SetWindowFocus("Inspector");

    ImGui::DockBuilderFinish(dockspace_id);
}

void ModuleEditor::DrawConsoleWindow()
{
    if (!ImGui::Begin("Console", &showConsoleWindow))
    {
        ImGui::End();
        return;
    }

    // Button to clear the console
    if (ImGui::Button("Clear"))
    {
        Log::logBuffer.clear();
    }

    ImGui::SameLine();
    // Button to copy the full console
    if (ImGui::Button("Copy to Clipboard"))
    {
        ImGui::LogToClipboard();
        ImGui::LogText("%s", Log::logBuffer.c_str());
        ImGui::LogFinish();
    }

    ImGui::Separator();

    // Scroll on the console
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Show the text acumulated from the begining and the new
    ImGui::TextUnformatted(Log::logBuffer.c_str());

    // Auto-scroll if is close to the end
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}


void ModuleEditor::DrawConfigurationWindow()
{
    if (!ImGui::Begin("Configuration", &showConfigurationWindow))
    {
        ImGui::End();
        return;
    }

    // FPS graphs
    if (ImGui::CollapsingHeader("Application"))
    {
        char title[50];
        sprintf_s(title, "FPS: %.1f", fpsLog.back());
        ImGui::PlotHistogram("##fps", &fpsLog[0], fpsLog.size(), 0, title, 0.0f, 100.0f, ImVec2(0, 80));
    }

    if (ImGui::CollapsingHeader("Modules"))
    {
        Window* window = Application::GetInstance().window.get();
        Render* render = Application::GetInstance().render.get();
        Input* input = Application::GetInstance().input.get();

        if (ImGui::TreeNode("Render"))
        {
            ImGui::SliderFloat("Camera Speed", &render->cameraSpeed, 0.1f, 10.0f);
            ImGui::SliderFloat("Camera Sensitivity", &render->cameraSensitivity, 0.01f, 1.0f);
            ImGui::SliderFloat("Camera FOV", &render->cameraFOV, 1.0f, 120.0f);

            // --- Debug Draw ---
            ImGui::Separator();
            ImGui::Text("Debug Visualization");
            ImGui::Checkbox("Draw Vertex Normals", &render->drawVertexNormals);
            ImGui::Checkbox("Draw Face Normals", &render->drawFaceNormals);
            ImGui::Checkbox("Draw AABBs", &render->drawAABBs);

            // --- Frustum Culling ---
            ImGui::Separator();
            ImGui::Text("Frustum Culling");
            ImGui::Checkbox("Enable Frustum Culling", &render->enableFrustumCulling);

            if (render->enableFrustumCulling)
            {
                ImGui::Indent();
                ImGui::Checkbox("Visualize Culling State", &render->visualizeFrustumCulling);

                if (render->visualizeFrustumCulling)
                {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Green = Visible");
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red = Culled");
                    ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "Blue = Selected");
                }
                ImGui::Unindent();
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Window"))
        {
            bool fs = window->fullscreen;
            if (ImGui::Checkbox("Fullscreen", &fs))
            {
                window->SetFullscreen(fs);
            }

            ImGui::BeginDisabled(fs);

            ImGui::SameLine();
            bool bd = window->borderless;
            if (ImGui::Checkbox("Borderless", &bd))
            {
                window->SetBorderless(bd);
            }

            ImGui::SameLine();
            bool rs = window->resizable;
            if (ImGui::Checkbox("Resizable", &rs))
            {
                window->SetResizable(rs);
            }

            if (ImGui::Button("Reset Size"))
            {
                window->ResetWindowSize();
            }

            ImGui::EndDisabled();

            ImGui::Text("Width: %d", window->width);
            ImGui::Text("Height: %d", window->height);
            ImGui::TreePop();
        }
    }

    // --- Hardware and software ---
    if (ImGui::CollapsingHeader("Hardware & Software Versions"))
    {

        const int compiled = SDL_VERSION;  /* hardcoded number from SDL headers */
        const int linked = SDL_GetVersion();  /* reported by linked SDL library */

        ImGui::Text("SDL3 Compiled Version: %d.%d.%d", SDL_VERSIONNUM_MAJOR(compiled), SDL_VERSIONNUM_MINOR(compiled), SDL_VERSIONNUM_MICRO(compiled));
        ImGui::Text("SDL3 Linked Version: %d.%d.%d", SDL_VERSIONNUM_MAJOR(linked), SDL_VERSIONNUM_MINOR(linked), SDL_VERSIONNUM_MICRO(linked));

        ImGui::Text("OpenGL Version: %s", glGetString(GL_VERSION));
        ImGui::Text("DevIL Version: %d", ilGetInteger(IL_VERSION_NUM));
        ImGui::Text("ImGui Version: %s", ImGui::GetVersion());

        ImGui::Separator();
        ImGui::Text("Hardware:");
        ImGui::Text("CPU Cores: %d", SDL_GetNumLogicalCPUCores());
        ImGui::Text("System RAM: %.2f GB", (int)SDL_GetSystemRAM() / 1024.0f);

        ImGui::Text("Process RAM Usage: %d MB", ram_usage_mb);

        ImGui::Separator();
        ImGui::Text("GPU Vendor: %s", glGetString(GL_VENDOR));
        ImGui::Text("GPU Renderer: %s", glGetString(GL_RENDERER));

        if (isNVIDIA)
        {
            // Calculate the actual usage
            int vram_usage_mb = vram_budget_mb - vram_available_mb;

            ImGui::Text("VRAM Budget: %d MB", vram_budget_mb);
            ImGui::Text("VRAM Available: %d MB", vram_available_mb);
            ImGui::Text("VRAM Usage (Aprox.): %d MB", vram_usage_mb);

            // Usage progress bar VRAM
            float usage_percentage = (float)vram_usage_mb / (float)vram_budget_mb;
            char bar_label[64];
            sprintf_s(bar_label, "%d MB / %d MB", vram_usage_mb, vram_budget_mb);
            ImGui::ProgressBar(usage_percentage, ImVec2(0.f, 0.f), bar_label);
        }
        else
        {
            ImGui::Text("VRAM Info: Not available non-NVIDIA card detected");
        }
    }

    ImGui::End();
}

void ModuleEditor::DrawAboutWindow()
{
    if (!ImGui::Begin("About RGSEngine", &showAboutWindow))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("RGSEngine v0.1");
    ImGui::TextWrapped(
        "Motores, ensambladoras, 3 o 4 compiladoras,"
        " que no somos de aqui, que somos de otro lao,"
        " venimos a programar y no nos han dejao"
    );
    ImGui::Separator();

    ImGui::Text("By Loving Studios:");
    if (ImGui::Button("XXPabloS")) { SDL_OpenURL("https://github.com/XXPabloS"); }
    ImGui::SameLine();
    if (ImGui::Button("TheWolfG145")) { SDL_OpenURL("https://github.com/TheWolfG145"); }
    ImGui::SameLine();
    if (ImGui::Button("Claurm12")) { SDL_OpenURL("https://github.com/Claurm12"); }

    ImGui::Separator();

    ImGui::Text("Libraries used:");
    ImGui::BulletText("SDL3 (v%d.%d.%d)", SDL_VERSIONNUM_MAJOR(SDL_GetVersion()), SDL_VERSIONNUM_MINOR(SDL_GetVersion()), SDL_VERSIONNUM_MICRO(SDL_GetVersion()));
    ImGui::BulletText("OpenGL (%s)", glGetString(GL_VERSION));
    ImGui::BulletText("ImGui (%s)", ImGui::GetVersion());
    ImGui::BulletText("glad");
    ImGui::BulletText("glm");
    ImGui::BulletText("assimp");
    ImGui::BulletText("DevIL (v%d)", ilGetInteger(IL_VERSION_NUM));

    ImGui::Separator();

    ImGui::Text("License:");
    ImGui::Text("MIT License");
    ImGui::Text("Copyright (c) 2025 Loving Studios");
    ImGui::TextWrapped(
        "Permission is hereby granted, free of charge, to any person obtaining a copy "
        "of this software and associated documentation files (the \"Software\"), to deal "
        "in the Software without restriction, including without limitation the rights "
        "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
        "copies of the Software, and to permit persons to whom the Software is "
        "furnished to do so, subject to the following conditions:"
    );

    ImGui::Spacing();

    ImGui::TextWrapped(
        "The above copyright notice and this permission notice shall be included in all "
        "copies or substantial portions of the Software."
    );

    ImGui::Spacing();

    ImGui::TextWrapped(
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
        "SOFTWARE."
    );

    ImGui::End();
}

void ModuleEditor::UpdateMemoryStats()
{
    if (isNVIDIA)
    {
        // GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
        glGetIntegerv(0x9048, &vram_budget_mb);
        // GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
        glGetIntegerv(0x9049, &vram_available_mb);

        // Kb to mb
        vram_budget_mb /= 1024;
        vram_available_mb /= 1024;
    }

    PROCESS_MEMORY_COUNTERS pmc;
    // GetCurrentProcess() gives back a handle of the actual process
    // GetProcessMemoryInfo() fills the structure pmc with the info
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        // pmc.WorkingSetSize is the usage of physic RAM in bytes
        ram_usage_mb = (int)(pmc.WorkingSetSize / (1024 * 1024)); // Convert to MB
    }
}




void ModuleEditor::DrawTimeDebugWindow()
{
    if (!ImGui::Begin("Time Debug", &showTimeDebugWindow))
    {
        ImGui::End();
        return;
    }

    // === TIMER VISUAL ===
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
    ImGui::SetWindowFontScale(1.5f);

    int minutes = (int)(Time::time / 60.0f);
    int seconds = (int)(Time::time) % 60;
    int milliseconds = (int)((Time::time - (int)Time::time) * 1000.0f);

    ImGui::Text("TIMER: %02d:%02d.%03d", minutes, seconds, milliseconds);

    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("GAME CLOCK");
    ImGui::Text("Time: %.3f s", Time::time);
    ImGui::Text("Delta Time: %.4f s (%.1f FPS)", Time::deltaTime, 1.0f / Time::deltaTime);
    ImGui::Text("Time Scale: %.2fx", Time::timeScale);
    ImGui::Text("Frame Count: %llu", Time::frameCount);

    ImGui::Separator();
    ImGui::Text("REAL TIME CLOCK");
    ImGui::Text("Real Time: %.3f s", Time::realTimeSinceStartup);
    ImGui::Text("Real Delta Time: %.4f s (%.1f FPS)",
        Time::realDeltaTime, 1.0f / Time::realDeltaTime);

    ImGui::Separator();
    ImGui::Text("SIMULATION STATE");
    ModuleScene* scene = Application::GetInstance().scene.get();
    const char* stateText = "STOPPED";
    if (scene->IsPlaying()) stateText = "PLAYING";
    else if (scene->IsPaused()) stateText = "PAUSED";
    ImGui::Text("State: %s", stateText);
    ImGui::Text("Is Paused: %s", Time::isPaused ? "YES" : "NO");

    ImGui::Separator();
    ImGui::Text("CONTROLS");

    float newTimeScale = Time::timeScale;
    if (ImGui::SliderFloat("Time Scale", &newTimeScale, 0.01f, 5.0f))
    {
        Time::SetTimeScale(newTimeScale);
    }

    if (ImGui::Button("Reset Time Scale"))
    {
        Time::SetTimeScale(1.0f);
    }

    ImGui::SameLine();
    if (ImGui::Button("0.5x")) Time::SetTimeScale(0.5f);
    ImGui::SameLine();
    if (ImGui::Button("2x")) Time::SetTimeScale(2.0f);

    ImGui::Separator();

    ImGui::BeginDisabled(!scene->IsPaused());
    if (ImGui::Button("Step 1 Frame"))
    {
        Time::Step();
    }
    ImGui::EndDisabled();

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Advance 1 frame while paused");
    }

    ImGui::End();
}

void ModuleEditor::DrawTimerWindow()
{
    if (!ImGui::Begin("Timer", &showTimerWindow, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    ModuleScene* scene = Application::GetInstance().scene.get();
    ModuleScene::SimulationState state = scene->GetSimulationState();

    // Calculate time components usando simulationElapsedTime
    int minutes = (int)(simulationElapsedTime / 60.0f);
    int seconds = (int)(simulationElapsedTime) % 60;
    int milliseconds = (int)((simulationElapsedTime - (int)simulationElapsedTime) * 1000.0f);

    // Large timer display
    ImVec4 timerColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    if (state == ModuleScene::SimulationState::PLAYING)
        timerColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
    else if (state == ModuleScene::SimulationState::PAUSED)
        timerColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Text, timerColor);

    // Make font bigger
    ImGui::SetWindowFontScale(2.5f);
    ImGui::Text("%02d:%02d.%03d", minutes, seconds, milliseconds);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::PopStyleColor();

    ImGui::Separator();

    // Control buttons
    ImGui::Text("Controls:");

    if (ImGui::Button("Play", ImVec2(80, 30)))
        scene->Play();

    ImGui::SameLine();

    ImGui::BeginDisabled(state != ModuleScene::SimulationState::PLAYING);
    if (ImGui::Button("Pause", ImVec2(80, 30)))
        scene->Pause();
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(state == ModuleScene::SimulationState::STOPPED);
    if (ImGui::Button("Stop", ImVec2(80, 30)))
        scene->Stop();
    ImGui::EndDisabled();

    ImGui::Separator();

    // Additional info
    ImGui::Text("State: %s",
        state == ModuleScene::SimulationState::PLAYING ? "PLAYING" :
        state == ModuleScene::SimulationState::PAUSED ? "PAUSED" : "STOPPED");

    ImGui::Text("Time Scale: %.2fx", Time::timeScale);
    ImGui::Text("Frame: %llu", Time::frameCount);

    ImGui::End();
}

void ModuleEditor::DrawPerformanceWindow()
{
    if (!ImGui::Begin("Performance Statistics", &showPerformanceWindow))
    {
        ImGui::End();
        return;
    }

    Render* render = Application::GetInstance().render.get();
    ModuleScene* scene = Application::GetInstance().scene.get();

    // FPS
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

    ImGui::Separator();
    ImGui::Text("Render Stats:");

    if (render->enableFrustumCulling)
    {
        ImGui::Text("Total Objects: %d", render->totalObjects);

        ImGui::Text("Rendered: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%d", render->renderedObjects);

        ImGui::Text("Culled: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%d", render->culledObjects);

        // Visual progress bar
        if (render->totalObjects > 0)
        {
            float cullPercentage = (float)render->culledObjects / (float)render->totalObjects * 100.0f;
            ImGui::Text("Cull Rate: %.1f%%", cullPercentage);
            ImGui::ProgressBar(cullPercentage / 100.0f, ImVec2(-1, 0.0f));
        }

        // === OCTREE STATS ===
        if (render->useOctreeForCulling && scene->octree && scene->octree->IsInitialized())
        {
            ImGui::Separator();
            ImGui::Text("Octree Optimization:");

            ImGui::Text("Objects in Octree: %d", scene->octree->GetObjectCount());
            ImGui::Text("Queried by Frustum: %d", render->octreeQueriedObjects);
            ImGui::Text("Skipped by Octree: %d", render->octreeSkippedObjects);

            if (scene->octree->GetObjectCount() > 0)
            {
                float skipRate = (float)render->octreeSkippedObjects /
                    (float)scene->octree->GetObjectCount() * 100.0f;

                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                    "Octree Skip Rate: %.1f%%", skipRate);
            }
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Frustum Culling Disabled");
    }

    ImGui::End();
}

void ModuleEditor::DrawOctreeDebugWindow()
{
    if (!ImGui::Begin("Octree Debug", &showOctreeDebugWindow))
    {
        ImGui::End();
        return;
    }

    ModuleScene* scene = Application::GetInstance().scene.get();
    Render* render = Application::GetInstance().render.get();

    if (!scene->octree || !scene->octree->IsInitialized())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Octree not initialized!");

        if (ImGui::Button("Initialize Octree"))
        {
            scene->RebuildOctree();
        }

        ImGui::End();
        return;
    }

    // === CONTROLS ===
    ImGui::Text("Octree Controls:");
    ImGui::Separator();

    ImGui::Checkbox("Use Octree (Scene)", &scene->useOctree);
    ImGui::Checkbox("Use Octree for Culling", &render->useOctreeForCulling);

    if (ImGui::Button("Rebuild Octree"))
    {
        scene->RebuildOctree();
    }

    ImGui::Separator();

    // === STATISTICS ===
    ImGui::Text("Octree Statistics:");
    ImGui::Separator();

    ImGui::Text("Total Nodes: %d", scene->octree->GetNodeCount());
    ImGui::Text("Total Objects: %d", scene->octree->GetObjectCount());
    ImGui::Text("Max Depth: %d", scene->octree->GetMaxDepth());

    ImGui::Separator();

    // === PERFORMANCE ===
    ImGui::Text("Performance:");
    ImGui::Separator();

    if (render->useOctreeForCulling)
    {
        ImGui::Text("Queried Objects: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%d", render->octreeQueriedObjects);

        ImGui::Text("Skipped Objects: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%d", render->octreeSkippedObjects);

        if (scene->octree->GetObjectCount() > 0)
        {
            float skipPercentage = (float)render->octreeSkippedObjects /
                (float)scene->octree->GetObjectCount() * 100.0f;

            ImGui::Text("Skip Rate: %.1f%%", skipPercentage);
            ImGui::ProgressBar(skipPercentage / 100.0f, ImVec2(-1, 0));

            if (skipPercentage > 0)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                    "Octree saved ~%.1f%% of checks!", skipPercentage);
            }
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
            "Octree culling disabled");
    }

    ImGui::Separator();

    // === VISUALISATION ===
    ImGui::Text("Visualization:");
    ImGui::Separator();

    ImGui::Checkbox("Visualize Octree", &visualizeOctree);

    if (visualizeOctree)
    {
        ImGui::Indent();
        ImGui::Checkbox("Leafs Only", &visualizeOctreeLeafsOnly);
        ImGui::Unindent();

        // We will need to draw the Octree boxes.
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
            "Visualization coming soon...");
    }

    ImGui::Separator();

    // === BOUNDS INFO ===
    if (ImGui::TreeNode("World Bounds"))
    {
        const OctreeNode* root = scene->octree->GetRoot();
        if (root)
        {
            const AABB& bounds = root->GetBounds();
            ImGui::Text("Min: (%.2f, %.2f, %.2f)",
                bounds.minPoint.x, bounds.minPoint.y, bounds.minPoint.z);
            ImGui::Text("Max: (%.2f, %.2f, %.2f)",
                bounds.maxPoint.x, bounds.maxPoint.y, bounds.maxPoint.z);

            glm::vec3 size = bounds.maxPoint - bounds.minPoint;
            ImGui::Text("Size: (%.2f, %.2f, %.2f)",
                size.x, size.y, size.z);
        }
        ImGui::TreePop();
    }

    ImGui::End();
}

static bool CheckIfGameObjectExists(GameObject* target, GameObject* root)
{
    if (!target || !root) return false;
    if (root == target) return true;

    for (const auto& child : root->GetChildren())
    {
        if (child.get() == target) return true;
        if (CheckIfGameObjectExists(target, child.get()))
            return true;
    }
    return false;
}