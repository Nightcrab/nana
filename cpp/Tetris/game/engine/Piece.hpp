#pragma once
#include <vector>
#include <cstddef>

#include "TetrisConstants.hpp"

class Piece {
   public:
    Piece(PieceType type) noexcept {
        this->type = type;
        rotation = RotationDirection::North;
        position = {10 / 2 - 1, 20 - 2};
        minos = piece_definitions[static_cast<size_t>(type)];
        spin = spinType::null;
    }
    Piece(PieceType type, RotationDirection dir) noexcept {
        this->type = type;
        rotation = dir;
        position = { 10 / 2 - 1, 20 - 2 };
        minos = rot_piece_def[static_cast<size_t>(dir)][static_cast<size_t>(type)];
        spin = spinType::null;
    }

    Piece() = delete;
    // Copy constructor
    Piece(const Piece& other) noexcept = default;
    // Move constructor
    Piece(Piece&& other) noexcept = default;
    ~Piece() noexcept = default;
    Piece& operator=(const Piece& other) noexcept = default;


    constexpr inline void rotate(TurnDirection direction) {

        return calculate_rotate(direction);
    }

    constexpr inline void calculate_rotate(TurnDirection direction) {
        if (direction == TurnDirection::Left) {
            rotation = static_cast<RotationDirection>((static_cast<int>(rotation) + (n_minos - 1)) % n_minos);
            for (auto& mino : minos) {
                Coord temp_mino = mino;
                temp_mino.y *= -1;
                mino = { temp_mino.y, temp_mino.x };
            }
        }
        else {
            rotation = static_cast<RotationDirection>((static_cast<int>(rotation) + 1) % n_minos);
            for (auto& mino : minos) {
                Coord temp_mino = mino;
                temp_mino.x *= -1;
                mino = { temp_mino.y, temp_mino.x };
            }
        }
    }

    constexpr inline uint32_t hash() const {
        return ((int)type << 24) | (position.x << 16) | (position.y << 8) | (rotation);
    }

    constexpr inline uint32_t compact_hash() const {
        return rotation + position.x * n_minos + position.y * 10 * n_minos + (int)type * 10 * 20 * n_minos;
    }

    std::array<Coord, n_minos> minos;
    Coord position;
    RotationDirection rotation;
    PieceType type;
    spinType spin;
};