#pragma once

#include "Piece.hpp"
#include "TetrisConstants.hpp"
#include <optional>

class Move {
public:
	Move() {
		this->piece = Piece(PieceType::Empty);
		this->null_move = true;
	}

	Move(Piece piece, bool null_move) {
		this->piece = piece;
		this->null_move = false;
	}

	Move(std::optional<Piece> piece) {
		if (!piece) {
			this->piece = Piece(PieceType::Empty);
			this->null_move = true;
		}
		else {
			this->piece = *piece;
			this->null_move = false;
		}
	}

	Move(std::pair<Piece, bool>&& move_pair) {
		this->piece = move_pair.first;
		this->null_move = move_pair.second;
	}

	Piece piece = Piece(PieceType::Empty);
	bool null_move;

	Piece first();
	bool second();

	std::pair<Piece, bool> as_pair();

	uint32_t hash() const;

	bool operator <(const Move& rhs) const
	{
		return this->hash() < rhs.hash();
	}
};