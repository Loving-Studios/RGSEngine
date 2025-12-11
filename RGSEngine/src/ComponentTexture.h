#pragma once

#include "Component.h"
#include "Application.h"
#include "LoadFiles.h"
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

    void Save(nlohmann::json& j) const override
    {
        j["Type"] = "TEXTURE";
        j["Path"] = path;
        j["LibraryPath"] = libraryPath;
    }

    void Load(const nlohmann::json& j) override
    {
        if (j.contains("Path")) path = j["Path"];
        if (j.contains("LibraryPath")) {
            libraryPath = j["LibraryPath"];

            // Reload the texture with LoadFIles
            TextureHeader header;
            char* buffer = nullptr;
            if (Application::GetInstance().loadFiles->LoadTextureFromCustomFormat(libraryPath.c_str(), header, buffer)) {
                this->textureID = Application::GetInstance().loadFiles->CreateTextureFromBuffer(header, buffer);
                this->width = header.width;
                this->height = header.height;
                delete[] buffer;
            }
        }
    }

public:
    unsigned int textureID;
    int width;
    int height;
    std::string path; // Keep the path for the Inspector
    std::string libraryPath; // Internal path Library/Textures/

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