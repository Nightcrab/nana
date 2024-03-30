#pragma once
#include "Board.hpp"
#include "rng.hpp"

class Chance {
   public:
    Chance() {
        p1_garbage_column = 0;
        p2_garbage_column = 0;
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

    RNG rng1;
    RNG rng2;

    Piece p1_next_piece = rng1.getPiece();
    Piece p2_next_piece = rng2.getPiece();

    void new_move() {
        p1_next_piece = rng1.getPiece();
        p2_next_piece = rng2.getPiece();
        p1_garbage_column = rng1.getRand(Board::width);
        p2_garbage_column = rng2.getRand(Board::width);
        garbage_amount = rng1.getRand(4);
    };
};