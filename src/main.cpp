#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <cstdio>
#include <algorithm>
#include <cmath>

#include "Camera.h"
#include "Shader.h"
#include "World.h"

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

// Pending block actions set by mouse callbacks
static bool        g_breakPending = false;
static bool        g_placePending = false;
static BlockType   g_placeType   = BlockType::Grass;

// ── Callbacks ─────────────────────────────────────────────────────────────────
static void onResize(GLFWwindow *, int w, int h) {
    glViewport(0, 0, w, h);
}

static void onMouseMove(GLFWwindow *, double xpos, double ypos) {
    if (g_firstMouse) { g_lastMouseX = (float)xpos; g_lastMouseY = (float)ypos; g_firstMouse = false; }
    float dx =  (float)xpos - g_lastMouseX;
    float dy = -((float)ypos - g_lastMouseY); // flipped: up = positive pitch
    g_lastMouseX = (float)xpos;
    g_lastMouseY = (float)ypos;
    g_cam.processMouseMove(dx, dy);
}

static void onMouseButton(GLFWwindow *, int button, int action, int) {
    if (action != GLFW_PRESS) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT)  g_breakPending = true;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) g_placePending = true;
}

static void onKey(GLFWwindow *win, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, GLFW_TRUE);
    if (key == GLFW_KEY_F)      { g_wireframe = !g_wireframe; glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL); }
    if (key == GLFW_KEY_1)      g_placeType = BlockType::Grass;
    if (key == GLFW_KEY_2)      g_placeType = BlockType::Dirt;
    if (key == GLFW_KEY_3)      g_placeType = BlockType::Stone;
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

// ── Procedural texture atlas ──────────────────────────────────────────────────
// 4 tiles x 1 tile, 16x16 px each -> 64x16 RGBA
struct AtlasColor { uint8_t r,g,b,a; };

static GLuint buildTextureAtlas() {
    constexpr int TILE = 16;
    constexpr int COLS = 4;
    constexpr int W    = TILE * COLS;
    constexpr int H    = TILE;

    static const AtlasColor k_base[COLS] = {
        {74,  148, 50,  255}, // 0 grass-top
        {93,  120, 56,  255}, // 1 grass-side
        {134, 96,  67,  255}, // 2 dirt
        {128, 128, 128, 255}  // 3 stone
    };

    auto clamp8 = [](int v) -> uint8_t {
        return v < 0 ? 0 : (v > 255 ? 255 : (uint8_t)v);
    };

    AtlasColor pixels[H][W];
    for (int ty = 0; ty < H; ty++)
    for (int col = 0; col < COLS; col++) {
        AtlasColor base = k_base[col];
        for (int tx = 0; tx < TILE; tx++) {
            int noise = ((tx ^ ty) & 3) * 6 - 9;
            int px = col * TILE + tx;
            pixels[ty][px].r = clamp8(base.r + noise);
            pixels[ty][px].g = clamp8(base.g + noise);
            pixels[ty][px].b = clamp8(base.b + noise);
            pixels[ty][px].a = 255;
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

    // Texture atlas
    LOG("Building texture atlas...");
    GLuint atlas = buildTextureAtlas();
    LOG("Atlas OK");

    // World — spawn camera above the surface
    g_cam.position = {8.f, 68.f, 8.f};
    World world;

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

        // ── Horizontal movement with AABB collision ────────────────────────
        glm::vec3 move{0.f};
        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) move += g_cam.front() * glm::vec3{1,0,1};
        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) move -= g_cam.front() * glm::vec3{1,0,1};
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) move -= g_cam.right();
        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) move += g_cam.right();

        if (glm::length(move) > 0.001f) {
            glm::vec3 delta = glm::normalize(move) * g_cam.speed * dt;

            // Try X — hard block on collision, no auto step-up
            {
                glm::vec3 tryPos = g_cam.position;
                tryPos.x += delta.x;
                if (!overlapsBlock(world, tryPos))
                    g_cam.position.x = tryPos.x;
            }

            // Try Z
            {
                glm::vec3 tryPos = g_cam.position;
                tryPos.z += delta.z;
                if (!overlapsBlock(world, tryPos))
                    g_cam.position.z = tryPos.z;
            }
        }

        // ── Gravity ───────────────────────────────────────────────────────
        g_velY -= GRAVITY * dt;
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
                g_cam.position.y = groundF;
                g_velY = 0.f;
                g_onGround = true;
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

        // Fly down (shift) — for debugging/building
        if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            g_cam.position.y -= g_cam.speed * dt;

        // ── Block targeting (once per frame, used for interaction + highlight) ──
        RaycastResult target = world.raycast(g_cam.position, g_cam.front());

        if (g_breakPending || g_placePending) {
            if (target.hit) {
                if (g_breakPending) {
                    world.setBlock(target.blockPos.x, target.blockPos.y, target.blockPos.z, BlockType::Air);
                } else {
                    glm::ivec3 placePos = target.blockPos + target.faceNorm;
                    world.setBlock(placePos.x, placePos.y, placePos.z, g_placeType);
                }
            }
            g_breakPending = g_placePending = false;
        }

        // Load chunks around current player position
        {
            int cx = (int)std::floor(g_cam.position.x / CHUNK_W);
            int cz = (int)std::floor(g_cam.position.z / CHUNK_D);
            world.loadAround(cx, cz, RENDER_DIST);
        }

        // Rebuild at most 4 dirty chunks per frame to avoid startup freeze
        world.update(4);

        // ── Render ────────────────────────────────────────────────────────
        int fbW, fbH;
        glfwGetFramebufferSize(win, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);
        glClearColor(0.53f, 0.81f, 0.98f, 1.f); // sky blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas);
        shader.setInt("uTexture", 0);

        float aspect = fbH > 0 ? (float)fbW / (float)fbH : 1.f;
        shader.setMat4("uView",       g_cam.viewMatrix());
        shader.setMat4("uProjection", g_cam.projectionMatrix(aspect));

        world.draw();

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
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
