#pragma once
#include "engine/Board.hpp"
#include "util/rng.hpp"

class Chance {
   public:
    Chance() {
        p1_garbage_column = 0;
        p2_garbage_column = 0;
        new_move(false, false);
    }
    Chance(int column1, int column2) {
        p1_garbage_column = column1;
        p2_garbage_column = column2;
    }
    Chance(int column1, int column2, Chance& c_move) {
        p1_garbage_column = column1;
        p2_garbage_column = column2;
        rng_1 = c_move.rng_1;
        rng_2 = c_move.rng_2;
    }

    int garbage_amount;
    int time = 0;

    RNG rng_1;
    RNG rng_2;
    
    u8 p1_garbage_column;
    u8 p2_garbage_column;

    PieceType p1_next_piece;
    PieceType p2_next_piece;

    PieceType p1_extra_piece;
    PieceType p2_extra_piece;

    int get_garbage_column() {
        return rng_1.getRand(Board::width);
    }

    void reset_rng() {
        rng_1.new_seed();
    }

    void new_move(bool p1_first_hold, bool p2_first_hold) {
        p1_next_piece = rng_1.getPiece();
        p2_next_piece = rng_2.getPiece();
        if(p1_first_hold) {
			p1_extra_piece = p1_next_piece;
			p1_next_piece = rng_1.getPiece();
		}

        if(p2_first_hold) {
            p2_extra_piece = p2_next_piece;
            p2_next_piece = rng_2.getPiece();
        }

        time++;

        p1_garbage_column = rng_1.getRand(Board::width);
        p2_garbage_column = rng_1.getRand(Board::width);
    };
};