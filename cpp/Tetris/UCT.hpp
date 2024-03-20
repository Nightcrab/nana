#pragma once
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "rng.hpp"

class UCT;

// state
class UCTNode {
public:
	float R;
	int N;
	int ID;

	std::vector<UCTNode> children;

	UCTNode nodeSelect();
	UCTNode edgeSelect(RNG rng);
};

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

