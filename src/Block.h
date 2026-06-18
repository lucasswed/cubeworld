#pragma once
#include <cstdint>

enum class BlockType : uint8_t {
    Air    = 0,
    Grass  = 1,
    Dirt   = 2,
    Stone  = 3,
    Wood   = 4,
    Leaves = 5,
    COUNT
};

inline bool blockIsSolid(BlockType t) {
    return t != BlockType::Air;
}

// Returns true only for fully opaque blocks that completely hide a neighbor's face.
// Transparent/cutout blocks (Leaves, and future Glass/Water) return false so that
// adjacent solid faces are included in the mesh and visible through the gaps.
inline bool blockOccludesFace(BlockType t) {
    switch (t) {
        case BlockType::Grass:
        case BlockType::Dirt:
        case BlockType::Stone:
        case BlockType::Wood:
            return true;
        default:  // Air, Leaves, and any future transparent block
            return false;
    }
}

// Texture atlas — 11 tiles wide. Atlas columns:
//  0=grass_top 1=grass_side 2=dirt 3=stone 4=wood_top 5=wood_side 6=leaves
//  7=pickaxe   8=shovel     9=axe  10=sword
// Face order: +X, -X, +Y(top), -Y(bottom), +Z, -Z
struct BlockTextures {
    int posX, negX, posY, negY, posZ, negZ;
};

inline BlockTextures blockTextures(BlockType t) {
    switch (t) {
        case BlockType::Grass:  return {1, 1, 0, 2, 1, 1};
        case BlockType::Dirt:   return {2, 2, 2, 2, 2, 2};
        case BlockType::Stone:  return {3, 3, 3, 3, 3, 3};
        case BlockType::Wood:   return {5, 5, 4, 4, 5, 5}; // bark sides, ring top/bot
        case BlockType::Leaves: return {6, 6, 6, 6, 6, 6};
        default:                return {0, 0, 0, 0, 0, 0};
    }
}
