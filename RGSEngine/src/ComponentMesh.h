#pragma once

#include "Component.h"
#include <glad/glad.h>
#include "Log.h"
#include <vector>

class ComponentMesh : public Component
{
public:
    ComponentMesh(GameObject* owner)
        : Component(owner, ComponentType::MESH),
        VAO(0), VBO(0), IBO(0), VBO_UV(0), VBO_Normals(0),
        indexCount(0),
        normalsVAO(0), normalsVBO(0), normalVertexCount(0)
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

        if (normals != nullptr)
        {
            glGenBuffers(1, &VBO_Normals);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_Normals);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices * 3, normals, GL_STATIC_DRAW);

            // Attribute 2: Normals (nx, ny, nz)
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(2);

            LOG("Normals loaded to GPU (VBO_Normals: %d)", VBO_Normals);

            // Setup of buffers to show normals
            SetupNormalsBuffers(vertices, num_vertices, normals);

            glBindVertexArray(VAO);
        }
        else
        {
            LOG("No normals provided");
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

    void SetupNormalsBuffers(float* vertices, unsigned int num_vertices, float* normals)
    {
        const float NORMAL_LINE_LENGTH = 0.2f; // Length of the normal line
        normalVertexCount = num_vertices * 2; // 2 vertex for line
        std::vector<float> lineData(normalVertexCount * 3); // 3 floats for vertex (xyz)

        for (unsigned int i = 0; i < num_vertices; ++i)
        {
            // Starting point, vertex
            lineData[i * 6 + 0] = vertices[i * 3 + 0];
            lineData[i * 6 + 1] = vertices[i * 3 + 1];
            lineData[i * 6 + 2] = vertices[i * 3 + 2];

            // Ending point (vertex + normal * Length)
            lineData[i * 6 + 3] = vertices[i * 3 + 0] + normals[i * 3 + 0] * NORMAL_LINE_LENGTH;
            lineData[i * 6 + 4] = vertices[i * 3 + 1] + normals[i * 3 + 1] * NORMAL_LINE_LENGTH;
            lineData[i * 6 + 5] = vertices[i * 3 + 2] + normals[i * 3 + 2] * NORMAL_LINE_LENGTH;
        }

        // Create VAO and VBO for the lines of the normals
        glGenVertexArrays(1, &normalsVAO);
        glBindVertexArray(normalsVAO);

        glGenBuffers(1, &normalsVBO);
        glBindBuffer(GL_ARRAY_BUFFER, normalsVBO);
        glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_STATIC_DRAW);

        // Only needed the attribute of position (layout 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Unbind
        glBindVertexArray(0);

        LOG("Normals visualization buffers created: VAO=%d, VBO=%d, Lines=%d", normalsVAO, normalsVBO, num_vertices);
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

    void DrawNormals()
    {
        if (normalsVAO != 0 && normalVertexCount > 0)
        {
            glBindVertexArray(normalsVAO);
            // Draw GL_LINES, using the VBO prepared on the SetupNormalsBuffers
            glDrawArrays(GL_LINES, 0, normalVertexCount);
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
        if (VBO_Normals != 0)
        {
            glDeleteBuffers(1, &VBO_Normals);
            VBO_Normals = 0;
        }
        if (normalsVAO != 0)
        {
            glDeleteVertexArrays(1, &normalsVAO);
            normalsVAO = 0;
        }
        if (normalsVBO != 0)
        {
            glDeleteBuffers(1, &normalsVBO);
            normalsVBO = 0;
        }
        normalVertexCount = 0;
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
    unsigned int VBO_Normals;
    unsigned int IBO;
    unsigned int indexCount;

    unsigned int normalsVAO;
    unsigned int normalsVBO;
    unsigned int normalVertexCount;
};