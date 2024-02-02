#include "game.hpp"

#include <GLFW/glfw3.h>

#include <random>

#define SHADER(filename) "resources/shaders/" filename
#define TEXTURE(filename) "resources/textures/" filename
#define MODEL(filename) "resources/models/" filename

Game::Game(GLFWwindow* window) : renderer_{window} {
  glfwSetWindowUserPointer(window, this);

  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, mouse_move_callback);
  glfwSetScrollCallback(window, mouse_scroll_callback);

  glfwSetKeyCallback(window, key_callback);

  active_move_.is_completed = true;
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
  CHECK(picking_texture_, renderer_.create_framebuffer(k_window_size))
#undef CHECK

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
  picking_texture_ = renderer_.create_framebuffer(size);
  update_picking_texture_ = true;
  LOGF("GAME", "Resized picking texture to {} {}", size.x, size.y);
}

void Game::update() {
  if (game_over_) {
    return;
  }

  if (active_move_.is_completed) {
    const bool checkmate{board_.is_in_checkmate()};
    const bool draw{board_.is_in_draw()};
    if (checkmate) {
      LOGF("GAME", "{} won!",
           board_.get_turn() == PieceColor::White ? "Black" : "White");
    }
    if (draw) {
      LOGF("GAME", "Draw!");
    }
    if (checkmate || draw) {
      enable_cursor();
      game_over_ = true;
      return;
    }

    if (board_.get_turn() == ai_color_) {
      if (active_move_.is_undo) {
        undo();
        return;
      }
      set_active_move(ai_.think());
    }
    return;
  }

  if (active_move_.angle <= 0.0F) {
    if (active_move_.is_undo) {
      board_.undo();
      if (board_.get_records().empty()) {
        active_move_ = {};
        ai_color_ = PieceColor::None;
      }
    } else {
      PieceType promotion{};
      if (board_.get_type(active_move_.tile) == PieceType::Pawn &&
          (active_move_.target < 8 || active_move_.target > 55)) {
        promotion = PieceType::Queen;
      }
      board_.move({active_move_.tile, active_move_.target, promotion});
    }
    active_move_.angle = 0.0F;
    active_move_.is_completed = true;
    if (!is_controlling_camera() && board_.get_turn() != ai_color_) {
      enable_cursor();
    }
    update_picking_texture_ = true;
    return;
  }

  const glm::vec3 tile{calculate_tile_position(active_move_.tile)};
  const glm::vec3 target{calculate_tile_position(active_move_.target)};

  const glm::vec3 center{(tile + target) / 2.0F};
  const float radius{glm::length(target - tile) / 2.0F};
  const glm::vec3 horizontal{glm::normalize(target - tile) *
                             glm::cos(glm::radians(active_move_.angle))};

  active_move_.position =
      center + glm::vec3{horizontal.x,
                         glm::sin(glm::radians(active_move_.angle)),
                         horizontal.z} *
                   radius;

  active_move_.angle -= 270.0F * delta_time_;
  active_move_.angle = glm::clamp(active_move_.angle, 0.0F, 180.0F);
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

    for (int tile = 0; tile < 64; tile++) {
      const Piece piece{board_.get_tile(tile)};
      if (get_piece_type(piece) == PieceType::None) {
        continue;
      }

      Model* model = get_model(piece);
      renderer_.set_shader_uniform(picking_, "color", tile);
      renderer_.draw_model(calculate_piece_transform(tile), model, nullptr);
    }

    Model* model = get_model(ModelType::SelectableTile);
    for (int i = 0; i < selectable_tiles_.size; i++) {
      const int target{selectable_tiles_.data[i].target};
      renderer_.set_shader_uniform(picking_, "color", target);
      renderer_.draw_model(calculate_tile_transform(target), model, nullptr);
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
  bool is_in_check{};
  if (board_.is_in_checkmate() || board_.is_in_check()) {
    is_in_check = true;
  }

  for (int tile = 0; tile < 64; tile++) {
    if (board_.is_empty(tile)) {
      continue;
    }

    Transform transform{calculate_piece_transform(tile)};
    Model* model = get_model(board_.get_tile(tile));
    Material* material = board_.get_color(tile) == PieceColor::White
                             ? model->mesh.white
                             : model->mesh.black;

    if (!active_move_.is_completed) {
      if (active_move_.tile == tile) {
        transform.position = active_move_.position;
        renderer_.draw_model(transform, model, material);
        continue;
      }

      if (active_move_.target == tile && active_move_.angle <= 45.0F) {
        continue;
      }
    }

    const bool outline_hover{tile == pixel_};
    const bool outline_selected{tile == selected_tile_};
    const bool outline_king{
        is_in_check &&
        board_.is_piece(tile, board_.get_turn(), PieceType::King)};
    const bool outline_attackable{is_selectable_tile(tile)};
    const bool outline{outline_hover || outline_selected ||
                       outline_attackable || outline_king};
    if (outline) {
      // renderer_.clear_stencil();
      renderer_.begin_stencil_writing();
    }

    renderer_.draw_model(transform, model, material);

    if (outline) {
      renderer_.end_stencil_writing();

      renderer_.bind_shader(outlining_);
      glm::vec4 outline_color{k_picking_outline_color};
      if (!outline_hover && !outline_selected && !outline_attackable &&
          outline_king) {
        outline_color = k_check_outline_color;
      }
      renderer_.draw_model_outline(transform, model, 0.0125F, outline_color);
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

  int hover{-1};
  if (is_valid_tile(pixel_)) {
    hover = pixel_;
  }

  for (int i = 0; i < selectable_tiles_.size; i++) {
    Material* material{&selectable_tile_};
    const int target{selectable_tiles_.data[i].target};
    if (target == hover) {
      material = &selectable_tile_hover_;
    }
    renderer_.draw_model(calculate_tile_transform(target), model, material);
  }

  glDisable(GL_BLEND);
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

Model* Game::get_model(Piece piece) const {
  const PieceType type{get_piece_type(piece)};

#define PIECE_TO_MODEL(piece, model) \
  if (type == (piece)) {             \
    return get_model(model);         \
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

Model* Game::get_model(ModelType type) const {
  return models_[to_underlying(type)];
}

bool Game::is_selectable_tile(int tile) const {
  const auto begin{selectable_tiles_.data.begin()};
  const auto end{begin + selectable_tiles_.size};
  return std::find_if(begin, end, [tile](const Move& move) {
           return move.target == tile;
         }) != end;
}

void Game::set_active_move(const Move& move, bool is_undo) {
  active_move_ = {};
  active_move_.tile = is_undo ? move.target : move.tile;
  active_move_.target = is_undo ? move.tile : move.target;
  active_move_.position = calculate_tile_position(active_move_.tile);
  active_move_.is_undo = is_undo;
  clear_selections();
  disable_cursor();
}

void Game::undo() {
  const auto& records{board_.get_records()};
  if (!records.empty()) {
    set_active_move(records.back().move, true);
    game_over_ = false;
  }
}

Transform Game::calculate_piece_transform(int tile) {
  const Piece piece{board_.get_tile(tile)};
  return calculate_tile_transform(
      tile, get_piece_color(piece) == PieceColor::White ? -180.0F : 0.0F);
}

glm::vec3 Game::calculate_tile_position(int tile) {
  return (glm::vec3{-2.03F, 0.174F, -2.03F} +
          glm::vec3{7 - get_tile_column(tile), 0, get_tile_row(tile)} * 0.58F) *
         k_game_scale;
}

Transform Game::calculate_tile_transform(int tile, float rotation) {
  return {calculate_tile_position(tile), rotation, k_game_scale};
}

void Game::mouse_button_callback(GLFWwindow* window, int button, int action,
                                 int /*mods*/) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    if (is_valid_tile(game->pixel_)) {
      const int tile{game->pixel_};
      const Piece piece{game->board_.get_tile(tile)};
      if (game->selected_tile_ != -1 && game->is_selectable_tile(tile)) {
        game->set_active_move({game->selected_tile_, tile});
      } else if (get_piece_type(piece) != PieceType::None) {
        if (game->board_.get_records().empty()) {
          game->ai_color_ = get_opposite_color(game->board_.get_color(tile));
        }
        game->clear_selections();
        game->board_.generate_legal_moves(game->selectable_tiles_, tile);
        if (game->selectable_tiles_.size != 0) {
          game->selected_tile_ = tile;
        }
      }
    } else {
      game->clear_selections();
    }
  }

  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
    game->enable_cursor();
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

    game->update_picking_texture_ = true;
  }
}

void Game::mouse_scroll_callback(GLFWwindow* window, double /*xoffset*/,
                                 double yoffset) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  game->camera_.process_mouse_scroll(static_cast<float>(yoffset));
  game->update_picking_texture_ = true;
}

void Game::key_callback(GLFWwindow* window, int key, int /*scancode*/,
                        int action, int /*mods*/) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (!game->active_move_.is_completed) {
    return;
  }
  if (key == GLFW_KEY_U && action == GLFW_PRESS) {
    game->undo();
  } else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    game->board_.load_fen();
    game->ai_color_ = PieceColor::None;
    game->game_over_ = false;
  }
  game->clear_selections();
}
