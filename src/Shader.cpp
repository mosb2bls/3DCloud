#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

// Utility function to read the contents of a shader file
static std::string readFileContent(const char* filepath)
{
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit); // Enable exceptions for file errors
    std::stringstream content;
    try {
        file.open(filepath);
        content << file.rdbuf(); // Read file contents into a string stream
        file.close();
    } catch(std::ifstream::failure& e) {
        std::cerr << "Error::Shader::File not successfully read: " << filepath << std::endl;
    }
    return content.str();
}

// Constructor for the Shader class: Loads, compiles, and links vertex and fragment shaders
Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    // 1. Read shader source code from files
    std::string vCode = readFileContent(vertexPath);
    std::string fCode = readFileContent(fragmentPath);
    const char* vShaderCode = vCode.c_str();
    const char* fShaderCode = fCode.c_str();

    // 2. Compile vertex shader
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    {
        int success;
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if(!success) {
            char infoLog[1024];
            glGetShaderInfoLog(vertex, 1024, NULL, infoLog);
            std::cerr << "Error::Shader::Vertex::Compilation failed\n" << infoLog << std::endl;
        }
    }

    // 3. Compile fragment shader
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    {
        int success;
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if(!success) {
            char infoLog[1024];
            glGetShaderInfoLog(fragment, 1024, NULL, infoLog);
            std::cerr << "Error::Shader::Fragment::Compilation failed\n" << infoLog << std::endl;
        }
    }

    // 4. Link shaders into a single shader program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    {
        int success;
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success) {
            char infoLog[1024];
            glGetProgramInfoLog(ID, 1024, NULL, infoLog);
            std::cerr << "Error::Shader::Program::Linking failed\n" << infoLog << std::endl;
        }
    }

    // 5. Delete individual shaders after linking (no longer needed)
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

// Activates the shader program
void Shader::use() const
{
    glUseProgram(ID);
}

// Utility functions to set uniform values in the shader program

// Sets a boolean uniform
void Shader::setBool(const std::string &name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

// Sets an integer uniform
void Shader::setInt(const std::string &name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

// Sets a float uniform
void Shader::setFloat(const std::string &name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

// Sets a 3D vector uniform
void Shader::setVec3(const std::string &name, const glm::vec3 &value) const
{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

// Sets a 4D vector uniform
void Shader::setVec4(const std::string &name, const glm::vec4 &value) const
{
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

// Sets a 4x4 matrix uniform
void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

// Sets a 2D vector uniform
void Shader::setVec2(const std::string &name, const glm::vec2 &value) const
{
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}
