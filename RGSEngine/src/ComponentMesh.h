#pragma once

#include "Component.h"
#include <glad/glad.h>

class ComponentMesh : public Component
{
public:
    ComponentMesh(GameObject* owner)
        : Component(owner, ComponentType::MESH),
        VAO(0), VBO(0), IBO(0), indexCount(0)
    {
    }

    ~ComponentMesh()
    {
        // CleanUp of the buffers of the GPU when the component is detroyed
        CleanUp();
    }

    void LoadMesh(float* vertices, unsigned int num_vertices,
        unsigned int* indices, unsigned int num_indices,
        float* texCoords = nullptr, float* normals = nullptr)
    {
        // Clear previous buffers if they exist
        CleanUp();

        indexCount = num_indices;

        // Create  VAO
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        // Create  VBO for vertices
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 3, vertices, GL_STATIC_DRAW);

        // Attribute 0: positions (x, y, z)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        if (texCoords != nullptr)
        {
            glGenBuffers(1, &VBO_UV);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_UV);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 2, texCoords, GL_STATIC_DRAW);

            // Attribute 1: UV coordinates (u, v)
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);

            LOG("UV coordinates loaded to GPU (VBO_UV: %d)", VBO_UV);
        }
        else
        {
            LOG("No UV coordinates provided");
        }

        // Index IBO
        glGenBuffers(1, &IBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * num_indices, indices, GL_STATIC_DRAW);

        // Unlink VAO
        glBindVertexArray(0);

        LOG("Mesh loaded to GPU: VAO=%d, VBO=%d, IBO=%d, Vertices=%d, Indices=%d",
            VAO, VBO, IBO, num_vertices, indexCount);
    }

    // Function to draw the mesh
    void Draw()
    {
        if (VAO != 0 && indexCount > 0)
        {
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    void CleanUp()
    {
        if (VAO != 0)
        {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        if (VBO != 0)
        {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (VBO_UV != 0)
        {
            glDeleteBuffers(1, &VBO_UV);
            VBO_UV = 0;
        }
        if (IBO != 0)
        {
            glDeleteBuffers(1, &IBO);
            IBO = 0;
        }
        indexCount = 0;
    }

public:
    unsigned int VAO;
    unsigned int VBO;
    unsigned int VBO_UV;
    unsigned int IBO;
    unsigned int indexCount;
};