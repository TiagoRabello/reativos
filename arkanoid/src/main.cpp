#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <vector>

const SDL_Rect game_area = { 120, 0, 400, 480 };
const auto move_speed = 500;
auto player_lifes = 3;
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
  const vec2 box1_wh = { (double)box1.width, (double)box1.height };
  const vec2 box2_wh = { (double)box2.width, (double)box2.height };

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

  if (entry_time > exit_time || entry.x < 0.0f && entry.y < 0.0f || entry.x > 1.0f || entry.y > 1.0f) {
    return 1.0f;
  }
  else {
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
  SDL_Rect texture_area;

  void update(std::chrono::milliseconds elapsed)
  {
    const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count();
    velocity = velocity + acceleration * elapsed_seconds;
    shape.position = shape.position + velocity * elapsed_seconds;
  }
  void draw(SDL_Renderer *renderer)
  {
    const SDL_Rect rect = { (int)shape.position.x, (int)shape.position.y, shape.width, shape.height };
    SDL_RenderCopy(renderer, texture.get(), &texture_area, &rect);
  }
};
struct player_pallet : moveable_box
{
  void update(std::chrono::milliseconds elapsed)
  {
    moveable_box::update(elapsed);
    shape.position.x = clamp(shape.position.x, (double)game_area.x , (double)game_area.w + game_area.x - shape.width);
  }
};
struct block : moveable_box { int durability; };
using ball = moveable_box;
std::vector<block> make_blocks(SDL_Renderer *renderer, SDL_Rect blocks_area, int num_blocks_x, int num_blocks_y, int block_padding)
{
  std::vector<block> blocks;

  auto texture = load_texture(renderer, "assets/block_sprites.bmp");

  const auto block_width = (blocks_area.w / num_blocks_x) - (2 * block_padding);
  const auto block_height = (blocks_area.h / num_blocks_y) - (2 * block_padding);

  auto pos_y = blocks_area.y;
  for (auto j = 0; j < num_blocks_y; ++j) {
    auto pos_x = blocks_area.x;
    for (auto i = 0; i < num_blocks_x; ++i) {
      block b;
      b.shape = { { (double)pos_x + block_padding, (double)pos_y + block_padding }, block_width, block_height };
      b.durability = 1;
      b.velocity = { 0.0, 0.0 };
      b.acceleration = { 0.0, 0.0 };
      b.texture = texture;
      b.texture_area = { (j % 8) * 16, 0, 16, 8 };

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
  if (SDL_Init(SDL_INIT_EVERYTHING)) { return -1; }

  auto window = std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>{
    SDL_CreateWindow("Arkanoid", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN)
    , SDL_DestroyWindow
  };
  if (window == nullptr) { return 1; }

  auto renderer = std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>{ SDL_CreateRenderer(window.get(), -1, 0), SDL_DestroyRenderer };
  if (renderer == nullptr) { return 2; }

  auto player_texture = load_texture(renderer.get(), "assets/player_sprite.bmp");

  auto blocks = make_blocks(renderer.get(), { game_area.x + 10, game_area.y + 10, game_area.w - 20, (game_area.h - 10) / 4 }, 10, 6, 0);
  player_pallet player{};
  player.shape = { { game_area.x + 2.0 * game_area.w / 5, game_area.y + game_area.h - 22.0 }, 78, 18 };
  player.velocity = { 0.0, 0.0 };
  player.acceleration = { 0.0, 0.0 };
  player.texture = player_texture;
  player.texture_area = { 0, 0, 52, 12 };

  ball b = { { { game_area.x + game_area.w / 2.0 + 5, game_area.y + game_area.h - 20.0 }, 10, 10 }, { 100, -100 }, { 5, -5 }, player_texture, { 0, 0, 32, 8 } };
  const auto original_ball = b;

  auto prev_time = std::chrono::high_resolution_clock::now();

  bool must_close = false;
  while (must_close == false) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) { must_close = true; }
      else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
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

    if (b.shape.position.x > game_area.w + game_area.x || b.shape.position.x < game_area.x) {
      b.shape.position.x = clamp(b.shape.position.x, (double)game_area.x, (double)game_area.w + game_area.x);
      b.velocity.x = -b.velocity.x;
      b.acceleration.x = -b.acceleration.x;
    }
    if (b.shape.position.y > game_area.h + game_area.y) {
      if (--player_lifes == 0) { break; }
      b = original_ball;
    }
    if (b.shape.position.y < game_area.y) {
      b.shape.position.y = game_area.y;
      b.velocity.y = -b.velocity.y;
      b.acceleration.y = -b.acceleration.y;
    }

    vec2 closest_normal = {};
    double closest_t = 1.0;
    block *closest_block = nullptr;
    for (auto&& block : blocks) {
      auto t = sweep_aabb(ball_prev, block.shape, delta_position * closest_t, closest_normal);
      if (t < closest_t) {
        closest_t = t;
        closest_block = &block;
      }
    }
    {
      auto t = sweep_aabb(ball_prev, player.shape, delta_position * closest_t, closest_normal);
      if (t < closest_t) {
        closest_t = t;
        closest_block = nullptr;
      }
    }
    if (closest_t != 1.0) {
      process_collision(b, delta_position, closest_t, closest_normal);
      if (closest_block != nullptr) { --(closest_block->durability); }
    }

    auto new_end = std::remove_if(std::begin(blocks), std::end(blocks), [](const block& b) { return b.durability == 0; });
    blocks.erase(new_end, std::end(blocks));

    SDL_SetRenderDrawColor(renderer.get(), 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer.get());
    SDL_SetRenderDrawColor(renderer.get(), 0x00, 0x00, 0xBB, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer.get(), &game_area);

    for (auto&& block : blocks) { block.draw(renderer.get()); }

    player.draw(renderer.get());
    b.draw(renderer.get());

    SDL_RenderPresent(renderer.get());
  }
  SDL_Quit();
  return 0;
}