#pragma once

#include "Component.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "Log.h"

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

class ComponentCamera : public Component
{
public:
    ComponentCamera(GameObject* owner)
        : Component(owner, ComponentType::CAMERA),
        frustumVAO(0), frustumVBO(0)
    {
        LOG("Component Camera created");
        cameraFOV = 60.0f;
        nearPlane = 0.1f;
        farPlane = 20.0f;
        aspectRatio = 16.0f / 9.0f;

        GenerateFrustumGizmo();
    }

    ~ComponentCamera()
    {
        if (frustumVAO != 0) glDeleteVertexArrays(1, &frustumVAO);
        if (frustumVBO != 0) glDeleteBuffers(1, &frustumVBO);
    }

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
        if (screenHeight > 0)
            aspectRatio = (float)screenWidth / (float)screenHeight;

        return glm::perspective(glm::radians(cameraFOV), aspectRatio, nearPlane, farPlane);
    }

    void Update() override
    {
        GenerateFrustumGizmo();
    }

    void GenerateFrustumGizmo()
    {
        ComponentTransform* transform = owner->GetComponent<ComponentTransform>();
        if (!transform) return;

        // Calculate planes dimensions based on the tangent of the FOV
        float tanHalfFov = tan(glm::radians(cameraFOV) / 2.0f);

        // Dimensions on Near plane
        float heightNear = 2.0f * tanHalfFov * nearPlane;
        float widthNear = heightNear * aspectRatio;

        // Dimensions on Far plane
        float heightFar = 2.0f * tanHalfFov * farPlane;
        float widthFar = heightFar * aspectRatio;

        // Orient Vector of the transform
        glm::vec3 pos = transform->position;
        glm::vec3 front = transform->rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = transform->rotation * glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::cross(front, up);

        // Plane centers
        glm::vec3 centerNear = pos + front * nearPlane;
        glm::vec3 centerFar = pos + front * farPlane;

        // Calculate the 8 vertex of the piramid
        glm::vec3 ntl = centerNear + (up * (heightNear * 0.5f)) - (right * (widthNear * 0.5f));
        glm::vec3 ntr = centerNear + (up * (heightNear * 0.5f)) + (right * (widthNear * 0.5f));
        glm::vec3 nbl = centerNear - (up * (heightNear * 0.5f)) - (right * (widthNear * 0.5f));
        glm::vec3 nbr = centerNear - (up * (heightNear * 0.5f)) + (right * (widthNear * 0.5f));

        glm::vec3 ftl = centerFar + (up * (heightFar * 0.5f)) - (right * (widthFar * 0.5f));
        glm::vec3 ftr = centerFar + (up * (heightFar * 0.5f)) + (right * (widthFar * 0.5f));
        glm::vec3 fbl = centerFar - (up * (heightFar * 0.5f)) - (right * (widthFar * 0.5f));
        glm::vec3 fbr = centerFar - (up * (heightFar * 0.5f)) + (right * (widthFar * 0.5f));

        // Define the lines
        std::vector<float> vertices = {
            // Near Plane (Square)
            ntl.x, ntl.y, ntl.z, ntr.x, ntr.y, ntr.z,
            ntr.x, ntr.y, ntr.z, nbr.x, nbr.y, nbr.z,
            nbr.x, nbr.y, nbr.z, nbl.x, nbl.y, nbl.z,
            nbl.x, nbl.y, nbl.z, ntl.x, ntl.y, ntl.z,

            // Far Plane (Square)
            ftl.x, ftl.y, ftl.z, ftr.x, ftr.y, ftr.z,
            ftr.x, ftr.y, ftr.z, fbr.x, fbr.y, fbr.z,
            fbr.x, fbr.y, fbr.z, fbl.x, fbl.y, fbl.z,
            fbl.x, fbl.y, fbl.z, ftl.x, ftl.y, ftl.z,

            // Connections Near-Far (Lateral Edges)
            ntl.x, ntl.y, ntl.z, ftl.x, ftl.y, ftl.z,
            ntr.x, ntr.y, ntr.z, ftr.x, ftr.y, ftr.z,
            nbl.x, nbl.y, nbl.z, fbl.x, fbl.y, fbl.z,
            nbr.x, nbr.y, nbr.z, fbr.x, fbr.y, fbr.z
        };

        // Load to GPU
        if (frustumVAO == 0) {
            glGenVertexArrays(1, &frustumVAO);
            glGenBuffers(1, &frustumVBO);
        }

        glBindVertexArray(frustumVAO);
        glBindBuffer(GL_ARRAY_BUFFER, frustumVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void DrawFrustum()
    {
        if (frustumVAO == 0) return;

        glBindVertexArray(frustumVAO);
        glDrawArrays(GL_LINES, 0, 24); // 12 lines * 2 vertex
        glBindVertexArray(0);
    }

public:
    // Properties of the camera that can be editable at the inspector
    float cameraFOV;
    float nearPlane;
    float farPlane;
    float aspectRatio;

    private:

        unsigned int frustumVAO;
        unsigned int frustumVBO;
};
