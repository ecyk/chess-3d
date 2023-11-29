#include "board.hpp"

Board::Board() { reset(); }

void Board::move_to(int tile, int target) {
  ASSERT(get_color(tile) != PieceColor::None);
  ASSERT(get_type(tile) != PieceType::None);

  turn_ = (turn_ == PieceColor::White) ? PieceColor::Black : PieceColor::White;

  if (get_type(tile) == PieceType::King) {
    if (get_color(tile) == PieceColor::White) {
      white_king_tile_ = target;
    } else {
      black_king_tile_ = target;
    }
  }

  set_tile(target, get_tile(tile));
  set_tile(tile, {});
}

void Board::get_moves(Moves& moves, int tile) {
  if (get_color(tile) != turn_) {
    return;
  }

  generate_legal_moves(moves, tile);
}

void Board::reset() {
  tiles_.fill({});

  auto init_pieces = [this](PieceColor color, int piece_row, int pawn_row) {
    auto init_piece = [this](int tile, PieceColor color, PieceType type) {
      set_tile(tile, make_piece(color, type));
    };

    init_piece(8 * piece_row + 0, color, PieceType::Rook);
    init_piece(8 * piece_row + 1, color, PieceType::Knight);
    init_piece(8 * piece_row + 2, color, PieceType::Bishop);
    init_piece(8 * piece_row + 3, color, PieceType::Queen);
    init_piece(8 * piece_row + 4, color, PieceType::King);
    init_piece(8 * piece_row + 5, color, PieceType::Bishop);
    init_piece(8 * piece_row + 6, color, PieceType::Knight);
    init_piece(8 * piece_row + 7, color, PieceType::Rook);

    for (uint8_t i = 0; i < 8; i++) {
      init_piece(8 * pawn_row + i, color, PieceType::Pawn);
    }
  };

  init_pieces(PieceColor::White, 0, 1);
  init_pieces(PieceColor::Black, 7, 6);
}

void Board::generate_legal_moves(Moves& moves, int tile) {
  Moves possible_moves;
  generate_moves(possible_moves, tile);

  const PieceColor tile_color{get_color(tile)};

  for (const int target : possible_moves) {
    const Piece target_piece{get_tile(target)};
    move_to(tile, target);

    if (!is_threatened(get_king_tile(tile_color))) {
      moves.push_back(target);
    }

    move_to(target, tile);
    set_tile(target, target_piece);
  }
}

void Board::generate_moves(Moves& moves, int tile) const {
  const int row{get_tile_row(tile)};
  const int col{get_tile_column(tile)};

  const PieceType tile_type{get_type(tile)};

#define CHECK_MOVE_OFFSET(offset, condition)           \
  if (const int target = tile + (offset); condition) { \
    add_move(moves, tile, target);                     \
  }

#define CHECK_MOVE_DIRECTION(direction, condition)                          \
  for (int target = tile + (direction); condition; target += (direction)) { \
    add_move(moves, tile, target);                                          \
    if (!is_empty(target)) {                                                \
      break;                                                                \
    }                                                                       \
  }

  switch (tile_type) {
    case PieceType::King:
      CHECK_MOVE_OFFSET(1, col < 7);
      CHECK_MOVE_OFFSET(7, row < 7 && col > 0);
      CHECK_MOVE_OFFSET(8, row < 7);
      CHECK_MOVE_OFFSET(9, row < 7 && col < 7);
      CHECK_MOVE_OFFSET(-1, col > 0);
      CHECK_MOVE_OFFSET(-7, row > 0 && col < 7);
      CHECK_MOVE_OFFSET(-8, row > 0);
      CHECK_MOVE_OFFSET(-9, row > 0 && col > 0);
      break;
    case PieceType::Queen:
    case PieceType::Bishop:
      CHECK_MOVE_DIRECTION(7, target < 64 && get_tile_column(target) != 7);
      CHECK_MOVE_DIRECTION(9, target < 64 && get_tile_column(target) != 0);
      CHECK_MOVE_DIRECTION(-7, target >= 0 && get_tile_column(target) != 0);
      CHECK_MOVE_DIRECTION(-9, target >= 0 && get_tile_column(target) != 7);

      if (tile_type == PieceType::Bishop) {
        break;
      }
    case PieceType::Rook:
      CHECK_MOVE_DIRECTION(1, target < tile - col + 8);
      CHECK_MOVE_DIRECTION(8, target < 64);
      CHECK_MOVE_DIRECTION(-1, target >= tile - col);
      CHECK_MOVE_DIRECTION(-8, target >= 0);
      break;
    case PieceType::Knight:
      CHECK_MOVE_OFFSET(6, target < 64 && col > 1);
      CHECK_MOVE_OFFSET(10, target < 64 && col < 6);
      CHECK_MOVE_OFFSET(15, target < 64 && col > 0);
      CHECK_MOVE_OFFSET(17, target < 64 && col < 7);
      CHECK_MOVE_OFFSET(-6, target >= 0 && col < 6);
      CHECK_MOVE_OFFSET(-10, target >= 0 && col > 1);
      CHECK_MOVE_OFFSET(-15, target >= 0 && col < 7);
      CHECK_MOVE_OFFSET(-17, target >= 0 && col > 0);
      break;
    case PieceType::Pawn:
      switch (get_color(tile)) {
        case PieceColor::Black:
          CHECK_MOVE_OFFSET(-8, is_empty(target));
          CHECK_MOVE_OFFSET(
              -16, tile >= 48 && is_empty(tile - 8) && is_empty(target));
          CHECK_MOVE_OFFSET(-7,
                            col < 7 && get_color(target) == PieceColor::White);
          CHECK_MOVE_OFFSET(-9,
                            col > 0 && get_color(target) == PieceColor::White);
          break;
        case PieceColor::White:
          CHECK_MOVE_OFFSET(8, is_empty(target));
          CHECK_MOVE_OFFSET(
              16, tile < 16 && is_empty(tile + 8) && is_empty(target));
          CHECK_MOVE_OFFSET(7,
                            col > 0 && get_color(target) == PieceColor::Black);
          CHECK_MOVE_OFFSET(9,
                            col < 7 && get_color(target) == PieceColor::Black);
          break;
        default:
          ASSERT(false && "Unreachable");
          break;
      }
      break;
    default:
      ASSERT(false && "Unreachable");
      break;
  }

#undef CHECK_MOVE_OFFSET
#undef CHECK_MOVE_DIRECTION
}

bool Board::is_threatened(int tile) const {
  const int row{get_tile_row(tile)};
  const int col{get_tile_column(tile)};

  const PieceColor tile_color{get_color(tile)};

#define CHECK_THREAT_KNIGHT(offset, condition)                \
  if (const int target = tile + (offset);                     \
      (condition) && get_type(target) == PieceType::Knight && \
      get_color(target) != tile_color) {                      \
    return true;                                              \
  }

  CHECK_THREAT_KNIGHT(6, col > 1 && row < 7);
  CHECK_THREAT_KNIGHT(10, col < 6 && row < 7);
  CHECK_THREAT_KNIGHT(15, col > 0 && row < 6);
  CHECK_THREAT_KNIGHT(17, col < 7 && row < 6);
  CHECK_THREAT_KNIGHT(-6, col < 6 && row > 0);
  CHECK_THREAT_KNIGHT(-10, col > 1 && row > 0);
  CHECK_THREAT_KNIGHT(-15, col < 7 && row > 1);
  CHECK_THREAT_KNIGHT(-17, col > 0 && row > 1);

#undef CHECK_THREAT_KNIGHT

#define CHECK_THREAT_DIRECTION(direction, condition, threat_condition)      \
  for (int target = tile + (direction); condition; target += (direction)) { \
    if (is_empty(target)) {                                                 \
      continue;                                                             \
    }                                                                       \
    const PieceColor target_color{get_color(target)};                       \
    const PieceType target_type{get_type(target)};                          \
    if (target_color != tile_color && (threat_condition)) {                 \
      return true;                                                          \
    }                                                                       \
    break;                                                                  \
  }

  // Up
  CHECK_THREAT_DIRECTION(
      8, target < 64,
      target_type == PieceType::Queen || target_type == PieceType::Rook ||
          (target_type == PieceType::King && target == tile + 8));

  // Down
  CHECK_THREAT_DIRECTION(
      -8, target >= 0,
      target_type == PieceType::Queen || target_type == PieceType::Rook ||
          (target_type == PieceType::King && target == tile - 8));

  // Left
  CHECK_THREAT_DIRECTION(
      -1, target >= tile - col,
      target_type == PieceType::Queen || target_type == PieceType::Rook ||
          (target_type == PieceType::King && target == tile - 1));

  // Right
  CHECK_THREAT_DIRECTION(
      1, target < tile - col + 8,
      target_type == PieceType::Queen || target_type == PieceType::Rook ||
          (target_type == PieceType::King && target == tile + 1));

  // Up left
  CHECK_THREAT_DIRECTION(
      7, target < 64 && get_tile_column(target) != 7,
      target_type == PieceType::Queen || target_type == PieceType::Bishop ||
          (target == tile + 7 && (target_type == PieceType::King ||
                                  (target_type == PieceType::Pawn &&
                                   target_color == PieceColor::Black))));

  // Up right
  CHECK_THREAT_DIRECTION(
      9, target < 64 && get_tile_column(target) != 0,
      target_type == PieceType::Queen || target_type == PieceType::Bishop ||
          (target == tile + 9 && (target_type == PieceType::King ||
                                  (target_type == PieceType::Pawn &&
                                   target_color == PieceColor::Black))));

  // Down right
  CHECK_THREAT_DIRECTION(
      -7, target >= 0 && get_tile_column(target) != 0,
      target_type == PieceType::Queen || target_type == PieceType::Bishop ||
          (target == tile - 7 && (target_type == PieceType::King ||
                                  (target_type == PieceType::Pawn &&
                                   target_color == PieceColor::White))));

  // Down left
  CHECK_THREAT_DIRECTION(
      -9, target >= 0 && get_tile_column(target) != 7,
      target_type == PieceType::Queen || target_type == PieceType::Bishop ||
          (target == tile - 9 && (target_type == PieceType::King ||
                                  (target_type == PieceType::Pawn &&
                                   target_color == PieceColor::White))));

#undef CHECK_THREAT_DIRECTION

  return false;
}

void Board::add_move(Moves& moves, int from, int to) const {
  ASSERT(get_color(from) != PieceColor::None);

  if (get_color(from) != get_color(to)) {
    moves.push_back(to);
  }
}
