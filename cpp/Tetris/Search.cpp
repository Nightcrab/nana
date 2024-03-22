#include "Search.hpp"
#include <numeric>
#include <execution>


void Search::startSearch(EmulationGame state, int core_count) {
	searching = true;

	Search::core_count = core_count;

	// Create new search tree
	search_tree = UCT(core_count);

	// Initialise worker queues
	for (int i = 0; i < core_count; i++) {
		queues[i] = new zib::wait_mpsc_queue<Job>(core_count);
	}

	// Thread indices
	std::vector<int> indices(core_count);

	std::iota(indices.begin(), indices.end(), 0);

	// Spawn worker threads
	std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), search);
};


void Search::endSearch() {
	searching = false;
};


void Search::search(int threadIdx) {
	while (true) {
		if (!searching) {
			return;
		}
		Job job = queues[threadIdx]->dequeue();
		if (job.type == SELECT) {

		}
		else if (job.type == BACKPROP) {

		}
	}
}


Move Search::bestMove() {
	return Move();
}