#pragma once

#include <glm/glm.hpp>
#include <limits>
#include "Log.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"

struct AABB;
class GameObject;
class ComponentMesh;
struct Ray;

namespace Raycast {
    // We use “struct Ray” to refer to the global structure.
    bool RayAABBIntersection(const struct Ray& ray, const AABB& aabb, float& tMin);
}

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;

    Ray() : origin(0.0f), direction(0.0f, 0.0f, -1.0f) {}
    Ray(glm::vec3 orig, glm::vec3 dir) : origin(orig), direction(glm::normalize(dir)) {}

    // Point on the ray: P(t) = origin + t * direction
    glm::vec3 PointAt(float t) const
    {
        return origin + t * direction;
    }

    bool IntersectsAABB(const AABB& aabb) const;
    bool IntersectsAABB(const AABB& aabb, float& distance) const;
};

namespace Raycast
{
    inline Ray ScreenPointToRay(
        int mouseX, int mouseY,
        int screenWidth, int screenHeight,
        const glm::mat4& viewMatrix,
        const glm::mat4& projectionMatrix)
    {
        float x = (2.0f * mouseX) / screenWidth - 1.0f;
        float y = 1.0f - (2.0f * mouseY) / screenHeight;

        glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
        glm::mat4 invProjection = glm::inverse(projectionMatrix);
        glm::vec4 rayEye = invProjection * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

        glm::mat4 invView = glm::inverse(viewMatrix);
        glm::vec4 rayWorld = invView * rayEye;
        glm::vec3 rayDirection = glm::normalize(glm::vec3(rayWorld));
        glm::vec3 cameraPosition = glm::vec3(invView[3]);

        return Ray(cameraPosition, rayDirection);
    }

    inline bool RayAABBIntersection(const Ray& ray, const AABB& aabb, float& tMin)
    {
        glm::vec3 invDir = 1.0f / ray.direction;

        glm::vec3 t0 = (aabb.minPoint - ray.origin) * invDir;
        glm::vec3 t1 = (aabb.maxPoint - ray.origin) * invDir;

        glm::vec3 tmin = glm::min(t0, t1);
        glm::vec3 tmax = glm::max(t0, t1);

        tMin = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
        float tMax = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

        if (tMax < 0.0f) return false;
        if (tMin > tMax) return false;
        if (tMin < 0.0f) tMin = tMax;

        return true;
    }

    inline bool RayTriangleIntersection(const Ray& ray, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t)
    {
        const float EPSILON = 0.0000001f;
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        glm::vec3 h = glm::cross(ray.direction, edge2);
        float a = glm::dot(edge1, h);

        if (a > -EPSILON && a < EPSILON) return false;

        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);

        if (u < 0.0f || u > 1.0f) return false;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(ray.direction, q);

        if (v < 0.0f || u + v > 1.0f) return false;

        t = f * glm::dot(edge2, q);
        return (t > EPSILON);
    }

    inline bool RayMeshIntersection(const Ray& ray, ComponentMesh* mesh, const glm::mat4& modelMatrix, float& outDistance)
    {
        if (mesh == nullptr || mesh->cpuVertices.empty() || mesh->cpuIndices.empty())
            return false;

        bool hit = false;
        float closestT = std::numeric_limits<float>::max();

        for (size_t i = 0; i < mesh->cpuIndices.size(); i += 3)
        {
            unsigned int i0 = mesh->cpuIndices[i];
            unsigned int i1 = mesh->cpuIndices[i + 1];
            unsigned int i2 = mesh->cpuIndices[i + 2];

            glm::vec3 v0(mesh->cpuVertices[i0 * 3], mesh->cpuVertices[i0 * 3 + 1], mesh->cpuVertices[i0 * 3 + 2]);
            glm::vec3 v1(mesh->cpuVertices[i1 * 3], mesh->cpuVertices[i1 * 3 + 1], mesh->cpuVertices[i1 * 3 + 2]);
            glm::vec3 v2(mesh->cpuVertices[i2 * 3], mesh->cpuVertices[i2 * 3 + 1], mesh->cpuVertices[i2 * 3 + 2]);

            glm::vec4 v0World = modelMatrix * glm::vec4(v0, 1.0f);
            glm::vec4 v1World = modelMatrix * glm::vec4(v1, 1.0f);
            glm::vec4 v2World = modelMatrix * glm::vec4(v2, 1.0f);

            float t = 0.0f;
            if (RayTriangleIntersection(ray, glm::vec3(v0World), glm::vec3(v1World), glm::vec3(v2World), t))
            {
                if (t < closestT)
                {
                    closestT = t;
                    hit = true;
                }
            }
        }

        if (hit)
        {
            outDistance = closestT;
            return true;
        }
        return false;
    }

    inline bool IntersectGameObject(const Ray& ray, GameObject* go, float& outDistance)
    {
        if (go == nullptr || !go->IsActive()) return false;

        float tMinAABB = 0.0f;
        if (!RayAABBIntersection(ray, go->globalAABB, tMinAABB)) return false;

        ComponentMesh* mesh = go->GetComponent<ComponentMesh>();
        if (mesh != nullptr)
        {
            glm::mat4 modelMatrix = go->GetGlobalMatrix();
            return RayMeshIntersection(ray, mesh, modelMatrix, outDistance);
        }
        return false;
    }

    inline void FindIntersectionsRecursive(const Ray& ray, GameObject* go, GameObject*& closestObject, float& closestDistance)
    {
        if (go == nullptr || !go->IsActive()) return;

        float tMinAABB = 0.0f;
        if (RayAABBIntersection(ray, go->globalAABB, tMinAABB))
        {
            if (tMinAABB < closestDistance)
            {
                ComponentMesh* mesh = go->GetComponent<ComponentMesh>();
                if (mesh != nullptr)
                {
                    glm::mat4 modelMatrix = go->GetGlobalMatrix();
                    float meshDistance = 0.0f;
                    if (RayMeshIntersection(ray, mesh, modelMatrix, meshDistance))
                    {
                        if (meshDistance < closestDistance)
                        {
                            closestDistance = meshDistance;
                            closestObject = go;
                        }
                    }
                }
            }
        }

        for (const auto& child : go->GetChildren())
        {
            FindIntersectionsRecursive(ray, child.get(), closestObject, closestDistance);
        }
    }

    inline GameObject* FindClosestIntersection(const Ray& ray, GameObject* root, float& outDistance)
    {
        GameObject* closestObject = nullptr;
        float closestDistance = std::numeric_limits<float>::max();

        FindIntersectionsRecursive(ray, root, closestObject, closestDistance);

        outDistance = closestDistance;
        return closestObject;
    }
}

inline bool Ray::IntersectsAABB(const AABB& aabb) const
{
    float tMin = 0.0f;
    // We call the global function
    return Raycast::RayAABBIntersection(*this, aabb, tMin);
}

inline bool Ray::IntersectsAABB(const AABB& aabb, float& distance) const
{
    // We call the global function
    return Raycast::RayAABBIntersection(*this, aabb, distance);
}