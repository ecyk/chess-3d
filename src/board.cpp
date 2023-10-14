#include "board.hpp"

Board::Board() {
  active_tiles_.reserve(32);
  reset();
}

void Board::reset() {
  tiles_.fill({});

  auto set_pieces = [this](PieceColor color, int piece_row, int pawn_row) {
    auto set_tile = [this](int x, int y, uint8_t unique, PieceColor color,
                           PieceType type) {
      unique <<= 5U;
      at(x, y) = static_cast<Piece>(unique | to_underlying(color) |
                                    to_underlying(type));
    };

    set_tile(0, piece_row, 0, color, PieceType::Rook);
    set_tile(1, piece_row, 0, color, PieceType::Knight);
    set_tile(2, piece_row, 0, color, PieceType::Bishop);
    set_tile(3, piece_row, 0, color, PieceType::Queen);
    set_tile(4, piece_row, 0, color, PieceType::King);
    set_tile(5, piece_row, 1, color, PieceType::Bishop);
    set_tile(6, piece_row, 1, color, PieceType::Knight);
    set_tile(7, piece_row, 1, color, PieceType::Rook);

    for (int i = 0; i < 8; ++i) {
      set_tile(i, pawn_row, i, color, PieceType::Pawn);
    }
  };

  set_pieces(PieceColor::Black, 0, 1);
  set_pieces(PieceColor::White, 7, 6);

  update_active_tiles();
}

void Board::update_active_tiles() {
  active_tiles_.clear();

  for (int i = 0; i < 64; ++i) {
    const int x{i % 8};
    const int y{i / 8};

    const Piece piece = at(x, y);
    if (is_piece_type(piece, PieceType::None)) {
      continue;
    }

    active_tiles_.push_back({x, y, piece});
  }
}
