#pragma once
#include <optional>
#include <vector>

#include "Chance.hpp"
#include "Game.hpp"
#include "Move.hpp"
#include "Piece.hpp"

class Tetris;

enum Outcomes {
    P1_WIN,
    P2_WIN,
    DRAW,
    NONE
};

class VersusGame {
   public:
    Game p1_game;
    Game p2_game;

    Chance c_move = Chance();
    Move p1_move = Move(Piece(PieceType::Empty), false);
    Move p2_move = Move(Piece(PieceType::Empty), false);

    double p1_atk = 0;
    double p2_atk = 0;

    int p1_meter = 0;
    int p2_meter = 0;

    int turn = 0;
    bool game_over = false;

    // get attack per piece
    double get_app(int id) const {
        return id == 0 ? p1_atk / turn : p2_atk / turn;
    }

    double get_b2b(int id) const {
        return id == 0 ? p1_game.b2b : p2_game.b2b;
    }

    const Game& get_game(int id) const {
        return id == 0 ? p1_game : p2_game;
    }

    void set_move(int id, Move move);

    void play_moves();

    VersusGame play_moves_not_inplace(void);

    std::vector<Move> get_moves(int id) const;
    std::vector<Move> get_N_moves(int id, int N) const;

    Outcomes get_winner() const;

    friend class Tetris;
};