#pragma once

#include "common.hpp"

enum class PieceColor : uint8_t { None, Black = 8, White = 16 };

enum class PieceType : uint8_t {
  None,
  King,
  Queen,
  Bishop,
  Knight,
  Rook,
  Pawn
};

enum class Piece : uint8_t;

constexpr PieceColor get_piece_color(Piece piece) {
  return static_cast<PieceColor>(to_underlying(piece) & 24U);
}

constexpr uint8_t get_color_index(PieceColor color) {
  assert(color != PieceColor::None);
  return (to_underlying(color) >> 3U) - 1;
}

constexpr PieceColor get_opposite_color(PieceColor color) {
  assert(color != PieceColor::None);
  return color == PieceColor::White ? PieceColor::Black : PieceColor::White;
}

constexpr PieceType get_piece_type(Piece piece) {
  return static_cast<PieceType>(to_underlying(piece) & 7U);
}

constexpr Piece make_piece(PieceColor color, PieceType type) {
  return static_cast<Piece>(to_underlying(color) | to_underlying(type));
}
