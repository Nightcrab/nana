#pragma once

#include "Piece.hpp"
#include "TetrisConstants.hpp"
#include <optional>

class Move {
public:
	Move() :piece(Piece(PieceType::Empty)){
		this->null_move = true;
	}

	Move(const Piece &piece, bool null_move) : piece(piece) {
		this->null_move = null_move;
	}

	Move(const std::optional<Piece> &piece) :piece(Piece(PieceType::Empty)) {
		if (!piece) {
			this->null_move = true;
		}
		else {
			this->piece = *piece;
			this->null_move = false;
		}
	}

	Piece piece;
	bool null_move;

	Piece &first();
	bool &second();


	uint32_t hash() const;

	bool operator <(const Move& rhs) const
	{
		return this->hash() < rhs.hash();
	}
};