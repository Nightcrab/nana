#pragma once

#include <array>
#include <algorithm>
#include <utility>
#include <random>
#include "TetrisConstants.hpp"


typedef uint_fast8_t u8;   ///<   8-bit unsigned integer.
typedef uint_fast16_t u16; ///<  16-bit unsigned integer.
typedef uint_fast32_t u32; ///<  32-bit unsigned integer.
typedef uint_fast64_t u64; ///<  64-bit unsigned integer.

class pptRNG {
public:
    pptRNG() {
        std::mt19937 mt;
        mt.seed(std::random_device()());
        PPTRNG = mt();
        makebag();
    }
    u32 PPTRNG;
    std::array<PieceType, 7> bag;
    uint8_t bagiterator;

    PieceType getPiece();
    u32 GetRand(u32 upperBound);

    void makebag();

};