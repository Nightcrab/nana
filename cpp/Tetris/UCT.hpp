#pragma once
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "rng.hpp"
#include "EmulationGame.hpp"
#include "Move.hpp"

class UCT;

class Action {
public:
	Move move;
	int R;
	int N;
};

// state
class UCTNode {
public:
	int ID;
	int N;

	std::vector<Action> actions;

	Action select();
};

// one of these shared by all threads
class UCT {
public:
	UCT(int workers) {
		this->workers = workers;
	}

	int workers = 4;

	std::vector<std::unordered_map<int, UCTNode>> nodes;
	std::vector<RNG> rng;

	bool nodeExists(int nodeID);

	UCTNode getNode(int nodeID);

	void insertNode(int nodeID, UCTNode node);
};
