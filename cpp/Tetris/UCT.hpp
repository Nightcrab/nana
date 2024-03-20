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

	bool nodeExists(int workerID, int nodeID) {
		return nodes[nodeID % workers].find(nodeID) != nodes[nodeID % workers].end();
	};

	UCTNode getNode(int workerID, int nodeID) {
		return nodes[nodeID % workers].at(nodeID);
	};

	void updateNode(int workerID, int edgeID, float R) {
		int ownerID = edgeID % workers;

		if (workerID != ownerID) {
			throw;
		}

		UCTNode edge = getNode(workerID, edgeID);

		edge.R += R;
		edge.N += 1;
	};

	void insertNode(int workerID, int nodeID, UCTNode node) {
		int ownerID = nodeID % workers;

		if (workerID != ownerID) {
			throw;
		}

		nodes[nodeID % workers].insert({nodeID, node});
	};
};

namespace Search{

	std::atomic_bool searching = false;

	void startSearch(EmulationGame state);

	void endSearch();
}