#version 330 core

layout (location = 0) in vec3  aPos;
layout (location = 1) in vec2  aTexCoord;
layout (location = 2) in float aLight;
layout (location = 3) in float aWave;

out vec2  vTexCoord;
out float vLight;

uniform mat4  uView;
uniform mat4  uProjection;
uniform float uTime;

void main() {
    vec3 pos = aPos;
    // Gentle wave motion on water-surface vertices (aWave == 1.0)
    pos.y += aWave * 0.045 * sin(uTime * 1.4 + aPos.x * 0.9 + aPos.z * 0.7);

    gl_Position = uProjection * uView * vec4(pos, 1.0);

    // Slow UV scroll for water (all wave vertices)
    vTexCoord = aTexCoord + vec2(0.0, aWave * fract(uTime * 0.03));
    vLight    = aLight;
}
