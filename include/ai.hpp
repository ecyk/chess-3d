#pragma once

#include <thread>

#include "board.hpp"

class AI {
 public:
  explicit AI() : worker_{&AI::run, this} { worker_.detach(); }

  void think(const Board& board);

  Move get_best_move();
  bool is_thinking() const { return thinking_; }
  bool has_found_move() const { return found_move_; }

 private:
  [[noreturn]] void run();

  int search(int depth, int alpha, int beta);
  int quiesce(int alpha, int beta);
  int evaluate();

  void order_moves(Moves& moves);

  static int get_piece_value(PieceType type) {
    return std::array{0, 10000, 1000, 350, 350, 525, 100}[to_underlying(type)];
  };

  std::atomic<Move> best_move_;
  Board board_;

  std::atomic<bool> thinking_{};
  std::atomic<bool> found_move_{};
  std::thread worker_;
};
