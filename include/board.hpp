#pragma once

#include "piece.hpp"

inline constexpr bool is_valid_tile(int tile) {
  return 0 <= tile && tile <= 63;
}

inline constexpr glm::ivec2 calculate_tile_coord(int tile) {
  assert(is_valid_tile(tile));
  return {tile & 7, tile >> 3};
}

class Board {
 public:
  Board();

  void move_to(int tile, int target);

  void reset();

  [[nodiscard]] Piece get_tile(int tile) const {
    assert(is_valid_tile(tile));
    return tiles_[tile];
  }

 private:
  void set_tile(int tile, Piece piece) {
    assert(is_valid_tile(tile));
    tiles_[tile] = piece;
  }

  std::array<Piece, 64> tiles_{};
};
