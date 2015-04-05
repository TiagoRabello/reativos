#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <vector>

const SDL_Rect game_area = { 120, 0, 400, 480 };
const auto move_speed = 500;
auto lifes = 3;

template<typename T> T clamp(T value, T min, T max)
{
  return std::max(min, std::min(max, value));
}

struct vec2
{
  double x;
  double y;
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

  if (velocity.x != 0.0f) { entry.x = inv_entry.x / velocity.x; exit.x = inv_exit.x / velocity.x; }
  if (velocity.y != 0.0f) { entry.y = inv_entry.y / velocity.y; exit.y = inv_exit.y / velocity.y; }

  auto entry_time = std::max(entry.x, entry.y);
  auto exit_time = std::min(exit.x, exit.y);

  if (entry_time > exit_time || entry.x < 0.0f && entry.y < 0.0f || entry.x > 1.0f || entry.y > 1.0f)
  {
    return 1.0f;
  }
  else // if there was a collision
  {
    normal = { 0.0, 0.0 };

    if (entry.x > entry.y) { normal.x = (inv_entry.x < 0.0f) ? 1.0f : -1.0f; }
    else                   { normal.y = (inv_entry.y < 0.0f) ? 1.0f : -1.0f; }

    return entry_time;
  }
}
std::shared_ptr<SDL_Texture> load_texture(SDL_Renderer *renderer, const char *file)
{
  auto surface = SDL_LoadBMP(file);
  auto texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  return std::shared_ptr<SDL_Texture>{ texture, SDL_DestroyTexture };
}
struct moveable_box
{
  aabb shape;
  vec2 velocity; // Pixels per second.
  vec2 acceleration; // Pixel per second square
  std::shared_ptr<SDL_Texture> texture;

  void update(std::chrono::milliseconds elapsed)
  {
    const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count();
    velocity = velocity + acceleration * elapsed_seconds;
    shape.position = shape.position + velocity * elapsed_seconds;
  }
  void draw(SDL_Renderer *renderer)
  {
    const SDL_Rect rect = { (int)shape.position.x, (int)shape.position.y, shape.width, shape.height };
    SDL_RenderCopy(renderer, texture.get(), nullptr, &rect);
  }
};
using ball = moveable_box;
struct player_pallet : moveable_box
{
  void update(std::chrono::milliseconds elapsed)
  {
    moveable_box::update(elapsed);
    shape.position.x = clamp(shape.position.x, (double)game_area.x , (double)game_area.w + game_area.x - shape.width);
  }
};
struct block : moveable_box
{
  int durability;
};
std::vector<block> make_blocks(SDL_Renderer *renderer, SDL_Rect blocks_area, int num_blocks_x, int num_blocks_y, int block_padding)
{
  std::vector<block> blocks;

  auto texture = load_texture(renderer, "assets/block_sprites.bmp");

  const auto block_width = (blocks_area.w / num_blocks_x) - (2 * block_padding);
  const auto block_height = (blocks_area.h / num_blocks_y) - (2 * block_padding);

  auto pos_y = blocks_area.y;
  for (auto j = 0; j < num_blocks_y; ++j)
  {
    auto pos_x = blocks_area.x;
    for (auto i = 0; i < num_blocks_x; ++i)
    {
      block b;
      b.shape = { { pos_x + block_padding, pos_y + block_padding }, block_width, block_height };
      b.velocity = { 0.0, 0.0 };
      b.acceleration = { 0.0, 0.0 };
      b.durability = 1;
      b.texture = texture;

      blocks.push_back(b);
      pos_x += block_width + (2 * block_padding);
    }
    pos_y += block_height + (2 * block_padding);
  }

  return blocks;
}
void process_collision(ball &b, vec2 delta_position, double t, vec2 normal)
{
  const auto remaining_time = 1.0 - t;
  b.shape.position = b.shape.position - (delta_position * remaining_time);

  if (std::abs(normal.x) > 0.0) { b.velocity.x = -b.velocity.x; b.acceleration.x = -b.acceleration.x; delta_position.x = -delta_position.x; }
  if (std::abs(normal.y) > 0.0) { b.velocity.y = -b.velocity.y; b.acceleration.y = -b.acceleration.y; delta_position.y = -delta_position.y; }

  b.shape.position = b.shape.position + (delta_position * remaining_time);
}
int main(int, char *[])
{
  int err = SDL_Init(SDL_INIT_EVERYTHING);
  if (err != 0) { return err; }

  SDL_Window* window = SDL_CreateWindow("Arkanoid",
                                        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                        640, 480, SDL_WINDOW_SHOWN);
  if (window == NULL) { return 1; }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == NULL) { return 2; }

  auto player_texture = load_texture(renderer, "assets/player_sprite.bmp");

  auto blocks = make_blocks(renderer, { game_area.x + 10, game_area.y + 10, game_area.w - 20, (game_area.h - 10) / 4 }, 10, 5, 0);
  player_pallet player;
  player.shape = { { game_area.x + 2 * game_area.w / 5, game_area.y + game_area.h - 10 }, game_area.w / 5, 8 };
  player.velocity = { 0.0, 0.0 };
  player.acceleration = { 0.0, 0.0 };
  player.texture = player_texture;

  ball b = { { { game_area.x + game_area.w / 2 + 5, game_area.y + game_area.h - 20 }, 10, 10 }, { 100, -100 }, { 5, -5 }, player_texture };

  auto prev_time = std::chrono::high_resolution_clock::now();

  bool must_close = false;
  while (must_close == false)
  {
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
      if (e.type == SDL_QUIT)
      {
        must_close = true;
      }
      else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
      {
        if (e.key.repeat) { continue; }

             if (e.key.keysym.sym == SDLK_LEFT) { player.velocity.x += (e.type == SDL_KEYDOWN) ? -move_speed : move_speed; }
        else if (e.key.keysym.sym == SDLK_RIGHT) { player.velocity.x += (e.type == SDL_KEYDOWN) ? move_speed : -move_speed; }
      }
    }

    const auto curr_time = std::chrono::high_resolution_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - prev_time);
    prev_time = curr_time;

    const auto ball_prev = b.shape;
    player.update(elapsed);
    b.update(elapsed);
    auto delta_position = b.shape.position - ball_prev.position;

    if (b.shape.position.x > game_area.w + game_area.x || b.shape.position.x < game_area.x)
    {
      b.shape.position.x = clamp(b.shape.position.x, (double)game_area.x, (double)game_area.w + game_area.x);
      b.velocity.x = -b.velocity.x;
      b.acceleration.x = -b.acceleration.x;
    }
    if (b.shape.position.y > game_area.h + game_area.y)
    {
      --lifes;
      b = { { { game_area.x + game_area.w / 2 + 5, game_area.y + game_area.h - 20 }, 10, 10 }, { 100, -100 }, { 5, -5 }, player_texture };

      if (lifes == 0) { break; }
    }
    if (b.shape.position.y < game_area.y)
    {
      b.shape.position.y = game_area.y;
      b.velocity.y = -b.velocity.y;
      b.acceleration.y = -b.acceleration.y;
    }

    vec2 closest_normal = {};
    double closest_t = 1.0;
    block *closest_block = nullptr;
    for (auto&& block : blocks)
    {
      auto t = sweep_aabb(ball_prev, block.shape, delta_position * closest_t, closest_normal);
      if (t < closest_t)
      {
        closest_t = t;
        closest_block = &block;
      }
    }
    {
      auto t = sweep_aabb(ball_prev, player.shape, delta_position * closest_t, closest_normal);
      if (t < closest_t)
      {
        closest_t = t;
        closest_block = nullptr;
      }
    }

    if (closest_t != 1.0)
    {
      process_collision(b, delta_position, closest_t, closest_normal);
      if (closest_block != nullptr) { --(closest_block->durability); }
    }

    auto new_end = std::remove_if(std::begin(blocks), std::end(blocks), [](const block& b) { return b.durability == 0; });
    blocks.erase(new_end, std::end(blocks));

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xBB, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &game_area);

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