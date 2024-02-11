#pragma once

#include "Board.hpp"



namespace Eval {
	double eval(const Board& board);
	double eval(const Board& board, bool fast);
};