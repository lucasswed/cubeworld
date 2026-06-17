#pragma once
#include <cstdint>

enum class BlockType : uint8_t {
    Air   = 0,
    Grass = 1,
    Dirt  = 2,
    Stone = 3,
    COUNT
};

inline bool blockIsSolid(BlockType t) {
    return t != BlockType::Air;
}

// Texture atlas is 4 tiles wide × 1 tile tall (each tile = 16×16 px in a 64×16 atlas).
// Returns atlas column (0-3) for each face of a block.
// Face order: +X, -X, +Y(top), -Y(bottom), +Z, -Z
struct BlockTextures {
    int posX, negX, posY, negY, posZ, negZ;
};

inline BlockTextures blockTextures(BlockType t) {
    switch (t) {
        case BlockType::Grass:
            return {1, 1, 0, 2, 1, 1}; // sides=grass-side, top=grass-top, bottom=dirt
        case BlockType::Dirt:
            return {2, 2, 2, 2, 2, 2};
        case BlockType::Stone:
            return {3, 3, 3, 3, 3, 3};
        default:
            return {0, 0, 0, 0, 0, 0};
    }
}
