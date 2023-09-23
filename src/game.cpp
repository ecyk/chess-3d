#include "game.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

Game::Game(GLFWwindow* window) : window_{window} {
  glfwSetWindowUserPointer(window, this);

  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetScrollCallback(window, scroll_callback);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Game::run() {
  last_frame_ = static_cast<float>(glfwGetTime());
  while (glfwWindowShouldClose(window_) != 1) {
    const auto current_frame = static_cast<float>(glfwGetTime());
    delta_time_ = current_frame - last_frame_;
    last_frame_ = current_frame;

    glfwPollEvents();

    process_input();
    update();

    glClearColor(0.25F, 0.25F, 0.25F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    draw();

    glfwSwapBuffers(window_);
  }
}

void Game::update() {}

void Game::draw() {}

void Game::process_input() {
  if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window_, 1);
  }
}

void Game::mouse_callback(GLFWwindow* window, double xpos, double ypos) {}

void Game::key_callback(GLFWwindow* window, int key, int /*scancode*/,
                        int action, int /*mods*/) {}

void Game::scroll_callback(GLFWwindow* window, double /*xoffset*/,
                           double yoffset) {}
