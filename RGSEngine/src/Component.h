#pragma once

enum class ComponentType
{
    UNKNOWN = 0,
    TRANSFORM,
    MESH,
    TEXTURE,
    CAMERA
};

// Forward Declaration to avoid the GameObject and the Component is icluded mutuially and creates a loop
class GameObject;

class Component
{
public:
    // The constructor receives the ComponentType and his type
    Component(GameObject* owner, ComponentType type) : owner(owner), type(type), active(true) {}

    // Destructor
    virtual ~Component() {}

    // Virtual Functions so the component childs can implement
    virtual void Update() {}
    virtual void Enable() { active = true; }
    virtual void Disable() { active = false; }

    ComponentType GetType() const { return type; }
    bool IsActive() const { return active; }

public:
    GameObject* owner;      // Pointer to the GameObject
    bool active;            // If the component is active or not
    ComponentType type;     // Type of the component
};