#pragma once
#include "Piece.hpp"

#include <array>
#include <cstdint>
#include <bit>


class Board
{
public:
	static constexpr size_t width = 10;
	static constexpr size_t visual_height = 20;
	static constexpr size_t height = 32;
	Board() {
		board.fill(0);
	}

	~Board() = default;

	Board(const Board& other) = default;

	Board(Board&& other) noexcept = default;
	Board& operator=(const Board& other) = default;

	int get(size_t x, size_t y) const {
		return (board[x] & (1 << y)) != 0;
	}
	int get_column(size_t x) const
	{
		return board[x];
	}

	void set(size_t x, size_t y) {
		board[x] |= (1 << y);
	}

	void unset(size_t x, size_t y) {
		board[x] &= ~(1 << y);
	}

	void set(const Piece& piece) {
		for (const Coord& mino : piece.minos) {
			set(
				(size_t)mino.x + piece.position.x,
				(size_t)mino.y + piece.position.y
			);
		}
	}

	// taken from:
	// https://stackoverflow.com/questions/21144237/standard-c11-code-equivalent-to-the-pext-haswell-instruction-and-likely-to-be
	int32_t extract_bits(int32_t x, int32_t mask) {
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

	int clearLines() {
		uint32_t mask = UINT32_MAX;
		for (uint32_t& column : board)
			mask &= column;
		int lines_cleared = std::popcount(mask);
		mask = ~mask;

		for (uint32_t& column : board)
			column = extract_bits(column, mask);

		return lines_cleared;
	}


	std::array<uint32_t, Board::width> board;
};