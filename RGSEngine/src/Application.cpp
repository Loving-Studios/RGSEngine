#include "Application.h"
#include <iostream>
#include "Log.h"

#include "Window.h"
#include "Input.h"
#include "Render.h"
#include "LoadFiles.h"
#include "ModuleScene.h"
#include "ModuleEditor.h"
#include "Time.h"
#include "ResourceManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SDL3/SDL_scancode.h>

#include <filesystem>

// Constructor
Application::Application() {
    LOG("Constructor Application::Application");


    // Modules
    window = std::make_shared<Window>();
    input = std::make_shared<Input>();
    scene = std::make_shared<ModuleScene>();
    editor = std::make_shared<ModuleEditor>();
    render = std::make_shared<Render>();
    loadFiles = std::make_shared<LoadFiles>();

    // Ordered for awake / Start / Update
    // Reverse order of CleanUp
    AddModule(std::static_pointer_cast<Module>(window));
    AddModule(std::static_pointer_cast<Module>(input));
    AddModule(std::static_pointer_cast<Module>(loadFiles));
    AddModule(std::static_pointer_cast<Module>(scene));
    AddModule(std::static_pointer_cast<Module>(editor));
    AddModule(std::static_pointer_cast<Module>(render));
}

Application& Application::GetInstance() {
    static Application instance;
    return instance;
}

void Application::AddModule(std::shared_ptr<Module> module) {
    module->Init();
    moduleList.push_back(module);
}

// Called before render is available
bool Application::Awake() {
    LOG("Application::Awake");

    std::cout << "current directory: " << std::filesystem::current_path() << std::endl;


    if (!ResourceManager::GetInstance().Initialize())
    {
        return false;
    }

    bool result = true;
    for (const auto& module : moduleList) {
        result = module->Awake();
        if (!result) {
            break;
        }
    }

    return result;
}

// Called before the first frame
bool Application::Start() {
    LOG("Application::Start");

    Time::Init();

    bool result = true;
    for (const auto& module : moduleList) {
        result = module->Start();
        if (!result) {
            break;
        }
    }

    if (result)
    {

        int unprocessed = ResourceManager::GetInstance().CountUnprocessedAssets();

        if (unprocessed > 0)
        {

            ResourceManager::GetInstance().ProcessUnprocessedAssets();
        }
    }

    return result;
}

// Called each loop iteration
bool Application::Update() {
    bool ret = true;
    PrepareUpdate();

    if (input->GetWindowEvent(WE_QUIT) == true)
        ret = false;

    if (ret == true)
        ret = PreUpdate();

    if (ret == true)
        ret = DoUpdate();

    if (ret == true)
        ret = PostUpdate();

    FinishUpdate();
    return ret;
}

// Called before quitting
bool Application::CleanUp() {
    LOG("Application::CleanUp");

    ResourceManager::GetInstance().PrintResourceStats();

    bool result = true;
    for (const auto& module : moduleList) {
        result = module->CleanUp();
        if (!result) {
            break;
        }
    }

    return result;
}

// ---------------------------------------------
void Application::PrepareUpdate()
{
    Time::Update();
}

// ---------------------------------------------
void Application::FinishUpdate()
{
}

// Call modules before each loop iteration
bool Application::PreUpdate()
{
    bool result = true;
    for (const auto& module : moduleList) {
        result = module->PreUpdate();
        if (!result) {
            break;
        }
    }

    return result;
}

// Call modules on each loop iteration
bool Application::DoUpdate()
{
    bool result = true;
    for (const auto& module : moduleList) {
        result = module->Update(Time::deltaTime);
        if (!result) {
            break;
        }
    }
    return result;
}

// Call modules after each loop iteration
bool Application::PostUpdate()
{
    bool result = true;
    for (const auto& module : moduleList) {
        result = module->PostUpdate();
        if (!result) {
            break;
        }
    }

    return result;
}