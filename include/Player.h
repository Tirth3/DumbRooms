#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "Vector.h"
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
class Player {
public:
  Vector<float> vPosition;
  Vector<float> vsize;
  SDL_FRect rRect;

  Player() {
    vPosition.x = vPosition.y = 0;
    vsize.x = vsize.y = 0;
  }

  Player(float x, float y, float w) {
    vPosition.x = x;
    vPosition.y = y;
    vsize.x = vsize.y = w;
    rRect = {vPosition.x, vPosition.y, vsize.x, vsize.y};
  }

  void Update(float dt) {
    rRect.x = vPosition.x;
    rRect.y = vPosition.y;
  }

  void Draw(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &rRect);
  }
};

#endif // !__PLAYER_H__d
