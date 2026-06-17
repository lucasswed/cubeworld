#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

static constexpr glm::vec3 UP {0.f, 1.f, 0.f};

glm::vec3 Camera::front() const {
    float yR = glm::radians(yaw);
    float pR = glm::radians(pitch);
    return glm::normalize(glm::vec3{
        std::cos(yR) * std::cos(pR),
        std::sin(pR),
        std::sin(yR) * std::cos(pR)
    });
}

glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(front(), UP));
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position, position + front(), UP);
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, 0.05f, 1000.f);
}

void Camera::processMouseMove(float dx, float dy) {
    yaw   += dx * mouseSens;
    pitch += dy * mouseSens;
    pitch = std::clamp(pitch, -89.f, 89.f);
}
