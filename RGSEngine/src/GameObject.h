#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Component.h"
#include "Log.h"
#include "UIDGenerator.h"
#include <cstdint>

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