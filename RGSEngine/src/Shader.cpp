#include "Shader.h"
#include "Log.h"
#include <iostream>


namespace DefaultShaders
{
    const char* vertexShader = R"(
    #version 460 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
        TexCoord = aTexCoord;
    }
    )";

    const char* fragmentShader = R"(
    #version 460 core
    out vec4 FragColor;

    in vec2 TexCoord;
    uniform sampler2D tex1;

    void main()
    {
        FragColor = texture(tex1, TexCoord); 
    }
    )";
}

Shader::Shader(const char* vertexSource, const char* fragmentSource)
{
    
    if (vertexSource == nullptr)
        vertexSource = DefaultShaders::vertexShader;
    if (fragmentSource == nullptr)
        fragmentSource = DefaultShaders::fragmentShader;

    // vertex shader
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexSource, NULL);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");

    // fragment shader
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentSource, NULL);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");

    // create program and link shaders
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    CheckCompileErrors(ID, "PROGRAM");

    // remove shaders are already linked
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::Use()
{
    glUseProgram(ID);
}

void Shader::SetBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::CheckCompileErrors(unsigned int shader, std::string type)
{
    int success;
    char infoLog[1024];

    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            LOG("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s", type.c_str(), infoLog);
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            LOG("ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s", type.c_str(), infoLog);
        }
    }
}