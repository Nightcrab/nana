#pragma once
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "EmulationGame.hpp"
#include "Move.hpp"
#include "UCT.hpp"
#include "MPSC.hpp"


namespace Search {

	std::atomic_bool searching = false;

	int core_count = 4;

	int monte_carlo_depth = 2;

	UCT uct = UCT(4);

	EmulationGame root_state;

	zib::wait_mpsc_queue<Job>* queues[256];

	void startSearch(EmulationGame state, int core_count);

	void endSearch();

	void search(int threadIdx);

	float rollout(EmulationGame state, int threadIdx);

	// See the best move found so far.
	Move bestMove();
};