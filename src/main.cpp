#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <array>
#include <cstring>
#include <string>
#include <vector>
#include <stb_image.h>

#include "Camera.h"
#include "Shader.h"
#include "World.h"

// ── Dropped item entity ───────────────────────────────────────────────────────
struct DroppedItem {
    glm::vec3 pos;
    BlockType type;
    float     spin; // radians, increases over time
    float     bob;  // phase in radians
};

// ── Held-item type (blocks + tools) ──────────────────────────────────────────
enum class ItemType : int {
    None = -1,
    Grass = 0, Dirt, Stone, Wood, Leaves,  // placeable blocks
    Pickaxe, Shovel, Axe, Sword,           // tools
    COUNT
};
static constexpr int kNumItemTypes = (int)ItemType::COUNT; // 9
static constexpr int kHotbarN      = 8;   // hotbar slot count
static constexpr int kInvTotal     = 64;  // total inventory slots

static BlockType itemToBlock(ItemType it) {
    switch (it) {
        case ItemType::Grass:  return BlockType::Grass;
        case ItemType::Dirt:   return BlockType::Dirt;
        case ItemType::Stone:  return BlockType::Stone;
        case ItemType::Wood:   return BlockType::Wood;
        case ItemType::Leaves: return BlockType::Leaves;
        default:               return BlockType::Air;
    }
}

// Seconds to break each block with bare hand
static float blockResistance(BlockType bt) {
    switch (bt) {
        case BlockType::Grass:  return 1.5f;
        case BlockType::Dirt:   return 2.0f;
        case BlockType::Stone:  return 4.0f;
        case BlockType::Wood:   return 3.0f;
        case BlockType::Leaves: return 0.6f;
        default:                return 0.f;
    }
}

// Speed multiplier: right tool = 4× faster (like Minecraft efficiency)
static float toolSpeed(ItemType item, BlockType block) {
    if (item == ItemType::Shovel  && (block == BlockType::Grass || block == BlockType::Dirt))
        return 4.f;
    if (item == ItemType::Pickaxe && block == BlockType::Stone)
        return 4.f;
    if (item == ItemType::Axe    && block == BlockType::Wood)
        return 4.f;
    if (item == ItemType::Sword  && block == BlockType::Leaves)
        return 4.f;
    return 1.f;
}

// ── Inventory slot ────────────────────────────────────────────────────────────
struct InvSlot { ItemType item = ItemType::None; int count = 0; };

static ItemType blockToItem(BlockType bt) {
    switch (bt) {
        case BlockType::Grass:  return ItemType::Grass;
        case BlockType::Dirt:   return ItemType::Dirt;
        case BlockType::Stone:  return ItemType::Stone;
        case BlockType::Wood:   return ItemType::Wood;
        case BlockType::Leaves: return ItemType::Leaves;
        default:                return ItemType::None;
    }
}

// ── Settings ──────────────────────────────────────────────────────────────────
static constexpr int   WIN_W       = 1280;
static constexpr int   WIN_H       = 720;
static constexpr int   RENDER_DIST = 5;       // chunks in each direction
static constexpr float GRAVITY     = 20.f;
static constexpr float JUMP_VEL    = 8.f;

// ── Globals (kept minimal — only what callbacks need) ─────────────────────────
static Camera   g_cam;
static float    g_lastMouseX = WIN_W / 2.f;
static float    g_lastMouseY = WIN_H / 2.f;
static bool     g_firstMouse = true;
static bool     g_wireframe  = false;
static float    g_velY       = 0.f;
static bool     g_onGround   = false;
static int      g_health     = 10;   // out of 10 hearts
static float    g_regenTimer = 0.f;
static bool      g_freeCam   = false;
static glm::vec3 g_playerPos = {8.f, 68.f, 8.f};
static float     g_playerYaw = -90.f;

// Arm animation
static float g_armSwing    = 0.f;   // 0→1 swing progress
static bool  g_armSwinging = false;
static float g_armBobPhase = 0.f;   // walk-bob phase (radians)

// Pending block actions set by mouse callbacks
static bool g_breakPending = false;
static bool g_placePending = false;

// Inventory: slots 0-7 = hotbar, 8-63 = main bag
static InvSlot g_inv[kInvTotal] = {
    {ItemType::Pickaxe, 1},
    {ItemType::Shovel,  1},
    {ItemType::Axe,     1},
    {ItemType::Sword,   1},
};
static int  g_hotbarSel = 0;  // 0-7
static bool g_invOpen   = false;

static std::vector<DroppedItem> g_drops;

// Add one item to the first available inventory slot; returns false if full.
static bool addToInventory(ItemType it) {
    for (int i = 0; i < kInvTotal; ++i)
        if (g_inv[i].item == it && g_inv[i].count < 64)
            { g_inv[i].count++; return true; }
    for (int i = 0; i < kInvTotal; ++i)
        if (g_inv[i].item == ItemType::None)
            { g_inv[i] = {it, 1}; return true; }
    return false;
}

// Cursor drag state (item held on the mouse cursor inside the inventory UI)
static InvSlot g_cursorSlot;

// Framebuffer size — updated every frame, used by inventory hit-test
static int g_fbW = WIN_W, g_fbH = WIN_H;
// Mouse cursor position in window coords (x left, y top) — updated by onMouseMove
static double g_mouseX = 0.0, g_mouseY = 0.0;

// Return the inventory slot index under screen position (mx, my from top-left),
// or -1 if not over any slot.  Must mirror the layout math in the HUD block.
static int inventorySlotAt(double mx, double my) {
    const float SZ = 44.f, GP = 4.f, ISEP = 8.f;
    const int   COLS = kHotbarN, ROWS = kInvTotal / kHotbarN - 1; // 7
    float invW  = COLS*(SZ+GP) - GP;
    float mainH = ROWS*(SZ+GP) - GP;
    float invH  = mainH + ISEP + SZ;
    float invX  = ((float)g_fbW - invW) * 0.5f;
    float invY  = ((float)g_fbH - invH) * 0.5f; // y from screen bottom

    float hx = (float)mx;
    float hy = (float)g_fbH - (float)my; // convert GLFW y-from-top to y-from-bottom

    for (int i = 0; i < kInvTotal; ++i) {
        float sx = (i < COLS)
            ? invX + i*(SZ+GP)
            : invX + ((i-COLS)%COLS)*(SZ+GP);
        float sy = (i < COLS)
            ? invY
            : invY + SZ + ISEP + (ROWS-1-(i-COLS)/COLS)*(SZ+GP);
        if (hx >= sx && hx < sx+SZ && hy >= sy && hy < sy+SZ)
            return i;
    }
    return -1;
}

// Close the inventory: release cursor, restore camera input, return held item.
static void closeInventory(GLFWwindow *win) {
    g_invOpen = false;
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    g_firstMouse = true;
    // Return any item on cursor back to inventory
    for (int n = g_cursorSlot.count; n > 0 && addToInventory(g_cursorSlot.item); --n) {}
    g_cursorSlot = {};
}

// Hold-to-repeat state
static bool  g_lmbHeld  = false;
static bool  g_rmbHeld  = false;
static float g_rmbTimer = 0.f;
static constexpr float kClickInterval = 0.25f; // seconds between place repeats

// Mining progress
static bool       g_mineActive    = false;
static glm::ivec3 g_mineTarget    = {};
static BlockType  g_mineBlockType = BlockType::Air;
static float      g_mineProgress  = 0.f; // 0–1

// ── Callbacks ─────────────────────────────────────────────────────────────────
static void onResize(GLFWwindow *, int w, int h) {
    glViewport(0, 0, w, h);
}

static void onMouseMove(GLFWwindow *, double xpos, double ypos) {
    g_mouseX = xpos;
    g_mouseY = ypos;
    if (g_invOpen) return; // camera doesn't move while inventory is open
    if (g_firstMouse) { g_lastMouseX = (float)xpos; g_lastMouseY = (float)ypos; g_firstMouse = false; }
    float dx =  (float)xpos - g_lastMouseX;
    float dy = -((float)ypos - g_lastMouseY);
    g_lastMouseX = (float)xpos;
    g_lastMouseY = (float)ypos;
    g_cam.processMouseMove(dx, dy);
}

static void onMouseButton(GLFWwindow *, int button, int action, int) {
    if (g_invOpen) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            int slot = inventorySlotAt(g_mouseX, g_mouseY);
            if (slot >= 0) {
                InvSlot &s = g_inv[slot];
                // Stack same stackable item type instead of plain swap
                if (g_cursorSlot.item == s.item &&
                    g_cursorSlot.item >= ItemType::Grass &&
                    g_cursorSlot.item <= ItemType::Stone) {
                    int give = std::min(g_cursorSlot.count, 64 - s.count);
                    s.count += give;
                    g_cursorSlot.count -= give;
                    if (g_cursorSlot.count == 0) g_cursorSlot = {};
                } else {
                    std::swap(g_cursorSlot, s);
                }
            }
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            int slot = inventorySlotAt(g_mouseX, g_mouseY);
            if (slot >= 0) {
                InvSlot &s = g_inv[slot];
                bool stackable = (s.item >= ItemType::Grass && s.item <= ItemType::Stone) ||
                                 (g_cursorSlot.item >= ItemType::Grass && g_cursorSlot.item <= ItemType::Stone);
                if (g_cursorSlot.item == ItemType::None && s.item != ItemType::None && stackable) {
                    // Pick up half
                    int half = (s.count + 1) / 2;
                    g_cursorSlot = {s.item, half};
                    s.count -= half;
                    if (s.count == 0) s = {};
                } else if (g_cursorSlot.item != ItemType::None && stackable &&
                           (s.item == ItemType::None || s.item == g_cursorSlot.item) &&
                           (s.item == ItemType::None || s.count < 64)) {
                    // Place one
                    if (s.item == ItemType::None) s.item = g_cursorSlot.item;
                    s.count++;
                    g_cursorSlot.count--;
                    if (g_cursorSlot.count == 0) g_cursorSlot = {};
                }
            }
        }
        return;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS)   g_lmbHeld = true;
        if (action == GLFW_RELEASE) { g_lmbHeld = false; }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS)   { g_placePending = true; g_rmbHeld = true;  g_rmbTimer = kClickInterval; }
        if (action == GLFW_RELEASE) { g_rmbHeld = false; }
    }
}

static void onScroll(GLFWwindow *, double, double yoffset) {
    if (yoffset < 0) g_hotbarSel = (g_hotbarSel + 1) % kHotbarN;
    else             g_hotbarSel = (g_hotbarSel + kHotbarN - 1) % kHotbarN;
}

static void onKey(GLFWwindow *win, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) {
        if (g_invOpen) closeInventory(win);
        else           glfwSetWindowShouldClose(win, GLFW_TRUE);
        return;
    }
    if (key == GLFW_KEY_TAB) {
        if (!g_invOpen) {
            g_invOpen = true;
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwGetCursorPos(win, &g_mouseX, &g_mouseY);
        } else {
            closeInventory(win);
        }
        return;
    }
    if (g_invOpen) return; // block hotbar keys while inventory is open
    if (key == GLFW_KEY_F)      { g_wireframe = !g_wireframe; glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL); }
    if (key == GLFW_KEY_1) g_hotbarSel = 0;
    if (key == GLFW_KEY_2) g_hotbarSel = 1;
    if (key == GLFW_KEY_3) g_hotbarSel = 2;
    if (key == GLFW_KEY_4) g_hotbarSel = 3;
    if (key == GLFW_KEY_5) g_hotbarSel = 4;
    if (key == GLFW_KEY_6) g_hotbarSel = 5;
    if (key == GLFW_KEY_7) g_hotbarSel = 6;
    if (key == GLFW_KEY_8) g_hotbarSel = 7;
}

// ── Player AABB collision ─────────────────────────────────────────────────────
// pos = camera eye position (feet = pos.y - 1.7, head = pos.y + 0.1)
static bool overlapsBlock(const World &world, glm::vec3 pos) {
    constexpr float HW = 0.29f;
    float fy = pos.y - 1.7f;

    int x0 = (int)std::floor(pos.x - HW), x1 = (int)std::floor(pos.x + HW);
    // +0.01 so we skip the block the player stands ON (float error makes feetY
    // land just below an integer, which would wrongly include the floor block)
    int y0 = (int)std::floor(fy + 0.01f),  y1 = (int)std::floor(fy + 1.799f);
    int z0 = (int)std::floor(pos.z - HW), z1 = (int)std::floor(pos.z + HW);

    for (int bx = x0; bx <= x1; bx++)
    for (int by = y0; by <= y1; by++)
    for (int bz = z0; bz <= z1; bz++)
        if (blockIsSolid(world.getBlock(bx, by, bz))) return true;
    return false;
}

// ── Arm/item box geometry ─────────────────────────────────────────────────────
// Writes a 6-face box (pos+normal per vertex) into buf starting at offset.
// Each box = 6 faces × 6 verts × 6 floats = 216 floats. Returns new offset.
static int buildArmBox(float *buf, int off,
                       float x0,float y0,float z0,
                       float x1,float y1,float z1)
{
    auto v = [&](float px,float py,float pz,float nx,float ny,float nz){
        buf[off++]=px;buf[off++]=py;buf[off++]=pz;
        buf[off++]=nx;buf[off++]=ny;buf[off++]=nz;
    };
    v(x1,y0,z0,1,0,0);v(x1,y1,z0,1,0,0);v(x1,y1,z1,1,0,0); // +X CCW
    v(x1,y0,z0,1,0,0);v(x1,y1,z1,1,0,0);v(x1,y0,z1,1,0,0);
    v(x0,y0,z1,-1,0,0);v(x0,y1,z1,-1,0,0);v(x0,y1,z0,-1,0,0); // -X CCW
    v(x0,y0,z1,-1,0,0);v(x0,y1,z0,-1,0,0);v(x0,y0,z0,-1,0,0);
    v(x0,y1,z0,0,1,0);v(x0,y1,z1,0,1,0);v(x1,y1,z1,0,1,0); // +Y CCW
    v(x0,y1,z0,0,1,0);v(x1,y1,z1,0,1,0);v(x1,y1,z0,0,1,0);
    v(x0,y0,z1,0,-1,0);v(x0,y0,z0,0,-1,0);v(x1,y0,z0,0,-1,0); // -Y CCW
    v(x0,y0,z1,0,-1,0);v(x1,y0,z0,0,-1,0);v(x1,y0,z1,0,-1,0);
    v(x1,y0,z1,0,0,1);v(x1,y1,z1,0,0,1);v(x0,y1,z1,0,0,1); // +Z CCW
    v(x1,y0,z1,0,0,1);v(x0,y1,z1,0,0,1);v(x0,y0,z1,0,0,1);
    v(x0,y0,z0,0,0,-1);v(x0,y1,z0,0,0,-1);v(x1,y1,z0,0,0,-1); // -Z CCW
    v(x0,y0,z0,0,0,-1);v(x1,y1,z0,0,0,-1);v(x1,y0,z0,0,0,-1);
    return off;
}

// ── Texture atlas ─────────────────────────────────────────────────────────────
// ── Texture atlas ─────────────────────────────────────────────────────────────
// Drop one image per block/tool into assets/textures/:
//   grass.png   — 1 col = all faces same; 2 cols = [top|side]; 3 cols = [top|side|bottom]
//   dirt.png, stone.png — single tile, all faces the same
//   pickaxe.png, shovel.png, axe.png, sword.png — single icon tile
// Image height = tile size (any resolution). Missing files fall back to procedural art.
static GLuint buildTextureAtlas(FILE *logFile = nullptr) {
    auto LOG = [logFile](const char *m) {
        if (logFile) { fputs(m, logFile); fputc('\n', logFile); fflush(logFile); }
    };

    constexpr int NCOLS = 12;

    // Each entry maps an image file to atlas slots, one slot per face column.
    // Image width / image height = number of face columns in that image.
    // If image has fewer columns than numSlots, the last column repeats.
    // Atlas layout: 0=grass_top 1=grass_side 2=dirt 3=stone
    //               4=wood_top  5=wood_side  6=leaves
    //               7=pickaxe   8=shovel     9=axe   10=sword  11=water
    struct BlockEntry { const char *file; int slots[3]; int numSlots; };
    static const BlockEntry kBlocks[] = {
        {"grass",   {0, 1, 2}, 3}, // col0=top(0), col1=side(1), col2=bottom(2)
        {"dirt",    {2},        1},
        {"stone",   {3},        1},
        {"wood",    {4, 5},     2}, // col0=top rings(4), col1=side bark(5)
        {"leaves",  {6},        1},
        {"pickaxe", {7},        1},
        {"shovel",  {8},        1},
        {"axe",     {9},        1},
        {"sword",   {10},       1},
        {"water",   {11},       1},
    };
    static constexpr int kNumBlocks = 10;
    static const char *kExts[] = {".png", ".jpg", ".jpeg", ".bmp", nullptr};

    stbi_set_flip_vertically_on_load(1);

    struct Img { uint8_t *data = nullptr; int w = 0, h = 0; };
    Img imgs[kNumBlocks];
    bool slotFilled[NCOLS] = {};
    int TILE = 16;
    bool tileSet = false;

    for (int i = 0; i < kNumBlocks; i++) {
        for (int e = 0; kExts[e] && !imgs[i].data; e++) {
            std::string path = std::string("assets/textures/") + kBlocks[i].file + kExts[e];
            int w, h, dummy;
            uint8_t *d = stbi_load(path.c_str(), &w, &h, &dummy, 4);
            if (d) {
                imgs[i] = {d, w, h};
                if (!tileSet) { TILE = std::max(16, std::min(256, h)); tileSet = true; }
                char msg[128];
                int cols = std::max(1, w / h);
                snprintf(msg, sizeof(msg), "Loaded %s (%dx%d, %d face col%s)",
                         kBlocks[i].file, w, h, cols, cols == 1 ? "" : "s");
                LOG(msg);
            }
        }
        if (!imgs[i].data) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Procedural: %s", kBlocks[i].file);
            LOG(msg);
        }
    }

    int W = TILE * NCOLS;
    std::vector<uint8_t> atlas(W * TILE * 4, 0);

    // Extract one tile column from img (nearest-neighbour scaled to TILE×TILE).
    auto putTileFrom = [&](const Img &img, int srcCol, int atlasSlot) {
        int th = img.h, tw = th;
        int srcX = srcCol * tw, dstX = atlasSlot * TILE;
        for (int dy = 0; dy < TILE; dy++)
            for (int dx = 0; dx < TILE; dx++) {
                int sy = dy * th / TILE, sx = srcX + dx * tw / TILE;
                const uint8_t *s = img.data + (sy * img.w + sx) * 4;
                uint8_t *d = atlas.data() + (dy * W + dstX + dx) * 4;
                d[0]=s[0]; d[1]=s[1]; d[2]=s[2]; d[3]=s[3];
            }
    };

    for (int i = 0; i < kNumBlocks; i++) {
        if (!imgs[i].data) continue;
        const BlockEntry &be = kBlocks[i];
        int numCols = std::max(1, imgs[i].w / imgs[i].h);
        for (int s = 0; s < be.numSlots; s++) {
            int col = std::min(s, numCols - 1);
            putTileFrom(imgs[i], col, be.slots[s]);
            slotFilled[be.slots[s]] = true;
        }
        stbi_image_free(imgs[i].data);
    }

    // ── Procedural fallback for unfilled slots ─────────────────────────────────
    {
        constexpr int B = 16;
        std::vector<uint8_t> buf(B * B * 4, 0);
        using P4 = std::array<uint8_t, 4>;

        auto c8 = [](int v) -> uint8_t { return v<0?0:(v>255?255:(uint8_t)v); };
        auto Nh = [](int x, int y, int s) -> int {
            unsigned h = (unsigned)(x*1313 ^ y*7177 ^ s*9781);
            h ^= h>>13; h *= 0x85ebca6bu; h ^= h>>16;
            return (int)(h & 0x1F) - 16;
        };
        auto spx = [&](int x, int y, P4 c) {
            if ((unsigned)x<B && (unsigned)y<B) {
                auto *p = buf.data()+(y*B+x)*4;
                p[0]=c[0]; p[1]=c[1]; p[2]=c[2]; p[3]=c[3];
            }
        };
        auto ln = [&](int x0,int y0,int x1,int y1, P4 c) {
            int dx=abs(x1-x0), dy=abs(y1-y0), sx=x0<x1?1:-1, sy=y0<y1?1:-1, err=dx-dy;
            for(;;) { spx(x0,y0,c); if(x0==x1&&y0==y1) break;
                int e2=2*err; if(e2>-dy){err-=dy;x0+=sx;} if(e2<dx){err+=dx;y0+=sy;} }
        };
        auto rc = [&](int x0,int y0,int x1,int y1, P4 c) {
            for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++) spx(x,y,c);
        };
        auto commit = [&](int slot) {
            if (slotFilled[slot]) return;
            for (int dy=0; dy<TILE; dy++)
                for (int dx=0; dx<TILE; dx++) {
                    int sy=dy*B/TILE, sx=dx*B/TILE;
                    const uint8_t *s = buf.data()+(sy*B+sx)*4;
                    uint8_t *d = atlas.data()+(dy*W + slot*TILE+dx)*4;
                    d[0]=s[0];d[1]=s[1];d[2]=s[2];d[3]=s[3];
                }
            slotFilled[slot] = true;
        };
        auto gen = [&](int slot, auto fn) {
            if (slotFilled[slot]) return;
            std::fill(buf.begin(), buf.end(), 0); fn(); commit(slot);
        };

        gen(0, [&]{ for(int y=0;y<B;y++) for(int x=0;x<B;x++) {
            int n=Nh(x,y,0); spx(x,y,{c8(88+n),c8(152+n*2),c8(50+n),255}); } });
        gen(1, [&]{ for(int y=0;y<B;y++) for(int x=0;x<B;x++) {
            int n=Nh(x,y,1);
            if(y>=12) spx(x,y,{c8(88+n),c8(152+n*2),c8(50+n),255});
            else { int nd=Nh(x,y,11); spx(x,y,{c8(134+nd),c8(96+nd),c8(57+(nd>>1)),255}); }
        } });
        gen(2, [&]{ for(int y=0;y<B;y++) for(int x=0;x<B;x++) {
            int n=Nh(x,y,2); spx(x,y,{c8(134+n),c8(96+n),c8(57+(n>>1)),255}); } });
        gen(3, [&]{
            for(int y=0;y<B;y++) for(int x=0;x<B;x++) {
                int n=Nh(x,y,3); spx(x,y,{c8(128+n),c8(128+n),c8(128+n),255}); }
            P4 dk={92,92,92,255};
            for(int x=1;x<=11;x++) spx(x,7,dk);
            spx(4,8,dk);spx(5,6,dk);spx(8,8,dk);spx(10,6,dk);
            for(int y=3;y<=13;y++) spx(12,y,dk);
            spx(11,8,dk);spx(13,5,dk);
            P4 ch={115,115,115,255}; spx(1,14,ch); spx(14,1,ch);
        });
        // wood_top (4): concentric rings pattern
        gen(4, [&]{
            for(int y=0;y<B;y++) for(int x=0;x<B;x++) {
                int n=Nh(x,y,40);
                float cx=x-7.5f, cy=y-7.5f;
                float r=std::sqrt(cx*cx+cy*cy);
                int ring=(int)r & 1;
                int base = ring ? 130+n/2 : 108+n/2;
                spx(x,y,{c8(base),c8(base*75/100),c8(base*40/100),255});
            }
        });
        // wood_side (5): vertical bark stripes
        gen(5, [&]{
            for(int y=0;y<B;y++) for(int x=0;x<B;x++) {
                int n=Nh(x,y,50); int ns=Nh(x,y*3,51);
                int stripe=(x+ns/8)%4;
                int base=(stripe<2)?118:98; base+=n/2;
                spx(x,y,{c8(base),c8(base*74/100),c8(base*39/100),255});
            }
        });
        // leaves (6): irregular cutout green
        gen(6, [&]{
            for(int y=0;y<B;y++) for(int x=0;x<B;x++) {
                int n=Nh(x,y,60), ns=Nh(x,y,61);
                if(ns < -8){spx(x,y,{0,0,0,0});continue;}
                spx(x,y,{c8(58+n),c8(108+n*2),c8(32+n),255});
            }
        });
        const P4 kW={139,90,43,255},kWd={101,62,22,255};
        const P4 kI={176,176,176,255},kId={120,120,120,255};
        const P4 kS={216,216,230,255},kSd={168,168,188,255};
        gen(7, [&]{   // pickaxe
            rc(0,11,8,14,kI); rc(0,9,1,11,kI); rc(7,9,8,11,kI);
            ln(0,9,8,9,kId); ln(0,14,8,14,kId);
            ln(4,10,14,0,kW); ln(5,10,15,0,kWd);
        });
        gen(8, [&]{   // shovel
            rc(5,11,10,15,kI); ln(5,11,10,11,kId); ln(5,11,5,15,kId); spx(10,15,kId);
            spx(7,10,kId); spx(8,10,kI);
            rc(7,1,7,9,kW); rc(8,1,8,9,kWd);
            rc(6,0,9,1,kId); spx(7,0,kW); spx(8,0,kW);
        });
        gen(9, [&]{   // axe
            rc(0,10,7,15,kI); rc(1,9,5,14,kI);
            ln(0,9,7,9,kId); ln(0,9,0,15,kId); spx(7,15,kId);
            ln(6,9,14,1,kW); ln(7,9,15,1,kWd);
        });
        gen(10, [&]{  // sword
            rc(7,8,8,15,kS); rc(6,9,7,15,kS); ln(8,8,8,15,kSd);
            spx(6,15,kSd); spx(7,15,P4{240,240,255,255});
            rc(4,7,10,8,kI); ln(4,7,10,7,kId);
            rc(7,2,7,6,kW); rc(8,2,8,6,kWd);
            rc(6,0,9,1,kId); spx(7,0,kI); spx(8,0,kI);
        });
        gen(11, [&]{  // water: animated blue, semi-transparent
            for(int y=0;y<B;y++) for(int x=0;x<B;x++) {
                int n=Nh(x,y,80), nr=Nh(x,y,81);
                // Subtle ripple pattern
                int ripple = (((x*3+y*5)%7) < 2) ? 12 : 0;
                spx(x,y,{c8(18+n/2),c8(80+n+ripple),c8(210+n/3),c8(185+nr/4)});
            }
        });
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, TILE, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main() {
    // Simple log file — try CWD first, then C:\ as fallback
    FILE *g_log = fopen("cubeworld.log", "w");
    if (!g_log) g_log = fopen("C:\\cubeworld.log", "w");
    auto LOG = [&](const char *msg) {
        if (g_log) { fputs(msg, g_log); fputc('\n', g_log); fflush(g_log); }
    };
    LOG("=== CubeWorld starting ===");

    if (!glfwInit()) { LOG("FATAL: glfwInit failed"); return 1; }
    LOG("glfwInit OK");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow *win = glfwCreateWindow(WIN_W, WIN_H, "CubeWorld", nullptr, nullptr);
    if (!win) { LOG("FATAL: glfwCreateWindow failed"); glfwTerminate(); return 1; }
    LOG("Window created");

    glfwMakeContextCurrent(win);
    // No vsync — glfwSwapInterval(1) deadlocks with the WSL/Windows GL driver.
    // Frame rate is capped manually with glfwWaitEventsTimeout at the loop end.
    LOG("Context current");

    // Lock cursor
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Callbacks
    glfwSetFramebufferSizeCallback(win, onResize);
    glfwSetCursorPosCallback(win,       onMouseMove);
    glfwSetMouseButtonCallback(win,     onMouseButton);
    glfwSetScrollCallback(win,          onScroll);
    glfwSetKeyCallback(win,             onKey);
    LOG("Callbacks set");

    // Load GL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG("FATAL: gladLoadGLLoader failed");
        glfwDestroyWindow(win); glfwTerminate(); return 1;
    }
    LOG("GLAD loaded");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // World shader
    LOG("Loading chunk shaders...");
    Shader shader;
    if (!shader.load("assets/shaders/chunk.vert", "assets/shaders/chunk.frag")) {
        LOG("FATAL: chunk shaders failed");
        glfwDestroyWindow(win); glfwTerminate(); return 1;
    }
    LOG("Chunk shaders OK");

    // UI shader (crosshair)
    LOG("Loading UI shaders...");
    Shader uiShader;
    if (!uiShader.load("assets/shaders/ui.vert", "assets/shaders/ui.frag")) {
        LOG("FATAL: UI shaders failed");
        glfwDestroyWindow(win); glfwTerminate(); return 1;
    }
    LOG("UI shaders OK");

    // Arm shader (first-person arm + held item + drop cubes)
    LOG("Loading arm shaders...");
    Shader armShader;
    if (!armShader.load("assets/shaders/arm.vert", "assets/shaders/arm.frag")) {
        LOG("FATAL: arm shaders failed");
        glfwDestroyWindow(win); glfwTerminate(); return 1;
    }
    LOG("Arm shaders OK");
    // Set stable defaults so callers only need to change what differs
    armShader.use();
    armShader.setInt("uAtlas", 0);       // always texture unit 0
    armShader.setInt("uUseTexture", 0);  // default = flat colour
    armShader.setInt("uTileTop",  0);
    armShader.setInt("uTileSide", 0);
    armShader.setInt("uTileBot",  0);

    // Icon shader (textured 2-D quads for inventory icons)
    LOG("Loading icon shaders...");
    Shader iconShader;
    if (!iconShader.load("assets/shaders/icon.vert", "assets/shaders/icon.frag")) {
        LOG("FATAL: icon shaders failed");
        glfwDestroyWindow(win); glfwTerminate(); return 1;
    }
    iconShader.use();
    iconShader.setInt("uAtlas", 0);
    LOG("Icon shaders OK");

    // Crosshair VAO/VBO — 12 vertices (2 quads), updated each frame
    GLuint crossVAO, crossVBO;
    glGenVertexArrays(1, &crossVAO);
    glGenBuffers(1, &crossVBO);
    glBindVertexArray(crossVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crossVBO);
    glBufferData(GL_ARRAY_BUFFER, 12 * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    LOG("Crosshair VAO OK");

    // Block highlight — cube wireframe, slightly expanded to avoid z-fighting
    LOG("Loading highlight shaders...");
    Shader hlShader;
    if (!hlShader.load("assets/shaders/highlight.vert", "assets/shaders/highlight.frag")) {
        LOG("FATAL: highlight shaders failed");
        glfwDestroyWindow(win); glfwTerminate(); return 1;
    }
    LOG("Highlight shaders OK");

    GLuint hlVAO, hlVBO;
    glGenVertexArrays(1, &hlVAO);
    glGenBuffers(1, &hlVBO);
    {
        const float E = 0.005f, mn = -E, mx = 1.0f + E;
        float v[] = {
            // Bottom face
            mn,mn,mn,  mx,mn,mn,   mx,mn,mn,  mx,mn,mx,
            mx,mn,mx,  mn,mn,mx,   mn,mn,mx,  mn,mn,mn,
            // Top face
            mn,mx,mn,  mx,mx,mn,   mx,mx,mn,  mx,mx,mx,
            mx,mx,mx,  mn,mx,mx,   mn,mx,mx,  mn,mx,mn,
            // Verticals
            mn,mn,mn,  mn,mx,mn,   mx,mn,mn,  mx,mx,mn,
            mx,mn,mx,  mx,mx,mx,   mn,mn,mx,  mn,mx,mx,
        };
        glBindVertexArray(hlVAO);
        glBindBuffer(GL_ARRAY_BUFFER, hlVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    LOG("Highlight VAO OK");

    // FPS counter resources (7-segment digits, up to 4 digits × 7 segs × 6 verts × 2 floats)
    GLuint fpsVAO, fpsVBO;
    glGenVertexArrays(1, &fpsVAO);
    glGenBuffers(1, &fpsVBO);
    glBindVertexArray(fpsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fpsVBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * 7 * 6 * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // HUD VAO/VBO (health hearts + hotbar)
    GLuint hudVAO, hudVBO;
    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Arm VAO/VBO: 2 boxes (arm + held block), each 216 floats (pos+norm)
    GLuint armVAO, armVBO;
    glGenVertexArrays(1, &armVAO);
    glGenBuffers(1, &armVBO);
    glBindVertexArray(armVAO);
    glBindBuffer(GL_ARRAY_BUFFER, armVBO);
    {
        float armBuf[432]; int ao = 0;
        ao = buildArmBox(armBuf, ao, -0.08f,-0.50f,-0.08f,  0.08f, 0.0f,  0.08f); // forearm
        ao = buildArmBox(armBuf, ao, -0.14f, 0.0f, -0.14f,  0.14f, 0.28f, 0.14f); // held block
        glBufferData(GL_ARRAY_BUFFER, sizeof(armBuf), armBuf, GL_STATIC_DRAW);
    }
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Icon VAO/VBO: one textured quad (6 verts × 4 floats = pos2 + uv2), reused each draw
    GLuint iconVAO, iconVBO;
    glGenVertexArrays(1, &iconVAO);
    glGenBuffers(1, &iconVBO);
    glBindVertexArray(iconVAO);
    glBindBuffer(GL_ARRAY_BUFFER, iconVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    LOG("Icon VAO OK");

    // Character model VAO/VBO: 6 parts (head, torso, 2 arms, 2 legs), each 36 verts
    GLuint charVAO, charVBO;
    glGenVertexArrays(1, &charVAO);
    glGenBuffers(1, &charVBO);
    glBindVertexArray(charVAO);
    glBindBuffer(GL_ARRAY_BUFFER, charVBO);
    {
        float charBuf[6 * 216]; int co = 0;
        // All coords relative to feet (origin = bottom of character).
        // Character faces +Z in local space; rotation applied at draw time.
        co = buildArmBox(charBuf, co, -0.25f, 1.50f, -0.25f,  0.25f, 2.00f,  0.25f);  // head
        co = buildArmBox(charBuf, co, -0.25f, 0.75f, -0.125f, 0.25f, 1.50f,  0.125f); // torso
        co = buildArmBox(charBuf, co, -0.45f, 0.75f, -0.10f, -0.25f, 1.45f,  0.10f);  // left arm
        co = buildArmBox(charBuf, co,  0.25f, 0.75f, -0.10f,  0.45f, 1.45f,  0.10f);  // right arm
        co = buildArmBox(charBuf, co, -0.25f, 0.00f, -0.10f, -0.05f, 0.75f,  0.10f);  // left leg
        co = buildArmBox(charBuf, co,  0.05f, 0.00f, -0.10f,  0.25f, 0.75f,  0.10f);  // right leg
        glBufferData(GL_ARRAY_BUFFER, sizeof(charBuf), charBuf, GL_STATIC_DRAW);
    }
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Texture atlas
    LOG("Building texture atlas...");
    GLuint atlas = buildTextureAtlas(g_log);
    LOG("Atlas OK");

    // World — spawn camera above the terrain surface
    World world;
    {
        int surf = world.surfaceAt(8, 8);
        g_cam.position = {8.f, (float)surf + 2.7f, 8.f};
    }

    // Drain any pending window messages before entering the render loop.
    glfwPollEvents();
    // Explicit vsync-off so the driver default (which may be 1) doesn't block.
    glfwSwapInterval(0);

    LOG("Entering game loop");
    float prevTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(win)) {
        float now = (float)glfwGetTime();
        float dt  = now - prevTime;
        if (dt > 0.1f) dt = 0.1f;
        prevTime = now;

        glfwPollEvents();

        // V key: toggle free-cam (edge detection so one press = one toggle)
        {
            static bool prevV = false;
            bool curV = glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS;
            if (curV && !prevV) g_freeCam = !g_freeCam;
            prevV = curV;
        }

        // Track player body position/yaw (frozen while in free-cam)
        if (!g_freeCam) { g_playerPos = g_cam.position; g_playerYaw = g_cam.yaw; }

        if (g_freeCam) {
            // Free-fly: WASD along full 3-D camera direction, Space=up, Ctrl=down
            const float FREE_SPEED = 10.f;
            glm::vec3 mv{0.f};
            if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) mv += g_cam.front();
            if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) mv -= g_cam.front();
            if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) mv -= g_cam.right();
            if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) mv += g_cam.right();
            if (glfwGetKey(win, GLFW_KEY_SPACE)      == GLFW_PRESS) mv.y += 1.f;
            if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) mv.y -= 1.f;
            if (glm::length(mv) > 0.001f)
                g_cam.position += glm::normalize(mv) * FREE_SPEED * dt;
        } else {

        // ── Sprint / sneak ────────────────────────────────────────────────
        bool sprinting = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT)   == GLFW_PRESS;
        bool sneaking  = glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        float moveSpeed = g_cam.speed
                        * (sprinting ? 1.6f : sneaking ? 0.3f : 1.0f);

        // Returns true if at least one block column under pos is solid at foot level.
        // The -0.1f bias ensures we land on the block below feet rather than the
        // air cell at feet height (pos.y - 1.7f can be exactly groundY + epsilon).
        auto hasSolidGround = [&](glm::vec3 pos) -> bool {
            int gy = (int)std::floor(pos.y - 1.7f - 0.1f);
            constexpr float GHW = 0.29f;
            int gx0 = (int)std::floor(pos.x - GHW), gx1 = (int)std::floor(pos.x + GHW);
            int gz0 = (int)std::floor(pos.z - GHW), gz1 = (int)std::floor(pos.z + GHW);
            for (int gx = gx0; gx <= gx1; gx++)
            for (int gz = gz0; gz <= gz1; gz++)
                if (blockIsSolid(world.getBlock(gx, gy, gz))) return true;
            return false;
        };

        // ── Horizontal movement with AABB collision ────────────────────────
        glm::vec3 move{0.f};
        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) move += g_cam.front() * glm::vec3{1,0,1};
        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) move -= g_cam.front() * glm::vec3{1,0,1};
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) move -= g_cam.right();
        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) move += g_cam.right();

        if (glm::length(move) > 0.001f) {
            glm::vec3 delta = glm::normalize(move) * moveSpeed * dt;

            // Try X — also reject if sneaking would walk off an edge
            {
                glm::vec3 tryPos = g_cam.position;
                tryPos.x += delta.x;
                if (!overlapsBlock(world, tryPos))
                    if (!sneaking || !g_onGround || hasSolidGround(tryPos))
                        g_cam.position.x = tryPos.x;
            }

            // Try Z
            {
                glm::vec3 tryPos = g_cam.position;
                tryPos.z += delta.z;
                if (!overlapsBlock(world, tryPos))
                    if (!sneaking || !g_onGround || hasSolidGround(tryPos))
                        g_cam.position.z = tryPos.z;
            }
        }

        // ── Water detection ───────────────────────────────────────────────
        bool inWater = (world.getBlock(
            (int)std::floor(g_cam.position.x),
            (int)std::floor(g_cam.position.y - 0.8f),
            (int)std::floor(g_cam.position.z)) == BlockType::Water);

        // ── Gravity / buoyancy ────────────────────────────────────────────
        if (inWater) {
            g_velY -= GRAVITY * 0.12f * dt;   // heavily reduced (buoyancy)
            g_velY *= std::max(0.f, 1.f - 6.f * dt); // strong drag
            if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS)
                g_velY = std::min(g_velY + 10.f * dt, 4.f); // swim up
        } else {
            g_velY -= GRAVITY * dt;
        }
        g_cam.position.y += g_velY * dt;

        // ── Vertical collision (ground + ceiling) ─────────────────────────
        {
            float feetY = g_cam.position.y - 1.7f;

            // Ground: scan ALL columns under the player's footprint and take
            // the highest solid block found — fixes falling near block edges
            g_onGround = false;
            constexpr float GHW = 0.29f;
            int gx0 = (int)std::floor(g_cam.position.x - GHW);
            int gx1 = (int)std::floor(g_cam.position.x + GHW);
            int gz0 = (int)std::floor(g_cam.position.z - GHW);
            int gz1 = (int)std::floor(g_cam.position.z + GHW);
            int searchTop = std::min((int)std::floor(feetY) + 1, CHUNK_H - 1);

            int groundY = 0;
            for (int gx = gx0; gx <= gx1; gx++)
            for (int gz = gz0; gz <= gz1; gz++) {
                for (int by = searchTop; by >= 0; by--) {
                    if (blockIsSolid(world.getBlock(gx, by, gz))) {
                        if (by + 1 > groundY) groundY = by + 1;
                        break;
                    }
                }
            }
            float groundF = (float)groundY + 1.7f;
            if (g_cam.position.y <= groundF && g_velY <= 0.f) {
                float landVel = g_velY;
                g_cam.position.y = groundF;
                g_velY = 0.f;
                g_onGround = true;
                if (!inWater && landVel < -20.f) {
                    int dmg = (int)((-landVel - 20.f) / 3.f) + 1;
                    g_health -= dmg;
                    if (g_health < 0) g_health = 0;
                }
            }

            // Ceiling: stop upward movement if head hits a block
            if (g_velY > 0.f) {
                int cx = (int)std::floor(g_cam.position.x);
                int cz = (int)std::floor(g_cam.position.z);
                float headY = g_cam.position.y + 0.1f;
                if (blockIsSolid(world.getBlock(cx, (int)std::floor(headY), cz))) {
                    g_velY = 0.f;
                }
            }
        }

        // ── Jump ──────────────────────────────────────────────────────────
        if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS && g_onGround)
            g_velY = JUMP_VEL;

        } // end !g_freeCam physics block

        // ── Health: respawn on death, slow regen while grounded ───────────────
        if (g_health <= 0) {
            g_health = 10;
            int rsurf = world.surfaceAt(8, 8);
            g_cam.position = {8.f, (float)rsurf + 2.7f, 8.f};
            g_velY = 0.f;
        } else if (g_onGround && g_health < 10) {
            g_regenTimer += dt;
            if (g_regenTimer >= 4.f) { g_health++; g_regenTimer = 0.f; }
        } else {
            g_regenTimer = 0.f;
        }

        // ── Block targeting (once per frame, used for interaction + highlight) ──
        RaycastResult target = world.raycast(g_cam.position, g_cam.front());

        // ── Mining progress (LMB hold) ────────────────────────────────────────
        if (g_lmbHeld && target.hit) {
            if (!g_mineActive || target.blockPos != g_mineTarget) {
                g_mineActive    = true;
                g_mineTarget    = target.blockPos;
                g_mineBlockType = world.getBlock(target.blockPos.x, target.blockPos.y, target.blockPos.z);
                g_mineProgress  = 0.f;
            }
            float res = blockResistance(g_mineBlockType);
            float spd = toolSpeed(g_inv[g_hotbarSel].item, g_mineBlockType);
            if (res <= 0.f) {
                world.setBlock(g_mineTarget.x, g_mineTarget.y, g_mineTarget.z, BlockType::Air);
                g_mineActive = false; g_mineProgress = 0.f;
            } else {
                g_mineProgress += dt * spd / res;
                if (g_mineProgress >= 1.f) {
                    BlockType mined = g_mineBlockType;
                    world.setBlock(g_mineTarget.x, g_mineTarget.y, g_mineTarget.z, BlockType::Air);
                    g_drops.push_back({
                        glm::vec3(g_mineTarget) + glm::vec3(0.5f),
                        mined, 0.f, 0.f
                    });
                    g_armSwinging = true; g_armSwing = 0.f;
                    g_mineActive = false; g_mineProgress = 0.f;
                }
            }
        } else {
            g_mineActive = false; g_mineProgress = 0.f;
        }

        // ── Arm swing loops while actively mining ────────────────────────────
        if (g_mineActive && !g_armSwinging) { g_armSwinging = true; g_armSwing = 0.f; }

        // ── Place blocks (RMB hold-to-repeat) ────────────────────────────────
        if (g_rmbHeld) { g_rmbTimer -= dt; if (g_rmbTimer <= 0.f) { g_placePending = true; g_rmbTimer += kClickInterval; } }
        if (g_placePending) {
            g_armSwinging = true; g_armSwing = 0.f;
            if (target.hit) {
                InvSlot &slot = g_inv[g_hotbarSel];
                BlockType btp = itemToBlock(slot.item);
                if (btp != BlockType::Air && slot.count > 0) {
                    glm::ivec3 placePos = target.blockPos + target.faceNorm;
                    world.setBlock(placePos.x, placePos.y, placePos.z, btp);
                    slot.count--;
                    if (slot.count == 0) slot = {};
                }
            }
            g_placePending = false;
        }
        g_breakPending = false; // no longer used for breaking

        // Load chunks around current player position
        {
            int cx = (int)std::floor(g_cam.position.x / CHUNK_W);
            int cz = (int)std::floor(g_cam.position.z / CHUNK_D);
            world.loadAround(cx, cz, RENDER_DIST);
        }

        // Rebuild at most 4 dirty chunks per frame to avoid startup freeze
        world.update(4);

        // ── Arm animation ─────────────────────────────────────────────────
        // Swing: single arc forward (sin curve) over 0.35 s, like Minecraft
        if (g_armSwinging) {
            g_armSwing += dt / 0.35f;
            if (g_armSwing >= 1.f) {
                g_armSwing = 0.f;
                g_armSwinging = g_mineActive; // keep looping while mining
            }
        }
        // Walk bob: phase advances while WASD held on ground
        if (!g_freeCam && g_onGround &&
            (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS ||
             glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS ||
             glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS ||
             glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)) {
            g_armBobPhase += dt * 9.f;
        }

        // ── Update dropped items: spin, bob, magnetic pickup ───────────────
        {
            const float MAGNET_R = 3.5f;
            const float PICKUP_R = 1.0f;
            for (auto it = g_drops.begin(); it != g_drops.end(); ) {
                it->spin += dt * 1.8f;
                it->bob  += dt * 2.5f;

                glm::vec3 toPlayer = g_cam.position - it->pos;
                float dist = glm::length(toPlayer);
                if (dist < MAGNET_R && dist > 0.01f) {
                    float spd = 2.f + 6.f * (1.f - dist / MAGNET_R);
                    it->pos += glm::normalize(toPlayer) * spd * dt;
                    dist = glm::length(g_cam.position - it->pos);
                }
                if (dist < PICKUP_R) {
                    ItemType dropItem = blockToItem(it->type);
                    if (dropItem != ItemType::None && addToInventory(dropItem)) {
                        it = g_drops.erase(it);
                        continue;
                    }
                }
                ++it;
            }
        }

        // ── Render ────────────────────────────────────────────────────────
        int fbW, fbH;
        glfwGetFramebufferSize(win, &fbW, &fbH);
        g_fbW = fbW; g_fbH = fbH; // keep globals in sync for inventory hit-test
        glViewport(0, 0, fbW, fbH);
        glClearColor(0.53f, 0.81f, 0.98f, 1.f); // sky blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas);
        shader.setInt("uTexture", 0);
        shader.setFloat("uTime", now);

        float aspect = fbH > 0 ? (float)fbW / (float)fbH : 1.f;
        shader.setMat4("uView",       g_cam.viewMatrix());
        shader.setMat4("uProjection", g_cam.projectionMatrix(aspect));

        // Opaque pass — leaves need both-sided rendering, no culling
        glDisable(GL_CULL_FACE);
        world.draw();
        glEnable(GL_CULL_FACE);

        // Transparent pass (water) — blended, depth read-only so far geometry shows through
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        world.drawTransparent();
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // ── Block highlight ────────────────────────────────────────────────
        if (target.hit) {
            hlShader.use();
            hlShader.setMat4("uView",       g_cam.viewMatrix());
            hlShader.setMat4("uProjection", g_cam.projectionMatrix(aspect));
            hlShader.setVec3("uBlockPos",   glm::vec3(target.blockPos));
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthFunc(GL_LEQUAL);
            glBindVertexArray(hlVAO);
            glDrawArrays(GL_LINES, 0, 24);
            glDepthFunc(GL_LESS);
            glDisable(GL_BLEND);
        }

        // ── Dropped item cubes (world space, before arm depth-clear) ──────────
        if (!g_drops.empty()) {
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            armShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, atlas);
            armShader.setInt("uUseTexture", 1);
            glBindVertexArray(armVAO);
            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
            armShader.setVec3("uBoxMin", glm::vec3(-0.15f, -0.15f, -0.15f));
            armShader.setVec3("uBoxMax", glm::vec3( 0.15f,  0.15f,  0.15f));
            for (auto &d : g_drops) {
                float by = std::sin(d.bob) * 0.08f;
                glm::mat4 dropMdl =
                    glm::translate(glm::mat4(1.f), d.pos + glm::vec3(0.f, by, 0.f)) *
                    glm::rotate(glm::mat4(1.f), d.spin, glm::vec3(0.f,1.f,0.f));
                armShader.setMat4("uMVP",
                    g_cam.projectionMatrix(aspect) * g_cam.viewMatrix() * dropMdl);
                auto setTiles = [&](int top, int side, int bot) {
                    armShader.setInt("uTileTop", top);
                    armShader.setInt("uTileSide", side);
                    armShader.setInt("uTileBot", bot);
                };
                switch (d.type) {
                    case BlockType::Grass:  setTiles(0, 1, 2); break;
                    case BlockType::Dirt:   setTiles(2, 2, 2); break;
                    case BlockType::Stone:  setTiles(3, 3, 3); break;
                    case BlockType::Wood:   setTiles(4, 5, 4); break;
                    case BlockType::Leaves: setTiles(6, 6, 6); break;
                    default:               setTiles(0, 0, 0); break;
                }
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            armShader.setInt("uUseTexture", 0);
            glEnable(GL_CULL_FACE);
        }

        // ── First-person arm (normal mode only) / character model (free-cam) ──
        if (!g_freeCam) {
            // Swing: sin arc → arm pitches forward up to -80° at peak (Minecraft style)
            float swingPitch = std::sin(g_armSwing * 3.14159f) * 80.f;
            // Walk bob: gentle sway matching footstep rhythm
            float bobOffY = std::sin(g_armBobPhase)       * 0.018f;
            float bobRotX = std::sin(g_armBobPhase * 0.5f) * 2.5f;

            // 65° Y: shows inner-right face of arm like Minecraft first-person view
            glm::mat4 armMdl =
                glm::translate(glm::mat4(1.f), glm::vec3(0.50f, -0.48f + bobOffY, -0.90f)) *
                glm::rotate(glm::mat4(1.f), glm::radians(65.f),                       glm::vec3(0,1,0)) *
                glm::rotate(glm::mat4(1.f), glm::radians(-25.f - swingPitch + bobRotX), glm::vec3(1,0,0));
            glm::mat4 armMVP = g_cam.projectionMatrix(aspect) * armMdl;

            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            armShader.use();
            armShader.setMat4("uMVP", armMVP);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, atlas);
            // No vertex attributes used — geometry is purely uniform-driven.
            glBindVertexArray(armVAO);
            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);

            // Forearm — flat skin colour
            armShader.setInt("uUseTexture", 0);
            armShader.setVec3("uBoxMin", glm::vec3(-0.06f, -0.48f, -0.06f));
            armShader.setVec3("uBoxMax", glm::vec3( 0.06f,  0.00f,  0.06f));
            armShader.setVec4("uColor",  glm::vec4(0.80f, 0.63f, 0.47f, 1.f));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Held item — textured blocks, flat-colour tools
            {
                auto box = [&](glm::vec3 mn, glm::vec3 mx, glm::vec4 col) {
                    armShader.setInt("uUseTexture", 0);
                    armShader.setVec3("uBoxMin", mn);
                    armShader.setVec3("uBoxMax", mx);
                    armShader.setVec4("uColor",  col);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                };
                auto texBox = [&](glm::vec3 mn, glm::vec3 mx, int top, int side, int bot) {
                    armShader.setInt("uUseTexture", 1);
                    armShader.setInt("uTileTop",  top);
                    armShader.setInt("uTileSide", side);
                    armShader.setInt("uTileBot",  bot);
                    armShader.setVec3("uBoxMin", mn);
                    armShader.setVec3("uBoxMax", mx);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                };
                static const glm::vec4 kWood  {0.60f,0.40f,0.22f,1.f};
                static const glm::vec4 kIron  {0.72f,0.72f,0.74f,1.f};
                static const glm::vec4 kSilver{0.88f,0.88f,0.92f,1.f};

                ItemType held    = g_inv[g_hotbarSel].item;
                int      heldCnt = g_inv[g_hotbarSel].count;
                switch (held) {
                case ItemType::Grass:
                    if (heldCnt > 0) texBox({-0.12f,0.f,-0.12f},{0.12f,0.24f,0.12f}, 0, 1, 2);
                    break;
                case ItemType::Dirt:
                    if (heldCnt > 0) texBox({-0.12f,0.f,-0.12f},{0.12f,0.24f,0.12f}, 2, 2, 2);
                    break;
                case ItemType::Stone:
                    if (heldCnt > 0) texBox({-0.12f,0.f,-0.12f},{0.12f,0.24f,0.12f}, 3, 3, 3);
                    break;
                case ItemType::Wood:
                    if (heldCnt > 0) texBox({-0.12f,0.f,-0.12f},{0.12f,0.24f,0.12f}, 4, 5, 4);
                    break;
                case ItemType::Leaves:
                    if (heldCnt > 0) texBox({-0.12f,0.f,-0.12f},{0.12f,0.24f,0.12f}, 6, 6, 6);
                    break;
                case ItemType::Pickaxe:
                    box({-0.012f,-0.36f,-0.012f},{0.012f,0.16f,0.012f}, kWood);
                    box({-0.18f, 0.08f,-0.012f},{0.18f,0.18f,0.012f},  kIron);
                    box({-0.20f,-0.02f,-0.012f},{-0.15f,0.10f,0.012f}, kIron);
                    box({ 0.15f,-0.02f,-0.012f},{ 0.20f,0.10f,0.012f}, kIron);
                    break;
                case ItemType::Shovel:
                    box({-0.012f,-0.36f,-0.012f},{0.012f,0.12f,0.012f}, kWood);
                    box({-0.07f, 0.06f,-0.015f},{ 0.07f,0.30f,0.015f}, kIron);
                    break;
                case ItemType::Axe:
                    box({-0.012f,-0.36f,-0.012f},{0.012f,0.22f,0.012f}, kWood);
                    box({ 0.01f, 0.08f,-0.015f},{ 0.22f,0.28f,0.015f}, kIron);
                    box({-0.07f, 0.14f,-0.015f},{ 0.01f,0.28f,0.015f}, kIron);
                    break;
                case ItemType::Sword:
                    box({-0.016f,-0.14f,-0.016f},{0.016f,0.06f,0.016f}, kWood);
                    box({-0.10f,  0.04f,-0.012f},{0.10f, 0.09f,0.012f}, kIron);
                    box({-0.018f, 0.07f,-0.010f},{0.018f,0.42f,0.010f}, kSilver);
                    break;
                default: break;
                }
                armShader.setInt("uUseTexture", 0);
            }

            glDisable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
        }

        // ── Third-person character model (free-cam mode) ─────────────────
        if (g_freeCam) {
            // Character faces +Z locally; rotate to match saved player yaw.
            glm::vec3 feetPos = {g_playerPos.x, g_playerPos.y - 1.7f, g_playerPos.z};
            glm::mat4 bodyMdl =
                glm::translate(glm::mat4(1.f), feetPos) *
                glm::rotate(glm::mat4(1.f), glm::radians(90.f - g_playerYaw), glm::vec3(0,1,0));
            glm::mat4 charMVP = g_cam.projectionMatrix(aspect) * g_cam.viewMatrix() * bodyMdl;

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            armShader.use();
            armShader.setMat4("uMVP", charMVP);
            glBindVertexArray(charVAO);
            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);

            struct Part { glm::vec3 mn, mx; glm::vec4 col; };
            static const Part kParts[6] = {
                { {-0.25f,1.50f,-0.25f}, { 0.25f,2.00f, 0.25f}, {0.80f,0.63f,0.47f,1.f} }, // head
                { {-0.25f,0.75f,-0.125f},{ 0.25f,1.50f, 0.125f},{0.25f,0.35f,0.80f,1.f} }, // torso
                { {-0.45f,0.75f,-0.10f}, {-0.25f,1.45f, 0.10f}, {0.80f,0.63f,0.47f,1.f} }, // L arm
                { { 0.25f,0.75f,-0.10f}, { 0.45f,1.45f, 0.10f}, {0.80f,0.63f,0.47f,1.f} }, // R arm
                { {-0.25f,0.00f,-0.10f}, {-0.05f,0.75f, 0.10f}, {0.15f,0.20f,0.60f,1.f} }, // L leg
                { { 0.05f,0.00f,-0.10f}, { 0.25f,0.75f, 0.10f}, {0.15f,0.20f,0.60f,1.f} }, // R leg
            };
            for (const auto &p : kParts) {
                armShader.setVec3("uBoxMin", p.mn);
                armShader.setVec3("uBoxMax", p.mx);
                armShader.setVec4("uColor",  p.col);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
        }

        // ── Crosshair ─────────────────────────────────────────────────────
        {
            float aw = 10.f * 2.f / fbW; // horizontal arm half-length (NDC)
            float ah = 10.f * 2.f / fbH; // vertical   arm half-length
            float tw = 1.5f * 2.f / fbW; // bar half-thickness X
            float th = 1.5f * 2.f / fbH; // bar half-thickness Y
            float oaw = aw + 2.f/fbW, oah = ah + 2.f/fbH; // outline = 1 px bigger
            float ow  = tw + 2.f/fbW, oh  = th + 2.f/fbH;

            glDisable(GL_DEPTH_TEST);
            uiShader.use();
            uiShader.setVec2("uOffset", glm::vec2(0.f, 0.f));
            glBindVertexArray(crossVAO);
            glBindBuffer(GL_ARRAY_BUFFER, crossVBO);

            // Black outline (slightly larger)
            float outline[24] = {
                -oaw,-oh,  oaw,-oh,  oaw, oh,  -oaw,-oh,  oaw, oh,  -oaw, oh,
                -ow,-oah,  ow,-oah,  ow, oah,  -ow,-oah,  ow, oah,  -ow, oah,
            };
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(outline), outline);
            uiShader.setVec4("uColor", glm::vec4(0.f, 0.f, 0.f, 1.f));
            glDrawArrays(GL_TRIANGLES, 0, 12);

            // White fill
            float fill[24] = {
                -aw,-th,  aw,-th,  aw, th,  -aw,-th,  aw, th,  -aw, th,
                -tw,-ah,  tw,-ah,  tw, ah,  -tw,-ah,  tw, ah,  -tw, ah,
            };
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fill), fill);
            uiShader.setVec4("uColor", glm::vec4(1.f, 1.f, 1.f, 1.f));
            glDrawArrays(GL_TRIANGLES, 0, 12);

            glEnable(GL_DEPTH_TEST);
        }

        // ── FPS counter (7-segment style, top-right) ───────────────────────────
        {
            // Update displayed value at most once per 0.5 s (smooth but responsive)
            static int   fpsN   = 0;
            static float fpsT   = 0.f;
            static int   fpsVal = 0;
            fpsT += dt;
            if (++fpsN == 10 || fpsT >= 0.5f) {
                fpsVal = (int)((float)fpsN / fpsT + 0.5f);
                fpsN = 0; fpsT = 0.f;
            }

            // Segment masks: a(0)=top b(1)=topR c(2)=botR d(3)=bot e(4)=botL f(5)=topL g(6)=mid
            static const uint8_t kSeg[10] = {63,6,91,79,102,109,125,7,127,111};
            // Segment endpoints in local [0..1] x [0..2] space
            static const float kSV[7][4] = {
                {0.15f,1.85f,0.85f,1.85f}, // a top
                {0.90f,1.10f,0.90f,1.80f}, // b topR
                {0.90f,0.20f,0.90f,0.90f}, // c botR
                {0.15f,0.15f,0.85f,0.15f}, // d bot
                {0.10f,0.20f,0.10f,0.90f}, // e botL
                {0.10f,1.10f,0.10f,1.80f}, // f topL
                {0.15f,1.00f,0.85f,1.00f}, // g mid
            };

            int val = fpsVal > 9999 ? 9999 : (fpsVal < 0 ? 0 : fpsVal);
            int digs[4]; int nd = 0;
            do { digs[nd++] = val % 10; val /= 10; } while (val > 0 && nd < 4);
            for (int i = 0, j = nd-1; i < j; ++i, --j) { int t=digs[i]; digs[i]=digs[j]; digs[j]=t; }

            const float PX = 2.f / fbW, PY = 2.f / fbH;
            const float DW = 16.f * PX, DH = 28.f * PY; // digit width / height in NDC
            const float TH = 1.8f * PX, TV = 1.8f * PY; // segment half-thickness
            const float GAP = 4.f * PX;

            float totW = nd * DW + (nd - 1) * GAP;
            float x0 = 1.f - 12.f * PX - totW;
            float yB = 1.f - 12.f * PY - DH;

            float vbuf[4 * 7 * 12]; // 4 digits x 7 segs x 6 verts x 2 floats
            int nf = 0;
            for (int di = 0; di < nd; ++di) {
                float cx = x0 + di * (DW + GAP);
                float cy = yB;
                float sX = DW, sY = DH * 0.5f; // local [0..1]->NDC width, [0..2]->NDC height
                uint8_t seg = kSeg[digs[di]];
                for (int s = 0; s < 7; ++s) {
                    if (!(seg & (1 << s))) continue;
                    float lx0 = cx + kSV[s][0] * sX, ly0 = cy + kSV[s][1] * sY;
                    float lx1 = cx + kSV[s][2] * sX, ly1 = cy + kSV[s][3] * sY;
                    float qx0 = lx0 - TH, qx1 = lx1 + TH;
                    float qy0 = ly0 - TV, qy1 = ly1 + TV;
                    vbuf[nf++]=qx0; vbuf[nf++]=qy0;
                    vbuf[nf++]=qx1; vbuf[nf++]=qy0;
                    vbuf[nf++]=qx1; vbuf[nf++]=qy1;
                    vbuf[nf++]=qx0; vbuf[nf++]=qy0;
                    vbuf[nf++]=qx1; vbuf[nf++]=qy1;
                    vbuf[nf++]=qx0; vbuf[nf++]=qy1;
                }
            }

            glBindBuffer(GL_ARRAY_BUFFER, fpsVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, nf * sizeof(float), vbuf);

            glDisable(GL_DEPTH_TEST);
            uiShader.use();
            glBindVertexArray(fpsVAO);

            uiShader.setVec2("uOffset", glm::vec2(PX, -PY));
            uiShader.setVec4("uColor",  glm::vec4(0.f, 0.f, 0.f, 1.f));
            glDrawArrays(GL_TRIANGLES, 0, nf / 2);

            uiShader.setVec2("uOffset", glm::vec2(0.f, 0.f));
            uiShader.setVec4("uColor",  glm::vec4(1.f, 1.f, 1.f, 1.f));
            glDrawArrays(GL_TRIANGLES, 0, nf / 2);

            glEnable(GL_DEPTH_TEST);
        }

        // ── HUD: health hearts + hotbar ───────────────────────────────────────
        {
            const float PX = 2.f / fbW, PY = 2.f / fbH;

            float hbuf[4096];
            int   hn = 0;

            // Screen-pixel coords (x from left, y from bottom) → NDC quad
            auto qpx = [&](float sx, float sy, float ex, float ey) {
                float x0 = -1.f + sx*PX, y0 = -1.f + sy*PY;
                float x1 = -1.f + ex*PX, y1 = -1.f + ey*PY;
                hbuf[hn++]=x0; hbuf[hn++]=y0;
                hbuf[hn++]=x1; hbuf[hn++]=y0;
                hbuf[hn++]=x1; hbuf[hn++]=y1;
                hbuf[hn++]=x0; hbuf[hn++]=y0;
                hbuf[hn++]=x1; hbuf[hn++]=y1;
                hbuf[hn++]=x0; hbuf[hn++]=y1;
            };
            auto flush = [&](glm::vec4 col) {
                if (hn == 0) return;
                glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, hn * sizeof(float), hbuf);
                uiShader.setVec4("uColor", col);
                glDrawArrays(GL_TRIANGLES, 0, hn / 2);
                hn = 0;
            };

            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            uiShader.use();
            uiShader.setVec2("uOffset", glm::vec2(0.f, 0.f));
            glBindVertexArray(hudVAO);

            // ── Health hearts (bottom-left, 10 hearts) ─────────────────────────
            // Each heart = 5 quads in a 5×5-cell pixelated shape, cell=3px
            const float HS=3.f, HGP=3.f, HX0=10.f, HY0=12.f;
            static const float kH[5][4] = {
                {1,4, 2,5}, {3,4, 4,5}, // top bumps
                {0,2, 5,4},              // middle band
                {1,1, 4,2},              // lower middle
                {2,0, 3,1},              // bottom tip
            };

            for (int i = 0; i < 10; ++i) { // shadow pass
                float hx = HX0 + i*(5.f*HS+HGP), hy = HY0;
                for (auto &r : kH) qpx(hx+r[0]*HS-1, hy+r[1]*HS-1, hx+r[2]*HS+1, hy+r[3]*HS+1);
            }
            flush({0.f, 0.f, 0.f, 0.65f});

            for (int i = 0; i < g_health; ++i) { // full hearts
                float hx = HX0 + i*(5.f*HS+HGP), hy = HY0;
                for (auto &r : kH) qpx(hx+r[0]*HS, hy+r[1]*HS, hx+r[2]*HS, hy+r[3]*HS);
            }
            flush({0.88f, 0.12f, 0.12f, 1.f});

            for (int i = g_health; i < 10; ++i) { // empty hearts
                float hx = HX0 + i*(5.f*HS+HGP), hy = HY0;
                for (auto &r : kH) qpx(hx+r[0]*HS, hy+r[1]*HS, hx+r[2]*HS, hy+r[3]*HS);
            }
            flush({0.28f, 0.08f, 0.08f, 1.f});

            // ── Mining progress bar (above hotbar, visible while breaking) ───
            if (g_mineActive && g_mineProgress > 0.f) {
                const float BW = 160.f, BH = 6.f, BY = 64.f;
                float bx = (float)fbW * 0.5f - BW * 0.5f;
                // Background
                qpx(bx, BY, bx+BW, BY+BH); flush({0.f,0.f,0.f,0.70f});
                // Fill — red→yellow→green based on progress
                float p = g_mineProgress;
                glm::vec4 col = { 1.f-p, p, 0.f, 1.f };
                qpx(bx, BY, bx+BW*p, BY+BH); flush(col);
            }

            // ── Hotbar (bottom-center, 8 slots) ──────────────────────────────
            const float SLOT=44.f, SGAP=4.f, BORD=3.f;
            const float totHB = kHotbarN*(SLOT+SGAP) - SGAP;
            const float hbX0 = (float)fbW*0.5f - totHB*0.5f;
            const float hbY0 = 10.f;

            // Slot backgrounds
            for (int i = 0; i < kHotbarN; ++i) {
                float sx = hbX0 + i*(SLOT+SGAP);
                qpx(sx, hbY0, sx+SLOT, hbY0+SLOT);
            }
            flush({0.08f, 0.08f, 0.08f, 0.78f});

            // Selected slot border
            {
                float sx = hbX0 + g_hotbarSel*(SLOT+SGAP), sy = hbY0;
                qpx(sx-BORD, sy+SLOT,       sx+SLOT+BORD, sy+SLOT+BORD);
                qpx(sx-BORD, sy-BORD,       sx+SLOT+BORD, sy);
                qpx(sx-BORD, sy,             sx,           sy+SLOT);
                qpx(sx+SLOT, sy,             sx+SLOT+BORD, sy+SLOT);
            }
            flush({1.f, 0.85f, 0.f, 1.f});

            // ── Item icon helper (draws textured icon + stack count) ──────────
            static const uint8_t kCS[10] = {63,6,91,79,102,109,125,7,127,111};
            static const float kCV[7][4] = {
                {0.15f,1.85f,0.85f,1.85f},{0.90f,1.10f,0.90f,1.80f},
                {0.90f,0.20f,0.90f,0.90f},{0.15f,0.15f,0.85f,0.15f},
                {0.10f,0.20f,0.10f,0.90f},{0.10f,1.10f,0.10f,1.80f},
                {0.15f,1.00f,0.85f,1.00f},
            };
            // Helper: draw one textured atlas tile as a HUD quad (no alpha blending worries
            // since the icon shader discards transparent pixels). Restores uiShader/hudVAO.
            auto drawTexIcon = [&](int tileCol, float sx, float sy, float pad=5.f) {
                float ix0 = -1.f + (sx+pad)*PX, iy0 = -1.f + (sy+pad)*PY;
                float ix1 = -1.f + (sx+SLOT-pad)*PX, iy1 = -1.f + (sy+SLOT-pad)*PY;
                float u0 = float(tileCol)/11.f, u1 = u0 + 1.f/11.f;
                float verts[24] = {
                    ix0,iy0, u0,0.f,  ix1,iy0, u1,0.f,  ix1,iy1, u1,1.f,
                    ix0,iy0, u0,0.f,  ix1,iy1, u1,1.f,  ix0,iy1, u0,1.f,
                };
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, atlas);
                iconShader.use();
                glBindVertexArray(iconVAO);
                glBindBuffer(GL_ARRAY_BUFFER, iconVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                // Restore HUD state
                uiShader.use();
                uiShader.setVec2("uOffset", glm::vec2(0.f, 0.f));
                glBindVertexArray(hudVAO);
            };

            auto drawItemIcon = [&](ItemType item, int cnt, float sx, float sy) {
                if (item == ItemType::None) return;
                switch (item) {
                case ItemType::Grass:   drawTexIcon(0,  sx, sy); break;
                case ItemType::Dirt:    drawTexIcon(2,  sx, sy); break;
                case ItemType::Stone:   drawTexIcon(3,  sx, sy); break;
                case ItemType::Wood:    drawTexIcon(5,  sx, sy); break;  // bark side looks best as icon
                case ItemType::Leaves:  drawTexIcon(6,  sx, sy); break;
                case ItemType::Pickaxe: drawTexIcon(7,  sx, sy, 4.f); break;
                case ItemType::Shovel:  drawTexIcon(8,  sx, sy, 4.f); break;
                case ItemType::Axe:     drawTexIcon(9,  sx, sy, 4.f); break;
                case ItemType::Sword:   drawTexIcon(10, sx, sy, 4.f); break;
                default: return;
                }
                // Stack count overlay (7-segment digits) for stackable block items
                if (cnt > 0 && item >= ItemType::Grass && item <= ItemType::Leaves) {
                    const float CDW=5.f, CDH=9.f, CTH=0.7f, CTV=0.7f;
                    int d1=cnt/10, d2=cnt%10;
                    float nx=sx+SLOT-(d1>0?CDW*2+3.f:CDW+2.f)-2.f, ny=sy+3.f;
                    auto drawDig = [&](int d, float x) {
                        for (int s=0;s<7;++s) {
                            if (!(kCS[d]&(1<<s))) continue;
                            float lx0=x+kCV[s][0]*CDW,  ly0=ny+kCV[s][1]*(CDH*0.5f);
                            float lx1=x+kCV[s][2]*CDW,  ly1=ny+kCV[s][3]*(CDH*0.5f);
                            qpx(lx0-CTH,ly0-CTV,lx1+CTH,ly1+CTV);
                        }
                    };
                    if (d1>0) { drawDig(d1,nx); nx+=CDW+1.f; }
                    drawDig(d2,nx);
                    flush({1.f,1.f,1.f,1.f});
                }
            };

            // Draw hotbar icons
            for (int i = 0; i < kHotbarN; ++i) {
                float sx = hbX0 + i*(SLOT+SGAP);
                drawItemIcon(g_inv[i].item, g_inv[i].count, sx, hbY0);
            }

            // ── Full inventory overlay (Tab) ──────────────────────────────────
            if (g_invOpen) {
                const int   kInvCols = kHotbarN;
                const int   kInvRows = kInvTotal / kHotbarN - 1; // 7 main rows
                const float ISEP=8.f, IPAD=12.f;
                float invW  = kInvCols*(SLOT+SGAP) - SGAP;
                float mainH = kInvRows*(SLOT+SGAP) - SGAP;
                float invH  = mainH + ISEP + SLOT;
                float invX  = ((float)fbW - invW) * 0.5f;
                float invY  = ((float)fbH - invH) * 0.5f;

                // Panel background
                qpx(invX-IPAD, invY-IPAD, invX+invW+IPAD, invY+invH+IPAD);
                flush({0.06f, 0.06f, 0.10f, 0.93f});

                // Slot backgrounds (batch all 64)
                for (int i = 0; i < kInvTotal; ++i) {
                    float sx = (i < kHotbarN)
                        ? invX + i*(SLOT+SGAP)
                        : invX + ((i-kHotbarN)%kInvCols)*(SLOT+SGAP);
                    float sy = (i < kHotbarN)
                        ? invY
                        : invY + SLOT + ISEP + (kInvRows-1-(i-kHotbarN)/kInvCols)*(SLOT+SGAP);
                    qpx(sx+1, sy+1, sx+SLOT-1, sy+SLOT-1);
                }
                flush({0.14f, 0.14f, 0.18f, 1.f});

                // Hotbar selection highlight in panel
                {
                    float sx = invX + g_hotbarSel*(SLOT+SGAP), sy = invY;
                    qpx(sx-BORD,sy+SLOT,sx+SLOT+BORD,sy+SLOT+BORD);
                    qpx(sx-BORD,sy-BORD,sx+SLOT+BORD,sy);
                    qpx(sx-BORD,sy,sx,sy+SLOT);
                    qpx(sx+SLOT,sy,sx+SLOT+BORD,sy+SLOT);
                }
                flush({1.f, 0.85f, 0.f, 1.f});

                // Hover highlight
                {
                    int hov = inventorySlotAt(g_mouseX, g_mouseY);
                    if (hov >= 0) {
                        float sx = (hov < kInvCols)
                            ? invX + hov*(SLOT+SGAP)
                            : invX + ((hov-kInvCols)%kInvCols)*(SLOT+SGAP);
                        float sy = (hov < kInvCols)
                            ? invY
                            : invY + SLOT + ISEP + (kInvRows-1-(hov-kInvCols)/kInvCols)*(SLOT+SGAP);
                        qpx(sx+1, sy+1, sx+SLOT-1, sy+SLOT-1);
                        flush({1.f, 1.f, 1.f, 0.22f});
                    }
                }

                // Icons for all 64 slots
                for (int i = 0; i < kInvTotal; ++i) {
                    float sx = (i < kInvCols)
                        ? invX + i*(SLOT+SGAP)
                        : invX + ((i-kInvCols)%kInvCols)*(SLOT+SGAP);
                    float sy = (i < kInvCols)
                        ? invY
                        : invY + SLOT + ISEP + (kInvRows-1-(i-kInvCols)/kInvCols)*(SLOT+SGAP);
                    drawItemIcon(g_inv[i].item, g_inv[i].count, sx, sy);
                }

                // Cursor item (follows mouse)
                if (g_cursorSlot.item != ItemType::None) {
                    float cx = (float)g_mouseX - SLOT*0.5f;
                    float cy = (float)g_fbH - (float)g_mouseY - SLOT*0.5f;
                    drawItemIcon(g_cursorSlot.item, g_cursorSlot.count, cx, cy);
                }
            }

            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
        }

        glfwSwapBuffers(win);
    }
    LOG("Game loop exited");

    glDeleteTextures(1, &atlas);
    glDeleteVertexArrays(1, &hlVAO);
    glDeleteBuffers(1, &hlVBO);
    glDeleteVertexArrays(1, &crossVAO);
    glDeleteBuffers(1, &crossVBO);
    glDeleteVertexArrays(1, &fpsVAO);
    glDeleteBuffers(1, &fpsVBO);
    glDeleteVertexArrays(1, &hudVAO);
    glDeleteBuffers(1, &hudVBO);
    glDeleteVertexArrays(1, &iconVAO);
    glDeleteBuffers(1, &iconVBO);
    glDeleteVertexArrays(1, &armVAO);
    glDeleteBuffers(1, &armVBO);
    glDeleteVertexArrays(1, &charVAO);
    glDeleteBuffers(1, &charVBO);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
