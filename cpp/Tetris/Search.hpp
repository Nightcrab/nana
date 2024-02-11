#pragma once

#include "Board.hpp"
#include "VersusGame.hpp"




namespace Search {
	std::pair<Piece, bool> monte_carlo_best_move(const VersusGame& game, int samples, int id);
};