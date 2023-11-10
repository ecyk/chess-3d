#include "board.hpp"

Board::Board() { reset(); }

void Board::move_to(int tile, int target) {
  set_tile(target, get_tile(tile));
  set_tile(tile, {});
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
    init_piece(8 * piece_row + 3, color, PieceType::King);
    init_piece(8 * piece_row + 4, color, PieceType::Queen);
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
