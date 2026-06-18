#include "Chunk.h"
#include "World.h"
#include <cstring>

// Atlas: 8 tiles × 1 tile (128 × 16 px). Each tile occupies 0.125 of U.
static constexpr float TILE_W = 1.0f / 8.0f;

// Per-face: normal direction and light value (simple directional occlusion)
struct FaceInfo {
    // 4 corner offsets from (x,y,z) — wound counter-clockwise from outside
    float corners[4][3];
    float light;
};

static const FaceInfo k_faces[6] = {
    // +X
    { {{1,0,0},{1,0,1},{1,1,1},{1,1,0}},  0.6f },
    // -X
    { {{0,0,1},{0,0,0},{0,1,0},{0,1,1}},  0.6f },
    // +Y (top)
    { {{0,1,0},{1,1,0},{1,1,1},{0,1,1}},  1.0f },
    // -Y (bottom)
    { {{0,0,1},{1,0,1},{1,0,0},{0,0,0}},  0.3f },
    // +Z
    { {{1,0,1},{0,0,1},{0,1,1},{1,1,1}},  0.8f },
    // -Z
    { {{0,0,0},{1,0,0},{1,1,0},{0,1,0}},  0.8f },
};

// Neighbor offset per face
static const int k_nbrOff[6][3] = {
    {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
};

Chunk::Chunk(int cx, int cz) : chunkX(cx), chunkZ(cz) {
    memset(m_blocks, 0, sizeof(m_blocks));
}

Chunk::~Chunk() {
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); glDeleteBuffers(1, &m_vbo); glDeleteBuffers(1, &m_ebo); }
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_W || y < 0 || y >= CHUNK_H || z < 0 || z >= CHUNK_D)
        return BlockType::Air;
    return m_blocks[x][y][z];
}

void Chunk::setBlock(int x, int y, int z, BlockType t) {
    if (x < 0 || x >= CHUNK_W || y < 0 || y >= CHUNK_H || z < 0 || z >= CHUNK_D) return;
    m_blocks[x][y][z] = t;
    needsRebuild = true;
}

void Chunk::addFace(std::vector<Vertex> &verts, std::vector<uint32_t> &idx,
                    int x, int y, int z, int face, int texCol)
{
    const FaceInfo &fi = k_faces[face];
    float u0 = texCol * TILE_W;
    float u1 = u0 + TILE_W;
    // UV corners: bottom-left, bottom-right, top-right, top-left (matches winding)
    float uvs[4][2] = { {u0,1}, {u1,1}, {u1,0}, {u0,0} };

    uint32_t base = static_cast<uint32_t>(verts.size());
    float wx = static_cast<float>(chunkX * CHUNK_W + x);
    float wy = static_cast<float>(y);
    float wz = static_cast<float>(chunkZ * CHUNK_D + z);

    for (int i = 0; i < 4; i++) {
        verts.push_back({
            wx + fi.corners[i][0],
            wy + fi.corners[i][1],
            wz + fi.corners[i][2],
            uvs[i][0], uvs[i][1],
            fi.light
        });
    }
    // Two triangles — reversed winding so outward normals face OUT (CCW from outside)
    idx.insert(idx.end(), {base, base+3, base+2, base, base+2, base+1});
}

void Chunk::buildMesh(const World &world) {
    std::vector<Vertex>   verts;
    std::vector<uint32_t> idx;
    verts.reserve(4096);
    idx.reserve(6144);

    for (int x = 0; x < CHUNK_W; x++)
    for (int y = 0; y < CHUNK_H; y++)
    for (int z = 0; z < CHUNK_D; z++) {
        BlockType bt = m_blocks[x][y][z];
        if (bt == BlockType::Air) continue;

        BlockTextures tex = blockTextures(bt);
        int texPerFace[6] = { tex.posX, tex.negX, tex.posY, tex.negY, tex.posZ, tex.negZ };

        for (int f = 0; f < 6; f++) {
            int nx = x + k_nbrOff[f][0];
            int ny = y + k_nbrOff[f][1];
            int nz = z + k_nbrOff[f][2];

            // Check neighbor — may be in an adjacent chunk
            BlockType neighbor;
            if (nx >= 0 && nx < CHUNK_W && ny >= 0 && ny < CHUNK_H && nz >= 0 && nz < CHUNK_D) {
                neighbor = m_blocks[nx][ny][nz];
            } else {
                neighbor = world.getBlock(chunkX * CHUNK_W + nx,
                                          ny,
                                          chunkZ * CHUNK_D + nz);
            }

            if (!blockIsSolid(neighbor)) {
                addFace(verts, idx, x, y, z, f, texPerFace[f]);
            }
        }
    }

    uploadMesh(verts, idx);
    needsRebuild = false;
}

void Chunk::uploadMesh(const std::vector<Vertex> &verts, const std::vector<uint32_t> &idx) {
    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);
    }

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(uint32_t), idx.data(), GL_DYNAMIC_DRAW);

    // layout 0: position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    // layout 1: texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
    glEnableVertexAttribArray(1);
    // layout 2: light
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, light));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m_indexCount = static_cast<int>(idx.size());
}

void Chunk::draw() const {
    if (m_indexCount == 0) return;
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
}
