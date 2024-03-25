#include "rng.hpp"

PieceType RNG::getPiece() {
    if (bagiterator == 7) {
        makebag();
    }
    return bag[bagiterator++];
}

u32 RNG::getRand(u32 upperBound) {
    PPTRNG = PPTRNG * 0x5d588b65 + 0x269ec3;
    u32 uVar1 = PPTRNG >> 0x10;
    if (upperBound != 0) {
        uVar1 = uVar1 * upperBound >> 0x10;
    }
    return uVar1;
}

void RNG::makebag() {
    bagiterator = 0;
    u32 buffer = 0;

    std::array<PieceType, 7> pieces = {
        PieceType::S,
        PieceType::Z,
        PieceType::J,
        PieceType::L,
        PieceType::T,
        PieceType::O,
        PieceType::I};

    for (int_fast8_t i = 6; i >= 0; i--) {
        buffer = getRand(i + 1);
        bag[i] = pieces[buffer];
        std::swap(pieces[buffer], pieces[i]);
    }
}
