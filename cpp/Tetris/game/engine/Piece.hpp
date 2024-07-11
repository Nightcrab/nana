#pragma once
#include <vector>
#include <cstddef>

#include "TetrisConstants.hpp"

class Piece {
public:
    constexpr Piece(PieceType type) noexcept {
        this->type = type;
        rotation = RotationDirection::North;
        position = { 10 / 2 - 1, 20 - 2 };
        minos = piece_definitions[static_cast<size_t>(type)];
        spin = spinType::null;
    }
    constexpr Piece(PieceType type, RotationDirection dir) noexcept {
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
        if (direction == TurnDirection::Left) {
            rotation = static_cast<RotationDirection>((static_cast<int>(rotation) + (n_minos - 1)) % n_minos);
        }
        else {
            rotation = static_cast<RotationDirection>((static_cast<int>(rotation) + 1) % n_minos);
		}
        minos = rot_piece_def[static_cast<size_t>(rotation)][static_cast<size_t>(type)];

        return;

        calculate_rotate(direction);
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


// compile time unit tests for sanity
consteval bool piece_test_1() {
    for (size_t type = 0; type < static_cast<size_t>(PieceType::PieceTypes_N); ++type)
    {
        Piece first(static_cast<PieceType>(type));
        for (size_t rot = 0; rot < RotationDirection::RotationDirections_N; ++rot) {
            Piece second(static_cast<PieceType>(type), static_cast<RotationDirection>(rot));

            for (size_t mino = 0; mino < n_minos; ++mino) {
                if ((first.minos[mino].x != second.minos[mino].x) ||
                    (first.minos[mino].y != second.minos[mino].y))
                        return false;

            }

            first.calculate_rotate(TurnDirection::Right);
        }
    }
    return true;
}

// you will see this as a compile time error if it didnt work
static_assert(piece_test_1(), "piece_test_1 didnt work");
