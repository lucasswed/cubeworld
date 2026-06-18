#version 330 core
in vec3 vNorm;
in vec2 vUV;
uniform vec4      uColor;
uniform int       uUseTexture;   // 0 = flat uColor, 1 = sample uAtlas
uniform sampler2D uAtlas;
out vec4 FragColor;
void main(){
    float d = max(dot(normalize(vNorm), normalize(vec3(0.6, 1.0, 0.4))), 0.0);
    float light = 0.50 + 0.50 * d;
    if (uUseTexture != 0) {
        vec4 tc = texture(uAtlas, vUV);
        if (tc.a < 0.1) discard;
        FragColor = vec4(tc.rgb * light, tc.a);
    } else {
        FragColor = vec4(uColor.rgb * light, uColor.a);
    }
}
