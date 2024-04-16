#include "EmulationGame.hpp"

#include "fasthash.h"
#include <iostream>
void EmulationGame::set_move(Move move) {
    this->move = move;
};

void EmulationGame::chance_move() {
    chance.new_move(false, false);
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
    if (!move.null_move) {
        game.place_piece(move.piece);
        pieces += 1;
    }
    int cleared_lines = game.board.clearLines();

    bool pc = true;
    for (int i = 0; i < Board::width; i++) {
        if (game.board.get_column(i) != 0) {
            pc = false;
        }
    }

    // you can't pc by doing nothing lol
    if (move.null_move) {
        pc = false;
    }


    int damage = game.damage_sent(cleared_lines, spin, pc);

    attack += damage;

    // cancel damage but never send cause the opponent doesnt exist
    while (damage > 0 && garbage_meter.size() > 0) {
        int &incoming = garbage_meter.back();

        if (incoming >= damage) {
            incoming -= damage;
            damage = 0;
        } else {
            damage -= incoming;
            incoming = 0;
        }

        if (incoming == 0) {
            garbage_meter.pop_back();
        }
    }

    // combo block
    if (cleared_lines == 0) {

        int garbage_used = 0;

        // place garbage
        while (garbage_meter.size() > 0) {
            int& incoming = garbage_meter.back();

            int garbage = std::min(incoming, 8 - garbage_used);

            game.add_garbage(garbage, chance.get_garbage_column());

            incoming -= garbage;
            garbage_used += garbage;

            if (incoming == 0) {
                garbage_meter.pop_back();
            }

            if (garbage_used == 8) {
                break;
            }
        }
    }

    true_attack += damage;

    for (auto& piece : game.queue) {
        if (piece == PieceType::Empty) {
            piece = chance.rng.getPiece();
        }
    }

    // garbage delay
    if (chance.garbage_amount > 0) {
        garbage_meter.insert(garbage_meter.begin(), chance.garbage_amount);
    }

    game.app = app();

    chance_move();
};

std::vector<Move> EmulationGame::legal_moves() {
    std::vector<Piece> raw_actions = game.get_possible_piece_placements();

    std::vector<Move> moves;

    for (auto& raw_action : raw_actions) {
        //moves.push_back(Move(raw_action, true));
        moves.push_back(Move(raw_action, false));
    }

    return moves;
}

uint32_t EmulationGame::hash() const {
    uint32_t hash = fasthash32(&game.board, sizeof(Board), 0x1234567890123456);
    hash = fasthash32(&move.piece, sizeof(Piece), hash);
    if (game.hold)
        hash = fasthash32(&*game.hold, sizeof(*game.hold), hash);
    for (int garbage : garbage_meter) {
        hash = fasthash32(&garbage, sizeof(garbage), hash);
    }
    return hash;
}

float EmulationGame::app() {
    return (float) attack / (float) pieces;
}


float EmulationGame::true_app() {
    return (float)true_attack / (float)pieces;
}

float EmulationGame::b2b() {
    return (float) game.b2b; 
}