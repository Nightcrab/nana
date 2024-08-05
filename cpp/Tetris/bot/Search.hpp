#pragma once
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <memory>

#include "EmulationGame.hpp"
#include "Util/MPSC.hpp"
#include "Move.hpp"
#include "UCT.hpp"

#ifndef __SEARCH_HPP
#define __SEARCH_HPP

enum SearchType {
    CC,
    NANA
};


namespace Search {

    extern std::atomic_bool searching;

    constexpr SearchType search_style = CC;

    extern int core_count;

    extern int monte_carlo_depth;

    extern UCT uct;

    extern EmulationGame root_state;

    extern std::vector<std::unique_ptr<mpsc<Job>>> queues;

    extern bool initialised;

    void startSearch(const EmulationGame& state, int core_count);

    void continueSearch(EmulationGame state);

    void endSearch();

    void maybeSteal(int threadIdx, int targetThread, Job job);

    void maybeInsertNode(UCTNode node, const int threadIdx);

    void search(int threadIdx);

    void processJob(const int threadIdx, Job job);

    float rollout(EmulationGame& state, int threadIdx);

    void printStatistics();

    // See the best move found so far.
    Move bestMove();

};  // namespace Search
#endif