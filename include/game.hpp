#pragma once

#include "board.hpp"
#include "renderer.hpp"

struct GLFWwindow;

inline constexpr glm::vec2 k_window_size{1280.0F, 720.0F};

class Game {
  static constexpr float k_game_scale{10.0F};

  static constexpr glm::vec3 k_camera_position{0.0F, 40.0F, -40.0F};
  static constexpr glm::vec3 k_camera_target{};

  static constexpr glm::vec4 k_outline_color{0.0F, 1.0F, 0.0F, 1.0F};

 public:
  explicit Game(GLFWwindow* window);

  void run();

  void resize_picking_texture(const glm::vec2& size);

 private:
  void update();
  void draw();

  void process_input();

  void update_picking_texture();
  void draw_board();
  void draw_pieces();
  void draw_selectable_tiles();

  Renderer renderer_;

  Framebuffer* picking_texture_{};
  bool update_picking_texture_{true};

  int pixel_{-1};

  [[nodiscard]] bool pixel_is_piece() const;
  [[nodiscard]] Piece pixel_as_piece() const;
  [[nodiscard]] bool pixel_is_selectable_tile() const;
  [[nodiscard]] uint32_t pixel_as_selectable_tile() const;

  float delta_time_{};
  float last_frame_{};

  float time_passed_{};

  Camera camera_{k_camera_position, k_camera_target};

  glm::vec2 mouse_last_position_{k_window_size / 2.0F};
  bool first_mouse_input_{true};

  Shader* shader_{};
  Shader* picking_{};
  Shader* outlining_{};

  enum class ModelType {
    Board,
    King,
    Queen,
    Bishop,
    Knight,
    Rook,
    Pawn,
    SelectableTile,
    Count
  };

  using Models = std::array<Model*, to_underlying(ModelType::Count)>;

  Models models_{};

  [[nodiscard]] Model* get_model(ModelType type) const;
  [[nodiscard]] Model* get_model(Piece piece) const;

  Material selectable_tile_;
  Material selectable_tile_hover_;

  Board board_;

  std::vector<Tile> selectable_tiles_;

  Piece selected_piece_{};

  void move_piece_to_tile(Piece piece, const Tile& tile);

  static Transform calculate_tile_transform(const Tile& tile);

  static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                    int mods);
  static void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
  static void mouse_scroll_callback(GLFWwindow* window, double xoffset,
                                    double yoffset);
};
