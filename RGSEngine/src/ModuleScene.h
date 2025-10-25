#pragma once

#include "Module.h"
#include "GameObject.h"
#include <vector>
#include <memory> // Used for the std::shared_ptr

class ModuleScene : public Module
{
public:
    ModuleScene();
    ~ModuleScene();

    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    // Test function for the pyramid
    void CreateTestPyramid();

public:
    // This is a Smart Pointer for the rootObject
    // When the "rootObject" is destroyed, makes the CleanUp auto to all the childrens and components
    std::shared_ptr<GameObject> rootObject;
};