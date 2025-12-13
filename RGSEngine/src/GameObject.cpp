#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "ComponentCamera.h"
#include "ResourceManager.h"
#include "Log.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <cfloat>

GameObject::GameObject(string name)
    : name(name), parent(nullptr), active(true), uid(UIDGenerator::GenerateUID())
{
    localAABB.minPoint = glm::vec3(0.0f);
    localAABB.maxPoint = glm::vec3(0.0f);
    globalAABB = localAABB;
}

GameObject::~GameObject()
{
    LOG("GameObject destroyed: %s (UID: %llu)", name.c_str(), uid);

   
    ReleaseResourceReferences();

}

void GameObject::ReleaseResourceReferences()
{
    // Release mesh reference
    ComponentMesh* mesh = GetComponent<ComponentMesh>();
    if (mesh && !mesh->path.empty() && mesh->path.find("Primitive_") == std::string::npos)
    {
        auto& rm = ResourceManager::GetInstance();
        auto it = rm.assetPathToID.find(mesh->path);
        if (it != rm.assetPathToID.end())
        {
            rm.ReleaseResourceReference(it->second);
            LOG("  Released mesh reference: %s", mesh->path.c_str());
        }
    }

    // Release texture reference
    ComponentTexture* texture = GetComponent<ComponentTexture>();
    if (texture && !texture->path.empty() &&
        texture->path != "default_checker" &&
        !texture->useDefaultTexture)
    {
        auto& rm = ResourceManager::GetInstance();
        auto it = rm.assetPathToID.find(texture->path);
        if (it != rm.assetPathToID.end())
        {
            rm.ReleaseResourceReference(it->second);
            LOG("  Released texture reference: %s", texture->path.c_str());
        }
    }

    // Recursively release references from children
    for (auto& child : children)
    {
        if (child)
        {
            child->ReleaseResourceReferences();
        }
    }
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

    UpdateAABB();

    for (auto& child : children)
    {
        child->Update();
    }
}

void GameObject::UpdateAABB()
{
    // Obtain the global array of the object
    glm::mat4 globalTransform = GetGlobalMatrix();

    // Los 8 vértices del AABB local
    glm::vec3 min = localAABB.minPoint;
    glm::vec3 max = localAABB.maxPoint;

    // If the AABB has not been initialised (mesh not loaded), exit
    if (min == glm::vec3(FLT_MAX) || (min == glm::vec3(0.0f) && max == glm::vec3(0.0f)))
    {
        globalAABB.minPoint = glm::vec3(0.0f);
        globalAABB.maxPoint = glm::vec3(0.0f);
        return;
    }

    glm::vec3 corners[8] = {
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(max.x, max.y, max.z)
    };

    // Reset the global AABB
    globalAABB.minPoint = glm::vec3(FLT_MAX);
    globalAABB.maxPoint = glm::vec3(-FLT_MAX);

    // Transform each corner and expand the global AABB
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 transformed = globalTransform * glm::vec4(corners[i], 1.0f);
        glm::vec3 newPos = glm::vec3(transformed);

        globalAABB.minPoint = glm::min(globalAABB.minPoint, newPos);
        globalAABB.maxPoint = glm::max(globalAABB.maxPoint, newPos);
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

    LOG("Removing child: %s from parent: %s", child->name.c_str(), name.c_str());

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

void GameObject::Save(nlohmann::json& j)
{
    j["UID"] = this->uid;
    j["Name"] = this->name;
    j["Active"] = this->active;

    // Save the UID of the father for reference
    if (parent) j["ParentUID"] = parent->uid;

    // Components array
    j["Components"] = nlohmann::json::array();
    for (const auto& component : components)
    {
        nlohmann::json compJson;
        component->Save(compJson);
        j["Components"].push_back(compJson);
    }

    // Child array
    j["Children"] = nlohmann::json::array();
    for (const auto& child : children)
    {
        nlohmann::json childJson;
        child->Save(childJson);
        j["Children"].push_back(childJson);
    }
}

void GameObject::Load(const nlohmann::json& j)
{
    // Load basic data
    if (j.contains("UID")) this->uid = j["UID"];
    if (j.contains("Name")) this->name = j["Name"];
    if (j.contains("Active")) this->active = j["Active"];

    // Load Component
    if (j.contains("Components"))
    {
        for (const auto& compJson : j["Components"])
        {
            std::string type = compJson.value("Type", "UNKNOWN");
            std::shared_ptr<Component> newComponent = nullptr;

            if (type == "TRANSFORM")
            {
                // Create the transform
                newComponent = std::make_shared<ComponentTransform>(this);
            }
            else if (type == "MESH")
            {
                newComponent = std::make_shared<ComponentMesh>(this);
            }
            else if (type == "TEXTURE")
            {
                newComponent = std::make_shared<ComponentTexture>(this);
            }
            else if (type == "CAMERA")
            {
                newComponent = std::make_shared<ComponentCamera>(this);
            }

            // If created, load the data and fill
            if (newComponent)
            {
                newComponent->Load(compJson);
                this->AddComponent(newComponent);
            }
        }
    }

    // Load Childs
    if (j.contains("Children"))
    {
        for (const auto& childJson : j["Children"])
        {
            // Extract name for the Child
            std::string childName = childJson.value("Name", "Unnamed");

            // Create
            std::shared_ptr<GameObject> childGO = std::make_shared<GameObject>(childName);

            // Call to Load himself
            childGO->Load(childJson);

            // Add object
            this->AddChild(childGO);
        }
    }
}