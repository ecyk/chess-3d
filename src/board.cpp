#include "board.hpp"

Board::Board() {
  active_tiles_.reserve(32);
  reset();
}

void Board::move_to(Piece piece, const glm::ivec2& coord) {
  set_tile(get_coord(piece), {});
  set_tile(coord, piece);
  update_active_tiles();
}

glm::ivec2 Board::get_coord(Piece piece) const {
  assert(!is_piece_type(piece, PieceType::None));

  for (int i = 0; i < 64; ++i) {
    const int x{i % 8};
    const int y{i / 8};

    if (piece == get_tile({x, y})) {
      return {x, y};
    }
  }

  assert(true);
}

void Board::reset() {
  tiles_.fill({});

  auto init_pieces = [this](PieceColor color, int piece_row, int pawn_row) {
    auto init_tile = [this](int x, int y, uint8_t unique, PieceColor color,
                            PieceType type) {
      unique <<= 5U;
      set_tile({x, y}, static_cast<Piece>(unique | to_underlying(color) |
                                          to_underlying(type)));
    };

    init_tile(0, piece_row, 0, color, PieceType::Rook);
    init_tile(1, piece_row, 0, color, PieceType::Knight);
    init_tile(2, piece_row, 0, color, PieceType::Bishop);
    init_tile(3, piece_row, 0, color, PieceType::King);
    init_tile(4, piece_row, 0, color, PieceType::Queen);
    init_tile(5, piece_row, 1, color, PieceType::Bishop);
    init_tile(6, piece_row, 1, color, PieceType::Knight);
    init_tile(7, piece_row, 1, color, PieceType::Rook);

    for (int i = 0; i < 8; ++i) {
      init_tile(i, pawn_row, i, color, PieceType::Pawn);
    }
  };

  init_pieces(PieceColor::White, 0, 1);
  init_pieces(PieceColor::Black, 7, 6);

  update_active_tiles();
}

void Board::update_active_tiles() {
  active_tiles_.clear();

  for (int i = 0; i < 64; ++i) {
    const int x{i % 8};
    const int y{i / 8};

    const Piece piece = get_tile({x, y});
    if (is_piece_type(piece, PieceType::None)) {
      continue;
    }

    active_tiles_.push_back({{x, y}, piece});
  }
}
