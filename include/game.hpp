#pragma once

#include "camera.hpp"
#include "renderer.hpp"

struct GLFWwindow;

inline constexpr glm::vec2 k_window_size{1280, 720};

class Game {
  static constexpr float k_game_scale{10.0F};

  static constexpr glm::vec3 k_camera_position{0, 40.0F, -40.0F};
  static constexpr glm::vec3 k_camera_target{};

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

  Camera camera_{k_camera_position, k_camera_target};

  glm::vec2 mouse_last_position_{k_window_size / 2.0F};
  bool first_mouse_input_{true};

  Shader* base_shader_{};
  Shader* picking_shader_{};

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
  static void mouse_scroll_callback(GLFWwindow* window, double xoffset,
                                    double yoffset);
  static void key_callback(GLFWwindow* window, int key, int scancode,
                           int action, int mods);
};
