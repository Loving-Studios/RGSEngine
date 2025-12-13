#pragma once

#include <glm/glm.hpp>

// Structure for a 3D plane (defined by equation: ax + by + cz + d = 0)
struct Plane
{
    glm::vec3 normal;
    float distance;

    Plane() : normal(0.0f, 0.0f, 1.0f), distance(0.0f) {}
    Plane(const glm::vec3& n, float d) : normal(n), distance(d) {}

    // Normalise the plane
    void Normalize()
    {
        float length = glm::length(normal);
        // Evitar división por cero por seguridad
        if (length > 0.0f)
        {
            normal /= length;
            distance /= length;
        }
    }

    // Calculates the signed distance from a point to the plane
    float DistanceToPoint(const glm::vec3& point) const
    {
        return glm::dot(normal, point) + distance;
    }
};

// Frustum Structure
struct Frustum
{
    // CAMBIO IMPORTANTE: Renombramos los valores para evitar conflicto con macros de Windows
    enum PlaneType
    {
        PLANE_LEFT = 0,
        PLANE_RIGHT,
        PLANE_BOTTOM,
        PLANE_TOP,
        PLANE_NEAR,
        PLANE_FAR
    };

    Plane planes[6];

    Frustum() {}

    // Extract the 6 planes from the View-Projection matrix
    void ExtractFromMatrix(const glm::mat4& viewProjection)
    {
        // Left plane
        planes[PLANE_LEFT].normal.x = viewProjection[0][3] + viewProjection[0][0];
        planes[PLANE_LEFT].normal.y = viewProjection[1][3] + viewProjection[1][0];
        planes[PLANE_LEFT].normal.z = viewProjection[2][3] + viewProjection[2][0];
        planes[PLANE_LEFT].distance = viewProjection[3][3] + viewProjection[3][0];

        // Right plane
        planes[PLANE_RIGHT].normal.x = viewProjection[0][3] - viewProjection[0][0];
        planes[PLANE_RIGHT].normal.y = viewProjection[1][3] - viewProjection[1][0];
        planes[PLANE_RIGHT].normal.z = viewProjection[2][3] - viewProjection[2][0];
        planes[PLANE_RIGHT].distance = viewProjection[3][3] - viewProjection[3][0];

        // Bottom plane
        planes[PLANE_BOTTOM].normal.x = viewProjection[0][3] + viewProjection[0][1];
        planes[PLANE_BOTTOM].normal.y = viewProjection[1][3] + viewProjection[1][1];
        planes[PLANE_BOTTOM].normal.z = viewProjection[2][3] + viewProjection[2][1];
        planes[PLANE_BOTTOM].distance = viewProjection[3][3] + viewProjection[3][1];

        // Top plane
        planes[PLANE_TOP].normal.x = viewProjection[0][3] - viewProjection[0][1];
        planes[PLANE_TOP].normal.y = viewProjection[1][3] - viewProjection[1][1];
        planes[PLANE_TOP].normal.z = viewProjection[2][3] - viewProjection[2][1];
        planes[PLANE_TOP].distance = viewProjection[3][3] - viewProjection[3][1];

        // Near plane
        planes[PLANE_NEAR].normal.x = viewProjection[0][3] + viewProjection[0][2];
        planes[PLANE_NEAR].normal.y = viewProjection[1][3] + viewProjection[1][2];
        planes[PLANE_NEAR].normal.z = viewProjection[2][3] + viewProjection[2][2];
        planes[PLANE_NEAR].distance = viewProjection[3][3] + viewProjection[3][2];

        // Far plane
        planes[PLANE_FAR].normal.x = viewProjection[0][3] - viewProjection[0][2];
        planes[PLANE_FAR].normal.y = viewProjection[1][3] - viewProjection[1][2];
        planes[PLANE_FAR].normal.z = viewProjection[2][3] - viewProjection[2][2];
        planes[PLANE_FAR].distance = viewProjection[3][3] - viewProjection[3][2];

        // Normalise all planes
        for (int i = 0; i < 6; i++)
        {
            planes[i].Normalize();
        }
    }

    // Whether an AABB is inside (or intersects) the frustum
    bool IsAABBInside(const glm::vec3& minPoint, const glm::vec3& maxPoint) const
    {
        // For each plane of the frustum
        for (int i = 0; i < 6; i++)
        {
            // Calculate the positive vertex (P-Vertex)
            // Este es el punto de la caja más lejano en la dirección de la normal
            glm::vec3 positiveVertex = minPoint;

            if (planes[i].normal.x >= 0) positiveVertex.x = maxPoint.x;
            if (planes[i].normal.y >= 0) positiveVertex.y = maxPoint.y;
            if (planes[i].normal.z >= 0) positiveVertex.z = maxPoint.z;

            // Si el punto más "favorable" está detrás del plano, toda la caja está fuera
            if (planes[i].DistanceToPoint(positiveVertex) < 0)
            {
                return false;
            }
        }
        return true;
    }

    // Whether a point is inside the frustum
    bool IsPointInside(const glm::vec3& point) const
    {
        for (int i = 0; i < 6; i++)
        {
            if (planes[i].DistanceToPoint(point) < 0)
            {
                return false;
            }
        }
        return true;
    }

    // Whether a sphere is inside the frustum
    bool IsSphereInside(const glm::vec3& center, float radius) const
    {
        for (int i = 0; i < 6; i++)
        {
            if (planes[i].DistanceToPoint(center) < -radius)
            {
                return false;
            }
        }
        return true;
    }
};