#pragma once

#include "Board.hpp"
#include "VersusGame.hpp"
#include "Move.hpp"


namespace Search {
	Move monte_carlo_best_move(const VersusGame& game, int threads, int samples, int N, int id);
};