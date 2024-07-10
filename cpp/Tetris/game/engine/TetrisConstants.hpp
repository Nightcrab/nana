#pragma once

#include <array>
#include <cstdint>

typedef uint8_t u8;    ///<   8-bit unsigned integer.
typedef uint16_t u16;  ///<  16-bit unsigned integer.
typedef uint32_t u32;  ///<  32-bit unsigned integer.
typedef uint64_t u64;  ///<  64-bit unsigned integer.

enum class spinType {
    null,
    mini,
    normal,
};
struct Coord {
    int_fast8_t x;
    int_fast8_t y;
};

enum RotationDirection : uint_fast8_t {
    North,
    East,
    South,
    West,
    RotationDirections_N
};

enum ColorType : uint_fast8_t {

    // Color for pieces
    S,
    Z,
    J,
    L,
    T,
    O,
    I,
    // special types
    Empty,
    LineClear,
    Garbage,
    ColorTypes_N
};

enum class PieceType : uint_fast8_t {
    // actual pieces
    S = ColorType::S,
    Z = ColorType::Z,
    J = ColorType::J,
    L = ColorType::L,
    T = ColorType::T,
    O = ColorType::O,
    I = ColorType::I,
    Empty = ColorType::Empty,
    PieceTypes_N
};

enum TurnDirection : uint_fast8_t {
    Left,
    Right,
};

enum class Movement : uint_fast8_t {
    Left,
    Right,
    RotateClockwise,
    RotateCounterClockwise,
    SonicDrop,
};

// number of kicks srs has, including for initial
constexpr std::size_t srs_kicks = 5;
constexpr std::size_t n_minos = 4;

constexpr std::array<std::array<Coord, srs_kicks>, RotationDirections_N> piece_offsets_JLSTZ = {{
    {{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}},
    {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}},
    {{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}},
    {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}},
}};

constexpr std::array<std::array<Coord, srs_kicks>, RotationDirections_N> piece_offsets_O = {{
    {{{0, 0}}},
    {{{0, -1}}},
    {{{-1, -1}}},
    {{{-1, 0}}},
}};

constexpr std::array<std::array<Coord, srs_kicks>, RotationDirections_N> piece_offsets_I = {{

    {{{0, 0}, {-1, 0}, {2, 0}, {-1, 0}, {2, 0}}},
    {{{-1, 0}, {0, 0}, {0, 0}, {0, 1}, {0, -2}}},
    {{{-1, 1}, {1, 1}, {-2, 1}, {1, 0}, {-2, 0}}},
    {{{0, 1}, {0, 1}, {0, 1}, {0, -1}, {0, 2}}},
}};

constexpr std::array<std::array<Coord, n_minos>, (int)PieceType::PieceTypes_N> piece_definitions = {

    {
        {{{-1, 0}, {0, 0}, {0, 1}, {1, 1}}},   // S
        {{{-1, 1}, {0, 1}, {0, 0}, {1, 0}}},   // Z
        {{{-1, 0}, {0, 0}, {1, 0}, {-1, 1}}},  // J
        {{{-1, 0}, {0, 0}, {1, 0}, {1, 1}}},   // L
        {{{-1, 0}, {0, 0}, {1, 0}, {0, 1}}},   // T
        {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}},    // O
        {{{-1, 0}, {0, 0}, {1, 0}, {2, 0}}},   // I
        {{{0, 0}, {0, 0}, {0, 0}, {0, 0}}}     // NULL
    }};


consteval auto generate_rot_piece_def() {
    std::array<std::array<std::array<Coord, n_minos>, (int)PieceType::PieceTypes_N>, RotationDirection::RotationDirections_N> rot_piece_def{};

    for (size_t type = 0; type < piece_definitions.size(); ++type) {
        auto minos = piece_definitions[type];
        for (size_t rot = 0; rot < RotationDirection::RotationDirections_N; ++rot) {

            for (auto& mino : minos) {
                Coord temp_mino = mino;
                temp_mino.x *= -1;
                mino = { temp_mino.y, temp_mino.x };
            }

            for (size_t mino = 0; mino < n_minos; ++mino) {
                rot_piece_def[rot][type][mino] = minos[mino];
            }
        }
    }

    return rot_piece_def;
}

constexpr std::array<std::array<std::array<Coord, n_minos>, (int)PieceType::PieceTypes_N>, RotationDirection::RotationDirections_N> rot_piece_def = generate_rot_piece_def();