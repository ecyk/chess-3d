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
  int x{};
  int y{};
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

  Piece& at(int x, int y) { return tiles_.at(y * 8 + x); }

  void reset();

  [[nodiscard]] const ActiveTiles& get_active_tiles() const {
    return active_tiles_;
  }

 private:
  void update_active_tiles();

  Tiles tiles_{};
  ActiveTiles active_tiles_;
};
