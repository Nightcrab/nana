#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

#include "Piece.hpp"
#include "pext.hpp"

class Board {
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

    uint32_t get_column(size_t x) const {
        return board[x];
    }

    void set(size_t x, size_t y) {
        board[x] |= (1 << y);
    }

    void unset(size_t x, size_t y) {
        board[x] &= ~(1 << y);
    }

    void set(const Piece& piece) {
        for (size_t i = 0; i < n_minos; ++i) {
            set(
                (size_t)piece.x_minos[i] + piece.position.x,
                (size_t)piece.y_minos[i] + piece.position.y);
        }
    }

    

    int clearLines() {
        uint32_t mask = UINT32_MAX;
        for (uint32_t& column : board)
            mask &= column;
        int lines_cleared = std::popcount(mask);
        mask = ~mask;

        for (uint32_t& column : board)
            column = pext(column, mask);

        return lines_cleared;
    }

    int filledRows() {
        uint32_t mask = UINT32_MAX;
        for (uint32_t& column : board)
            mask &= column;

        return std::popcount(mask);
    }

    std::array<uint32_t, Board::width> board;
};