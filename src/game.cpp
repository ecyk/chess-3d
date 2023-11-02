#include "game.hpp"

#include <GLFW/glfw3.h>

#define SHADER(filename) "resources/shaders/" filename
#define TEXTURE(filename) "resources/textures/" filename
#define MODEL(filename) "resources/models/" filename

Game::Game(GLFWwindow* window) : renderer_{window} {
  glfwSetWindowUserPointer(window, this);

  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, mouse_move_callback);
  glfwSetScrollCallback(window, mouse_scroll_callback);
}

void Game::run() {
#define CHECK(assignee, expr)   \
  if (!((assignee) = (expr))) { \
    return;                     \
  }

#define LOAD_SHADER(shader, vert, frag) \
  CHECK(shader, renderer_.create_shader(vert, frag))

  LOAD_SHADER(shader_, SHADER("shader.vert"), SHADER("shader.frag"));
  LOAD_SHADER(lighting_, SHADER("lighting.vert"), SHADER("lighting.frag"));
  LOAD_SHADER(picking_, SHADER("shader.vert"), SHADER("picking.frag"));
  LOAD_SHADER(outlining_, SHADER("outlining.vert"), SHADER("outlining.frag"));
#undef LOAD_SHADER

#define LOAD_MODEL(model, path) \
  CHECK(models_[to_underlying(model)], renderer_.create_model(path))

  LOAD_MODEL(ModelType::Board, MODEL("board.gltf"))
  LOAD_MODEL(ModelType::King, MODEL("king.gltf"))
  LOAD_MODEL(ModelType::Queen, MODEL("queen.gltf"))
  LOAD_MODEL(ModelType::Bishop, MODEL("bishop.gltf"))
  LOAD_MODEL(ModelType::Knight, MODEL("knight.gltf"))
  LOAD_MODEL(ModelType::Rook, MODEL("rook.gltf"))
  LOAD_MODEL(ModelType::Pawn, MODEL("pawn.gltf"))
  LOAD_MODEL(ModelType::SelectableTile, MODEL("selectable_tile.gltf"))
#undef LOAD_MODEL

  CHECK(selectable_tile_.base_color,
        renderer_.create_texture(TEXTURE("selectable_tile.png")))
  CHECK(selectable_tile_hover_.base_color,
        renderer_.create_texture(TEXTURE("selectable_tile_hover.png")))
  CHECK(picking_texture_, renderer_.create_framebuffer(k_window_size + 1.0F))
#undef CHECK

  for (int y = 2; y < 6; ++y) {
    for (int x = 0; x < 8; ++x) {
      selectable_tiles_.emplace_back(glm::ivec2{x, y}, Piece{});
    }
  }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  last_frame_ = static_cast<float>(glfwGetTime());
  while (glfwWindowShouldClose(renderer_.get_window()) != 1) {
    const auto current_frame = static_cast<float>(glfwGetTime());
    delta_time_ = current_frame - last_frame_;
    last_frame_ = current_frame;

    time_passed_ += delta_time_;

    glfwPollEvents();

    process_input();
    update();

    renderer_.begin_drawing(camera_);
    draw();
    renderer_.end_drawing();
  }
}

void Game::resize_picking_texture(const glm::vec2& size) {
  Renderer::destroy_framebuffer(picking_texture_);
  picking_texture_ = renderer_.create_framebuffer(size + 1.0F);
  update_picking_texture_ = true;
  LOGF("GAME", "Resized picking texture to {} {}", size.x, size.y);
}

void Game::update() {
  if (!moves_.empty()) {
    Move& move = moves_.back();
    if (move.completed) {
      return;
    }

    if (move.angle <= 0.0F) {
      board_.move_to(move.from.piece, move.to.coord);
      if (!is_controlling_camera()) {
        enable_cursor();
      }
      update_picking_texture_ = true;
      move.angle = 0.0F;
      move.completed = true;
      return;
    }

    const glm::vec3 current{calculate_tile_transform(move.from).position};
    const glm::vec3 target{calculate_tile_transform(move.to).position};

    const glm::vec3 center{(current + target) / 2.0F};
    const float radius{glm::length(target - current) / 2.0F};
    const glm::vec3 horizontal{glm::normalize(target - current) *
                               glm::cos(glm::radians(move.angle))};

    move.position =
        center + glm::vec3{horizontal.x, glm::sin(glm::radians(move.angle)),
                           horizontal.z} *
                     radius;

    move.angle -= 270.0F * delta_time_;
    move.angle = glm::clamp(move.angle, 0.0F, 180.0F);
  }
}

void Game::draw() {
  update_picking_texture();

  draw_board();
  draw_pieces();
  draw_selectable_tiles();
}

void Game::process_input() {
  GLFWwindow* window = renderer_.get_window();
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  }
}

void Game::update_picking_texture() {
  if (update_picking_texture_) {
    renderer_.bind_shader(picking_);
    renderer_.bind_framebuffer(picking_texture_, GL_DRAW_FRAMEBUFFER);
    Renderer::clear_framebuffer();

    for (const auto& tile : board_.get_active_tiles()) {
      Model* model = get_model(tile.piece);
      renderer_.set_shader_uniform(picking_, "color",
                                   static_cast<int>(to_underlying(tile.piece)));
      renderer_.draw_model(calculate_tile_transform(tile), model, nullptr);
    }

    for (size_t i = 0; i < selectable_tiles_.size(); i++) {
      const Tile& tile = selectable_tiles_[i];
      renderer_.set_shader_uniform(picking_, "color",
                                   static_cast<int>(i << 3U));
      renderer_.draw_model(calculate_tile_transform(tile),
                           get_model(ModelType::SelectableTile), nullptr);
    }

    renderer_.unbind_framebuffer(GL_DRAW_FRAMEBUFFER);

    glFlush();
    glFinish();

    update_picking_texture_ = false;
  }

  GLFWwindow* window = renderer_.get_window();
  if (!is_cursor_active()) {
    pixel_ = -1;
    return;
  }

  renderer_.bind_framebuffer(picking_texture_, GL_READ_FRAMEBUFFER);

  int height{};
  glfwGetWindowSize(window, nullptr, &height);

  glm::ivec2 coord{mouse_last_position_};
  coord.y = height - coord.y;
  pixel_ = Renderer::read_pixel(coord);

  renderer_.unbind_framebuffer(GL_READ_FRAMEBUFFER);
}

void Game::draw_board() {
  Transform transform{};
  transform.scale = k_game_scale;
  transform.rotation = -90.0F;

  Model* model = get_model(ModelType::Board);

  renderer_.bind_shader(lighting_);
  renderer_.set_shader_uniform(lighting_, "light_pos", k_light_position);
  renderer_.set_shader_uniform(lighting_, "view_pos", camera_.get_position());
  renderer_.draw_model(transform, model, model->mesh.default_);
}

void Game::draw_pieces() {
  for (const auto& tile : board_.get_active_tiles()) {
    Transform transform{calculate_tile_transform(tile)};
    Model* model = get_model(tile.piece);
    Material* material = is_piece_color(tile.piece, PieceColor::Black)
                             ? model->mesh.black
                             : model->mesh.white;

    if (!moves_.empty()) {
      const Move& move = moves_.back();
      if (!move.completed && move.from.coord == tile.coord &&
          move.from.piece == tile.piece) {
        transform.position = move.position;
        renderer_.draw_model(transform, model, material);
        continue;
      }
    }

    const bool outline{pixel_ == static_cast<int>(tile.piece) ||
                       selected_piece_ == tile.piece};
    if (outline) {
      // renderer_.clear_stencil();
      renderer_.begin_stencil_writing();
    }

    renderer_.draw_model(transform, model, material);

    if (outline) {
      renderer_.end_stencil_writing();

      renderer_.bind_shader(outlining_);
      renderer_.draw_model_outline(transform, model, 0.0125F, k_outline_color);
      renderer_.bind_shader(lighting_);
      renderer_.set_shader_uniform(lighting_, "light_pos", k_light_position);
      renderer_.set_shader_uniform(lighting_, "view_pos",
                                   camera_.get_position());
    }
  }
}

void Game::draw_selectable_tiles() {
  renderer_.bind_shader(shader_);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  Model* model = get_model(ModelType::SelectableTile);

  uint32_t hovered_index{};
  if (pixel_is_selectable_tile()) {
    hovered_index = pixel_as_selectable_tile();

    for (uint32_t i = 0; i < hovered_index; ++i) {
      const auto& tile = selectable_tiles_[i];
      renderer_.draw_model(calculate_tile_transform(tile), model,
                           &selectable_tile_);
    }

    const auto& tile = selectable_tiles_[hovered_index];
    renderer_.draw_model(calculate_tile_transform(tile), model,
                         &selectable_tile_hover_);

    hovered_index += 1;
  }

  for (uint32_t i = hovered_index; i < selectable_tiles_.size(); ++i) {
    const auto& tile = selectable_tiles_[i];
    renderer_.draw_model(calculate_tile_transform(tile), model,
                         &selectable_tile_);
  }

  glDisable(GL_BLEND);
}

bool Game::pixel_is_piece() const {
  return !(pixel_ < 0) &&
         !is_piece_type(static_cast<Piece>(pixel_), PieceType::None);
}

Piece Game::pixel_as_piece() const {
  assert(!(pixel_ < 0));
  return static_cast<Piece>(pixel_);
}

bool Game::pixel_is_selectable_tile() const {
  return !(pixel_ < 0) &&
         is_piece_type(static_cast<Piece>(pixel_), PieceType::None);
}

uint32_t Game::pixel_as_selectable_tile() const {
  assert(!(pixel_ < 0));
  return static_cast<uint32_t>(pixel_) >> 3U;
}

bool Game::is_controlling_camera() const {
  return glfwGetMouseButton(renderer_.get_window(), GLFW_MOUSE_BUTTON_MIDDLE) ==
         GLFW_PRESS;
}

bool Game::is_cursor_active() const {
  return glfwGetInputMode(renderer_.get_window(), GLFW_CURSOR) ==
         GLFW_CURSOR_NORMAL;
}

void Game::enable_cursor() {
  glfwSetInputMode(renderer_.get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  mouse_last_position_ = mouse_last_position_real_;
}

void Game::disable_cursor() {
  glfwSetInputMode(renderer_.get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  mouse_last_position_real_ = mouse_last_position_;
}

Model* Game::get_model(ModelType type) const {
  return models_[to_underlying(type)];
}

Model* Game::get_model(Piece piece) const {
#define PIECE_TO_MODEL(piece_type, model_type) \
  if (is_piece_type(piece, piece_type)) {      \
    return get_model(model_type);              \
  }

  PIECE_TO_MODEL(PieceType::King, ModelType::King);
  PIECE_TO_MODEL(PieceType::Queen, ModelType::Queen);
  PIECE_TO_MODEL(PieceType::Bishop, ModelType::Bishop);
  PIECE_TO_MODEL(PieceType::Knight, ModelType::Knight);
  PIECE_TO_MODEL(PieceType::Rook, ModelType::Rook);
  PIECE_TO_MODEL(PieceType::Pawn, ModelType::Pawn);
#undef PIECE_TO_MODEL

  return nullptr;
}

void Game::move_piece_to_tile(Piece piece, const Tile& tile) {
  const Tile from{board_.get_coord(piece), piece};
  moves_.push_back(Move{from, tile, calculate_tile_transform(from).position});

  update_picking_texture_ = true;
  selected_piece_ = {};

  disable_cursor();
}

Transform Game::calculate_tile_transform(const Tile& tile) {
  return {(glm::vec3{-2.03F, 0.174F, 2.03F} +
           glm::vec3{tile.coord.x, 0, tile.coord.y - 7} * 0.58F) *
              k_game_scale,
          is_piece_color(tile.piece, PieceColor::Black) ? 0.0F : -180.0F,
          k_game_scale};
}

void Game::mouse_button_callback(GLFWwindow* window, int button, int action,
                                 int /*mods*/) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    const bool has_selected_piece{
        !is_piece_type(game->selected_piece_, PieceType::None)};
    if (game->pixel_is_piece()) {
      game->selected_piece_ = game->pixel_as_piece();
    } else if (game->pixel_is_selectable_tile() && has_selected_piece) {
      auto& tile{game->selectable_tiles_[game->pixel_as_selectable_tile()]};
      game->move_piece_to_tile(game->selected_piece_, tile);
      std::swap(game->selectable_tiles_.back(), tile);
      game->selectable_tiles_.pop_back();
    } else {
      game->selected_piece_ = {};
    }
  }

  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
    game->enable_cursor();
    game->update_picking_texture_ = true;
  }
}

void Game::mouse_move_callback(GLFWwindow* window, double xpos, double ypos) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (game->first_mouse_input_) {
    game->mouse_last_position_.x = static_cast<float>(xpos);
    game->mouse_last_position_.y = static_cast<float>(ypos);
    game->first_mouse_input_ = false;
  }

  const float offset_x{static_cast<float>(xpos) - game->mouse_last_position_.x};
  const float offset_y{game->mouse_last_position_.y - static_cast<float>(ypos)};

  game->mouse_last_position_.x = static_cast<float>(xpos);
  game->mouse_last_position_.y = static_cast<float>(ypos);

  if (game->is_controlling_camera()) {
    game->camera_.process_mouse_movement(offset_x, offset_y);

    if (game->is_cursor_active()) {
      game->disable_cursor();
    }
  }
}

void Game::mouse_scroll_callback(GLFWwindow* window, double /*xoffset*/,
                                 double yoffset) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  game->camera_.process_mouse_scroll(static_cast<float>(yoffset));
  game->update_picking_texture_ = true;
}
