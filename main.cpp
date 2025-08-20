#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

constexpr int SCR_W = 800;
constexpr int SCR_H = 600;
constexpr int TEX_W = 64;
constexpr int TEX_H = 64;

// --- Simple map (1=brick, 2=stone, etc.)
constexpr int MAP_W = 16;
constexpr int MAP_H = 16;
const int worldMap[MAP_H][MAP_W] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

// --- Very small procedural textures (replace with real ones later)
std::array<std::array<uint32_t, TEX_W * TEX_H>, 4> g_textures;

uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
  return (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) |
         uint32_t(a);
}

void makeDummyTextures() {
  // tex 0: checker
  for (int y = 0; y < TEX_H; y++)
    for (int x = 0; x < TEX_W; x++) {
      bool c = ((x / 8) ^ (y / 8)) & 1;
      g_textures[0][y * TEX_W + x] =
          c ? rgba(200, 200, 200) : rgba(100, 100, 100);
    }
  // tex 1: bricks
  for (int y = 0; y < TEX_H; y++)
    for (int x = 0; x < TEX_W; x++) {
      int mortar = (y % 16 == 15 || (x % 32) == 31) ? 60 : 0;
      g_textures[1][y * TEX_W + x] =
          rgba(160 + mortar, 40 + mortar / 2, 40 + mortar / 2);
    }
  // tex 2: blue tiles
  for (int y = 0; y < TEX_H; y++)
    for (int x = 0; x < TEX_W; x++) {
      bool c = ((x / 16) ^ (y / 16)) & 1;
      g_textures[2][y * TEX_W + x] = c ? rgba(60, 60, 200) : rgba(30, 30, 120);
    }
  // tex 3: stripes
  for (int y = 0; y < TEX_H; y++)
    for (int x = 0; x < TEX_W; x++) {
      g_textures[3][y * TEX_W + x] =
          ((x / 8) & 1) ? rgba(210, 180, 80) : rgba(120, 90, 30);
    }
}

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL init failed: " << SDL_GetError() << "\n";
    return 1;
  }

  SDL_Window *window =
      SDL_CreateWindow("Raycaster (SDL3)", SCR_W, SCR_H, SDL_WINDOW_BORDERLESS);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
  if (!renderer) {
    std::cerr << "Renderer failed: " << SDL_GetError() << "\n";
    return 1;
  }

  // Streaming texture we update each frame from our CPU pixel buffer
  SDL_Texture *screenTex =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_STREAMING, SCR_W, SCR_H);
  std::vector<uint32_t> framebuffer(SCR_W * SCR_H);

  makeDummyTextures();

  // Player
  double px = 3.5, py = 3.5;          // position in map units
  double dirX = 1.0, dirY = 0.0;      // facing direction
  double planeX = 0.0, planeY = 0.80; // camera plane (≈ FOV 66°)

  // FPS locking variables
  const double FRAME_TIME = 1.0 / 60.0; // 60 FPS = 16.67ms
  Uint64 frameStart, frameEnd;
  double frameDuration;

  bool running = true;
  SDL_Event ev;

  auto getTex = [&](int id) -> const std::array<uint32_t, TEX_W * TEX_H> & {
    // map ids: 1→tex1, 2→tex2 else tex0
    if (id == 1)
      return g_textures[1];
    if (id == 2)
      return g_textures[2];
    return g_textures[0];
  };

  while (running) {
    frameStart = SDL_GetTicksNS();
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_EVENT_QUIT)
        running = false;
      if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE)
        running = false;
    }

    // --- basic movement (WASD + rotate with arrows)
    const bool *ks = SDL_GetKeyboardState(nullptr);
    double move = 0.05, rot = 0.03;
    if (ks[SDL_SCANCODE_W]) {
      if (worldMap[int(py)][int(px + dirX * move)] == 0)
        px += dirX * move;
      if (worldMap[int(py + dirY * move)][int(px)] == 0)
        py += dirY * move;
    }
    if (ks[SDL_SCANCODE_S]) {
      if (worldMap[int(py)][int(px - dirX * move)] == 0)
        px -= dirX * move;
      if (worldMap[int(py - dirY * move)][int(px)] == 0)
        py -= dirY * move;
    }
    if (ks[SDL_SCANCODE_A]) { // strafe left
      double oldDirX = dirX;
      dirX = dirX * std::cos(-rot) - dirY * std::sin(-rot);
      dirY = oldDirX * std::sin(-rot) + dirY * std::cos(-rot);
      double oldPlaneX = planeX;
      planeX = planeX * std::cos(-rot) - planeY * std::sin(-rot);
      planeY = oldPlaneX * std::sin(-rot) + planeY * std::cos(-rot);
    }
    if (ks[SDL_SCANCODE_D]) { // strafe right
      double oldDirX = dirX;
      dirX = dirX * std::cos(rot) - dirY * std::sin(rot);
      dirY = oldDirX * std::sin(rot) + dirY * std::cos(rot);
      double oldPlaneX = planeX;
      planeX = planeX * std::cos(rot) - planeY * std::sin(rot);
      planeY = oldPlaneX * std::sin(rot) + planeY * std::cos(rot);
    }
    if (ks[SDL_SCANCODE_LEFT]) {
      double sx = dirY, sy = -dirX;
      if (worldMap[int(py)][int(px + sx * move)] == 0)
        px += sx * move;
      if (worldMap[int(py + sy * move)][int(px)] == 0)
        py += sy * move;
    }
    if (ks[SDL_SCANCODE_RIGHT]) {
      double sx = -dirY, sy = dirX;
      if (worldMap[int(py)][int(px + sx * move)] == 0)
        px += sx * move;
      if (worldMap[int(py + sy * move)][int(px)] == 0)
        py += sy * move;
    }

    // --- clear background: ceiling/floor
    for (int y = 0; y < SCR_H; ++y) {
      uint32_t c = (y < SCR_H / 2) ? rgba(60, 60, 80) : rgba(40, 40, 40);
      std::fill_n(&framebuffer[y * SCR_W], SCR_W, c);
    }

    // --- cast one ray per column
    for (int x = 0; x < SCR_W; ++x) {
      double cameraX = 2.0 * x / double(SCR_W) - 1.0;
      double rayDirX = dirX + planeX * cameraX;
      double rayDirY = dirY + planeY * cameraX;

      int mapX = int(px), mapY = int(py);

      double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1.0 / rayDirX);
      double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1.0 / rayDirY);
      double sideDistX, sideDistY;
      int stepX, stepY;

      if (rayDirX < 0) {
        stepX = -1;
        sideDistX = (px - mapX) * deltaDistX;
      } else {
        stepX = 1;
        sideDistX = (mapX + 1.0 - px) * deltaDistX;
      }
      if (rayDirY < 0) {
        stepY = -1;
        sideDistY = (py - mapY) * deltaDistY;
      } else {
        stepY = 1;
        sideDistY = (mapY + 1.0 - py) * deltaDistY;
      }

      int hit = 0, side = 0;
      while (!hit) {
        if (sideDistX < sideDistY) {
          sideDistX += deltaDistX;
          mapX += stepX;
          side = 0;
        } else {
          sideDistY += deltaDistY;
          mapY += stepY;
          side = 1;
        }
        if (mapX < 0 || mapX >= MAP_W || mapY < 0 || mapY >= MAP_H) {
          hit = 1;
          break;
        }
        if (worldMap[mapY][mapX] > 0)
          hit = 1;
      }

      // Perpendicular distance to the wall
      double perpWallDist;
      if (side == 0)
        perpWallDist =
            (mapX - px + (1 - stepX) / 2.0) / (rayDirX == 0 ? 1e-9 : rayDirX);
      else
        perpWallDist =
            (mapY - py + (1 - stepY) / 2.0) / (rayDirY == 0 ? 1e-9 : rayDirY);
      if (perpWallDist <= 1e-6)
        perpWallDist = 1e-6;

      int lineH = int(SCR_H / perpWallDist);
      int drawStart = std::max(-lineH / 2 + SCR_H / 2, 0);
      int drawEnd = std::min(lineH / 2 + SCR_H / 2, SCR_H - 1);

      // Where exactly did the wall get hit? (for texX)
      double wallX;
      if (side == 0)
        wallX = py + perpWallDist * rayDirY;
      else
        wallX = px + perpWallDist * rayDirX;
      wallX -= std::floor(wallX);

      int texX = int(wallX * double(TEX_W));
      // Flip for certain sides so textures don't mirror
      if (side == 0 && rayDirX > 0)
        texX = TEX_W - texX - 1;
      if (side == 1 && rayDirY < 0)
        texX = TEX_W - texX - 1;

      int tid = worldMap[mapY][mapX];
      auto const &tex = getTex(tid);

      // texture step per screen pixel
      double step = double(TEX_H) / double(lineH);
      double texPos = (drawStart - (-lineH / 2 + SCR_H / 2)) * step;

      for (int y = drawStart; y <= drawEnd; ++y) {
        int texY = int(texPos) & (TEX_H - 1); // TEX_H=64 power-of-two
        texPos += step;
        uint32_t c = tex[texY * TEX_W + texX];

        // simple shading for Y sides
        if (side == 0) {
          uint8_t r = (c >> 24) & 0xFF, g = (c >> 16) & 0xFF,
                  b = (c >> 8) & 0xFF, a = c & 0xFF;
          r = (r * 180) / 255;
          g = (g * 180) / 255;
          b = (b * 180) / 255;
          c = (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) |
              a;
        }
        framebuffer[y * SCR_W + x] = c;
      }
    }

    // upload framebuffer → texture → screen
    SDL_UpdateTexture(screenTex, nullptr, framebuffer.data(),
                      SCR_W * sizeof(uint32_t));
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, screenTex, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    // Lock FPS to 60
    frameEnd = SDL_GetTicksNS();
    frameDuration = (frameEnd - frameStart) / 1e9;

    if (frameDuration < FRAME_TIME) {
      Uint32 delayMs = (Uint32)((FRAME_TIME - frameDuration) * 1000.0);
      SDL_Delay(delayMs);
    }
  }

  SDL_DestroyTexture(screenTex);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
