#include "Application.h"
#include <iostream>
#include "Log.h"

#include "Window.h"
#include "Input.h"
#include "Render.h"
#include "LoadFiles.h"
#include "ModuleScene.h"
#include "ModuleEditor.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SDL3/SDL_scancode.h>

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
    AddModule(std::static_pointer_cast<Module>(scene));
    AddModule(std::static_pointer_cast<Module>(editor));
    AddModule(std::static_pointer_cast<Module>(render));
    AddModule(std::static_pointer_cast<Module>(loadFiles));

    // Render last 

}

// Static method to get the instance of the Application class, following the singletn pattern
Application& Application::GetInstance() {
    static Application instance; // Guaranteed to be destroyed and instantiated on first use
    return instance;
}

void Application::AddModule(std::shared_ptr<Module> module) {
    module->Init();
    moduleList.push_back(module);
}

// Called before render is available
bool Application::Awake() {

    LOG("Application::Awake");

    //Iterates the module list and calls Awake on each module
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

    //Iterates the module list and calls Start on each module
    bool result = true;
    for (const auto& module : moduleList) {
        result = module->Start();
        if (!result) {
            break;
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

    //Iterates the module list and calls CleanUp on each module
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
    uint64_t currentTime = SDL_GetTicks();
    if (lastFrameTime == 0)
        lastFrameTime = currentTime;

    dt = (currentTime - lastFrameTime) / 1000.0f;
    lastFrameTime = currentTime;
}

// ---------------------------------------------
void Application::FinishUpdate()
{

}

// Call modules before each loop iteration
bool Application::PreUpdate()
{
    //Iterates the module list and calls PreUpdate on each module
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
    //Iterates the module list and calls Update on each module
    bool result = true;
    for (const auto& module : moduleList) {
        result = module->Update(dt);
        if (!result) {
            break;
        }
    }

    return result;
}

// Call modules after each loop iteration
bool Application::PostUpdate()
{
    //Iterates the module list and calls PostUpdate on each module
    bool result = true;
    for (const auto& module : moduleList) {
        result = module->PostUpdate();
        if (!result) {
            break;
        }
    }

    return result;
}