#pragma once

#include "renderer.hpp"

struct GLFWwindow;

class Game {
 public:
  explicit Game(GLFWwindow* window);

  void run();

 private:
  void update();
  void draw();

  void process_input();

  Renderer renderer_;

  float delta_time_{};
  float last_frame_{};

  static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
  static void key_callback(GLFWwindow* window, int key, int scancode,
                           int action, int mods);
  static void scroll_callback(GLFWwindow* window, double xoffset,
                              double yoffset);
};
