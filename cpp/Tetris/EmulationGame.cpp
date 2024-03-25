#include "EmulationGame.hpp"

void EmulationGame::set_move(Move move) {
    this->move = move;
};

void EmulationGame::chance_move() {
    chance.new_move();
};

void EmulationGame::play_moves(){
    // garbage meter and cancelling calculation and whatnot
};

std::vector<Move> EmulationGame::legal_moves() {
    std::vector<Piece> raw_actions = game.get_possible_piece_placements();

    std::vector<Move> moves;

    for (auto& raw_action : raw_actions) {
        moves.push_back(Move(raw_action, true));
        moves.push_back(Move(raw_action, false));
    }

    return moves;
}

int EmulationGame::hash() {
    return 0;
}