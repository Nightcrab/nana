#pragma once
#include "Board.hpp"
#include "rng.hpp"

class Chance {
   public:
    Chance() {
        p1_garbage_column = 0;
        p2_garbage_column = 0;
        new_move();
    }
    Chance(int column1, int column2) {
        p1_garbage_column = column1;
        p2_garbage_column = column2;
    }
    Chance(int column1, int column2, Chance& c_move) {
        p1_garbage_column = column1;
        p2_garbage_column = column2;
        rng1 = c_move.rng1;
        rng2 = c_move.rng2;
    }

    int p1_garbage_column;
    int p2_garbage_column;

    int garbage_amount;

    RNG rng;
    RNG emulator_rng;

    RNG rng1;
    RNG rng2;

    PieceType p1_next_piece;
    PieceType p2_next_piece;

    int get_garbage_column() {
        return emulator_rng.getRand(Board::width);
    }

    void new_move() {
        p1_next_piece = rng1.getPiece();
        p2_next_piece = rng2.getPiece();
        p1_garbage_column = rng1.getRand(Board::width);
        p2_garbage_column = rng2.getRand(Board::width);
        garbage_amount = emulator_rng.getRand(4);
    };
};