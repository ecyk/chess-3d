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

inline constexpr PieceColor piece_color(Piece piece) {
  return static_cast<PieceColor>(to_underlying(piece) & 24U);
}

inline constexpr PieceType piece_type(Piece piece) {
  return static_cast<PieceType>(to_underlying(piece) & 7U);
}

inline constexpr Piece make_piece(PieceColor color, PieceType type) {
  return static_cast<Piece>(to_underlying(color) | to_underlying(type));
}
