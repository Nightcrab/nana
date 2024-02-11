#include "Board.hpp"
#include <bit>
int Board::get(size_t x, size_t y) const {
	return board[x] & (1 << y);
}

int Board::get_column(size_t x) const
{
	return board[x];
}

void Board::set(size_t x, size_t y) {
	board[x] |= (1 << y);
}

void Board::unset(size_t x, size_t y) {
	board[x] &= ~(1 << y);
}

void Board::set(const Piece& piece) {
	for (const Coord& mino : piece.minos) {
		set(
			(size_t)mino.x + piece.position.x,
			(size_t)mino.y + piece.position.y
		);
	}
}

// taken from:
// https://stackoverflow.com/questions/21144237/standard-c11-code-equivalent-to-the-pext-haswell-instruction-and-likely-to-be
int32_t Board::extract_bits(int32_t x, int32_t mask) {
	int32_t res = 0;
	int bb = 1;

	do {
		int32_t lsb = mask & -mask;
		mask &= ~lsb;
		bool isset = x & lsb;
		res |= isset ? bb : 0;
		bb += bb;
	} while (mask);

	return res;
}

int Board::clearLines() {
	uint32_t mask = UINT32_MAX;
	for (uint32_t& column : board)
		mask &= column;
	int lines_cleared = std::popcount(mask);
	mask = ~mask;

	for (uint32_t& column : board)
		column = extract_bits(column, mask);

	return lines_cleared;
}
