#pragma once

#include "board.hpp"

class AI {
 public:
  explicit AI(Board& board) : board_{&board} {}

  Move think();

 private:
  int search(int depth);
  int evaluate();

  static int get_piece_value(PieceType type) {
    return std::array{0, 10000, 1000, 350, 350, 525, 100}[to_underlying(type)];
  };

  Move best_move_;
  Board* board_{};
};
