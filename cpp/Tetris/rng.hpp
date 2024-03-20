#pragma once

#include <array>
#include <algorithm>
#include <utility>
#include <random>
#include "TetrisConstants.hpp"



class RNG {
public:
    RNG() {
        PPTRNG = std::random_device()();
        makebag();
    }
    u32 PPTRNG;
    std::array<PieceType, 7> bag;
    uint8_t bagiterator;

    PieceType getPiece();
    u32 getRand(u32 upperBound);

    void makebag();

};