#include "game.hpp"

#include <GLFW/glfw3.h>

Game::Game(GLFWwindow* window) : renderer_{window} {
  glfwSetWindowUserPointer(window, this);

  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetScrollCallback(window, scroll_callback);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Game::run() {
  last_frame_ = static_cast<float>(glfwGetTime());
  while (glfwWindowShouldClose(renderer_.get_window()) != 1) {
    const auto current_frame = static_cast<float>(glfwGetTime());
    delta_time_ = current_frame - last_frame_;
    last_frame_ = current_frame;

    glfwPollEvents();

    process_input();
    update();

    renderer_.begin_drawing();
    draw();
    renderer_.end_drawing();
  }
}

void Game::update() {}

void Game::draw() {}

void Game::process_input() {
  GLFWwindow* window = renderer_.get_window();
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  }
}

void Game::mouse_callback(GLFWwindow* window, double xpos, double ypos) {}

void Game::key_callback(GLFWwindow* window, int key, int /*scancode*/,
                        int action, int /*mods*/) {}

void Game::scroll_callback(GLFWwindow* window, double /*xoffset*/,
                           double yoffset) {}
