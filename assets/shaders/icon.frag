#version 330 core
in vec2 vUV;
uniform sampler2D uAtlas;
out vec4 FragColor;
void main() {
    vec4 c = texture(uAtlas, vUV);
    if (c.a < 0.1) discard;
    FragColor = c;
}
