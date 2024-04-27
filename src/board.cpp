#include "board.hpp"

Board::Board() { load_fen(); }

void Board::make_move(Move move) {
  this->move(move);
  const bool has_legal_moves{this->has_legal_moves()};
  is_in_check_ = is_threatened(king_tiles_[get_color_index(turn_)],
                               get_opposite_color(turn_));
  is_in_checkmate_ = is_in_check_ && !has_legal_moves;
  is_in_draw_ = !is_in_check_ && !has_legal_moves;
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
    king_tiles_[get_color_index(get_color(record.move.tile))] =
        record.move.tile;
  }

  if (record.promotion != PieceType::None) {
    set_tile(record.move.tile,
             make_piece(
                 record.move.target < 8 ? PieceColor::Black : PieceColor::White,
                 PieceType::Pawn));
  }

  turn_ = get_opposite_color(turn_);
  castling_rights_ = record.castling_rights;
  enpassant_tile_ = record.enpassant_tile;
  is_in_check_ = record.is_in_check_;
  is_in_checkmate_ = record.is_in_checkmate_;
  is_in_draw_ = record.is_in_draw_;

  records_.pop_back();
}

void Board::generate_all_legal_moves(Moves& moves, bool only_captures) {
  for (int tile = 0; tile < 64; tile++) {
    generate_legal_moves(moves, tile, only_captures);
  }
}

void Board::generate_legal_moves(Moves& moves, int tile, bool only_captures) {
  if (turn_ != get_color(tile)) {
    return;
  }
  auto check_king = [this] {
    return is_threatened(
        king_tiles_[get_color_index(get_opposite_color(turn_))], turn_);
  };
  int end{moves.size};
  generate_moves(moves, tile);
  for (int i = end; i < moves.size; i++) {
    const bool captured{!is_empty(moves.data[i].target)};
    move(moves.data[i]);
    if (!check_king() && (!only_captures || (only_captures && captured))) {
      moves.data[end++] = moves.data[i];
    }
    undo();
  }
  moves.size = end;
}

uint64_t Board::perft(int depth) {
  uint64_t nodes{};
  if (depth == 0) {
    return 1;
  }

  Moves moves;
  generate_all_legal_moves(moves);
  for (int i = 0; i < moves.size; i++) {
    move(moves.data[i]);
    nodes += perft(depth - 1);
    undo();
  }

  return nodes;
}

void Board::load_fen(std::string_view fen) {
  turn_ = {};
  castling_rights_ = {};
  king_tiles_ = {};
  enpassant_tile_ = -1;
  tiles_ = {};
  is_in_check_ = false;
  is_in_checkmate_ = false;
  is_in_draw_ = false;
  records_ = {};

  std::array<std::string_view, 6> parts{};
  for (int i = 0, begin = 0, end = 0; i < 6; i++) {
    begin = end;
    end = static_cast<int>(fen.find_first_of(' ', begin + 1));
    parts[i] = fen.substr(begin, end - begin);
    end++;  // Skip whitespace
  }

  for (int i = 0, tile = 0; i < parts[0].length(); i++) {
    const char ch{parts[0][i]};

    if (ch == '/') {
      continue;
    }

    if (ch >= '0' && ch <= '8') {
      tile += ch - '0';
      continue;
    }

    auto color{PieceColor::Black};
    if (ch >= 'A' && ch < 'Z') {
      color = PieceColor::White;
    }

    const int tile_rot{8 * (7 - get_tile_row(tile)) + get_tile_column(tile)};

    PieceType type{};
    switch (ch) {
      case 'K':
        king_tiles_[1] = tile_rot;
        [[fallthrough]];
      case 'k':
        type = PieceType::King;
        if (ch == 'K') {
          break;
        }
        king_tiles_[0] = tile_rot;
        break;
      case 'Q':
      case 'q':
        type = PieceType::Queen;
        break;
      case 'B':
      case 'b':
        type = PieceType::Bishop;
        break;
      case 'N':
      case 'n':
        type = PieceType::Knight;
        break;
      case 'R':
      case 'r':
        type = PieceType::Rook;
        break;
      case 'P':
      case 'p':
        type = PieceType::Pawn;
        break;
      default:
        break;
    }

    if (type != PieceType::None) {
      set_tile(tile_rot, make_piece(color, type));
    }
    tile++;
  }

  if (parts[1] == "w") {
    turn_ = PieceColor::White;
  } else if (parts[1] == "b") {
    turn_ = PieceColor::Black;
  }

  auto set_castling_right = [this](int index, CastlingRight right) {
    castling_rights_[index] = static_cast<CastlingRight>(
        to_underlying(castling_rights_[index]) | to_underlying(right));
  };

  for (const char ch : parts[2]) {
    switch (ch) {
      case 'K':
        set_castling_right(1, CastlingRight::Short);
        break;
      case 'k':
        set_castling_right(0, CastlingRight::Short);
        break;
      case 'Q':
        set_castling_right(1, CastlingRight::Long);
        break;
      case 'q':
        set_castling_right(0, CastlingRight::Long);
        break;
      default:
        break;
    }
  }

  if (parts[3] != "-") {
    enpassant_tile_ = 8 * (parts[3][1] - '0' - 1) + (parts[3][0] - 'a');
  }
}

void Board::move(Move move) {
  assert(get_color(move.tile) != PieceColor::None &&
         get_type(move.tile) != PieceType::None);

  const MoveRecord& record{records_.emplace_back(
      move, move.promotion, get_tile(move.target), castling_rights_,
      enpassant_tile_, is_in_check_, is_in_checkmate_, is_in_draw_)};
  set_tile(move.target, get_tile(move.tile));
  set_tile(move.tile, {});

  turn_ = get_opposite_color(turn_);
  enpassant_tile_ = -1;

  auto clear_castling_rights = [this](int tile, PieceColor color) {
    auto clear_castling_right = [this](int color_index, CastlingRight right) {
      castling_rights_[color_index] = static_cast<CastlingRight>(
          to_underlying(castling_rights_[color_index]) & ~to_underlying(right));
    };

    switch (tile) {
      case 0:
        if (color == PieceColor::White) {
          clear_castling_right(1, CastlingRight::Long);
        }
        break;
      case 7:
        if (color == PieceColor::White) {
          clear_castling_right(1, CastlingRight::Short);
        }
        break;
      case 56:
        if (color == PieceColor::Black) {
          clear_castling_right(0, CastlingRight::Long);
        }
        break;
      case 63:
        if (color == PieceColor::Black) {
          clear_castling_right(0, CastlingRight::Short);
        }
        break;
      default:
        break;
    }
  };

  const uint8_t color_index{get_color_index(get_color(move.target))};
  if (castling_rights_[color_index] != CastlingRight::None &&
      get_piece_type(record.captured_piece) == PieceType::Rook) {
    clear_castling_rights(move.target, get_piece_color(record.captured_piece));
  }

  switch (get_type(move.target)) {
    case PieceType::King:
      if (glm::abs(move.target - move.tile) == 2) {
        const int rook_tile{move.tile + (move.tile < move.target ? 3 : -4)};
        set_tile((move.tile + move.target) / 2, get_tile(rook_tile));
        set_tile(rook_tile, {});
      }
      castling_rights_[color_index] = CastlingRight::None;
      king_tiles_[color_index] = move.target;
      break;
    case PieceType::Rook:
      if (castling_rights_[color_index] != CastlingRight::None) {
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

bool Board::has_legal_moves() {
  Moves moves;
  for (int tile = 0; tile < 64; tile++) {
    generate_legal_moves(moves, tile);
    if (moves.size != 0) {
      return true;
    }
  }
  return false;
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

  auto add_move = [this](Moves& moves, int tile, int target) {
    assert(get_color(tile) != PieceColor::None);
    if (get_color(tile) != get_color(target)) {
      moves.data[moves.size++] = {tile, target};
    }
  };

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
      if (tile == 4 && castling_rights_[1] != CastlingRight::None &&
          !is_threatened(4, PieceColor::Black)) {
        CHECK_MOVE_OFFSET(2, is_piece(7, PieceColor::White, PieceType::Rook) &&
                                 (castling_rights_[1] == CastlingRight::Short ||
                                  castling_rights_[1] == CastlingRight::Both) &&
                                 is_empty(5) && is_empty(6) &&
                                 !is_threatened(5, PieceColor::Black) &&
                                 !is_threatened(6, PieceColor::Black));
        CHECK_MOVE_OFFSET(-2,
                          is_piece(0, PieceColor::White, PieceType::Rook) &&
                              (castling_rights_[1] == CastlingRight::Long ||
                               castling_rights_[1] == CastlingRight::Both) &&
                              is_empty(1) && is_empty(2) && is_empty(3) &&
                              !is_threatened(2, PieceColor::Black) &&
                              !is_threatened(3, PieceColor::Black));
      } else if (tile == 60 && castling_rights_[0] != CastlingRight::None &&
                 !is_threatened(60, PieceColor::White)) {
        CHECK_MOVE_OFFSET(2, is_piece(63, PieceColor::Black, PieceType::Rook) &&
                                 (castling_rights_[0] == CastlingRight::Short ||
                                  castling_rights_[0] == CastlingRight::Both) &&
                                 is_empty(61) && is_empty(62) &&
                                 !is_threatened(61, PieceColor::White) &&
                                 !is_threatened(62, PieceColor::White));
        CHECK_MOVE_OFFSET(-2,
                          is_piece(56, PieceColor::Black, PieceType::Rook) &&
                              (castling_rights_[0] == CastlingRight::Long ||
                               castling_rights_[0] == CastlingRight::Both) &&
                              is_empty(57) && is_empty(58) && is_empty(59) &&
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
    case PieceType::Pawn: {
#define CHECK_PAWN_MOVE_OFFSET(offset, condition)      \
  if (const int target = tile + (offset); condition) { \
    add_pawn_move(moves, tile, target);                \
  }

      auto add_pawn_move = [this](Moves& moves, int tile, int target) {
        assert(get_color(tile) != PieceColor::None);
        if (get_color(tile) != get_color(target)) {
          if (target >= 8 && target < 56) {
            moves.data[moves.size++] = {tile, target};
            return;
          }
          moves.data[moves.size++] = {tile, target, PieceType::Queen};
          moves.data[moves.size++] = {tile, target, PieceType::Rook};
          moves.data[moves.size++] = {tile, target, PieceType::Bishop};
          moves.data[moves.size++] = {tile, target, PieceType::Knight};
        }
      };

      switch (get_color(tile)) {
        case PieceColor::Black:
          CHECK_PAWN_MOVE_OFFSET(-8, is_empty(target));
          CHECK_MOVE_OFFSET(
              -16, tile >= 48 && is_empty(tile - 8) && is_empty(target));
          CHECK_PAWN_MOVE_OFFSET(
              -7, col < 7 && (get_color(target) == PieceColor::White ||
                              (target == enpassant_tile_ &&
                               get_color(target) == PieceColor::None)));
          CHECK_PAWN_MOVE_OFFSET(
              -9, col > 0 && (get_color(target) == PieceColor::White ||
                              (target == enpassant_tile_ &&
                               get_color(target) == PieceColor::None)));
          break;
        case PieceColor::White:
          CHECK_PAWN_MOVE_OFFSET(8, is_empty(target));
          CHECK_MOVE_OFFSET(
              16, tile < 16 && is_empty(tile + 8) && is_empty(target));
          CHECK_PAWN_MOVE_OFFSET(
              7, col > 0 && (get_color(target) == PieceColor::Black ||
                             (target == enpassant_tile_ &&
                              get_color(target) == PieceColor::None)));
          CHECK_PAWN_MOVE_OFFSET(
              9, col < 7 && (get_color(target) == PieceColor::Black ||
                             (target == enpassant_tile_ &&
                              get_color(target) == PieceColor::None)));
          break;
        default:
          break;
      }
    }

#undef CHECK_PAWN_MOVE_OFFSET
    break;
    default:
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

  CHECK_THREAT_DIRECTION(
      8, target < 64,
      target_type == PieceType::Queen || target_type == PieceType::Rook ||
          (target_type == PieceType::King && target == tile + 8));
  CHECK_THREAT_DIRECTION(
      -8, target >= 0,
      target_type == PieceType::Queen || target_type == PieceType::Rook ||
          (target_type == PieceType::King && target == tile - 8));
  CHECK_THREAT_DIRECTION(
      -1, target >= tile - col,
      target_type == PieceType::Queen || target_type == PieceType::Rook ||
          (target_type == PieceType::King && target == tile - 1));
  CHECK_THREAT_DIRECTION(
      1, target < tile - col + 8,
      target_type == PieceType::Queen || target_type == PieceType::Rook ||
          (target_type == PieceType::King && target == tile + 1));
  CHECK_THREAT_DIRECTION(
      7, target < 64 && get_tile_column(target) != 7,
      target_type == PieceType::Queen || target_type == PieceType::Bishop ||
          (target == tile + 7 && (target_type == PieceType::King ||
                                  (target_type == PieceType::Pawn &&
                                   target_color == PieceColor::Black))));
  CHECK_THREAT_DIRECTION(
      9, target < 64 && get_tile_column(target) != 0,
      target_type == PieceType::Queen || target_type == PieceType::Bishop ||
          (target == tile + 9 && (target_type == PieceType::King ||
                                  (target_type == PieceType::Pawn &&
                                   target_color == PieceColor::Black))));
  CHECK_THREAT_DIRECTION(
      -7, target >= 0 && get_tile_column(target) != 0,
      target_type == PieceType::Queen || target_type == PieceType::Bishop ||
          (target == tile - 7 && (target_type == PieceType::King ||
                                  (target_type == PieceType::Pawn &&
                                   target_color == PieceColor::White))));
  CHECK_THREAT_DIRECTION(
      -9, target >= 0 && get_tile_column(target) != 7,
      target_type == PieceType::Queen || target_type == PieceType::Bishop ||
          (target == tile - 9 && (target_type == PieceType::King ||
                                  (target_type == PieceType::Pawn &&
                                   target_color == PieceColor::White))));

#undef CHECK_THREAT_DIRECTION

  return false;
}
