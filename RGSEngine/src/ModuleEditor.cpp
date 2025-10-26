#include <SDL3/SDL.h>
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

        // --- Help Menu---
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("ImGui Demo", NULL, &showDemoWindow);
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

    // Draw the TreeNode
    bool nodeOpen = ImGui::TreeNodeEx(go->GetName().c_str(), nodeFlags);

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
            if (ImGui::CollapsingHeader("Transform"))
            {
                // Show the info, only lecture mode
                ImGui::InputFloat3("Position", (float*)&transform->position, "%.3f", ImGuiInputTextFlags_ReadOnly);

                // Show the quaternion, lecture mode
                ImGui::Text("Rotation: (%.3f, %.3f, %.3f, %.3f)",
                    transform->rotation.x, transform->rotation.y, transform->rotation.z, transform->rotation.w);

                ImGui::InputFloat3("Scale", (float*)&transform->scale, "%.3f", ImGuiInputTextFlags_ReadOnly);
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
            ImGui::DragFloat("Camera Speed", &render->cameraSpeed, 0.1f);
            ImGui::DragFloat("Camera Sensitivity", &render->cameraSensitivity, 0.01f);
            ImGui::SliderFloat("Camera FOV", &render->cameraFOV, 1.0f, 120.0f);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Window"))
        {
            ImGui::Text("Width: %d", window->width);
            ImGui::Text("Height: %d", window->height);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Input"))
        {
            int mouseX, mouseY;
            input->GetMousePosition(mouseX, mouseY);
            ImGui::Text("Mouse X: %d", mouseX);
            ImGui::Text("Mouse Y: %d", mouseY);
            ImGui::TreePop();
        }
    }

    // Hardware and software ---
    if (ImGui::CollapsingHeader("Hardware & Software Versions"))
    {
        //SDL_version sdl_ver;
        //SDL_GetVersion(&sdl_ver);
        //ImGui::Text("SDL3 Version: %d.%d.%d", sdl_ver.major, sdl_ver.minor, sdl_ver.patch);

        //ImGui::Text("OpenGL Version: %s", glGetString(GL_VERSION));
        //ImGui::Text("DevIL Version: %d", ilGetInteger(IL_VERSION_NUM));
        //ImGui::Text("ImGui Version: %s", ImGui::GetVersion());

        //ImGui::Separator();

        //ImGui::Text("CPU Cores: %d", SDL_GetNumLogicalCPUCores());
        //ImGui::Text("RAM: %.2f GB", (float)SDL_GetSystemRAMMegabytes() / 1024.0f);

        //ImGui::Separator();
        //ImGui::Text("GPU Vendor: %s", glGetString(GL_VENDOR));
        //ImGui::Text("GPU Renderer: %s", glGetString(GL_RENDERER));
    }

    ImGui::End();
}