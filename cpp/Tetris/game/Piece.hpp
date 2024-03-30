#pragma once
#include <cstddef>

#include "TetrisConstants.hpp"

class Piece {
   public:
    Piece(PieceType type) {
        this->type = type;
        rotation = RotationDirection::North;
        position = {10 / 2 - 1, 20 - 2};
        minos = piece_definitions[static_cast<size_t>(type)];
        spin = spinType::null;
    }

    Piece() = delete;
    Piece(const Piece& other) = default;
    Piece(Piece&& other) noexcept = default;
    ~Piece() = default;
    Piece& operator=(const Piece& other) = default;

    void rotate(TurnDirection direction);
    uint32_t hash() const;

    std::array<Coord, 4> minos;
    Coord position;
    RotationDirection rotation;
    PieceType type;
    spinType spin;
};