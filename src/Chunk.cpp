#include "Chunk.h"
#include "World.h"
#include <cstring>

static constexpr float TILE_W = 1.0f / 12.0f;  // 12-tile atlas

struct FaceInfo {
    float corners[4][3];
    float light;
};

static const FaceInfo k_faces[6] = {
    { {{1,0,0},{1,0,1},{1,1,1},{1,1,0}},  0.6f }, // +X
    { {{0,0,1},{0,0,0},{0,1,0},{0,1,1}},  0.6f }, // -X
    { {{0,1,0},{1,1,0},{1,1,1},{0,1,1}},  1.0f }, // +Y top
    { {{0,0,1},{1,0,1},{1,0,0},{0,0,0}},  0.3f }, // -Y bottom
    { {{1,0,1},{0,0,1},{0,1,1},{1,1,1}},  0.8f }, // +Z
    { {{0,0,0},{1,0,0},{1,1,0},{0,1,0}},  0.8f }, // -Z
};

static const int k_nbrOff[6][3] = {
    {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
};

Chunk::Chunk(int cx, int cz) : chunkX(cx), chunkZ(cz) {
    memset(m_blocks, 0, sizeof(m_blocks));
}

Chunk::~Chunk() {
    if (m_vao)      { glDeleteVertexArrays(1,&m_vao);      glDeleteBuffers(1,&m_vbo);      glDeleteBuffers(1,&m_ebo); }
    if (m_transVao) { glDeleteVertexArrays(1,&m_transVao); glDeleteBuffers(1,&m_transVbo); glDeleteBuffers(1,&m_transEbo); }
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (x<0||x>=CHUNK_W||y<0||y>=CHUNK_H||z<0||z>=CHUNK_D) return BlockType::Air;
    return m_blocks[x][y][z];
}

void Chunk::setBlock(int x, int y, int z, BlockType t) {
    if (x<0||x>=CHUNK_W||y<0||y>=CHUNK_H||z<0||z>=CHUNK_D) return;
    m_blocks[x][y][z] = t;
    needsRebuild = true;
}

void Chunk::addFace(std::vector<Vertex> &verts, std::vector<uint32_t> &idx,
                    int x, int y, int z, int face, int texCol, float wave)
{
    const FaceInfo &fi = k_faces[face];
    float u0 = texCol * TILE_W;
    float u1 = u0 + TILE_W;
    float uvs[4][2] = { {u0,1},{u1,1},{u1,0},{u0,0} };

    uint32_t base = (uint32_t)verts.size();
    float wx = (float)(chunkX * CHUNK_W + x);
    float wy = (float)y;
    float wz = (float)(chunkZ * CHUNK_D + z);

    for (int i = 0; i < 4; i++) {
        verts.push_back({
            wx + fi.corners[i][0],
            wy + fi.corners[i][1],
            wz + fi.corners[i][2],
            uvs[i][0], uvs[i][1],
            fi.light,
            wave
        });
    }
    idx.insert(idx.end(), {base, base+3, base+2, base, base+2, base+1});
}

void Chunk::buildMesh(const World &world) {
    std::vector<Vertex>   verts,     transVerts;
    std::vector<uint32_t> idx,       transIdx;
    verts.reserve(4096);       idx.reserve(6144);
    transVerts.reserve(1024);  transIdx.reserve(1536);

    for (int x = 0; x < CHUNK_W; x++)
    for (int y = 0; y < CHUNK_H; y++)
    for (int z = 0; z < CHUNK_D; z++) {
        BlockType bt = m_blocks[x][y][z];
        if (bt == BlockType::Air) continue;

        bool isWater = blockIsWaterlike(bt);
        BlockTextures tex = blockTextures(bt);
        int texPerFace[6] = { tex.posX, tex.negX, tex.posY, tex.negY, tex.posZ, tex.negZ };

        for (int f = 0; f < 6; f++) {
            int nx = x + k_nbrOff[f][0];
            int ny = y + k_nbrOff[f][1];
            int nz = z + k_nbrOff[f][2];

            BlockType nbr;
            if (nx>=0&&nx<CHUNK_W&&ny>=0&&ny<CHUNK_H&&nz>=0&&nz<CHUNK_D)
                nbr = m_blocks[nx][ny][nz];
            else
                nbr = world.getBlock(chunkX*CHUNK_W+nx, ny, chunkZ*CHUNK_D+nz);

            // Cull face if fully occluded, or if both blocks are the same waterlike type
            if (blockOccludesFace(nbr)) continue;
            if (isWater && blockIsWaterlike(nbr)) continue;

            if (isWater) {
                // Face 2 = +Y (top): wave animation flag
                float wave = (f == 2) ? 1.0f : 0.0f;
                addFace(transVerts, transIdx, x, y, z, f, texPerFace[f], wave);
            } else {
                addFace(verts, idx, x, y, z, f, texPerFace[f], 0.f);
            }
        }
    }

    uploadTo(m_vao,      m_vbo,      m_ebo,      m_indexCount,      verts,      idx);
    uploadTo(m_transVao, m_transVbo, m_transEbo, m_transIndexCount, transVerts, transIdx);
    needsRebuild = false;
}

void Chunk::uploadTo(GLuint &vao, GLuint &vbo, GLuint &ebo, int &count,
                     const std::vector<Vertex> &verts, const std::vector<uint32_t> &idx)
{
    if (!vao) { glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glGenBuffers(1,&ebo); }

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(Vertex), verts.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size()*sizeof(uint32_t), idx.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,u));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,light));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,wave));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
    count = (int)idx.size();
}

void Chunk::draw() const {
    if (m_indexCount == 0) return;
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
}

void Chunk::drawTransparent() const {
    if (m_transIndexCount == 0) return;
    glBindVertexArray(m_transVao);
    glDrawElements(GL_TRIANGLES, m_transIndexCount, GL_UNSIGNED_INT, nullptr);
}
