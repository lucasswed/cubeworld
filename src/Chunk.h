#pragma once
#include "Block.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

constexpr int CHUNK_W = 16;
constexpr int CHUNK_H = 128;
constexpr int CHUNK_D = 16;

// A single vertex in the chunk mesh
struct Vertex {
    float x, y, z;
    float u, v;
    float light;
    float wave;  // 1.0 on water-top vertices (drives wave animation), else 0
};

class Chunk {
public:
    int chunkX = 0; // chunk grid coords (not world coords)
    int chunkZ = 0;

    Chunk(int cx, int cz);
    ~Chunk();

    BlockType getBlock(int x, int y, int z) const;
    void      setBlock(int x, int y, int z, BlockType t);

    // Re-build GPU mesh from block data. Call after any setBlock().
    void buildMesh(const class World &world);

    // Draw the already-built mesh (no shader binding — caller does that).
    void draw()            const;
    void drawTransparent() const;

    bool needsRebuild = true;

private:
    BlockType m_blocks[CHUNK_W][CHUNK_H][CHUNK_D]{};

    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
    int    m_indexCount = 0;
    GLuint m_transVao = 0, m_transVbo = 0, m_transEbo = 0;
    int    m_transIndexCount = 0;

    void uploadTo(GLuint &vao, GLuint &vbo, GLuint &ebo, int &count,
                  const std::vector<Vertex> &verts, const std::vector<uint32_t> &idx);
    void addFace(std::vector<Vertex> &verts,
                 std::vector<uint32_t> &idx,
                 int x, int y, int z, int face, int texCol, float wave = 0.f);
};
