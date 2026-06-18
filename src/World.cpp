#include "World.h"
#include <cmath>
#include <algorithm>

// ─── Value noise helpers ──────────────────────────────────────────────────────

static float fade(float t) { return t * t * (3.f - 2.f * t); }
static float lerp(float a, float b, float t) { return a + (b - a) * t; }

static float hashf2(int x, int z, int seed) {
    uint32_t h = (uint32_t)(x * 1619 ^ z * 31337 ^ seed * 6971);
    h ^= h >> 13; h *= 0x85ebca6bu; h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.f;
}

// 2-D smooth value noise [0, 1]
static float noise2(float x, float z, int seed) {
    int ix = (int)floorf(x), iz = (int)floorf(z);
    float ux = fade(x - ix), uz = fade(z - iz);
    return lerp(lerp(hashf2(ix,   iz,   seed), hashf2(ix+1, iz,   seed), ux),
                lerp(hashf2(ix,   iz+1, seed), hashf2(ix+1, iz+1, seed), ux), uz);
}

// Fractional Brownian Motion 2-D — returns [0, 1]
static float fbm2(float x, float z, int seed, int oct = 5) {
    float v = 0, amp = 0.5f, freq = 1.f, sum = 0;
    for (int i = 0; i < oct; i++) {
        v += noise2(x * freq, z * freq, seed + i) * amp;
        sum += amp; amp *= 0.5f; freq *= 2.f;
    }
    return v / sum;
}

static float hashf3(int x, int y, int z, int seed) {
    uint32_t h = (uint32_t)(x * 1619 ^ y * 1013 ^ z * 31337 ^ seed * 6971);
    h ^= h >> 13; h *= 0x85ebca6bu; h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.f;
}

// 3-D smooth value noise [0, 1]
static float noise3(float x, float y, float z, int seed) {
    int ix = (int)floorf(x), iy = (int)floorf(y), iz = (int)floorf(z);
    float ux = fade(x - ix), uy = fade(y - iy), uz = fade(z - iz);
    float c000 = hashf3(ix,   iy,   iz,   seed);
    float c100 = hashf3(ix+1, iy,   iz,   seed);
    float c010 = hashf3(ix,   iy+1, iz,   seed);
    float c110 = hashf3(ix+1, iy+1, iz,   seed);
    float c001 = hashf3(ix,   iy,   iz+1, seed);
    float c101 = hashf3(ix+1, iy,   iz+1, seed);
    float c011 = hashf3(ix,   iy+1, iz+1, seed);
    float c111 = hashf3(ix+1, iy+1, iz+1, seed);
    return lerp(lerp(lerp(c000, c100, ux), lerp(c010, c110, ux), uy),
                lerp(lerp(c001, c101, ux), lerp(c011, c111, ux), uy), uz);
}

// ─── Terrain parameters ───────────────────────────────────────────────────────

static constexpr int SEA_LEVEL = 62;

// World Y of the top solid block at (wx, wz). Range roughly [30, 115].
static int terrainHeight(int wx, int wz) {
    // Three noise layers at different scales
    float cont = fbm2(wx / 400.f, wz / 400.f,  0, 5); // continental
    float hill = fbm2(wx / 80.f,  wz / 80.f,   7, 4); // hills
    float surf = fbm2(wx / 20.f,  wz / 20.f,  13, 3); // surface detail

    // S-curve on continent: deepen oceans, push mountains higher
    cont = cont * cont * (3.f - 2.f * cont);
    cont = std::max(0.f, std::min(1.f, cont * 1.4f - 0.2f));

    float h = cont * 0.60f + hill * 0.28f + surf * 0.12f;
    return 30 + (int)(h * 85.f);  // [30, 115]
}

// True if the block at (wx, wy, wz) should be a cave void.
// Only applies to underground stone — preserves the dirt/grass cap.
static bool isCave(int wx, int wy, int wz) {
    if (wy <= 3 || wy > 50) return false;
    const float S = 14.f;
    float n1 = noise3(wx / S,          wy / (S * 0.6f),          wz / S,          500);
    float n2 = noise3(wx / S + 213.7f, wy / (S * 0.6f) + 131.4f, wz / S + 97.2f,  600);
    // Tunnel where both noise fields are near 0.5
    return std::abs(n1 - 0.5f) < 0.09f && std::abs(n2 - 0.5f) < 0.09f;
}

// ─── Tree planting ────────────────────────────────────────────────────────────

static uint32_t chunkRng(int cx, int cz) {
    uint32_t s = (uint32_t)(cx * 1619 + cz * 31337 + 9001);
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return s;
}
static uint32_t rngNext(uint32_t &s) {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return s;
}

static void plantTree(Chunk &c, int x, int z, int baseY, int height) {
    for (int i = 0; i < height; i++)
        c.setBlock(x, baseY + i, z, BlockType::Wood);

    int topY = baseY + height - 1;

    // Top 2 leaf layers: 3×3 minus corners
    for (int dy = 0; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            for (int dz = -1; dz <= 1; dz++)
                if (!(std::abs(dx) == 1 && std::abs(dz) == 1))
                    if (c.getBlock(x+dx, topY+dy, z+dz) == BlockType::Air)
                        c.setBlock(x+dx, topY+dy, z+dz, BlockType::Leaves);

    // Lower 2 leaf layers: 5×5 minus far diagonals
    for (int dy = -2; dy <= -1; dy++)
        for (int dx = -2; dx <= 2; dx++)
            for (int dz = -2; dz <= 2; dz++)
                if (std::abs(dx) + std::abs(dz) <= 3)
                    if (c.getBlock(x+dx, topY+dy, z+dz) == BlockType::Air)
                        c.setBlock(x+dx, topY+dy, z+dz, BlockType::Leaves);
}

// ─── World ────────────────────────────────────────────────────────────────────

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

void World::generateChunk(Chunk &c) {
    int baseX = c.chunkX * CHUNK_W;
    int baseZ = c.chunkZ * CHUNK_D;

    for (int x = 0; x < CHUNK_W; x++)
    for (int z = 0; z < CHUNK_D; z++) {
        int wx = baseX + x, wz = baseZ + z;
        int h  = terrainHeight(wx, wz);

        for (int y = 0; y < CHUNK_H; y++) {
            BlockType bt = BlockType::Air;

            if (y == 0) {
                bt = BlockType::Stone;                         // unbreakable base
            } else if (y < h - 4) {
                bt = BlockType::Stone;
            } else if (y < h) {
                bt = BlockType::Dirt;
            } else if (y == h) {
                bt = (h > SEA_LEVEL) ? BlockType::Grass : BlockType::Dirt;
            }

            // Carve caves only through stone (preserves the dirt/grass cap)
            if (bt == BlockType::Stone && isCave(wx, y, wz))
                bt = BlockType::Air;

            c.setBlock(x, y, z, bt);
        }
    }

    // Fill ocean basins with water up to sea level
    for (int x = 0; x < CHUNK_W; x++)
    for (int z = 0; z < CHUNK_D; z++) {
        int wx = baseX + x, wz = baseZ + z;
        int h  = terrainHeight(wx, wz);
        if (h >= SEA_LEVEL) continue;
        for (int y = h + 1; y <= SEA_LEVEL; y++)
            if (c.getBlock(x, y, z) == BlockType::Air)
                c.setBlock(x, y, z, BlockType::Water);
    }

    // Trees — only on grass columns above sea level
    uint32_t rng = chunkRng(c.chunkX, c.chunkZ);
    int numTrees = rngNext(rng) % 4;
    for (int t = 0; t < numTrees; t++) {
        int tx = 2 + (int)(rngNext(rng) % (CHUNK_W - 4));
        int tz = 2 + (int)(rngNext(rng) % (CHUNK_D - 4));
        int surfH = terrainHeight(baseX + tx, baseZ + tz);
        if (surfH > SEA_LEVEL && c.getBlock(tx, surfH, tz) == BlockType::Grass) {
            int th = 4 + (int)(rngNext(rng) % 3);
            plantTree(c, tx, tz, surfH + 1, th);
        }
    }

    c.needsRebuild = true;
}

int World::surfaceAt(int wx, int wz) {
    for (int y = CHUNK_H - 1; y >= 0; y--)
        if (blockIsSolid(getBlock(wx, y, wz))) return y;
    return 0;
}

BlockType World::getBlock(int x, int y, int z) const {
    if (y < 0 || y >= CHUNK_H) return BlockType::Air;
    int cx = (int)std::floor((float)x / CHUNK_W);
    int cz = (int)std::floor((float)z / CHUNK_D);
    const Chunk *c = getChunk(cx, cz);
    if (!c) return BlockType::Air;
    return c->getBlock(x - cx * CHUNK_W, y, z - cz * CHUNK_D);
}

void World::setBlock(int x, int y, int z, BlockType t) {
    if (y < 0 || y >= CHUNK_H) return;
    int cx = (int)std::floor((float)x / CHUNK_W);
    int cz = (int)std::floor((float)z / CHUNK_D);
    int lx = x - cx * CHUNK_W;
    int lz = z - cz * CHUNK_D;
    Chunk *c = getOrCreate(cx, cz);
    c->setBlock(lx, y, lz, t);

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

void World::drawTransparent() const {
    for (auto &[key, chunk] : m_chunks)
        chunk->drawTransparent();
}

// DDA voxel raycast
RaycastResult World::raycast(const glm::vec3 &origin, const glm::vec3 &dir, float maxDist) const {
    RaycastResult res;
    glm::vec3 d = glm::normalize(dir);
    glm::ivec3 pos = { (int)std::floor(origin.x), (int)std::floor(origin.y), (int)std::floor(origin.z) };
    glm::vec3  stepF = { d.x < 0 ? -1.f : 1.f, d.y < 0 ? -1.f : 1.f, d.z < 0 ? -1.f : 1.f };
    glm::ivec3 step  = { (int)stepF.x, (int)stepF.y, (int)stepF.z };

    constexpr float BIG = 1e30f;
    auto delta = [](float v) { return std::abs(v) < 1e-7f ? BIG : std::abs(1.f / v); };
    glm::vec3 tDelta = { delta(d.x), delta(d.y), delta(d.z) };

    auto tmax_init = [](float orig, float pf, float s, float td) -> float {
        if (td >= 1e29f) return 1e30f;
        return (s > 0 ? (pf + 1.f - orig) : (orig - pf)) * td;
    };
    glm::vec3 tMax;
    tMax.x = tmax_init(origin.x, (float)pos.x, stepF.x, tDelta.x);
    tMax.y = tmax_init(origin.y, (float)pos.y, stepF.y, tDelta.y);
    tMax.z = tmax_init(origin.z, (float)pos.z, stepF.z, tDelta.z);

    glm::ivec3 normal = {0,0,0};
    float dist = 0.f;

    while (dist < maxDist) {
        if (blockIsSolid(getBlock(pos.x, pos.y, pos.z))) {
            res.hit = true; res.blockPos = pos; res.faceNorm = normal;
            return res;
        }
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            dist = tMax.x; tMax.x += tDelta.x; normal = {-step.x, 0, 0}; pos.x += step.x;
        } else if (tMax.y < tMax.z) {
            dist = tMax.y; tMax.y += tDelta.y; normal = {0, -step.y, 0}; pos.y += step.y;
        } else {
            dist = tMax.z; tMax.z += tDelta.z; normal = {0, 0, -step.z}; pos.z += step.z;
        }
    }
    return res;
}
