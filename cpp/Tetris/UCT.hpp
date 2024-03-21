#pragma once
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "rng.hpp"
#include "EmulationGame.hpp"
#include "Move.hpp"

class UCT;

// state
class UCTNode {
public:
	float R;
	int N;
	int ID;

	std::vector<UCTNode> children;

	UCTNode nodeSelect();
};

// state + action
class UCTEdge : UCTNode {
	Move action;
	UCTNode edgeSelect(RNG rng);
	
	void rollout();
};

// one of these shared by all threads
class UCT {
public:
	int workers = 4;

	std::vector<std::unordered_map<int, UCTNode>> nodes;
	std::vector<RNG> rng;

	bool nodeExists(int workerID, int nodeID);

	UCTNode getNode(int workerID, int nodeID);

	void updateNode(int workerID, int edgeID, float R);

	void insertNode(int workerID, int nodeID, UCTNode node);
};

namespace Search{

	std::atomic_bool searching = false;

	void startSearch(EmulationGame state);

	void endSearch();
}