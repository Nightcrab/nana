#pragma once
#include "TetrisConstants.hpp"

class Piece
{
public:
	Piece(PieceType type) {
		this->type = type;
		rotation = RotationDirection::North;
		position = { 4, 18 };
		minos = piece_definitions[static_cast<size_t>(type)];
	}

	Piece() = delete;
	Piece(const Piece& other) = default;
	Piece(Piece&& other) noexcept = default;
	~Piece() = default;
	Piece& operator=(const Piece& other) = default;

	void rotate(TurnDirection direction);
	uint32_t hash() const;
		
	std::array<Coord, 4> minos;
	Coord position;
	RotationDirection rotation;
	PieceType type;
	spinType spin;
};