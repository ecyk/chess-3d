#include "game.hpp"

#include <GLFW/glfw3.h>

#define SHADER(filename) "resources/shaders/" filename
#define MODEL(filename) "resources/models/" filename

Game::Game(GLFWwindow* window) : renderer_{window} {
  glfwSetWindowUserPointer(window, this);

  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, mouse_move_callback);
  glfwSetScrollCallback(window, mouse_scroll_callback);
}

void Game::run() {
#define LOAD_SHADER(shader, vert, frag)                    \
  if (!((shader) = renderer_.create_shader(vert, frag))) { \
    return;                                                \
  }

  LOAD_SHADER(shader_, SHADER("shader.vert"), SHADER("shader.frag"));
#undef LOAD_SHADER

#define LOAD_MODEL(model, path)                                          \
  if (!(models_[to_underlying(model)] = renderer_.create_model(path))) { \
    return;                                                              \
  }

  LOAD_MODEL(ModelType::Board, MODEL("board.gltf"))
  LOAD_MODEL(ModelType::King, MODEL("king.gltf"))
  LOAD_MODEL(ModelType::Queen, MODEL("queen.gltf"))
  LOAD_MODEL(ModelType::Bishop, MODEL("bishop.gltf"))
  LOAD_MODEL(ModelType::Knight, MODEL("knight.gltf"))
  LOAD_MODEL(ModelType::Rook, MODEL("rook.gltf"))
  LOAD_MODEL(ModelType::Pawn, MODEL("pawn.gltf"))
#undef LOAD_MODEL

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

void Game::update() {}

void Game::draw() {
  renderer_.bind_shader(shader_);
  renderer_.set_shader_uniform(shader_, "outline_thickness", 0.0F);
  renderer_.set_shader_uniform(shader_, "solid_color", glm::vec4{});
  renderer_.set_shader_uniform(shader_, "blend_factor", 1.0F);

  draw_picking_texture();

  Transform transform{};
  transform.rotation = -90.0F;
  transform.scale = k_game_scale;
  Model* model = get_model(ModelType::Board);
  renderer_.draw_model(transform, model, model->mesh.default_);

  Tile selected_tile;

  for (const auto& tile : board_.get_active_tiles()) {
    if (tile.piece == selected_piece_) {
      selected_tile = tile;
      continue;
    }

    draw_piece(tile, true);
  }

  if (!is_piece_type(selected_tile.piece, PieceType::None)) {
    Renderer::begin_stencil_writing();

    draw_piece(selected_tile, true);

    Renderer::end_stencil_writing();

    renderer_.begin_outline_drawing(0.0125F, {0.0F, 1.0F, 0.0F, 1.0F});

    draw_piece(selected_tile, false);

    renderer_.end_outline_drawing();
  }
}

void Game::process_input() {
  GLFWwindow* window = renderer_.get_window();
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  }
}

void Game::draw_picking_texture() {
  if (update_picking_texture_) {
    renderer_.begin_picking_texture_writing();

    for (const auto& tile : board_.get_active_tiles()) {
      renderer_.bind_picking_texture_id(to_underlying(tile.piece));
      draw_piece(tile, false);
    }

    renderer_.end_picking_texture_writing();
    update_picking_texture_ = false;
  }
}

void Game::draw_piece(const Tile& tile, bool use_material) {
  const Transform transform{
      calculate_piece_transform(tile.x, tile.y, tile.piece)};
  Model* model = get_model(tile.piece);
  Material* material = is_piece_color(tile.piece, PieceColor::Black)
                           ? model->mesh.black
                           : model->mesh.white;
  renderer_.draw_model(transform, model, use_material ? material : nullptr);
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

Transform Game::calculate_piece_transform(int x, int y, Piece piece) {
  return {(glm::vec3{-2.03F, 0.174F, 2.03F} + glm::vec3{x, 0, y - 7} * 0.58F) *
              k_game_scale,
          is_piece_color(piece, PieceColor::Black) ? -180.0F : 0.0F,
          k_game_scale};
}

void Game::mouse_button_callback(GLFWwindow* window, int button, int action,
                                 int /*mods*/) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    int id{game->renderer_.get_picking_texture_id(game->mouse_last_position_)};
    LOGF("GAME", "id {}", id);
  }

  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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

  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    game->camera_.process_mouse_movement(offset_x, offset_y);
  }

  const int id{
      game->renderer_.get_picking_texture_id(game->mouse_last_position_)};
  game->selected_piece_ = (id != -1) ? static_cast<Piece>(id) : Piece{};
}

void Game::mouse_scroll_callback(GLFWwindow* window, double /*xoffset*/,
                                 double yoffset) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  game->camera_.process_mouse_scroll(static_cast<float>(yoffset));
  game->update_picking_texture_ = true;
}
