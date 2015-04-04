#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <vector>

namespace
{

const auto screen_width = 640;
const auto screen_height = 480;
const auto screen_title = "Arkanoid";

struct block
{
  SDL_Rect shape;

  void draw(SDL_Renderer *renderer)
  {
    SDL_RenderFillRect(renderer, &shape);
  }
};

struct ball
{
  double x;
  double y;
  int    width;
  int    height;
  double velocity_x; // Pixels per second.
  double velocity_y; // Pixels per second.

  void update(std::chrono::milliseconds elapsed)
  {
    x += velocity_x / 1000 * elapsed.count();
    y += velocity_y / 1000 * elapsed.count();
  }

  void draw(SDL_Renderer *renderer)
  {
    SDL_Rect rect = { (int)x, (int)y, width, height };
    SDL_RenderFillRect(renderer, &rect);
  }
};

std::vector<block> make_blocks(SDL_Rect blocks_area, int num_blocks_x, int num_blocks_y, int block_padding)
{
  std::vector<block> blocks;

  const auto block_width = (blocks_area.w / num_blocks_x) - (2 * block_padding);
  const auto block_height = (blocks_area.h / num_blocks_y) - (2 * block_padding);

  auto pos_y = blocks_area.y;
  for (auto j = 0; j < num_blocks_y; ++j)
  {
    auto pos_x = blocks_area.x;
    for (auto i = 0; i < num_blocks_x; ++i)
    {
      blocks.push_back({ { pos_x + block_padding, pos_y + block_padding, block_width, block_height } });
      pos_x += block_width + (2 * block_padding);
    }
    pos_y += block_height + (2 * block_padding);
  }

  return blocks;
}

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

  auto blocks = make_blocks({10, 10, screen_width - 20, (screen_height - 10) / 4}, 4, 4, 2);
  block player = { {2 * screen_width / 5, screen_height - 10, screen_width / 5, 8} };

  ball b = { screen_width / 2 + 5, screen_height - 20, 10, 10, 0, -50 };

  auto prev_time = std::chrono::high_resolution_clock::now();

  bool must_close = false;
  while (must_close == false)
  {
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
      switch (e.type)
      {
        case SDL_QUIT: must_close = true; break;
        case SDL_KEYDOWN:
          switch (e.key.keysym.sym)
          {
            case SDLK_LEFT: player.shape.x -= screen_width / 50; break;
            case SDLK_RIGHT: player.shape.x += screen_width / 50; break;
          }
      }
    }

    auto curr_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - prev_time);
    prev_time = curr_time;

    b.update(elapsed);

    auto new_end = std::remove_if(std::begin(blocks), std::end(blocks), [b](block blk) -> bool {
      if (blk.shape.x > b.x + b.width) { return false; }
      if (blk.shape.x + blk.shape.w < b.x) { return false; }
      if (blk.shape.y > b.y + b.height) { return false; }
      if (blk.shape.y + blk.shape.h < b.y) { return false; }

      return true;
    });

    blocks.erase(new_end, std::end(blocks));

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0x00);
    SDL_RenderFillRect(renderer, NULL);

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0x00);
    for (auto&& block : blocks)
    {
      block.draw(renderer);
    }

    SDL_RenderFillRect(renderer, &player.shape);
    b.draw(renderer);

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}