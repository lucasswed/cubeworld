#include "World.h"
#include <cmath>
#include <algorithm>

World::World() {}

Chunk *World::getChunk(int cx, int cz) const {
    auto it = m_chunks.find(chunkKey(cx, cz));
    return it != m_chunks.end() ? it->second.get() : nullptr;
}

Chunk *World::getOrCreate(int cx, int cz) {
    auto key = chunkKey(cx, cz);
    auto it  = m_chunks.find(key);
    if (it == m_chunks.end()) {
        auto c = std::make_unique<Chunk>(cx, cz);
        generateChunk(*c);
        it = m_chunks.emplace(key, std::move(c)).first;
    }
    return it->second.get();
}

// Flat world: a few layers of stone, one of dirt, one of grass
void World::generateChunk(Chunk &c) {
    for (int x = 0; x < CHUNK_W; x++)
    for (int z = 0; z < CHUNK_D; z++) {
        c.setBlock(x, 0,  z, BlockType::Stone);
        for (int y = 1; y <= 60; y++) c.setBlock(x, y, z, BlockType::Stone);
        c.setBlock(x, 61, z, BlockType::Dirt);
        c.setBlock(x, 62, z, BlockType::Dirt);
        c.setBlock(x, 63, z, BlockType::Grass);
    }
    c.needsRebuild = true;
}

BlockType World::getBlock(int x, int y, int z) const {
    if (y < 0 || y >= CHUNK_H) return BlockType::Air;
    int cx = (int)std::floor((float)x / CHUNK_W);
    int cz = (int)std::floor((float)z / CHUNK_D);
    int lx = x - cx * CHUNK_W;
    int lz = z - cz * CHUNK_D;
    const Chunk *c = getChunk(cx, cz);
    if (!c) return BlockType::Air;
    return c->getBlock(lx, y, lz);
}

void World::setBlock(int x, int y, int z, BlockType t) {
    if (y < 0 || y >= CHUNK_H) return;
    int cx = (int)std::floor((float)x / CHUNK_W);
    int cz = (int)std::floor((float)z / CHUNK_D);
    int lx = x - cx * CHUNK_W;
    int lz = z - cz * CHUNK_D;
    Chunk *c = getOrCreate(cx, cz);
    c->setBlock(lx, y, lz, t);

    // If block is on a chunk border, mark neighbor dirty too
    auto markNeighbor = [&](int ncx, int ncz) {
        Chunk *n = getChunk(ncx, ncz);
        if (n) n->needsRebuild = true;
    };
    if (lx == 0)           markNeighbor(cx - 1, cz);
    if (lx == CHUNK_W - 1) markNeighbor(cx + 1, cz);
    if (lz == 0)           markNeighbor(cx, cz - 1);
    if (lz == CHUNK_D - 1) markNeighbor(cx, cz + 1);
}

void World::loadAround(int cx, int cz, int radius) {
    for (int dx = -radius; dx <= radius; dx++)
    for (int dz = -radius; dz <= radius; dz++)
        getOrCreate(cx + dx, cz + dz);
}

void World::update(int maxPerFrame) {
    int rebuilt = 0;
    for (auto &[key, chunk] : m_chunks) {
        if (chunk->needsRebuild) {
            chunk->buildMesh(*this);
            if (++rebuilt >= maxPerFrame) break;
        }
    }
}

void World::draw() const {
    for (auto &[key, chunk] : m_chunks)
        chunk->draw();
}

// DDA-style voxel ray cast
RaycastResult World::raycast(const glm::vec3 &origin, const glm::vec3 &dir, float maxDist) const {
    RaycastResult res;

    glm::vec3 d = glm::normalize(dir);
    glm::ivec3 pos = { (int)std::floor(origin.x), (int)std::floor(origin.y), (int)std::floor(origin.z) };

    glm::vec3 stepF = { d.x < 0 ? -1.f : 1.f, d.y < 0 ? -1.f : 1.f, d.z < 0 ? -1.f : 1.f };
    glm::ivec3 step = { (int)stepF.x, (int)stepF.y, (int)stepF.z };

    // When a direction component is ~0, that axis should never advance dist.
    // Use 1e30 (not 1e7!) so tMax for that axis is always larger than maxDist.
    constexpr float BIG = 1e30f;
    auto delta = [](float v) { return std::abs(v) < 1e-7f ? BIG : std::abs(1.f / v); };
    glm::vec3 tDelta = { delta(d.x), delta(d.y), delta(d.z) };

    glm::vec3 tMax;
    auto tmax_init = [](float orig, float pf, float s, float td) -> float {
        if (td >= 1e29f) return 1e30f;  // axis is ~0 — never step it
        return (s > 0 ? (pf + 1.f - orig) : (orig - pf)) * td;
    };
    tMax.x = tmax_init(origin.x, (float)pos.x, stepF.x, tDelta.x);
    tMax.y = tmax_init(origin.y, (float)pos.y, stepF.y, tDelta.y);
    tMax.z = tmax_init(origin.z, (float)pos.z, stepF.z, tDelta.z);

    glm::ivec3 normal = {0,0,0};
    float dist = 0.f;

    while (dist < maxDist) {
        if (blockIsSolid(getBlock(pos.x, pos.y, pos.z))) {
            res.hit      = true;
            res.blockPos = pos;
            res.faceNorm = normal;
            return res;
        }

        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            dist   = tMax.x; tMax.x += tDelta.x;
            normal = {-step.x, 0, 0}; pos.x += step.x;
        } else if (tMax.y < tMax.z) {
            dist   = tMax.y; tMax.y += tDelta.y;
            normal = {0, -step.y, 0}; pos.y += step.y;
        } else {
            dist   = tMax.z; tMax.z += tDelta.z;
            normal = {0, 0, -step.z}; pos.z += step.z;
        }
    }
    return res;
}
