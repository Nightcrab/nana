#pragma once

#include "Board.hpp"
#include "Game.hpp"
#include "Move.hpp"

namespace Eval {
double eval_LUT(const Board& board);
double eval_CC(const Board& board);
double eval_CC(Game game, Move move);
};  // namespace Eval