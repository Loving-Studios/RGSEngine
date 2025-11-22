#include "GameObject.h"
#include "ComponentTransform.h"
#include "Log.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

GameObject::GameObject(string name)
    : name(name), parent(nullptr), active(true), uid(UIDGenerator::GenerateUID())
{
}

GameObject::~GameObject()
{
    // Cause we use the shared_ptr it cleans auto the components and the childens
}

void GameObject::Update()
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

void GameObject::AddComponent(shared_ptr<Component> component)
{
    components.push_back(component);
}

void GameObject::AddChild(shared_ptr<GameObject> child)
{
    if (child)
    {
        child->parent = this;
        children.push_back(child);
    }
}

void GameObject::RemoveChild(GameObject* child)
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

glm::mat4 GameObject::GetGlobalMatrix()
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

void GameObject::SetLocalFromGlobal(const glm::mat4& newGlobalMatrix)
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

bool GameObject::IsAncestorOf(GameObject* potentialChild)
{
    GameObject* current = potentialChild->parent;
    while (current != nullptr)
    {
        if (current == this) return true;
        current = current->parent;
    }
    return false;
}

void GameObject::SetParent(GameObject* newParent)
{
    if (parent == newParent) return;

    // Save the actual global transform before moving
    glm::mat4 globalMatrix = GetGlobalMatrix();

    // Manage shared_ptr ownership transfer
    shared_ptr<GameObject> myPtr = nullptr;

    if (parent != nullptr)
    {
        auto& brothers = parent->children;
        for (auto it = brothers.begin(); it != brothers.end(); ++it)
        {
            if (it->get() == this)
            {
                myPtr = *it; // Copy the shared_ptr (increase ref count)
                brothers.erase(it); // Remove form the old parent
                break;
            }
        }
    }

    // If we successfully retrieved the shared_ptr (we were not an orphan or root)
    if (myPtr != nullptr)
    {
        // Assign the new father
        if (newParent != nullptr)
        {
            newParent->children.push_back(myPtr);
            parent = newParent;
        }
        else
        {
            // If newParent is null, it typically means moving to SceneRoot.
            // This logic should be handled by the caller or by passing SceneRoot explicitly.
            LOG("Warning: SetParent(nullptr) called. Object logic might be incomplete if not attached to SceneRoot.");
        }
    }

    // Recalculate the local transform to mantain the visual position
    SetLocalFromGlobal(globalMatrix);
}

const string& GameObject::GetName() const { return name; }
GameObject* GameObject::GetParent() const { return parent; }
const vector<shared_ptr<GameObject>>& GameObject::GetChildren() const { return children; }
bool GameObject::IsActive() const { return active; }