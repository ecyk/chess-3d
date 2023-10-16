#pragma once

#include "board.hpp"
#include "renderer.hpp"

struct GLFWwindow;

inline constexpr glm::vec2 k_window_size{1280.0F, 720.0F};

class Game {
  static constexpr float k_game_scale{10.0F};

  static constexpr glm::vec3 k_camera_position{0.0F, 40.0F, -40.0F};
  static constexpr glm::vec3 k_camera_target{};

 public:
  explicit Game(GLFWwindow* window);

  void run();

  void resize_picking_texture(const glm::vec2& size);

 private:
  void update();
  void draw();

  void process_input();

  void draw_picking_texture();

  Renderer renderer_;

  Framebuffer* picking_texture_;
  bool update_picking_texture_{true};

  float delta_time_{};
  float last_frame_{};

  float time_passed_{};

  Camera camera_{k_camera_position, k_camera_target};

  glm::vec2 mouse_last_position_{k_window_size / 2.0F};
  bool first_mouse_input_{true};

  Shader* shader_{};
  Shader* picking_{};

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

  [[nodiscard]] Model* get_model(ModelType type) const;
  [[nodiscard]] Model* get_model(Piece piece) const;

  Board board_;

  Piece selected_piece_{};

  static Transform calculate_piece_transform(const Tile& tile);

  static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                    int mods);
  static void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
  static void mouse_scroll_callback(GLFWwindow* window, double xoffset,
                                    double yoffset);
};
