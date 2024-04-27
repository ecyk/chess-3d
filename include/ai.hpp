#pragma once

#include <functional>
#include <thread>

#include "board.hpp"

class AI {
 public:
  AI() : worker_{std::bind_front(&AI::run, this)} {
    LOGF("AI", "Thread started");
  }

  void think(const Board& board);

  Move get_best_move();
  [[nodiscard]] bool is_thinking() const { return thinking_; }
  [[nodiscard]] bool has_found_move() const { return found_move_; }

 private:
  void run(const std::stop_token& stop_token);

  int search(int depth, int alpha, int beta);
  int quiesce(int alpha, int beta);
  int evaluate() const;

  void order_moves(Moves& moves) const;

  static int get_piece_value(PieceType type) {
    return std::array{0, 10000, 1000, 350, 350, 525, 100}[to_underlying(type)];
  };

  Move best_move_;
  Board board_;

  std::atomic<bool> thinking_;
  std::atomic<bool> found_move_;

  std::jthread worker_;
};
