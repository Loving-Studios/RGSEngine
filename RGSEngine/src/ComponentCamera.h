#pragma once

#include "Component.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "Log.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

class ComponentCamera : public Component
{
public:
    ComponentCamera(GameObject* owner)
        : Component(owner, ComponentType::CAMERA)
    {
        LOG("Component Camera created");
        cameraFOV = 60.0f;
        nearPlane = 0.1f;
        farPlane = 100.0f;
    }

    ~ComponentCamera() {}

    // The View Matrix is calculated from the transform of the GameObject
    glm::mat4 GetViewMatrix()
    {
        ComponentTransform* transform = owner->GetComponent<ComponentTransform>();
        if (transform == nullptr)
        {
            LOG("WARNING: Camera component owner has no Transform!");
            return glm::mat4(1.0f);
        }

        // The front vector and up vector are obtained applying the rotation, Quaternion, to the vectors of the world 
        glm::vec3 pos = transform->position;
        glm::vec3 front = transform->rotation * glm::vec3(0.0f, 0.0f, -1.0f); // Local Front Vector
        glm::vec3 up = transform->rotation * glm::vec3(0.0f, 1.0f, 0.0f);    // Local Up Vector

        // lookAt(position, where to look, up vector)
        return glm::lookAt(pos, pos + front, up);
    }

    // The projection matrix is calculated from the properties of the camera
    glm::mat4 GetProjectionMatrix(int screenWidth, int screenHeight)
    {
        float aspectRatio = (float)screenWidth / (float)screenHeight;
        return glm::perspective(glm::radians(cameraFOV), aspectRatio, nearPlane, farPlane);
    }

public:
    // Properties of the camera that can be editable at the inspector
    float cameraFOV;
    float nearPlane;
    float farPlane;
};
