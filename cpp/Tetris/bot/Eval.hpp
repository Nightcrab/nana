#pragma once

#include "Board.hpp"
#include "Game.hpp"
#include "Move.hpp"

namespace Eval {
double eval_LUT(const Board& board);
double eval_CC(const Board& board);
double eval_CC(Game game, Move move);

static bool is_top_quarter(const Board& board);
static bool is_top_half(const Board& board);

static std::pair<int, int> n_cavities(const Board& board);

// lowest height, highest height
static std::pair<int, int> height_features(const Board& board);

std::pair<int, int> n_covered_cells(Board board);

static std::pair<int, int> get_bumpiness(const Board& board);
// Identify clean count to 4
static bool ct4(const Board& board);

};  // namespace Eval