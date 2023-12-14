#pragma once

#include "piece.hpp"

inline constexpr bool is_valid_tile(int tile) {
  return 0 <= tile && tile <= 63;
}

inline constexpr int get_tile_row(int tile) {
  ASSERT(is_valid_tile(tile));
  return static_cast<uint8_t>(tile) >> 3U;
}

inline constexpr int get_tile_column(int tile) {
  ASSERT(is_valid_tile(tile));
  return static_cast<uint8_t>(tile) & 7U;
}

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
    PieceType promotion{};
    Piece captured_piece{};
    CastlingRights castling_rights{};
    int enpassant_tile{};
  };

  using Records = std::vector<MoveRecord>;

  static constexpr std::string_view DEFAULT_FEN{
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};

 public:
  struct Moves {
    int size{};
    std::array<Move, 256> data{};
  };

  Board();

  void move(Move move);
  void undo();

  void get_moves(Moves& moves, int tile);
  bool is_game_over();

  uint64_t perft(int depth);

  void load_fen(std::string_view fen = DEFAULT_FEN);

  [[nodiscard]] Piece get_tile(int tile) const { return tiles_[tile]; }

  [[nodiscard]] PieceColor get_color(int tile) const {
    return get_piece_color(get_tile(tile));
  }

  [[nodiscard]] PieceType get_type(int tile) const {
    return get_piece_type(get_tile(tile));
  }

  [[nodiscard]] const Records& get_records() const { return records_; }

 private:
  void set_tile(int tile, Piece piece) { tiles_[tile] = piece; }

  [[nodiscard]] bool is_piece(int tile, PieceColor color,
                              PieceType type) const {
    return get_color(tile) == color && get_type(tile) == type;
  }

  [[nodiscard]] bool is_empty(int tile) const {
    return get_piece_type(get_tile(tile)) == PieceType::None;
  }

  void generate_legal_moves(Moves& moves, int tile);
  void generate_moves(Moves& moves, int tile) const;
  [[nodiscard]] bool is_threatened(int tile, PieceColor attacker_color) const;
  bool is_in_check();

  void reset();

  PieceColor turn_{};

  // Black, white
  CastlingRights castling_rights_{};

  void set_castling_right(int index, CastlingRight right);
  void clear_castling_right(int index, CastlingRight right);
  void clear_castling_rights(int tile, PieceColor color);

  // Black, white
  std::array<int, 2> king_tiles_{};

  int enpassant_tile_{-1};
  std::array<Piece, 64> tiles_{};

  Records records_;
};
