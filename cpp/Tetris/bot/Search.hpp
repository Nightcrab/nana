#pragma once
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "EmulationGame.hpp"
#include "Move.hpp"
#include "UCT.hpp"
#include "MPSC.hpp"

enum SearchType {
	CC,
	NANA
};

#ifndef __SEARCH_HPP
#define __SEARCH_HPP
namespace Search {

	extern std::atomic_bool searching;

	extern SearchType search_style;

	extern int core_count;

	extern int monte_carlo_depth;

	extern UCT uct;

	extern EmulationGame root_state;

	extern zib::wait_mpsc_queue<Job>* queues[256];

	void startSearch(EmulationGame state, int core_count);

	void endSearch();

	void search(int threadIdx);

	float rollout(EmulationGame state, int threadIdx);

	// See the best move found so far.
	Move bestMove();
};
#endif