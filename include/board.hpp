#pragma once

#include "common.hpp"

enum class PieceColor : uint8_t { Black = 8, White = 16 };

enum class PieceType : uint8_t {
  None,
  King,
  Queen,
  Bishop,
  Knight,
  Rook,
  Pawn
};

enum class Piece : uint8_t;

inline bool is_piece_color(Piece piece, PieceColor color) {
  return (to_underlying(piece) & 24U) == to_underlying(color);
}

inline bool is_piece_type(Piece piece, PieceType type) {
  return (to_underlying(piece) & 7U) == to_underlying(type);
}

struct Tile {
  glm::ivec2 coord{};
  Piece piece{};
};

/*
       WHITE
 X 0 1 2 3 4 5 6 7
Y
0  r n b k q b n r
1  p p p p p p p p
2  . . . . . . . .
3  . . . . . . . .
4  . . . . . . . .
5  . . . . . . . .
6  p p p p p p p p
7  r n b k q b n r
       BLACK
*/

class Board {
  using Tiles = std::array<Piece, 64>;
  using ActiveTiles = std::vector<Tile>;

 public:
  Board();

  void move_to(Piece piece, const glm::ivec2& coord);
  [[nodiscard]] glm::ivec2 get_coord(Piece piece) const;

  void reset();

  [[nodiscard]] Piece get_tile(const glm::ivec2& coord) const {
    return tiles_.at(coord.y * 8 + coord.x);
  }

  [[nodiscard]] const ActiveTiles& get_active_tiles() const {
    return active_tiles_;
  }

 private:
  void update_active_tiles();

  void set_tile(const glm::ivec2& coord, Piece piece) {
    tiles_.at(coord.y * 8 + coord.x) = piece;
  }

  Tiles tiles_{};
  ActiveTiles active_tiles_;
};
