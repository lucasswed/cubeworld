#version 330 core

in vec2  vTexCoord;
in float vLight;

out vec4 FragColor;

uniform sampler2D uTexture;

void main() {
    vec4 color = texture(uTexture, vTexCoord);
    if (color.a < 0.05)
        discard;

    float light = vLight;

    // Grid lines only on fully opaque tiles (skip for water / future glass)
    if (color.a > 0.9) {
        float tileU = fract(vTexCoord.x * 12.0);
        float tileV  = vTexCoord.y;
        float lw     = 0.05;
        float grid   = (tileU < lw || tileU > 1.0 - lw || tileV < lw || tileV > 1.0 - lw) ? 1.0 : 0.0;
        light *= mix(1.0, 0.35, grid);
    }

    FragColor = vec4(color.rgb * light, color.a);
}
