#include "EmulationGame.hpp"

#include "fasthash.h"
void EmulationGame::set_move(Move move) {
    this->move = move;
};

void EmulationGame::chance_move() {
    chance.new_move();
};

void EmulationGame::play_moves(){
    if (game_over) {
        return;
    }

    if (game.collides(game.board, game.current_piece)) {
        game_over = true;
        return;
    }

    // game is not over, do a null move if we have one
    if (move.null_move) {
        return;
    }

    // no damage to be dealt against opponent
    // but we need to keep track of combo and b2b because cancelling

    spinType spin = move.piece.spin;
    game.place_piece(move.piece);
    int cleared_lines = game.board.clearLines();

    bool pc = true;
    for (int i = 0; i < Board::width; i++) {
        if (game.board.get_column(i) != 0) {
            pc = false;
        }
    }

    int damage = game.damage_sent(cleared_lines, spin, pc);

    // cancel damage but never send cause the opponent doesnt exist
    if (garbage_meter.size() > 0) {
        while (damage > 0 && garbage_meter.size() > 0) {
            int &incoming = garbage_meter.front();

            if (incoming >= damage) {
                incoming -= damage;
                damage = 0;
            } else {
                damage -= incoming;
                incoming = 0;
            }

            if (incoming == 0) {
                garbage_meter.pop();
            }

        }
    }
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

uint32_t EmulationGame::hash() {
    uint32_t hash = fasthash32(&game.board, sizeof(Board), 0x1234567890123456);
    hash = fasthash32(&move.piece, sizeof(Piece), hash);
    if (game.hold)
        hash = fasthash32(&*game.hold, sizeof(*game.hold), hash);
    return hash;
}