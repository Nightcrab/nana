#pragma once

#include <array>
#include <algorithm>
#include <utility>
#include <random>
#include "TetrisConstants.hpp"



class pptRNG {
public:
    pptRNG() {
        PPTRNG = std::random_device()();
        makebag();
    }
    u32 PPTRNG;
    std::array<PieceType, 7> bag;
    uint8_t bagiterator;

    PieceType getPiece();
    u32 GetRand(u32 upperBound);

    void makebag();

};