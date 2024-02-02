#include "ai.hpp"

Move AI::think() {
  int score{search(4)};
  LOGF("AI", "Best move for {} is {} -> {} ({})",
       board_->get_turn() == PieceColor::White ? "white" : "black",
       best_move_.tile, best_move_.target, score);
  return best_move_;
}

int AI::search(int depth) {
  if (depth == 0 || board_->is_in_checkmate() || board_->is_in_draw()) {
    return evaluate();
  }
  int max{std::numeric_limits<int>::min()};
  Move best_move;
  Moves all_legal_moves;
  board_->generate_all_legal_moves(all_legal_moves);
  ASSERT(all_legal_moves.size != 0);
  for (int i = 0; i < all_legal_moves.size; i++) {
    const Move& move = all_legal_moves.data[i];
    board_->move(move);
    const int score{-search(depth - 1)};
    board_->undo();
    if (score > max) {
      max = score;
      best_move = move;
    }
  }
  best_move_ = best_move;
  return max;
}

int AI::evaluate() {
  if (board_->is_in_checkmate()) {
    return -std::numeric_limits<int>::max();
  }

  if (board_->is_in_draw()) {
    return 0;
  }

  int score{};
  for (int tile = 0; tile < 64; tile++) {
    if (board_->is_empty(tile)) {
      continue;
    }

    const PieceType type{board_->get_type(tile)};
    if (board_->get_turn() == board_->get_color(tile)) {
      score += get_piece_value(type);
    } else {
      score -= get_piece_value(type);
    }
  }

  return score;
}
