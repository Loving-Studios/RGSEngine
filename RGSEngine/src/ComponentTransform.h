#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class ComponentTransform : public Component
{
public:
    // Constructor
    ComponentTransform(GameObject* owner)
        : Component(owner, ComponentType::TRANSFORM),
        position(0.0f, 0.0f, 0.0f),
        rotation(1.0f, 0.0f, 0.0f, 0.0f), // Identity quaternion without rotation
        scale(1.0f, 1.0f, 1.0f)
    {
    }

    ~ComponentTransform() {}

    // --- Main methods ---

    // Calculates and retourns the Model Matrix
    glm::mat4 GetModelMatrix() const
    {
        // 1. Creates the traslation matrix
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);

        // 2. Creates the rotation matrix form the quaternion given
        glm::mat4 rot = glm::mat4_cast(rotation);

        // 3. Creates the matrix of the scaling
        glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);

        // The final matrix is T * R * S
        return trans * rot * sca;
    }

    // --- Setters (so we can modify them form the inspector ---

    void SetPosition(const glm::vec3& newPos)
    {
        position = newPos;
    }

    void SetRotation(const glm::quat& newRot)
    {
        rotation = newRot;
    }

    void SetScale(const glm::vec3& newScale)
    {
        scale = newScale;
    }

public:
    glm::vec3 position;
    glm::quat rotation; // Used the quaternion to avoid the "Gimbal Lock"
    glm::vec3 scale;
};