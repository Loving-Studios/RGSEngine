#include <SDL3/SDL.h>
#include <SDL3/SDL_version.h>
#include <glad/glad.h>

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

#include <IL/il.h>
#include <glm/gtc/type_ptr.hpp>

ModuleEditor::ModuleEditor() : Module(), oldCerrStreamBuf(nullptr)
{
    name = "editor";
}

ModuleEditor::~ModuleEditor()
{
}

bool ModuleEditor::Start()
{
    LOG("ModuleEditor Start");

    // Redirect std::cerr to our stringstream
    oldCerrStreamBuf = std::cerr.rdbuf(); // Save the original Buffer
    std::cerr.rdbuf(consoleStream.rdbuf()); // Redirect to our

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

    if (strstr((const char*)glGetString(GL_VENDOR), "NVIDIA"))
    {
        // GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
        glGetIntegerv(0x9048, &vram_budget_mb);
        // GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
        glGetIntegerv(0x9049, &vram_available_mb);

        // Los valores vienen en KB, los pasamos a MB
        vram_budget_mb /= 1024;
        vram_available_mb /= 1024;
    }

    return true;
}

bool ModuleEditor::PreUpdate()
{
    // New ImGui Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

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


    // --- FOCUS ON SELECTED GAMEOBJECT ---
    Input* input = Application::GetInstance().input.get();

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

    // Start the Container Window
    // Flagged as ImGuiWindowFlags_MenuBar so it can host the main menu
    ImGui::Begin("DockSpace", nullptr, window_flags);

    // --- WINDOWS ---
    // Main menu is drawed inside ImGui::Begin
    DrawMainMenuBar();

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    // If it's the first time opening applies the default view
    if (firstTimeLayout)
    {
        ApplyDefaultDockingLayout();
        firstTimeLayout = false;
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

    if (showAboutWindow)
        DrawAboutWindow();

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

    // Restart the original buffer of std::cerr
    std::cerr.rdbuf(oldCerrStreamBuf);
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
            // Exit option
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
            // Each window has toggles, checkboxes to be activated
            ImGui::MenuItem("Hierarchy", NULL, &showHierarchyWindow);
            ImGui::MenuItem("Inspector", NULL, &showInspectorWindow);
            ImGui::MenuItem("Configuration", NULL, &showConfigurationWindow);
            ImGui::MenuItem("Console", NULL, &showConsoleWindow);

            ImGui::Separator();

            if (ImGui::MenuItem("Reset Layout"))
            {
                // True applied so on the next frame, on the Update() we apply ApplyDefaultDockingLayout()
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

        ImGui::EndMenuBar();
    }
}

void ModuleEditor::DrawHierarchyWindow()
{
    if (!ImGui::Begin("Hierarchy", &showHierarchyWindow))
    {
        // If i's closed, exit
        ImGui::End();
        return;
    }

    // Obtain the rootObject of the scene
    GameObject* root = Application::GetInstance().scene->rootObject.get();
    if (root)
    {
        // Call to the recursive function to draw the root node
        DrawHierarchyNode(root);
    }

    ImGui::End();
}

void ModuleEditor::DrawHierarchyNode(GameObject* go)
{
    if (go == nullptr) return;

    // SceneRoot cannot be desactivated
    if (go->GetParent() != nullptr)
    {
        // The checkbox is unique for this object
        ImGui::PushID(go);

        ImGui::Checkbox("##active", &go->active);

        ImGui::PopID();
        ImGui::SameLine();
    }

    // Configuration the flags for the TreeNode
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    // If it's the selected node it's applied the flag selectedGameObject
    if (go == selectedGameObject)
    {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    // If it has no childs  it's a node marked as 'leaf'
    if (go->GetChildren().empty())
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
    }

    // If the node is open and it's not marked as 'leaf' the childrens are drawn
    if (nodeOpen && !go->GetChildren().empty())
    {
        for (const auto& child : go->GetChildren())
        {
            DrawHierarchyNode(child.get());
        }
        ImGui::TreePop(); // Close the node
    }
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

    // --- If there is something selected ---

    // Show the name
    ImGui::Text("GameObject: %s", selectedGameObject->GetName().c_str());
    ImGui::Separator();

    // Iterate all of the components
    for (auto& component : selectedGameObject->components)
    {
        // Show the UI for all the type of component
        switch (component->GetType())
        {
        case ComponentType::TRANSFORM:
        {
            // Cast to access all of the data
            ComponentTransform* transform = static_cast<ComponentTransform*>(component.get());
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) // Open by default
            {
                // Use of DragFloat3 to have best control
                if (ImGui::DragFloat3("Position", (float*)&transform->position, 0.1f))
                {}
                // Convert the quaternion into Euler Angles in degrees for the UI
                glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transform->rotation));

                // Show the DragFloat3 for the degrees
                if (ImGui::DragFloat3("Rotation", (float*)&eulerAngles, 1.0f))
                {
                    // If the user changes it, convert back to quaternion with glm::radians that converts degrees to radians
                    transform->SetRotation(glm::quat(glm::radians(eulerAngles)));
                }
                if (ImGui::DragFloat3("Scale", (float*)&transform->scale, 0.1f))
                {}
            }
            break;
        }

        case ComponentType::MESH:
        {
            ComponentMesh* mesh = static_cast<ComponentMesh*>(component.get());
            if (ImGui::CollapsingHeader("Mesh"))
            {
                // Mesh info
                ImGui::Text("Index Count: %d", mesh->indexCount);
                ImGui::Text("VAO: %d, VBO: %d, IBO: %d", mesh->VAO, mesh->VBO, mesh->IBO);
            }
            break;
        }

        case ComponentType::TEXTURE:
        {
            ComponentTexture* texture = static_cast<ComponentTexture*>(component.get());
            if (ImGui::CollapsingHeader("Texture"))
            {
                // Texture info
                ImGui::Text("Path: %s", texture->path.c_str());
                ImGui::Text("Size: %d x %d", texture->width, texture->height);
                ImGui::Text("Texture ID: %d", texture->textureID);
            }
            break;
        }
        }
    }

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

    // The space left after all the divisions on dock_main_id it's the Viewport 3D, the passthrough

    // Apply the Dock of our windows to the id's created
    ImGui::DockBuilderDockWindow("Hierarchy", dock_left_id);
    ImGui::DockBuilderDockWindow("Inspector", dock_right_id);
    ImGui::DockBuilderDockWindow("Console", dock_bottom_id);
    ImGui::DockBuilderDockWindow("Configuration", dock_right_id); // Same Tab as the Inspector
    ImGui::DockBuilderDockWindow("Dear ImGui Demo", dock_main_id); // Centered just in case


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
        consoleStream.str(""); // Clear the stringstream
        consoleStream.clear(); // Clear the state flags
    }
    ImGui::Separator();

    // Scroll on the console
    ImGui::BeginChild("ScrollingRegion");

    // Show the text
    ImGui::TextUnformatted(consoleStream.str().c_str());

    // Auto-scroll if we are next to the end
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

        ImGui::Separator();
        ImGui::Text("GPU Vendor: %s", glGetString(GL_VENDOR));
        ImGui::Text("GPU Renderer: %s", glGetString(GL_RENDERER));

        if (vram_budget_mb > 0)
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