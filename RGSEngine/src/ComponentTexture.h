#pragma once

#include "Component.h"
#include <glad/glad.h>
#include <string>

class ComponentTexture : public Component
{
public:
    ComponentTexture(GameObject* owner)
        : Component(owner, ComponentType::TEXTURE),
        textureID(0), width(0), height(0),
        useDefaultTexture(false), originalTextureID(0)
    {
    }

    ~ComponentTexture()
    {
        CleanUp();
    }

    // Function to link the texture
    void Bind()
    {
        if (textureID != 0)
        {
            glBindTexture(GL_TEXTURE_2D, textureID);
        }
    }

    // Function to unlink
    void Unbind()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void CleanUp()
    {
        if (textureID != 0)
        {
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }
    }

public:
    unsigned int textureID;
    int width;
    int height;
    std::string path; // Keep the path for the Inspector

    bool useDefaultTexture;
    unsigned int originalTextureID;
    std::string originalPath;

    // Alpha Test
    bool enableAlphaTest = false;
    float alphaThreshold = 0.5f;

    // Blending
    bool enableBlending = false;
    GLenum blendSrc = GL_SRC_ALPHA;
    GLenum blendDst = GL_ONE_MINUS_SRC_ALPHA;
};