#include "Search.hpp"


void Search::startSearch(EmulationGame state, int core_count) {
	searching = true;

	// Create new search tree
	search_tree = UCT(core_count);
};


void Search::endSearch() {
	searching = false;
};


void Search::search() {
	while (true) {
		if (!searching) {
			return;
		}

	}
}


Move Search::bestMove() {
	return Move();
}