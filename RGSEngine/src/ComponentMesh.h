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
    unsigned int IBO;
    unsigned int indexCount;
};