#pragma once
#include "Block.h"
#include "Chunk.h"
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

// Pack chunk coords into a single 64-bit key
inline int64_t chunkKey(int cx, int cz) {
    return (static_cast<int64_t>(cx) << 32) | (static_cast<uint32_t>(cz));
}

struct RaycastResult {
    bool      hit       = false;
    glm::ivec3 blockPos = {0,0,0}; // world position of hit block
    glm::ivec3 faceNorm = {0,0,0}; // face normal (which side was hit)
};

class World {
public:
    World();

    // Get/set block by world coordinates
    BlockType getBlock(int x, int y, int z) const;
    void      setBlock(int x, int y, int z, BlockType t);

    // Rebuild at most maxPerFrame dirty chunks, then draw all loaded chunks
    void update(int maxPerFrame = 4);
    void draw()            const;
    void drawTransparent() const;

    // Cast a ray from origin in dir, max distance. Returns hit info.
    RaycastResult raycast(const glm::vec3 &origin, const glm::vec3 &dir, float maxDist = 8.f) const;

    // Load/unload chunks around a center chunk position
    void loadAround(int cx, int cz, int radius = 6);

    // Y of the highest solid block at world column (wx, wz). Triggers chunk generation.
    int surfaceAt(int wx, int wz);

private:
    std::unordered_map<int64_t, std::unique_ptr<Chunk>> m_chunks;

    Chunk *getChunk(int cx, int cz) const;
    Chunk *getOrCreate(int cx, int cz);
    void   generateChunk(Chunk &c);
};
