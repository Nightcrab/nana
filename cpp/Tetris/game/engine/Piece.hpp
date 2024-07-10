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
        x_minos = rot_piece_def_x[static_cast<size_t>(rotation)][static_cast<size_t>(type)];
        y_minos = rot_piece_def_y[static_cast<size_t>(rotation)][static_cast<size_t>(type)];
        spin = spinType::null;
    }
    constexpr Piece(PieceType type, RotationDirection dir) noexcept {
        this->type = type;
        rotation = dir;
        position = { 10 / 2 - 1, 20 - 2 };
        x_minos = rot_piece_def_x[static_cast<size_t>(rotation)][static_cast<size_t>(type)];
        y_minos = rot_piece_def_y[static_cast<size_t>(rotation)][static_cast<size_t>(type)];
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
        x_minos = rot_piece_def_x[static_cast<size_t>(rotation)][static_cast<size_t>(type)];
        y_minos = rot_piece_def_y[static_cast<size_t>(rotation)][static_cast<size_t>(type)];

        return;

        calculate_rotate(direction);
    }

    constexpr inline void calculate_rotate(TurnDirection direction) {
        if (direction == TurnDirection::Left) {
            rotation = static_cast<RotationDirection>((static_cast<int>(rotation) + (n_minos - 1)) % n_minos);
            for (size_t i = 0; i < n_minos; ++i) {

                Coord temp_mino = { x_minos[i], y_minos[i] };
                temp_mino.y *= -1;
                
                std::swap(temp_mino.x, temp_mino.y);

                x_minos[i] = temp_mino.x;
                y_minos[i] = temp_mino.y;
            }
        }
        else {
            rotation = static_cast<RotationDirection>((static_cast<int>(rotation) + 1) % n_minos);
            for (size_t i = 0; i < n_minos; ++i) {

                Coord temp_mino = { x_minos[i], y_minos[i] };
                temp_mino.x *= -1;

                std::swap(temp_mino.x, temp_mino.y);

                x_minos[i] = temp_mino.x;
                y_minos[i] = temp_mino.y;
            }
        }
    }

    constexpr inline uint32_t hash() const {
        return ((int)type << 24) | (position.x << 16) | (position.y << 8) | (rotation);
    }

    constexpr inline uint32_t compact_hash() const {
        return rotation + position.x * n_minos + position.y * 10 * n_minos + (int)type * 10 * 20 * n_minos;
    }

    std::array<int8_t, n_minos> x_minos;
    std::array<int8_t, n_minos> y_minos;
    alignas(16) Coord position;
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
                if ((first.x_minos[mino] != second.x_minos[mino]) ||
                    (first.y_minos[mino] != second.y_minos[mino]))
                        return false;

            }

            first.calculate_rotate(TurnDirection::Right);
        }
    }
    return true;
}

// you will see this as a compile time error if it didnt work
static_assert(piece_test_1(), "piece_test_1 didnt work");
