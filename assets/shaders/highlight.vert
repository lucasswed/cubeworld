#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uBlockPos;

void main() {
    gl_Position = uProjection * uView * vec4(aPos + uBlockPos, 1.0);
}
