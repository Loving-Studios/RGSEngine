#include "SceneState.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "Log.h"

void SceneState::Capture(GameObject* rootObject)
{
    if (!rootObject) return;

    savedStates.clear();

    CaptureGameObject(rootObject, 0);

    LOG("Scene state captured: %d objects", (int)savedStates.size());
}

void SceneState::CaptureGameObject(GameObject* go, uint64_t parentUID)
{
    if (!go) return;

    GameObjectState state;
    state.uid = go->uid;
    state.name = go->name;
    state.active = go->active;
    state.parentUID = parentUID;

    // Capture transform
    ComponentTransform* transform = go->GetComponent<ComponentTransform>();
    if (transform)
    {
        state.position = transform->position;
        state.rotation = transform->rotation;
        state.scale = transform->scale;
    }
    else
    {
        state.position = glm::vec3(0.0f);
        state.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        state.scale = glm::vec3(1.0f);
    }

    // Save state of components
    for (const auto& comp : go->components)
    {
        state.componentActiveStates.push_back(comp->IsActive());
    }

    savedStates.push_back(state);

    // Recursively capture children
    for (const auto& child : go->GetChildren())
    {
        CaptureGameObject(child.get(), go->uid);
    }
}

void SceneState::Restore(GameObject* rootObject)
{
    if (!rootObject || savedStates.empty())
    {
        return;
    }

    LOG(" RESTORING SCENE STATE ");

    try
    {
       
        // This avoids the static variable problem
        std::vector<uint64_t> originalUIDs;
        for (const auto& state : savedStates)
        {
            originalUIDs.push_back(state.uid);
        }

        LOG("Original objects in saved state: %d", (int)originalUIDs.size());

        // Cleanup objects created during simulation
        CleanupCreatedObjects(rootObject, originalUIDs);

        // Restore state of original objects
        RestoreGameObject(rootObject);

    }
    catch (const std::exception& e)
    {
        LOG("ERROR during restore: %s", e.what());
    }
}

void SceneState::CleanupCreatedObjects(GameObject* go,
    const std::vector<uint64_t>& originalUIDs)
{
    if (!go) return;

    auto& children = go->children;

    // Iterate backwards to safely erase while iterating
    for (auto it = children.rbegin(); it != children.rend(); )
    {
        if (!(*it))
        {
            it = std::reverse_iterator(children.erase(std::next(it).base()));
            continue;
        }

        // Check if this object existed before simulation
        bool wasInOriginalState = false;
        for (uint64_t uid : originalUIDs)
        {
            if ((*it)->uid == uid)
            {
                wasInOriginalState = true;
                break;
            }
        }

        if (!wasInOriginalState)
        {
            // Object was created during simulation - remove it
            it = std::reverse_iterator(children.erase(std::next(it).base()));
        }
        else
        {
            // Object existed before - recursively cleanup its children
            CleanupCreatedObjects((*it).get(), originalUIDs);
            ++it;
        }
    }
}

void SceneState::RestoreGameObject(GameObject* go)
{
    if (!go) return;

    // Find saved state by UID
    for (const auto& state : savedStates)
    {
        if (state.uid == go->uid)
        {
            // Restore basic properties
            go->name = state.name;
            go->active = state.active;

            // Restore transform
            ComponentTransform* transform = go->GetComponent<ComponentTransform>();
            if (transform)
            {
                transform->SetPosition(state.position);
                transform->SetRotation(state.rotation);
                transform->SetScale(state.scale);
            }

            // Restore component active states
            if (state.componentActiveStates.size() == go->components.size())
            {
                for (size_t i = 0; i < go->components.size(); ++i)
                {
                    if (state.componentActiveStates[i])
                    {
                        go->components[i]->Enable();
                    }
                    else
                    {
                        go->components[i]->Disable();
                    }
                }
            }

            break;
        }
    }

    // Recursively restore children
    for (const auto& child : go->GetChildren())
    {
        if (child)
        {
            RestoreGameObject(child.get());
        }
    }
}

void SceneState::Clear()
{
    savedStates.clear();
    LOG("Scene state cleared");
}