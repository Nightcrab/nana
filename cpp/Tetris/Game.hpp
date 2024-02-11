#pragma once
#include "Board.hpp"
#include "Piece.hpp"
#include "rng.hpp"
#include "TetrisConstants.hpp"

#include <vector>
#include <optional>
#include <array>
#include <ranges>
#include <cmath>


enum damage_type { classic_guideline, modern_guideline };
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
	const std::array< std::array<int, 13>, 2> combotable = { {
		{0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5},
		{0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4}
	} };
}


constexpr int QUEUE_SIZE = 5;

class Game {
public:

	Game()
		:rng(), current_piece(rng.getPiece())
	{
		for (auto& p : queue) {
			p = rng.getPiece();
		}
	}
	Game& operator=(const Game& other) {
		if (this != &other) {
			rng = other.rng;
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

	bool collides(const Board& board, const Piece& piece) const;

	void rotate(Piece& piece, TurnDirection dir) const;

	void shift(Piece& piece, int dir) const;

	void sonic_drop(const Board board, Piece& piece) const;

	void add_garbage(int lines, int loc);

	int damage_sent(int linesCleared, spinType spinType, bool pc);

	void process_movement(Piece& piece, Movement movement) const;

	std::vector<Piece> movegen(PieceType piece_type) const;

	std::pair<Piece, bool> get_best_piece() const;

	pptRNG rng;
	Board board;
	Piece current_piece;
	std::optional<Piece> hold;
	std::array<PieceType, QUEUE_SIZE> queue;
	int garbage_meter = 0;
	int b2b = 0;
	int combo = 0;

	// tetrio stuff
	int currentcombopower = 0;
	int currentbtbchainpower = 0;

	const struct options {
		bool b2bchaining = true;
		int garbagemultiplier = 1;
	} options;
};