#version 330 core

in vec2  vTexCoord;
in float vLight;

out vec4 FragColor;

uniform sampler2D uTexture;

void main() {
    vec4 color = texture(uTexture, vTexCoord);
    if (color.a < 0.1)
        discard;

    // Grid lines: atlas has 4 tiles wide; remap U into per-tile [0,1] space
    float tileU = fract(vTexCoord.x * 4.0);
    float tileV = vTexCoord.y;
    float lw    = 0.06;
    float grid  = (tileU < lw || tileU > 1.0 - lw || tileV < lw || tileV > 1.0 - lw) ? 1.0 : 0.0;

    float light = vLight * mix(1.0, 0.35, grid);
    FragColor = vec4(color.rgb * light, color.a);
}
