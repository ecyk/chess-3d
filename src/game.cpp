#include "game.hpp"

#include <algorithm>

#define SHADER(filename) "resources/shaders/" filename
#define MODEL(filename) "resources/models/" filename

Game::Game(GLFWwindow* window) : renderer_{window, camera_} {
  glfwSetWindowUserPointer(window, this);

  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, mouse_move_callback);
  glfwSetScrollCallback(window, mouse_scroll_callback);

  glfwSetKeyCallback(window, key_callback);

  active_move_.is_completed = true;
}

void Game::run() {
#define CHECK(expr) \
  if (!(expr)) {    \
    return;         \
  }
  // clang-format off
  CHECK(renderer_.load_shader("basic", {SHADER("basic.vert"), SHADER("basic.frag")}))
  CHECK(renderer_.load_shader("lighting", {SHADER("lighting.vert"), SHADER("lighting.frag")}))
  CHECK(renderer_.load_shader("picking", {SHADER("basic.vert"), SHADER("picking.frag")}))
  CHECK(renderer_.load_shader("outlining", {SHADER("outlining.vert"), SHADER("outlining.frag")}))
  // clang-format on
  CHECK(renderer_.load_model("board", MODEL("board.gltf")))
  CHECK(renderer_.load_model("king", MODEL("king.gltf")))
  CHECK(renderer_.load_model("queen", MODEL("queen.gltf")))
  CHECK(renderer_.load_model("bishop", MODEL("bishop.gltf")))
  CHECK(renderer_.load_model("knight", MODEL("knight.gltf")))
  CHECK(renderer_.load_model("rook", MODEL("rook.gltf")))
  CHECK(renderer_.load_model("pawn", MODEL("pawn.gltf")))
  CHECK(renderer_.load_model("tile", MODEL("tile.gltf")))
#undef CHECK

  last_frame_ = static_cast<float>(glfwGetTime());
  while (glfwWindowShouldClose(renderer_.get_window()) != 1) {
    const auto current_frame = static_cast<float>(glfwGetTime());
    delta_time_ = current_frame - last_frame_;
    last_frame_ = current_frame;

    glfwPollEvents();

    process_input();
    update();

    draw_picking_texture();
    renderer_.begin_drawing(k_light_position);
    draw();
    renderer_.end_drawing();
  }
}

void Game::update() {
  if (game_over_ || ai_.is_thinking()) {
    return;
  }

  if (ai_.has_found_move()) {
    set_active_move(ai_.get_best_move());
  }

  if (active_move_.is_completed) {
    if (board_.is_in_checkmate()) {
      LOGF("GAME", "{} won!",
           board_.get_turn() == PieceColor::White ? "Black" : "White");
    }
    if (board_.is_in_draw()) {
      LOG("GAME", "Draw!");
    }
    if (board_.is_in_checkmate() || board_.is_in_draw()) {
      enable_cursor();
      game_over_ = true;
      return;
    }

    if (board_.get_turn() == ai_color_) {
      if (active_move_.is_undo) {
        undo();
        return;
      }

      ai_.think(board_);
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
      board_.make_move({active_move_.tile, active_move_.target, promotion});
    }
    active_move_.angle = 0.0F;
    active_move_.is_completed = true;
    if (!is_controlling_camera() && board_.get_turn() != ai_color_) {
      enable_cursor();
    }
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
  renderer_.draw_model("board", {.rotation = -90.0F, .scale = k_game_scale});
  draw_pieces();
  draw_selectable_tiles();
}

void Game::process_input() {
  GLFWwindow* window = renderer_.get_window();
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  }
}

void Game::draw_picking_texture() {
  renderer_.begin_picking(Renderer::PickingMode::Write);

  for (int tile = 0; tile < 64; tile++) {
    if (board_.is_empty(tile)) {
      continue;
    }

    const std::string_view model_name{get_model_name(board_.get_tile(tile))};
    renderer_.set_shader_uniform("color", tile);
    renderer_.draw_model(model_name, calculate_piece_transform(tile));
  }

  for (int i = 0; i < selectable_tiles_.size; i++) {
    const int target{selectable_tiles_.data[i].target};
    renderer_.set_shader_uniform("color", target);
    renderer_.draw_model("tile", calculate_tile_transform(target));
  }

  renderer_.end_picking(Renderer::PickingMode::Write);

  GLFWwindow* window = renderer_.get_window();
  if (!is_cursor_active()) {
    pixel_ = -1;
    return;
  }

  renderer_.begin_picking(Renderer::PickingMode::Read);

  int height{};
  glfwGetWindowSize(window, nullptr, &height);

  glm::ivec2 coord{mouse_last_position_};
  coord.y = height - coord.y;
  pixel_ = renderer_.read_pixel(coord);

  renderer_.end_picking(Renderer::PickingMode::Read);
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
    const std::string_view model_name{get_model_name(board_.get_tile(tile))};
    const bool is_black{board_.get_color(tile) == PieceColor::Black};

    if (!active_move_.is_completed) {
      if (active_move_.tile == tile) {
        transform.position = active_move_.position;
        renderer_.draw_model(model_name, transform, is_black);
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
      renderer_.begin_outlining();
    }

    renderer_.draw_model(model_name, transform, is_black);

    if (outline) {
      renderer_.end_outlining();
      renderer_.install_shader("outlining");
      glm::vec4 outline_color{k_picking_outline_color};
      if (!outline_hover && !outline_selected && !outline_attackable &&
          outline_king) {
        outline_color = k_check_outline_color;
      }
      renderer_.draw_model_outline(model_name, transform, 0.0125F,
                                   outline_color);
      renderer_.install_shader("lighting");
    }
  }
}

void Game::draw_selectable_tiles() {
  renderer_.install_shader("basic");

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  int hover{-1};
  if (is_valid_tile(pixel_)) {
    hover = pixel_;
  }

  for (int i = 0; i < selectable_tiles_.size; i++) {
    const int target{selectable_tiles_.data[i].target};
    if (!board_.is_empty(target)) {
      continue;
    }
    renderer_.draw_model("tile", calculate_tile_transform(target),
                         target == hover);
  }

  glDisable(GL_BLEND);
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

std::string_view Game::get_model_name(Piece piece) {
  const PieceType type{get_piece_type(piece)};

#define PIECE_TO_MODEL(piece, model_name) \
  if (type == (piece)) {                  \
    return model_name;                    \
  }

  PIECE_TO_MODEL(PieceType::King, "king");
  PIECE_TO_MODEL(PieceType::Queen, "queen");
  PIECE_TO_MODEL(PieceType::Bishop, "bishop");
  PIECE_TO_MODEL(PieceType::Knight, "knight");
  PIECE_TO_MODEL(PieceType::Rook, "rook");
  PIECE_TO_MODEL(PieceType::Pawn, "pawn");
#undef PIECE_TO_MODEL

  return {};
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
        if (game->board_.get_records().empty() && !game->ai_.is_thinking()) {
          game->ai_color_ = get_opposite_color(game->board_.get_color(tile));
          glm::vec3 position{k_camera_position};
          if (game->ai_color_ == PieceColor::White) {
            position.z = 40.0F;
            game->ai_.think(game->board_);
            game->disable_cursor();
          } else {
            position.z = -40.0F;
          }
          game->camera_.set_position(position);
          game->clear_selections();
        }
        if (game->board_.get_turn() != game->ai_color_) {
          game->board_.generate_legal_moves(game->selectable_tiles_, tile);
          if (game->selectable_tiles_.size != 0) {
            game->selected_tile_ = tile;
          }
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
  }
}

void Game::mouse_scroll_callback(GLFWwindow* window, double /*xoffset*/,
                                 double yoffset) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  game->camera_.process_mouse_scroll(static_cast<float>(yoffset));
}

void Game::key_callback(GLFWwindow* window, int key, int /*scancode*/,
                        int action, int /*mods*/) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (key == GLFW_KEY_F && action == GLFW_PRESS) {
    if (game->is_fullscreen) {
      glfwSetWindowMonitor(window, nullptr, game->window_old_pos_.x,
                           game->window_old_pos_.y, game->window_old_size_.x,
                           game->window_old_size_.y, 0);
      game->is_fullscreen = false;
    } else {
      glfwGetWindowPos(window, &game->window_old_pos_.x,
                       &game->window_old_pos_.y);
      glfwGetWindowSize(window, &game->window_old_size_.x,
                        &game->window_old_size_.y);
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, 0);
      game->is_fullscreen = true;
    }
    return;
  }
  if (!game->active_move_.is_completed ||
      game->board_.get_turn() == game->ai_color_) {
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
