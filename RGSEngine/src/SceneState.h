#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>


struct GameObjectState
{
    uint64_t uid;
    std::string name;
    bool active;

    // Transform data
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    // Parent UID 
    uint64_t parentUID;

    // Component active states
    std::vector<bool> componentActiveStates;
};

class SceneState
{
public:
    SceneState() = default;
    ~SceneState() = default;

    // Save the current scene state
    void Capture(class GameObject* rootObject);

    // Restores the scene to saved state
    void Restore(class GameObject* rootObject);

    // Clear the saved state
    void Clear();

    // Check if there's a saved state
    bool IsEmpty() const { return savedStates.empty(); }

private:
    std::vector<GameObjectState> savedStates;

    // Capture a single GameObject and its children recursively
    void CaptureGameObject(GameObject* go, uint64_t parentUID);

    // Restore a single GameObject and its children recursively
    void RestoreGameObject(GameObject* go);

    void CleanupCreatedObjects(GameObject* go, const std::vector<uint64_t>& originalUIDs);
};