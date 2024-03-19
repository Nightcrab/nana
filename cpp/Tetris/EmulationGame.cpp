#include "EmulationGame.hpp"

void EmulationGame::set_move(Move move) {
	this->move = move;
};

void EmulationGame::chance_move() {
	chance.new_move();
};

void EmulationGame::play_moves() {
	// garbage meter and cancelling calculation and whatnot
};