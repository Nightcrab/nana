#pragma once

#include "Board.hpp"


namespace Eval {
	double eval_LUT(const Board& board);
	double eval_CC(const Board& board);
};