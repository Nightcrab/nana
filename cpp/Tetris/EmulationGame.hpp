#pragma once
#include "Chance.hpp"
#include "Game.hpp"

#include <stack>

// analogous to an APM survival mode
// simulates a versus game with a highly simplified opponent model
class EmulationGame {
   public:
    Game game;

    std::stack<int> garbage_meter;

    // can be set if the opponent already has some B2B
    float multiplier = 1.0;

    // can use below if we know opponent is combo'ing and likely to extend
    // opponent's existing combo
    int combo = 0;
    // longest combo extension possible
    int combo_length = 0;

    Move move;
    Chance chance;

    bool game_over = false;

    void set_move(Move move);

    void chance_move();

    void play_moves();

    std::vector<Move> legal_moves();

    int hash();
};