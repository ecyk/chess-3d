#pragma once

#include "piece.hpp"

class Board {
  struct Move {
    int tile{-1};
    int target{-1};
    PieceType promotion{};
  };

  enum class CastlingRight : uint8_t { None, Short = 1, Long = 2, Both = 3 };

  using CastlingRights = std::array<CastlingRight, 2>;

  struct MoveRecord {
    Move move;
    Piece captured_piece{};
    CastlingRights castling_rights{};
    int enpassant_tile{};
  };

  using Records = std::vector<MoveRecord>;

 public:
  using Moves = std::vector<int>;

  Board();

  void move(const Move& move);
  void undo();

  void get_moves(Moves& moves, int tile);
  bool is_game_over();

  void reset();

  [[nodiscard]] Piece get_tile(int tile) const { return tiles_[tile]; }

  [[nodiscard]] PieceColor get_color(int tile) const {
    return piece_color(get_tile(tile));
  }

  [[nodiscard]] PieceType get_type(int tile) const {
    return piece_type(get_tile(tile));
  }

  [[nodiscard]] bool is_empty(int tile) const {
    return piece_type(get_tile(tile)) == PieceType::None;
  }

  [[nodiscard]] const Records& get_records() const { return records_; }

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

  void generate_legal_moves(Moves& moves, int tile);
  void generate_moves(Moves& moves, int tile) const;
  [[nodiscard]] bool is_threatened(int tile, PieceColor attacker_color) const;

  void add_move(Moves& moves, int from, int to) const;

  [[nodiscard]] uint8_t get_color_index(int tile) const {
    return get_color_index(get_color(tile));
  }

  [[nodiscard]] static uint8_t get_color_index(PieceColor color) {
    ASSERT(color != PieceColor::None);
    return (to_underlying(color) >> 3U) - 1;
  }

  [[nodiscard]] static PieceColor get_opposite_color(PieceColor color) {
    return color == PieceColor::White ? PieceColor::Black : PieceColor::White;
  }

  // Black, white
  CastlingRights castling_rights_{CastlingRight::Both, CastlingRight::Both};

  void clear_castling_rights(int tile, PieceColor color);

  // Black, white
  std::array<int, 2> king_tiles_{60, 4};

  int enpassant_tile_{-1};
  std::array<Piece, 64> tiles_{};

  Records records_;
};
