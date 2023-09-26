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

  float time_passed_{};

  Shader* shader_{};

  enum class ModelType {
    Board,
    King,
    Queen,
    Bishop,
    Knight,
    Rook,
    Pawn,
    Count
  };

  using Models = std::array<Model*, to_underlying(ModelType::Count)>;

  Models models_{};

  uint32_t current_model_{};

  static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                    int mods);
  static void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
  static void key_callback(GLFWwindow* window, int key, int scancode,
                           int action, int mods);
  static void scroll_callback(GLFWwindow* window, double xoffset,
                              double yoffset);
};
