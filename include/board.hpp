#pragma once

#include "piece.hpp"

class Board {
  using Moves = std::vector<int>;

 public:
  Board();

  void move_to(int tile, int target);
  void get_moves(Moves& moves, int tile);

  void reset();

  [[nodiscard]] Piece get_tile(int tile) const { return tiles_[tile]; }

  static int get_tile_row(int tile) {
    ASSERT(is_valid_tile(tile));
    return static_cast<uint8_t>(tile) >> 3U;
  }

  static int get_tile_column(int tile) {
    ASSERT(is_valid_tile(tile));
    return static_cast<uint8_t>(tile) & 7U;
  }

  static bool is_valid_tile(int tile) { return 0 <= tile && tile <= 63; }

 private:
  void set_tile(int tile, Piece piece) { tiles_[tile] = piece; }

  [[nodiscard]] bool is_empty(int tile) const {
    return piece_type(get_tile(tile)) == PieceType::None;
  }

  [[nodiscard]] PieceColor get_color(int tile) const {
    return piece_color(get_tile(tile));
  }

  [[nodiscard]] PieceType get_type(int tile) const {
    return piece_type(get_tile(tile));
  }

  void generate_legal_moves(Moves& moves, int tile);
  void generate_moves(Moves& moves, int tile) const;
  [[nodiscard]] bool is_threatened(int tile) const;

  void add_move(Moves& moves, int from, int to) const;

  int white_king_tile_{4};
  int black_king_tile_{60};

  int get_king_tile(PieceColor color) {
    ASSERT(color != PieceColor::None);
    return color == PieceColor::White ? white_king_tile_ : black_king_tile_;
  }

  PieceColor turn_{PieceColor::White};
  std::array<Piece, 64> tiles_{};
};
