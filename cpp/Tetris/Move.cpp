#pragma once

#include "Move.hpp"
#include "Piece.hpp"


Piece Move::first() {
	return this->piece;
}


bool Move::second() {
	return this->null_move;
}


uint32_t Move::hash() const {

	if (!this->null_move) {
		return 0b01010101010101;
	}
	return (this->piece).hash();
}


std::pair<Piece, bool> Move::as_pair(){
	return std::make_pair(piece, null_move);
}