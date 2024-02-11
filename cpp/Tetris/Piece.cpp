#include "Piece.hpp"

void Piece::rotate(TurnDirection direction) {
	if (direction == TurnDirection::Left) {
		rotation = static_cast<RotationDirection>((static_cast<int>(rotation) + 3) % 4);
		for (auto& mino : minos) {
			Coord temp_mino = mino;
			temp_mino.y *= -1;
			mino = { temp_mino.y, temp_mino.x };
		}
	}
	else {
		rotation = static_cast<RotationDirection>((static_cast<int>(rotation) + 1) % 4);
		for (auto& mino : minos) {
			Coord temp_mino = mino;
			temp_mino.x *= -1;
			mino = { temp_mino.y, temp_mino.x };
		}
	}
}

uint32_t Piece::hash() const
{
	return ((int)type << 24)
		| (position.x << 16)
		| (position.y << 8)
		| (rotation);
}
