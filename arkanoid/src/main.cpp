#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <vector>

namespace
{

const auto screen_width = 640;
const auto screen_height = 480;
const auto screen_title = "Arkanoid";

struct vec2
{
  double x;
  double y;

  vec2& operator+=(vec2 vec)
  {
    x += vec.x;
    y += vec.y;
    return *this;
  }
};

vec2 operator-(vec2 vec) { return{ -vec.x, -vec.y }; }
vec2 operator*(vec2 vec, double factor) { return{ vec.x * factor, vec.y * factor }; }
vec2 operator*(double factor, vec2 vec) { return vec * factor; }
vec2 operator+(vec2 lhs, vec2 rhs) { return{ lhs.x + rhs.x, lhs.y + rhs.y }; }

struct block
{
  SDL_Rect shape;

  void draw(SDL_Renderer *renderer)
  {
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0x00);
    SDL_RenderFillRect(renderer, &shape);
  }
};

struct ball
{
  vec2 position;
  vec2 velocity; // Pixels per second.
  vec2 acceleration;

  int width;
  int height;

  void update(std::chrono::milliseconds elapsed)
  {
    const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count();
    velocity += acceleration * elapsed_seconds;
    position += velocity * elapsed_seconds;
  }

  void draw(SDL_Renderer *renderer)
  {
    const SDL_Rect rect = { (int)position.x, (int)position.y, width, height };

    SDL_SetRenderDrawColor(renderer, 0x55, 0x55, 0x55, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &rect);
  }
};

struct player_pallet
{
  vec2 position;
  vec2 velocity; // Pixels per second.

  int    width;
  int    height;

  static const auto move_speed = 500;

  void on_keydown(SDL_Keysym key)
  {
    switch (key.sym)
    {
      case SDLK_LEFT: velocity.x = -move_speed; break;
      case SDLK_RIGHT: velocity.x = move_speed; break;
    }
  }

  void on_keyup(SDL_Keysym key)
  {
    switch (key.sym)
    {
      case SDLK_LEFT:
      case SDLK_RIGHT:
        velocity.x = 0;
    }
  }

  void update(std::chrono::milliseconds elapsed)
  {
    const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count();
    position += velocity * elapsed_seconds;
  }

  void draw(SDL_Renderer *renderer)
  {
    const SDL_Rect rect = { (int)position.x, (int)position.y, width, height };

    SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, SDL_ALPHA_OPAQUE);
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

template<typename T> T clamp(T value, T min, T max)
{
  return std::max(min, std::min(max, value));
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

  auto blocks = make_blocks({ 10, 10, screen_width - 20, (screen_height - 10) / 4 }, 4, 4, 2);
  player_pallet player = { { 2 * screen_width / 5, screen_height - 10 }, { 0, 0 }, screen_width / 5, 8 };

  ball b = { { screen_width / 2 + 5, screen_height - 20 }, { 50, -50 }, { 5, -5 }, 10, 10 };

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
          player.on_keydown(e.key.keysym);
          break;
        case SDL_KEYUP:
          player.on_keyup(e.key.keysym);
          break;
      }
    }

    auto curr_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - prev_time);
    prev_time = curr_time;

    b.update(elapsed);
    player.update(elapsed);

    if (b.position.x > screen_width || b.position.x < 0)
    {
      b.position.x = clamp(b.position.x, 0.0, (double)screen_width);
      b.velocity.x = -b.velocity.x;
      b.acceleration.x = -b.acceleration.x;
    }
    if (b.position.y > screen_height || b.position.y < 0)
    {
      b.position.y = clamp(b.position.y, 0.0, (double)screen_height);
      b.velocity.y = -b.velocity.y;
      b.acceleration.y = -b.acceleration.y;
    }

    auto new_end = std::remove_if(std::begin(blocks), std::end(blocks), [&b](block blk) -> bool {
      if (blk.shape.x > b.position.x + b.width) { return false; }
      if (blk.shape.x + blk.shape.w < b.position.x) { return false; }
      if (blk.shape.y > b.position.y + b.height) { return false; }
      if (blk.shape.y + blk.shape.h < b.position.y) { return false; }

      b.velocity = -b.velocity;
      b.acceleration = -b.acceleration;
      return true;
    });

    blocks.erase(new_end, std::end(blocks));

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    for (auto&& block : blocks) { block.draw(renderer); }

    player.draw(renderer);
    b.draw(renderer);

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}