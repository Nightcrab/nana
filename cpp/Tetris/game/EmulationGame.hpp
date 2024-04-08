#pragma once
#include "Chance.hpp"
#include "Game.hpp"

#include <queue>

// analogous to an APM survival mode
// simulates a versus game with a highly simplified opponent model
class EmulationGame {
   public:
    Game game;

    std::vector<int> garbage_meter;

    // can be set if the opponent already has some B2B
    float multiplier = 1.0;

    // can use below if we know opponent is combo'ing and likely to extend
    // opponent's existing combo
    int combo = 0;
    // longest combo extension possible
    int combo_length = 0;

    Move move;
    Chance chance;

    int pieces = 0;
    int attack = 0;

    bool game_over = false;

    float app();

    float b2b();

    void set_move(Move move);

    void chance_move();

    void play_moves();

    float eval(Move move);

    std::vector<Move> legal_moves();

    uint32_t hash() const;
};