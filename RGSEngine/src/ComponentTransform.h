#pragma once

#include "Component.h"

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace glm {
    inline void to_json(json& j, const vec3& v) {
        j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
    }

    inline void from_json(const json& j, vec3& v) {
        v.x = j.at("x").get<float>();
        v.y = j.at("y").get<float>();
        v.z = j.at("z").get<float>();
    }

    inline void to_json(json& j, const quat& q) {
        j = json{ {"x", q.x}, {"y", q.y}, {"z", q.z}, {"w", q.w} };
    }

    inline void from_json(const json& j, quat& q) {
        q.x = j.at("x").get<float>();
        q.y = j.at("y").get<float>();
        q.z = j.at("z").get<float>();
        q.w = j.at("w").get<float>();
    }
}

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

    void Save(json& j) const override {
        j["Type"] = "TRANSFORM";
        j["Position"] = position;
        j["Rotation"] = rotation;
        j["Scale"] = scale;
    }

    void Load(const json& j) override {
        if (j.contains("Position")) position = j["Position"];
        if (j.contains("Rotation")) rotation = j["Rotation"];
        if (j.contains("Scale")) scale = j["Scale"];
    }

    // --- Main methods ---

    // Calculates and retourns the Model Matrix
    glm::mat4 GetModelMatrix() const
    {
        // Creation of the traslation matrix
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);

        // Creation of the rotation matrix form the quaternion given
        glm::mat4 rot = glm::mat4_cast(rotation);

        // Creation of the matrix of the scaling
        glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);

        // The final matrix is T * R * S
        return trans * rot * sca;
    }

    // --- Setters (so we can modify them later form the inspector) ---

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