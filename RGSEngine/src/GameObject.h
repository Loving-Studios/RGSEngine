#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Component.h"
#include "Log.h"
#include "UIDGenerator.h"
#include <cstdint>
#include "ComponentTransform.h"
#include <glm/glm.hpp>

// std::shared_ptr so the memory of the components and childens is auto managed
using std::string;
using std::vector;
using std::shared_ptr;

class GameObject
{
public:
    GameObject(string name) : name(name), parent(nullptr), active(true), uid(UIDGenerator::GenerateUID()) {}

    ~GameObject()
    {
        // Cause we use the shared_ptr it cleans auto the components and the childens
    }

    // Update of GameObject calls to the Update of all the components and then at the update of all of his childrens
    void Update()
    {
        if (!active) return;

        for (auto& component : components)
        {
            if (component->IsActive())
            {
                component->Update();
            }
        }

        for (auto& child : children)
        {
            child->Update();
        }
    }

    // --- Component Management ---

    void AddComponent(shared_ptr<Component> component)
    {
        components.push_back(component);
    }

    // This GetComponent is used to ask the component his type class
    // GetComponent<ComponentTransform>()
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

    void AddChild(shared_ptr<GameObject> child)
    {
        if (child)
        {
            child->parent = this;
            children.push_back(child);
        }
    }

    void RemoveChild(GameObject* child)
    {
        if (child == nullptr) return;

        // Search the shared_ptr matching with the pointer and delete
        children.erase(
            std::remove_if(children.begin(), children.end(),
                [child](const shared_ptr<GameObject>& p) { return p.get() == child; }),
            children.end()
        );

        child->parent = nullptr; // Break the link with the parent
    }

    // Calculate the matrix of the transform world
    glm::mat4 GetGlobalMatrix()
    {
        ComponentTransform* transform = GetComponent<ComponentTransform>();
        glm::mat4 localMatrix = (transform != nullptr) ? transform->GetModelMatrix() : glm::mat4(1.0f);

        if (parent != nullptr)
        {
            // Multiply the global matrix of the parent for the local matrix of the object
            return parent->GetGlobalMatrix() * localMatrix;
        }
        else
        {
            // If theres no parent, its SceneRoot, the local matrix is the global matrix
            return localMatrix;
        }
    }

    // ImGuizmo provides the new global matrix, needs to be calculated the local matrix of the object and separate the position, rotation and scale
    void SetLocalFromGlobal(const glm::mat4& newGlobalMatrix)
    {
        ComponentTransform* transform = GetComponent<ComponentTransform>();
        if (transform == nullptr) return;

        // Calculate the global matrix of the parent
        glm::mat4 parentGlobalMatrix = (parent != nullptr) ? parent->GetGlobalMatrix() : glm::mat4(1.0f);

        // Calculate the new local matrix newLocal = inverse(parentGlobal) * newGlobal
        glm::mat4 newLocalMatrix = glm::inverse(parentGlobalMatrix) * newGlobalMatrix;

        // Separate the local matrix on position, rotation and scale
        glm::vec3 newPos, newScale, skew;
        glm::quat newRot;
        glm::vec4 perspective;

        // Check if the decompose was successful
        if (glm::decompose(newLocalMatrix, newScale, newRot, newPos, skew, perspective))
        {
            // Define minimum scale to avoid negative scale
            const float MIN_SCALE = 0.001f;
            if (newScale.x < MIN_SCALE) newScale.x = MIN_SCALE;
            if (newScale.y < MIN_SCALE) newScale.y = MIN_SCALE;
            if (newScale.z < MIN_SCALE) newScale.z = MIN_SCALE;

            // Assign the new values to the transform
            transform->SetPosition(newPos);
            transform->SetRotation(newRot);
            transform->SetScale(newScale);
        }
    }

    // --- Getters and Setters ---
    const string& GetName() const { return name; }
    GameObject* GetParent() const { return parent; }
    const vector<shared_ptr<GameObject>>& GetChildren() const { return children; }

    bool IsActive() const { return active; }

public:
    string name;
    bool active;
    GameObject* parent; // Pointer to the father, we don't use here the shared_ptr to avoid loops
    uint64_t uid;

    vector<shared_ptr<Component>> components;
    vector<shared_ptr<GameObject>> children;
};