#pragma once

#include "Module.h"
#include "GameObject.h"
#include "SceneState.h"
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

    // Test functions for the pyramid
    void CreatePyramid();
    void CreateTriangle();
    void CreateSquare();
    void CreateRectangle();
    void CreateCube();
    void CreateSphere();
    void CreateEmptyGameObject();

    void AddGameObject(std::shared_ptr<GameObject> gameObject);

    // Simulation Control
    enum class SimulationState
    {
        STOPPED,  // Editor mode - objects can be edited
        PLAYING,  // Simulation running
        PAUSED    // Simulation paused
    };

    void Play();
    void Pause();
    void Stop();
    void Step();

    SimulationState GetSimulationState() const { return simulationState; }
    bool IsPlaying() const { return simulationState == SimulationState::PLAYING; }
    bool IsPaused() const { return simulationState == SimulationState::PAUSED; }
    bool IsStopped() const { return simulationState == SimulationState::STOPPED; }

public:
    // This is a Smart Pointer for the rootObject
    // When the "rootObject" is destroyed, makes the CleanUp auto to all the childrens and components
    std::shared_ptr<GameObject> rootObject;

private:
    SimulationState simulationState;
    SceneState savedState;
};