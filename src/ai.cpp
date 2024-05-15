#include "ai.hpp"

#include <chrono>

#include "board.hpp"

void AI::think(const Board& board) {
  assert(!thinking_);
  board_ = board;
  found_move_ = false;
  thinking_ = true;
}

Move AI::get_best_move() {
  assert(found_move_);
  found_move_ = false;
  return best_move_;
}

void AI::run(const std::stop_token& stop_token) {
  while (!stop_token.stop_requested()) {
    std::this_thread::sleep_for(1s);
    if (thinking_ && !found_move_) {
      auto start{std::chrono::high_resolution_clock::now()};
      int score{-1000000};
      for (int i = 1;; i++) {
        score = std::max(score, search(i, -1000000, 1000000));
        auto now{std::chrono::high_resolution_clock::now()};
        auto ms{
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start)};
        LOGF("AI", "Depth: {} Time: {}ms", i, ms.count());
        if (score >= 100000 || ms.count() > 500) {
          break;
        }
      }
      thinking_ = false;
      found_move_ = true;
    }
  }
  LOG("AI", "Thread stopped");
}

int AI::search(int depth, int alpha, int beta) {
  if (depth == 0 || board_.is_in_checkmate() || board_.is_in_draw()) {
    return quiesce(alpha, beta);
  }
  int max{-1000000};
  Move best_move;
  Moves all_legal_moves;
  board_.generate_all_legal_moves(all_legal_moves);
  assert(all_legal_moves.size != 0);
  order_moves(all_legal_moves);
  for (int i = 0; i < all_legal_moves.size; i++) {
    const Move& move = all_legal_moves.data[i];
    board_.make_move(move);
    const int score{-search(depth - 1, -beta, -alpha)};
    board_.undo();
    if (score > max) {
      max = score;
      best_move = move;
    }
    if (score > alpha) {
      alpha = score;
    }
    if (alpha >= beta) {
      break;
    }
  }
  best_move_ = best_move;
  return max;
}

int AI::quiesce(int alpha, int beta) {
  int score{evaluate()};

  if (score >= beta) {
    return beta;
  }
  if (alpha < score) {
    alpha = score;
  }

  Moves all_legal_moves;
  board_.generate_all_legal_moves(all_legal_moves, true);
  order_moves(all_legal_moves);
  for (int i = 0; i < all_legal_moves.size; i++) {
    const Move& move = all_legal_moves.data[i];
    board_.make_move(move);
    score = -quiesce(-beta, -alpha);
    board_.undo();

    if (score >= beta) {
      return beta;
    }
    if (score > alpha) {
      alpha = score;
    }
  }

  return alpha;
}

int AI::evaluate() const {
  if (board_.is_in_checkmate()) {
    return -500000;
  }

  if (board_.is_in_draw()) {
    return 0;
  }

  int score{};
  for (int tile = 0; tile < 64; tile++) {
    if (board_.is_empty(tile)) {
      continue;
    }
    const PieceColor color = board_.get_color(tile);
    const int side{board_.get_turn() == color ? 1 : -1};

#define CASE_PIECE(type, table)                                            \
  case PieceType::type:                                                    \
    score += get_piece_value(PieceType::type) * side;                      \
    score +=                                                               \
        (table)[color == PieceColor::White                                 \
                    ? 8 * (7 - get_tile_row(tile)) + get_tile_column(tile) \
                    : tile] *                                              \
        side;                                                              \
    break;

    switch (board_.get_type(tile)) {
      case PieceType::None:
        assert(false);
        break;
        CASE_PIECE(King, k_king_table)
        CASE_PIECE(Queen, k_queen_table)
        CASE_PIECE(Bishop, k_bishop_table)
        CASE_PIECE(Knight, k_knight_table)
        CASE_PIECE(Rook, k_rook_table)
        CASE_PIECE(Pawn, k_pawn_table)
    }
#undef CASE_PIECE
  }
  return score;
}

void AI::order_moves(Moves& moves) const {
  const auto& begin{moves.data.begin()};
  // clang-format off
  std::sort(begin, begin + moves.size, [this](const Move& left, const Move& right) {
    if (!board_.is_empty(left.target) && !board_.is_empty(right.target)) {
      if (board_.get_type(left.target) != board_.get_type(right.target)) {
        return get_piece_value(board_.get_type(left.target)) > get_piece_value(board_.get_type(right.target));
      }
      if (board_.get_type(left.tile) != board_.get_type(right.tile)) {
        return get_piece_value(board_.get_type(left.tile)) < get_piece_value(board_.get_type(right.tile));
      }
    } else if (!board_.is_empty(left.target)) {
      return true;
    } else if (!board_.is_empty(right.target)) {
      return false;
    }
    return get_piece_value(board_.get_type(left.tile)) < get_piece_value(board_.get_type(right.tile));
  });
  // clang-format on
}
