#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "ai.hpp"
#include "board.hpp"
#include "renderer.hpp"

inline constexpr glm::vec2 k_window_size{1280.0F, 720.0F};

class Game {
  static constexpr float k_game_scale{10.0F};
  static constexpr float k_ms_per_update{1.0F / 60.0F};
  static constexpr glm::vec3 k_camera_initial_position{0.05F, 56.0F, 0.0F};
  static constexpr glm::vec3 k_camera_w_side_position{0.0F, 40.0F, 40.0F};
  static constexpr glm::vec3 k_camera_b_side_position{0.0F, 40.0F, -40.0F};
  static constexpr glm::vec4 k_picking_outline_color{0.0F, 1.0F, 0.0F, 1.0F};
  static constexpr glm::vec4 k_check_outline_color{1.0F, 0.0F, 0.0F, 1.0F};
  static constexpr glm::vec3 k_light_position{0.0F, 40.0F, 0.0F};

 public:
  explicit Game(GLFWwindow* window);

  void run();

 private:
  void update();
  void draw();

  void process_input() const;

  void draw_picking_texture();
  void draw_pieces();
  void draw_selectable_tiles();

  bool is_fullscreen{};
  glm::ivec2 window_old_pos_{};
  glm::ivec2 window_old_size_{};

  Renderer renderer_;

  int pixel_{-1};

  float delta_time_{};
  float last_frame_{};

  // clang-format off
  bool is_controlling_camera() const { return glfwGetMouseButton(renderer_.get_window(), GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS; }
  bool is_cursor_active() const { return glfwGetInputMode(renderer_.get_window(), GLFW_CURSOR) == GLFW_CURSOR_NORMAL; }
  void enable_cursor() { glfwSetInputMode(renderer_.get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL); mouse_last_position_ = mouse_last_position_real_; }
  void disable_cursor() { glfwSetInputMode(renderer_.get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED); mouse_last_position_real_ = mouse_last_position_; }
  void clear_selections() { selectable_tiles_ = {}; selected_tile_ = -1; }

  void process_camera_movement();
  void set_camera_target_position(const glm::vec3& position) { camera_target_position_ = position; is_camera_moving_ = true; }
  glm::vec3 get_player_camera_target_position() const { return ai_color_ == PieceColor::Black ? k_camera_b_side_position : k_camera_w_side_position; }
  glm::vec3 get_ai_camera_target_position() const { return ai_color_ == PieceColor::White ? k_camera_w_side_position : k_camera_b_side_position; }
  // clang-format on
  Camera camera_{k_camera_initial_position, {}};
  glm::vec3 camera_target_position_{};
  bool is_camera_moving_{};

  glm::vec2 mouse_last_position_{k_window_size / 2.0F};
  glm::vec2 mouse_last_position_real_{mouse_last_position_};
  bool first_mouse_input_{true};

  [[nodiscard]] bool is_selectable_tile(int tile) const;

  Board board_;
  Moves selectable_tiles_;
  int selected_tile_{-1};

  struct ActiveMove {
    int tile{-1};
    int target{-1};
    glm::vec3 position{};
    float angle{180.0F};
    bool is_undo{};
    bool is_completed{};
  };

  void process_active_move();
  void set_active_move(const Move& move, bool is_undo = false);
  void undo();

  ActiveMove active_move_;

  bool is_ai_turn() const { return board_.get_turn() == ai_color_; }

  AI ai_;
  PieceColor ai_color_{};

  bool game_over_{};

  Transform calculate_piece_transform(int tile) const;
  static glm::vec3 calculate_tile_position(int tile);
  static Transform calculate_tile_transform(int tile, float rotation = 0.0F);
  static std::string_view get_model_name(Piece piece);
  // clang-format off
  static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
  static void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
  static void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
  static void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/);
  // clang-format on
};
