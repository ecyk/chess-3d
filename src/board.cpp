#include "board.hpp"

Board::Board() { reset(); }

void Board::move(const Move& move) {
  ASSERT(get_color(move.tile) != PieceColor::None &&
         get_type(move.tile) != PieceType::None);

  const MoveRecord& record{records_.emplace_back(
      move, get_tile(move.target), castling_rights_, enpassant_tile_)};
  set_tile(move.target, get_tile(move.tile));
  set_tile(move.tile, {});

  enpassant_tile_ = -1;

  const uint8_t moved_color_index{get_color_index(move.target)};
  if (castling_rights_[moved_color_index] != CastlingRight::None &&
      piece_type(record.captured_piece) == PieceType::Rook) {
    clear_castling_rights(move.target, piece_color(record.captured_piece));
  }

  switch (get_type(move.target)) {
    case PieceType::King:
      if (glm::abs(move.target - move.tile) == 2) {
        const int rook_tile{move.tile + (move.tile < move.target ? 3 : -4)};
        set_tile((move.tile + move.target) / 2, get_tile(rook_tile));
        set_tile(rook_tile, {});
      }
      castling_rights_[moved_color_index] = CastlingRight::None;
      king_tiles_[moved_color_index] = move.target;
      break;
    case PieceType::Rook:
      if (castling_rights_[moved_color_index] != CastlingRight::None) {
        clear_castling_rights(move.tile, get_color(move.target));
      }
      break;
    case PieceType::Pawn:
      if (glm::abs(move.target - move.tile) == 16) {
        enpassant_tile_ = (move.tile + move.target) / 2;
      } else if (move.target == record.enpassant_tile) {
        const int captured_tile{
            move.target +
            (get_color(move.target) == PieceColor::White ? -8 : 8)};
        records_.back().captured_piece = get_tile(captured_tile);
        set_tile(captured_tile, {});
      } else if (move.promotion != PieceType::None) {
        set_tile(move.target,
                 make_piece(get_color(move.target), move.promotion));
      }
      break;
    default:
      break;
  }
}

void Board::undo() {
  if (records_.empty()) {
    return;
  }

  const MoveRecord& record{records_.back()};
  const PieceType moved_type{get_type(record.move.target)};
  set_tile(record.move.tile, get_tile(record.move.target));

  int captured_tile{record.move.target};
  if (moved_type == PieceType::Pawn &&
      record.enpassant_tile == record.move.target) {
    captured_tile =
        record.move.target +
        (get_color(record.move.target) == PieceColor::White ? -8 : 8);
    set_tile(record.move.target, {});
  }

  set_tile(captured_tile, record.captured_piece);

  if (moved_type == PieceType::King) {
    if (glm::abs(record.move.target - record.move.tile) == 2) {
      set_tile(
          record.move.tile + (record.move.tile < record.move.target ? 3 : -4),
          make_piece(
              record.move.target < 8 ? PieceColor::White : PieceColor::Black,
              PieceType::Rook));
      set_tile((record.move.tile + record.move.target) / 2, {});
    }
    king_tiles_[get_color_index(record.move.tile)] = record.move.tile;
  }

  if (record.move.promotion != PieceType::None) {
    set_tile(record.move.tile,
             make_piece(
                 record.move.target < 8 ? PieceColor::Black : PieceColor::White,
                 PieceType::Pawn));
  }

  castling_rights_ = record.castling_rights;
  enpassant_tile_ = record.enpassant_tile;

  records_.pop_back();
}

void Board::get_moves(Moves& moves, int tile) {
  if (!records_.empty()) {
    const MoveRecord& record{records_.back()};
    if (get_color(record.move.target) != get_color(tile)) {
      generate_legal_moves(moves, tile);
    }
  } else {
    generate_legal_moves(moves, tile);
  }
}

bool Board::is_game_over() {
  if (records_.empty()) {
    return false;
  }

  Moves moves;
  const auto& record{records_.back()};
  const PieceColor turn{get_opposite_color(get_color(record.move.target))};
  for (int i = 0; i < 64; i++) {
    if (get_color(i) == turn) {
      get_moves(moves, i);

      if (!moves.empty()) {
        return false;
      }

      moves.clear();
    }
  }

  return true;
}

void Board::reset() {
  castling_rights_ = {CastlingRight::Both, CastlingRight::Both};
  king_tiles_ = {60, 4};
  enpassant_tile_ = -1;
  tiles_.fill({});
  records_.clear();

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

  const PieceColor attacker_color{get_opposite_color(get_color(tile))};
  const uint8_t color_index{get_color_index(tile)};

  for (const int target : possible_moves) {
    move({tile, target});

    const int king_tile{king_tiles_[color_index]};
    if (!is_threatened(king_tile, attacker_color)) {
      moves.push_back(target);
    }

    undo();
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

      if (tile == 4 && castling_rights_[1] != CastlingRight::None) {
        CHECK_MOVE_OFFSET(2, (castling_rights_[1] == CastlingRight::Short ||
                              castling_rights_[1] == CastlingRight::Both) &&
                                 is_empty(5) && is_empty(6) &&
                                 !is_threatened(5, PieceColor::Black) &&
                                 !is_threatened(6, PieceColor::Black));
        CHECK_MOVE_OFFSET(-2, (castling_rights_[1] == CastlingRight::Long ||
                               castling_rights_[1] == CastlingRight::Both) &&
                                  is_empty(1) && is_empty(2) && is_empty(3) &&
                                  !is_threatened(2, PieceColor::Black) &&
                                  !is_threatened(3, PieceColor::Black));
      } else if (tile == 60 && castling_rights_[0] != CastlingRight::None) {
        CHECK_MOVE_OFFSET(2, (castling_rights_[0] == CastlingRight::Short ||
                              castling_rights_[0] == CastlingRight::Both) &&
                                 is_empty(61) && is_empty(62) &&
                                 !is_threatened(61, PieceColor::White) &&
                                 !is_threatened(62, PieceColor::White));
        CHECK_MOVE_OFFSET(-2, (castling_rights_[0] == CastlingRight::Long ||
                               castling_rights_[0] == CastlingRight::Both) &&
                                  is_empty(57) && is_empty(58) &&
                                  is_empty(59) &&
                                  !is_threatened(58, PieceColor::White) &&
                                  !is_threatened(59, PieceColor::White));
      }
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
          CHECK_MOVE_OFFSET(
              -7, col < 7 && (get_color(target) == PieceColor::White ||
                              (target == enpassant_tile_ &&
                               get_color(target) == PieceColor::None)));
          CHECK_MOVE_OFFSET(
              -9, col > 0 && (get_color(target) == PieceColor::White ||
                              (target == enpassant_tile_ &&
                               get_color(target) == PieceColor::None)));
          break;
        case PieceColor::White:
          CHECK_MOVE_OFFSET(8, is_empty(target));
          CHECK_MOVE_OFFSET(
              16, tile < 16 && is_empty(tile + 8) && is_empty(target));
          CHECK_MOVE_OFFSET(
              7, col > 0 && (get_color(target) == PieceColor::Black ||
                             (target == enpassant_tile_ &&
                              get_color(target) == PieceColor::None)));
          CHECK_MOVE_OFFSET(
              9, col < 7 && (get_color(target) == PieceColor::Black ||
                             (target == enpassant_tile_ &&
                              get_color(target) == PieceColor::None)));
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

bool Board::is_threatened(int tile, PieceColor attacker_color) const {
  const int row{get_tile_row(tile)};
  const int col{get_tile_column(tile)};

#define CHECK_THREAT_KNIGHT(offset, condition)                \
  if (const int target = tile + (offset);                     \
      (condition) && get_type(target) == PieceType::Knight && \
      get_color(target) == attacker_color) {                  \
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
    if (target_color == attacker_color && (threat_condition)) {             \
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

void Board::clear_castling_rights(int tile, PieceColor color) {
  auto clear_right = [this](int index, CastlingRight right) {
    castling_rights_[index] = static_cast<CastlingRight>(
        to_underlying(castling_rights_[index]) & ~to_underlying(right));
  };

  switch (tile) {
    case 0:
      if (color == PieceColor::White) {
        clear_right(1, CastlingRight::Long);
      }
      break;
    case 7:
      if (color == PieceColor::White) {
        clear_right(1, CastlingRight::Short);
      }
      break;
    case 56:
      if (color == PieceColor::Black) {
        clear_right(0, CastlingRight::Long);
      }
      break;
    case 63:
      if (color == PieceColor::Black) {
        clear_right(0, CastlingRight::Short);
      }
      break;
    default:
      break;
  }
}
