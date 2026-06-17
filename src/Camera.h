#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 position  {0.f, 70.f, 0.f};
    float     yaw       = -90.f;
    float     pitch     = 0.f;
    float     speed     = 4.5f;
    float     mouseSens = 0.1f;
    float     fov       = 70.f;

    glm::vec3 front() const;
    glm::vec3 right() const;
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;

    void processMouseMove(float dx, float dy);
};
