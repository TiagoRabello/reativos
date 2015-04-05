#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <vector>

namespace
{

const auto screen_width = 640;
const auto screen_height = 480;
const auto screen_title = "Arkanoid";

template<typename T> T clamp(T value, T min, T max)
{
  return std::max(min, std::min(max, value));
}

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
vec2 operator-(vec2 lhs, vec2 rhs) { return{ lhs.x - rhs.x, lhs.y - rhs.y }; }

struct aabb
{
  vec2 position;
  int width;
  int height;
};

double sweep_aabb(aabb box1, aabb box2, vec2 velocity, vec2 &normal)
{
  const vec2 box1_wh = { box1.width, box1.height };
  const vec2 box2_wh = { box2.width, box2.height };

  vec2 inv_entry = box2.position - (box1.position + box1_wh);
  vec2 inv_exit = (box2.position + box2_wh) - box1.position;

  if (velocity.x < 0.0) { std::swap(inv_entry.x, inv_exit.x); }
  if (velocity.y < 0.0) { std::swap(inv_entry.y, inv_exit.y); }

  vec2 entry = { -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() };
  vec2 exit = { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };

  if (velocity.x != 0.0f)
  {
    entry.x = inv_entry.x / velocity.x;
    exit.x = inv_exit.x / velocity.x;
  }

  if (velocity.y != 0.0f)
  {
    entry.y = inv_entry.y / velocity.y;
    exit.y = inv_exit.y / velocity.y;
  }

  auto entry_time = std::max(entry.x, entry.y);
  auto exit_time = std::min(exit.x, exit.y);

  normal = { 0.0, 0.0 };

  if (entry_time > exit_time || entry.x < 0.0f && entry.y < 0.0f || entry.x > 1.0f || entry.y > 1.0f)
  {
    return 1.0f;
  }
  else // if there was a collision
  {
    if (entry.x > entry.y) { normal.x = (inv_entry.x < 0.0f) ? 1.0f : -1.0f; }
    else                   { normal.y = (inv_entry.y < 0.0f) ? 1.0f : -1.0f; }

    return entry_time;
  }
}

struct block
{
  aabb shape;

  void draw(SDL_Renderer *renderer)
  {
    const SDL_Rect rect = { (int)shape.position.x, (int)shape.position.y, shape.width, shape.height };
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0x00);
    SDL_RenderFillRect(renderer, &rect);
  }
};

struct ball
{
  aabb shape;
  vec2 velocity; // Pixels per second.
  vec2 acceleration;

  void update(std::chrono::milliseconds elapsed)
  {
    const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count();
    velocity += acceleration * elapsed_seconds;
    shape.position += velocity * elapsed_seconds;
  }

  void draw(SDL_Renderer *renderer)
  {
    const SDL_Rect rect = { (int)shape.position.x, (int)shape.position.y, shape.width, shape.height };
    SDL_SetRenderDrawColor(renderer, 0x55, 0x55, 0x55, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &rect);
  }
};

struct player_pallet
{
  aabb shape;
  vec2 velocity; // Pixels per second.

  static const auto move_speed = 500;

  void on_keydown(SDL_KeyboardEvent key_event)
  {
    if (key_event.repeat) { return; }

    switch (key_event.keysym.sym)
    {
      case SDLK_LEFT: velocity.x -= move_speed; break;
      case SDLK_RIGHT: velocity.x += move_speed; break;
    }
  }

  void on_keyup(SDL_KeyboardEvent key_event)
  {
    if (key_event.repeat) { return; }

    switch (key_event.keysym.sym)
    {
      case SDLK_LEFT: velocity.x += move_speed; break;
      case SDLK_RIGHT: velocity.x -= move_speed; break;
    }
  }

  void update(std::chrono::milliseconds elapsed)
  {
    const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count();
    shape.position += velocity * elapsed_seconds;
    shape.position.x = clamp(shape.position.x, 0.0, (double)screen_width - shape.width);
  }

  void draw(SDL_Renderer *renderer)
  {
    const SDL_Rect rect = { (int)shape.position.x, (int)shape.position.y, shape.width, shape.height };
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
      blocks.push_back({ { { pos_x + block_padding, pos_y + block_padding }, block_width, block_height } });
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

  auto blocks = make_blocks({ 10, 10, screen_width - 20, (screen_height - 10) / 4 }, 4, 4, 2);
  player_pallet player = { { { 2 * screen_width / 5, screen_height - 10 }, screen_width / 5, 8 }, { 0, 0 } };

  ball b = { { { screen_width / 2 + 5, screen_height - 20 }, 10, 10 }, { 50, -50 }, { 5, -5 } };

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
          player.on_keydown(e.key);
          break;
        case SDL_KEYUP:
          player.on_keyup(e.key);
          break;
      }
    }

    auto curr_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - prev_time);
    prev_time = curr_time;

    const auto ball_prev = b.shape;
    b.update(elapsed);
    player.update(elapsed);

    if (b.shape.position.x > screen_width || b.shape.position.x < 0)
    {
      b.shape.position.x = clamp(b.shape.position.x, 0.0, (double)screen_width);
      b.velocity.x = -b.velocity.x;
      b.acceleration.x = -b.acceleration.x;
    }
    if (b.shape.position.y > screen_height || b.shape.position.y < 0)
    {
      b.shape.position.y = clamp(b.shape.position.y, 0.0, (double)screen_height);
      b.velocity.y = -b.velocity.y;
      b.acceleration.y = -b.acceleration.y;
    }

    auto new_end = std::remove_if(std::begin(blocks), std::end(blocks), [&b, ball_prev, elapsed](block blk) -> bool {
      vec2 normal;
      const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count();
      auto t = sweep_aabb(ball_prev, blk.shape, b.velocity * elapsed_seconds, normal);

      const auto remaining_time = 1.0 - t;
      b.shape.position = b.shape.position - (b.velocity * elapsed_seconds * remaining_time);

      if (std::abs(normal.x) > 0.0) { b.velocity.x = -b.velocity.x; b.acceleration.x = -b.acceleration.x; }
      if (std::abs(normal.y) > 0.0) { b.velocity.y = -b.velocity.y; b.acceleration.y = -b.acceleration.y; }

      b.shape.position = b.shape.position + (b.velocity * elapsed_seconds * remaining_time);

      return t != 1.0f;
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