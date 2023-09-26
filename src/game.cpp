#include "game.hpp"

#include <GLFW/glfw3.h>

Game::Game(GLFWwindow* window) : renderer_{window} {
  glfwSetWindowUserPointer(window, this);

  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, mouse_move_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetScrollCallback(window, scroll_callback);

  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Game::run() {
  shader_ = renderer_.create_shader("resources/shaders/vertex.vert",
                                    "resources/shaders/fragment.frag");

  if (shader_ == nullptr) {
    return;
  }

#define LOAD_MODEL(model, path)                                          \
  if (!(models_[to_underlying(model)] = renderer_.create_model(path))) { \
    return;                                                              \
  }

  LOAD_MODEL(ModelType::Board, "resources/models/board.gltf")
  LOAD_MODEL(ModelType::King, "resources/models/king.gltf")
  LOAD_MODEL(ModelType::Queen, "resources/models/queen.gltf")
  LOAD_MODEL(ModelType::Bishop, "resources/models/bishop.gltf")
  LOAD_MODEL(ModelType::Knight, "resources/models/knight.gltf")
  LOAD_MODEL(ModelType::Rook, "resources/models/rook.gltf")
  LOAD_MODEL(ModelType::Pawn, "resources/models/pawn.gltf")
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

    renderer_.begin_drawing();
    draw();
    renderer_.end_drawing();
  }
}

void Game::update() {}

void Game::draw() {
  static Transform transform;
  transform.rotation += 45.0F * delta_time_;

  Model* model = models_[current_model_];
  Material* material = model->mesh.default_;
  if (current_model_ != to_underlying(ModelType::Board)) {
    material = (static_cast<int>(time_passed_) % 2) != 0 ? model->mesh.white
                                                         : model->mesh.black;
  }

  renderer_.bind_shader(shader_);
  renderer_.draw_model(transform, model, material);
}

void Game::process_input() {
  GLFWwindow* window = renderer_.get_window();
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  }
}

void Game::mouse_button_callback(GLFWwindow* window, int button, int action,
                                 int mods) {
  auto* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    if (++game->current_model_ == to_underlying(ModelType::Count)) {
      game->current_model_ = 0;
    }
  }
}

void Game::mouse_move_callback(GLFWwindow* window, double xpos, double ypos) {}

void Game::key_callback(GLFWwindow* window, int key, int /*scancode*/,
                        int action, int /*mods*/) {}

void Game::scroll_callback(GLFWwindow* window, double /*xoffset*/,
                           double yoffset) {}
