#pragma once
#include "Piece.hpp"

#include <array>
#include <cstdint>

constexpr uint32_t BOARD_WIDTH = 10;
constexpr uint32_t BOARD_HEIGHT = 32;

class Board
{
public:
	Board() {
		board.fill(0);
	}

	~Board() = default;

	Board(const Board& other) = default;

	Board(Board&& other) noexcept = default;
	Board & operator=(const Board& other) = default;

	int get(size_t x, size_t y) const;
	int get_column(size_t x) const;

	void set(size_t x, size_t y);

	void unset(size_t x, size_t y);

	void set(const Piece& piece);

	// taken from:
	// https://stackoverflow.com/questions/21144237/standard-c11-code-equivalent-to-the-pext-haswell-instruction-and-likely-to-be
	int32_t extract_bits(int32_t x, int32_t mask);

	int clearLines();

	std::array<uint32_t, BOARD_WIDTH> board;
};