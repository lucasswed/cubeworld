#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

Shader::~Shader() {
    if (m_id) glDeleteProgram(m_id);
}

std::string Shader::readFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[Shader] Cannot open: " << path << "\n";
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint Shader::compileShader(GLenum type, const std::string &src) {
    GLuint id = glCreateShader(type);
    const char *cstr = src.c_str();
    glShaderSource(id, 1, &cstr, nullptr);
    glCompileShader(id);

    GLint ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Compile error:\n" << log << "\n";
        glDeleteShader(id);
        return 0;
    }
    return id;
}

bool Shader::load(const std::string &vertPath, const std::string &fragPath) {
    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);
    if (vertSrc.empty() || fragSrc.empty()) return false;

    GLuint vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!vert || !frag) { glDeleteShader(vert); glDeleteShader(frag); return false; }

    m_id = glCreateProgram();
    glAttachShader(m_id, vert);
    glAttachShader(m_id, frag);
    glLinkProgram(m_id);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Link error:\n" << log << "\n";
        glDeleteProgram(m_id);
        m_id = 0;
        return false;
    }
    return true;
}

void Shader::use() const { glUseProgram(m_id); }

void Shader::setInt(const std::string &name, int v) const {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), v);
}
void Shader::setFloat(const std::string &name, float v) const {
    glUniform1f(glGetUniformLocation(m_id, name.c_str()), v);
}
void Shader::setVec2(const std::string &name, const glm::vec2 &v) const {
    glUniform2fv(glGetUniformLocation(m_id, name.c_str()), 1, glm::value_ptr(v));
}
void Shader::setVec3(const std::string &name, const glm::vec3 &v) const {
    glUniform3fv(glGetUniformLocation(m_id, name.c_str()), 1, glm::value_ptr(v));
}
void Shader::setVec4(const std::string &name, const glm::vec4 &v) const {
    glUniform4fv(glGetUniformLocation(m_id, name.c_str()), 1, glm::value_ptr(v));
}
void Shader::setMat4(const std::string &name, const glm::mat4 &m) const {
    glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, glm::value_ptr(m));
}
