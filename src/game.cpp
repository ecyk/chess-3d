#include "game.hpp"

#include <GLFW/glfw3.h>

#define SHADER(filename) "resources/shaders/" filename
#define MODEL(filename) "resources/models/" filename

Game::Game(GLFWwindow* window) : renderer_{window} {
  glfwSetWindowUserPointer(window, this);

  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, mouse_move_callback);
  glfwSetScrollCallback(window, mouse_scroll_callback);
  glfwSetKeyCallback(window, key_callback);

  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Game::run() {
#define LOAD_SHADER(shader, vert, frag)                    \
  if (!((shader) = renderer_.create_shader(vert, frag))) { \
    return;                                                \
  }

  LOAD_SHADER(base_shader_, SHADER("base.vert"), SHADER("base.frag"));
  LOAD_SHADER(picking_shader_, SHADER("picking.vert"), SHADER("picking.frag"));
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

void Game::update() {
  int width{};
  int height{};
  glfwGetWindowSize(renderer_.get_window(), &width, &height);

  uint32_t id = renderer_.get_picking_texture_id(
      {mouse_last_position_.x,
       static_cast<float>(height) - mouse_last_position_.y});
  LOGF("GAME", "id {}", id);
}

void Game::draw() {
  const Transform transform1{{}, 0.0F, k_game_scale};
  const Transform transform2{{14.5F, 1.1F, 14.5F}, 0.0F, k_game_scale};

  Model* model = models_[current_model_];
  Material* material = model->mesh.default_;
  if (current_model_ != to_underlying(ModelType::Board)) {
    material = (static_cast<int>(time_passed_) % 2) != 0 ? model->mesh.white
                                                         : model->mesh.black;
  }

  Model* model2 = models_[to_underlying(ModelType::King)];

  renderer_.bind_shader(picking_shader_);
  renderer_.enable_picking_texture_writing();

  renderer_.set_picking_texture_id(2);
  renderer_.draw_model(transform1, model, nullptr);

  renderer_.set_picking_texture_id(3);
  renderer_.draw_model(transform2, model2, nullptr);

  renderer_.disable_picking_texture_writing();

  renderer_.bind_shader(base_shader_);

  renderer_.draw_model(transform1, model, material);

  renderer_.draw_model(transform2, model2, model2->mesh.white);
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
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    if (++game->current_model_ == to_underlying(ModelType::Count)) {
      game->current_model_ = 0;
    }
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

  const int mouse_left{glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)};
  if (mouse_left == GLFW_PRESS) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    game->camera_.process_mouse_movement(offset_x, offset_y);
  } else if (mouse_left == GLFW_RELEASE) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }
}

void Game::mouse_scroll_callback(GLFWwindow* window, double /*xoffset*/,
                                 double yoffset) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  game->camera_.process_mouse_scroll(static_cast<float>(yoffset));
}

void Game::key_callback(GLFWwindow* window, int key, int /*scancode*/,
                        int action, int /*mods*/) {}
