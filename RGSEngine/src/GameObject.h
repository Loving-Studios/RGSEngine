#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>

#include "Component.h"
#include "UIDGenerator.h"

// Forward declaration to avoid circular dependency
class ComponentTransform;

// std::shared_ptr so the memory of the components and childens is auto managed
using std::string;
using std::vector;
using std::shared_ptr;

class GameObject
{
public:
    GameObject(string name);

    ~GameObject();

    // Update of GameObject calls to the Update of all the components and then at the update of all of his childrens
    void Update();

    // --- Component Management ---
    void AddComponent(shared_ptr<Component> component);

    // This GetComponent is used to ask the component his type class
        // GetComponent<ComponentTransform>()
        // Templates must remain in the header file
    template<typename T>
    T* GetComponent() const
    {
        for (auto& component : components)
        {
            // Converting pointer to type T
            T* castedComponent = dynamic_cast<T*>(component.get());
            if (castedComponent != nullptr)
            {
                return castedComponent;
            }
        }
        return nullptr;
    }

    // --- Child Management ---
    void AddChild(shared_ptr<GameObject> child);
    void RemoveChild(GameObject* child);

    // Moves this object to a new parent maintaining its world position
    void SetParent(GameObject* newParent);

    // Checks if this object is an ancestor of potentialChild to avoid cycles
    bool IsAncestorOf(GameObject* potentialChild);

    // Calculate the matrix of the transform world
    glm::mat4 GetGlobalMatrix();

    // ImGuizmo provides the new global matrix, needs to be calculated the local matrix of the object and separate the position, rotation and scale
    void SetLocalFromGlobal(const glm::mat4& newGlobalMatrix);

    // --- Getters and Setters ---
    const string& GetName() const;
    GameObject* GetParent() const;
    const vector<shared_ptr<GameObject>>& GetChildren() const;
    bool IsActive() const;

public:
    string name;
    bool active;
    GameObject* parent; // Pointer to the father, we don't use here the shared_ptr to avoid loops
    uint64_t uid;

    vector<shared_ptr<Component>> components;
    vector<shared_ptr<GameObject>> children;
};