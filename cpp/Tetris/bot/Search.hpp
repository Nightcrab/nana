#pragma once
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <memory>

#include "EmulationGame.hpp"
#include "MPSC.hpp"
#include "Move.hpp"
#include "UCT.hpp"

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

extern std::vector<std::unique_ptr<zib::wait_mpsc_queue<Job>>> queues;

void startSearch(const EmulationGame& state, int core_count);

void continueSearch(EmulationGame state);

void endSearch();

void search(int threadIdx);

float rollout(EmulationGame& state, int threadIdx);

void printStatistics();

// See the best move found so far.
Move bestMove();

};  // namespace Search
#endif