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

  
    for (const auto& child : go->GetChildren())
    {
        CaptureGameObject(child.get(), go->uid);
    }
}

void SceneState::Restore(GameObject* rootObject)
{
    if (!rootObject || savedStates.empty()) return;

    LOG(" RESTORING SCENE STATE ");

    try
    {
        // Mark new objects for removal
        CleanupCreatedObjects(rootObject);

        // Restore the state of original objects
        RestoreGameObject(rootObject);

        LOG("Scene state restored successfully");
    }
    catch (const std::exception& e)
    {
        LOG("ERROR during restore: %s", e.what());
    }
}

void SceneState::CleanupCreatedObjects(GameObject* go)
{
    if (!go) return;

    // Create list of original UIDs 
    static std::vector<uint64_t> originalUIDs;
    static bool initialized = false;

    if (!initialized)
    {
        for (const auto& state : savedStates)
        {
            originalUIDs.push_back(state.uid);
        }
        initialized = true;
    }

   
    auto& children = go->children;

   
    for (auto it = children.begin(); it != children.end(); )
    {
        if (!(*it))
        {
            it = children.erase(it);
            continue;
        }

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
            // Created during Play
            LOG("Removing created object: %s (UID: %llu)",
                (*it)->GetName().c_str(), (*it)->uid);

           
            CleanupCreatedObjects((*it).get());

          
            it = children.erase(it);
        }
        else
        {
           
            CleanupCreatedObjects((*it).get());
            ++it;
        }
    }

    // Reset when finished
    static bool isRoot = true;
    if (isRoot)
    {
        isRoot = false;
    }
    else if (go->parent == nullptr)
    {
        
        originalUIDs.clear();
        initialized = false;
        isRoot = true;
    }
}

void SceneState::RestoreGameObject(GameObject* go)
{
    if (!go) return;

    // Find saved status by UID
    for (const auto& state : savedStates)
    {
        if (state.uid == go->uid)
        {
            go->name = state.name;
            go->active = state.active;

            ComponentTransform* transform = go->GetComponent<ComponentTransform>();
            if (transform)
            {
                transform->SetPosition(state.position);
                transform->SetRotation(state.rotation);
                transform->SetScale(state.scale);
            }

            // Restore state of each component
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