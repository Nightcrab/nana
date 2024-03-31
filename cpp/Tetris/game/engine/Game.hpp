#pragma once
#include <array>
#include <cmath>
#include <optional>
#include <ranges>
#include <vector>

#include "Board.hpp"
#include "Move.hpp"
#include "Piece.hpp"
#include "TetrisConstants.hpp"

enum damage_type { classic_guideline,
                   modern_guideline };
namespace GarbageValues {
const int SINGLE = 0;
const int DOUBLE = 1;
const int TRIPLE = 2;
const int QUAD = 4;
const int TSPIN_MINI = 0;
const int TSPIN = 0;
const int TSPIN_MINI_SINGLE = 0;
const int TSPIN_SINGLE = 2;
const int TSPIN_MINI_DOUBLE = 1;
const int TSPIN_DOUBLE = 4;
const int TSPIN_TRIPLE = 6;
const int TSPIN_QUAD = 10;
const int BACKTOBACK_BONUS = 1;
const float BACKTOBACK_BONUS_LOG = .8f;
const int COMBO_MINIFIER = 1;
const float COMBO_MINIFIER_LOG = 1.25;
const float COMBO_BONUS = .25;
const int ALL_CLEAR = 10;
const std::array<std::array<int, 13>, 2> combotable = {{{0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5},
                                                        {0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4}}};
}  // namespace GarbageValues

constexpr int QUEUE_SIZE = 5;

class Game {
   public:
    Game() : current_piece(PieceType::Empty) {
        for (auto& p : queue) {
            p = PieceType::Empty;
        }
    }
    Game& operator=(const Game& other) {
        if (this != &other) {
            board = other.board;
            current_piece = other.current_piece;
            hold = other.hold;
            queue = other.queue;
            garbage_meter = other.garbage_meter;
            b2b = other.b2b;
            combo = other.combo;
        }
        return *this;
    }
    ~Game() {}

    void place_piece();

    void place_piece(Piece& piece);

    void do_hold();

    bool collides(const Board& board, const Piece& piece) const;

    void rotate(Piece& piece, TurnDirection dir) const;

    void shift(Piece& piece, int dir) const;

    void sonic_drop(const Board board, Piece& piece) const;

    void add_garbage(int lines, int location);

    int damage_sent(int linesCleared, spinType spinType, bool pc);

    void process_movement(Piece& piece, Movement movement) const;

    std::vector<Piece> movegen(PieceType piece_type) const;

    std::vector<Piece> get_possible_piece_placements() const;

    Board board;
    Piece current_piece;
    std::optional<Piece> hold;
    int garbage_meter = 0;
    int b2b = 0;
    int combo = 0;

    // tetrio stuff
    int currentcombopower = 0;
    int currentbtbchainpower = 0;

    const struct options {
        int garbagemultiplier = 1;
        bool b2bchaining = true;
    } options;
    std::array<PieceType, QUEUE_SIZE> queue;
};