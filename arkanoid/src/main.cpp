#include <SDL.h>

namespace
{

const auto screen_width = 640;
const auto screen_height = 480;
const auto screen_title = "Arkanoid";

}

int main(int, char *[])
{
  int err = SDL_Init(SDL_INIT_EVERYTHING);
  if (err != 0) { return err; }

  SDL_Window* window = SDL_CreateWindow(screen_title,
                                        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                        screen_width, screen_height, SDL_WINDOW_SHOWN
                                        );
  if (window == NULL) { return 1; }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == NULL) { return 2; }

  SDL_Rect r = { 200, 200, 50, 50 };

  bool must_close = false;
  while (must_close == false)
  {
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
      if (e.type == SDL_QUIT) {  must_close = true; }
    }

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0x00);
    SDL_RenderFillRect(renderer, NULL);

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0x00);
    SDL_RenderFillRect(renderer, &r);

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}