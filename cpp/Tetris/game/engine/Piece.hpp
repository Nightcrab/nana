#pragma once
#include <vector>
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

    static constexpr std::vector<std::array<Coord, 4>> minoLUT() {
        std::vector<std::array<Coord, 4>> table = {};
        // how tf to iterate an enum
        for (int t = 0; t < (int) PieceType::PieceTypes_N; ++t) {
            for (int r = 0; r < 4; ++r) {
                Piece piece = Piece((PieceType) t);
                for (int i = 0; i < r; i++) {
                    piece.calculate_rotate(Right);
                }
                table.push_back(piece.minos);
            }
        }
        return table;
    }

    Piece() = delete;
    Piece(const Piece& other) = default;
    Piece(Piece&& other) noexcept = default;
    ~Piece() = default;
    Piece& operator=(const Piece& other) = default;

    void calculate_rotate(TurnDirection direction);
    void rotate(TurnDirection direction);
    uint32_t hash() const;

    std::array<Coord, 4> minos;
    Coord position;
    RotationDirection rotation;
    PieceType type;
    spinType spin;
};