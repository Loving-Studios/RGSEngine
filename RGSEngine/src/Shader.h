#pragma once

#include <glad/glad.h>
#include <string>

class Shader
{
public:
    
    unsigned int ID;

    // Constructor
    Shader(const char* vertexSource = nullptr, const char* fragmentSource = nullptr);

    
    void Use();

    
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;

private:
   
    void CheckCompileErrors(unsigned int shader, std::string type);
};