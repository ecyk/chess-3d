#pragma once

#include "ai.hpp"
#include "board.hpp"
#include "renderer.hpp"

struct GLFWwindow;

inline constexpr glm::vec2 k_window_size{1280.0F, 720.0F};

class Game {
  static constexpr float k_game_scale{10.0F};

  static constexpr glm::vec3 k_camera_position_white{0.0F, 40.0F, -40.0F};
  static constexpr glm::vec3 k_camera_position_black{0.0F, 40.0F, 40.0F};
  static constexpr glm::vec3 k_camera_position{k_camera_position_white};
  static constexpr glm::vec3 k_camera_target{};

  static constexpr glm::vec4 k_picking_outline_color{0.0F, 1.0F, 0.0F, 1.0F};
  static constexpr glm::vec4 k_check_outline_color{1.0F, 0.0F, 0.0F, 1.0F};

  static constexpr glm::vec3 k_light_position{0.0F, 20.0F, 0.0F};

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

  float delta_time_{};
  float last_frame_{};

  float time_passed_{};

  Camera camera_{k_camera_position, k_camera_target};

  [[nodiscard]] bool is_controlling_camera() const;

  glm::vec2 mouse_last_position_{k_window_size / 2.0F};
  glm::vec2 mouse_last_position_real_{mouse_last_position_};
  bool first_mouse_input_{true};

  [[nodiscard]] bool is_cursor_active() const;
  void enable_cursor();
  void disable_cursor();

  Shader* shader_{};
  Shader* lighting_{};
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

  [[nodiscard]] Model* get_model(Piece piece) const;
  [[nodiscard]] Model* get_model(ModelType type) const;

  Material selectable_tile_;
  Material selectable_tile_hover_;

  Board board_;

  Moves selectable_tiles_;

  [[nodiscard]] bool is_selectable_tile(int tile) const;

  int selected_tile_{-1};

  void clear_selections() {
    selectable_tiles_ = {};
    selected_tile_ = -1;
    update_picking_texture_ = true;
  }

  struct ActiveMove {
    int tile{-1};
    int target{-1};
    glm::vec3 position{};
    float angle{180.0F};
    bool is_undo{};
    bool is_completed{};
  };

  ActiveMove active_move_;

  void set_active_move(const Move& move, bool is_undo = false);
  void undo();

  AI ai_{board_};
  PieceColor ai_color_{};

  bool game_over_{};

  Transform calculate_piece_transform(int tile);

  static glm::vec3 calculate_tile_position(int tile);
  static Transform calculate_tile_transform(int tile, float rotation = 0.0F);

  static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                    int mods);
  static void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
  static void mouse_scroll_callback(GLFWwindow* window, double xoffset,
                                    double yoffset);
  static void key_callback(GLFWwindow* window, int key, int /*scancode*/,
                           int action, int /*mods*/);
};
