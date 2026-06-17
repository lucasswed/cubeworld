#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    Shader() = default;
    ~Shader();

    bool load(const std::string &vertPath, const std::string &fragPath);
    void use() const;

    void setInt  (const std::string &name, int v)               const;
    void setFloat(const std::string &name, float v)             const;
    void setVec2 (const std::string &name, const glm::vec2 &v)  const;
    void setVec3 (const std::string &name, const glm::vec3 &v)  const;
    void setVec4 (const std::string &name, const glm::vec4 &v)  const;
    void setMat4 (const std::string &name, const glm::mat4 &m)  const;

private:
    GLuint m_id = 0;

    static GLuint compileShader(GLenum type, const std::string &src);
    static std::string readFile(const std::string &path);
};
