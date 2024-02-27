#pragma once

#include "piece.hpp"

inline constexpr bool is_valid_tile(int tile) {
  return 0 <= tile && tile <= 63;
}

inline constexpr int get_tile_row(int tile) {
  assert(is_valid_tile(tile));
  return static_cast<uint8_t>(tile) >> 3U;
}

inline constexpr int get_tile_column(int tile) {
  assert(is_valid_tile(tile));
  return static_cast<uint8_t>(tile) & 7U;
}

struct Move {
  int tile{-1};
  int target{-1};
  PieceType promotion{};
};

struct Moves {
  int size{};
  std::array<Move, 256> data{};
};

class Board {
  enum class CastlingRight : uint8_t { None, Short = 1, Long = 2, Both = 3 };

  using CastlingRights = std::array<CastlingRight, 2>;

  struct MoveRecord {
    Move move;
    PieceType promotion{};
    Piece captured_piece{};
    CastlingRights castling_rights{};
    int enpassant_tile{};
  };

  using Records = std::vector<MoveRecord>;

  static constexpr std::string_view k_initial_fen{
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};

 public:
  Board();

  void move(Move move);
  void undo();

  void generate_all_legal_moves(Moves& moves);
  void generate_legal_moves(Moves& moves, int tile);

  bool is_in_check() {
    return is_threatened(king_tiles_[get_color_index(turn_)],
                         get_opposite_color(turn_));
  }
  bool is_in_checkmate() { return is_in_check() && !has_legal_moves(); }
  bool is_in_draw() { return !is_in_check() && !has_legal_moves(); }

  uint64_t perft(int depth);

  void load_fen(std::string_view fen = k_initial_fen);

  [[nodiscard]] PieceColor get_turn() const { return turn_; }

  [[nodiscard]] Piece get_tile(int tile) const { return tiles_[tile]; }
  [[nodiscard]] PieceColor get_color(int tile) const {
    return get_piece_color(get_tile(tile));
  }
  [[nodiscard]] PieceType get_type(int tile) const {
    return get_piece_type(get_tile(tile));
  }

  [[nodiscard]] bool is_empty(int tile) const {
    return get_piece_type(get_tile(tile)) == PieceType::None;
  }
  [[nodiscard]] bool is_piece(int tile, PieceColor color,
                              PieceType type) const {
    return get_color(tile) == color && get_type(tile) == type;
  }

  [[nodiscard]] const Records& get_records() const { return records_; }

 private:
  void set_tile(int tile, Piece piece) { tiles_[tile] = piece; }

  bool has_legal_moves();

  void generate_moves(Moves& moves, int tile) const;
  [[nodiscard]] bool is_threatened(int tile, PieceColor attacker_color) const;

  PieceColor turn_{};
  CastlingRights castling_rights_{};
  std::array<int, 2> king_tiles_{};
  int enpassant_tile_{-1};
  std::array<Piece, 64> tiles_{};
  Records records_;
};
