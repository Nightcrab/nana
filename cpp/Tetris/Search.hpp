#pragma once
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "EmulationGame.hpp"
#include "Move.hpp"
#include "UCT.hpp"


namespace Search {

	std::atomic_bool searching = false;

	UCT search_tree = UCT(4);

	EmulationGame root_state;

	void startSearch(EmulationGame state, int core_count);

	void endSearch();

	void search();

	// See the best move found so far.
	Move bestMove();
}