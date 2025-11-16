#pragma once

#include "Component.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ComponentCamera : public Component
{
public:
    ComponentCamera(GameObject* owner)
        : Component(owner, ComponentType::CAMERA)
    {
        LOG("Component Camera created");
        // Default values
        cameraFOV = 60.0f;
        nearPlane = 0.1f;
        farPlane = 100.0f;
    }

    ~ComponentCamera() {}

    // View matrix is dependent from the position on the world
    glm::mat4 GetViewMatrix()
    {
        // Needs the ComponentTransform of the own GameObject
        ComponentTransform* transform = owner->GetComponent<ComponentTransform>();
        if (transform == nullptr)
        {
            // If theres no transform, return identity matrix
            return glm::mat4(1.0f);
        }

        // Calculate the vectors of the camera based on quaternion rotation of the transform
        glm::vec3 pos = transform->position;
        glm::vec3 front = transform->rotation * glm::vec3(0.0f, 0.0f, -1.0f); // Local Vector Front
        glm::vec3 up = transform->rotation * glm::vec3(0.0f, 1.0f, 0.0f);    // Local Vector Up

        return glm::lookAt(pos, pos + front, up);
    }

    // Projection matrix depends of the fov and aspect ratio of the window
    glm::mat4 GetProjectionMatrix(int screenWidth, int screenHeight)
    {
        float aspectRatio = (float)screenWidth / (float)screenHeight;
        return glm::perspective(glm::radians(cameraFOV), aspectRatio, nearPlane, farPlane);
    }

public:

    float cameraFOV;
    float nearPlane;
    float farPlane;
};