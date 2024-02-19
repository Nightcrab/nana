#pragma once

#include <array>
#include <cstdint>

typedef uint_fast8_t u8;   ///<   8-bit unsigned integer.
typedef uint_fast16_t u16; ///<  16-bit unsigned integer.
typedef uint_fast32_t u32; ///<  32-bit unsigned integer.
typedef uint_fast64_t u64; ///<  64-bit unsigned integer.

enum class spinType {
    null,
    mini,
    normal,
};
struct Coord
{
    int_fast8_t x;
    int_fast8_t y;
};

enum RotationDirection : uint_fast8_t
{
    North,
    East,
    South,
    West,
    RotationDirections_N
};

enum ColorType : uint_fast8_t
{

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

enum class PieceType : uint_fast8_t
{
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

enum TurnDirection : uint_fast8_t
{
    Left,
    Right,
};

enum class Movement: uint_fast8_t {
    Left,
    Right,
    RotateClockwise,
    RotateCounterClockwise,
    SonicDrop,
};

// number of kicks srs has, including for initial
constexpr auto srs_kicks = 5;

constexpr std::array<std::array<Coord, srs_kicks>, RotationDirections_N> piece_offsets_JLSTZ = { {
    {{{0, 0}, { 0, 0}, { 0,  0}, {0, 0}, { 0, 0}}},
    {{{0, 0}, { 1, 0}, { 1, -1}, {0, 2}, { 1, 2}}},
    {{{0, 0}, { 0, 0}, { 0,  0}, {0, 0}, { 0, 0}}},
    {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}},
} };

constexpr std::array<std::array<Coord, srs_kicks>, RotationDirections_N> piece_offsets_O = { {
   {{{0, 0}}},
   {{{0, -1}}},
   {{{-1, -1}}},
    {{{-1, 0}}},
} };

constexpr std::array<std::array<Coord, srs_kicks>, RotationDirections_N> piece_offsets_I = { {

    {{{0,  0}, {-1, 0}, { 2, 0}, {-1,  0}, { 2, 0 }}},
    {{{-1, 0}, {0,  0}, { 0, 0}, { 0,  1}, { 0, -2}}},
    {{{-1, 1}, {1,  1}, {-2, 1}, { 1,  0}, {-2, 0 }}},
    {{{0,  1}, {0,  1}, { 0, 1}, { 0, -1}, { 0, 2 }}},
} };

constexpr std::array<std::array<Coord, 4>, (int)PieceType::PieceTypes_N> piece_definitions = {

    {
        {{{-1, 0}, {0, 0}, {0, 1}, {1, 1}}},  // S
        {{{-1, 1}, {0, 1}, {0, 0}, {1, 0}}},  // Z
        {{{-1, 0}, {0, 0}, {1, 0}, {-1, 1}}}, // J
        {{{-1, 0}, {0, 0}, {1, 0}, {1, 1}}},  // L
        {{{-1, 0}, {0, 0}, {1, 0}, {0, 1}}},  // T
        {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}},   // O
        {{{-1, 0}, {0, 0}, {1, 0}, {2, 0}}},  // I
        {{{0, 0}, {0, 0}, {0, 0}, {0, 0}}}    // NULL
    } };