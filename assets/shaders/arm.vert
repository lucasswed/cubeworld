#version 330 core
// No vertex attributes — geometry computed entirely from gl_VertexID.
// One box per draw call: caller sets uBoxMin / uBoxMax uniforms.
uniform mat4 uMVP;
uniform vec3 uBoxMin;
uniform vec3 uBoxMax;
// Texture tile columns in the 8-tile atlas (top face, side faces, bottom face).
// Ignored when uUseTexture == 0.
uniform int  uTileTop;
uniform int  uTileSide;
uniform int  uTileBot;
out vec3 vNorm;
out vec2 vUV;

void main(){
    // Corners indexed by 3-bit code: bit0=X, bit1=Y, bit2=Z (0=min, 1=max)
    const int kCorners[24] = int[24](
        1,3,7,5,  // +X
        4,6,2,0,  // -X
        2,6,7,3,  // +Y (top)
        4,0,1,5,  // -Y (bottom)
        5,7,6,4,  // +Z
        0,2,3,1   // -Z
    );
    const vec3 kNorm[6] = vec3[6](
        vec3( 1,0,0), vec3(-1,0,0),
        vec3(0, 1,0), vec3(0,-1,0),
        vec3(0,0, 1), vec3(0,0,-1)
    );
    // Per-quad-corner UV in [0,1]^2 (matches CCW winding of kCorners quads)
    const vec2 kQUV[4] = vec2[4](
        vec2(0,0), vec2(0,1), vec2(1,1), vec2(1,0)
    );

    int id   = gl_VertexID;
    int face = id / 6;
    int vi   = id % 6;

    // Quad split into two triangles: (c0,c1,c2) and (c0,c2,c3)
    int ci     = (vi == 3) ? 0 : (vi == 4) ? 2 : (vi == 5) ? 3 : vi;
    int corner = kCorners[face * 4 + ci];

    vec3 pos = vec3(
        ((corner & 1) != 0) ? uBoxMax.x : uBoxMin.x,
        ((corner & 2) != 0) ? uBoxMax.y : uBoxMin.y,
        ((corner & 4) != 0) ? uBoxMax.z : uBoxMin.z
    );

    gl_Position = uMVP * vec4(pos, 1.0);
    vNorm = kNorm[face];

    // Atlas UV: 8 tiles wide total.  face 2=+Y(top), face 3=-Y(bot), rest=side.
    int tileCol = (face == 2) ? uTileTop : (face == 3) ? uTileBot : uTileSide;
    vec2 qUV = kQUV[ci];
    vUV = vec2((float(tileCol) + qUV.x) / 8.0, qUV.y);
}
