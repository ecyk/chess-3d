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
  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
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
  const Transform board_transform{{}, 0.0F, k_game_scale};
  const Transform white_king_transform{
      {14.5F, 1.5F, 14.5F}, 0.0F, k_game_scale};
  const Transform black_king_transform{
      {-14.5F, 1.5F, -14.5F}, 0.0F, k_game_scale};

  Model* board_model = models_[to_underlying(ModelType::Board)];
  Model* king_model = models_[to_underlying(ModelType::King)];

  renderer_.bind_shader(shader_);

  if (update_picking_texture_) {
    renderer_.picking_texture_writing_begin();

    renderer_.bind_picking_texture_id(0);
    renderer_.draw_model(board_transform, board_model);

    renderer_.bind_picking_texture_id(1);
    renderer_.draw_model(white_king_transform, king_model);

    renderer_.bind_picking_texture_id(2);
    renderer_.draw_model(black_king_transform, king_model);

    renderer_.picking_texture_writing_end();

    update_picking_texture_ = false;
  }

  renderer_.draw_model(board_transform, board_model,
                       board_model->mesh.default_);

  if (selected_ == 1) {
    Renderer::stencil_writing_begin();
  }

  renderer_.draw_model(white_king_transform, king_model,
                       king_model->mesh.white);

  Renderer::stencil_writing_end();

  if (selected_ == 2) {
    Renderer::stencil_writing_begin();
  }

  renderer_.draw_model(black_king_transform, king_model,
                       king_model->mesh.black);

  Renderer::stencil_writing_end();

  renderer_.outline_drawing_begin(0.0125F, {0.0F, 1.0F, 0.0F, 1.0F});

  switch (selected_) {
    case 1:
      renderer_.draw_model(white_king_transform, king_model);
      break;
    case 2:
      renderer_.draw_model(black_king_transform, king_model);
      break;
  }

  renderer_.outline_drawing_end();
}

void Game::process_input() {
  GLFWwindow* window = renderer_.get_window();
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  }
}

void Game::mouse_button_callback(GLFWwindow* window, int button, int action,
                                 int /*mods*/) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    int id = game->renderer_.get_picking_texture_id(game->mouse_last_position_);
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

  game->selected_ =
      game->renderer_.get_picking_texture_id(game->mouse_last_position_);
}

void Game::mouse_scroll_callback(GLFWwindow* window, double /*xoffset*/,
                                 double yoffset) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  game->camera_.process_mouse_scroll(static_cast<float>(yoffset));
  game->update_picking_texture_ = true;
}
