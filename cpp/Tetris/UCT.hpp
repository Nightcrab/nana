#pragma once
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "rng.hpp"
#include "EmulationGame.hpp"
#include "Move.hpp"
#include "MPSC.hpp"

/*
	Data structures for performing multithreaded UCT search.
*/

class UCT;

class Action {
public:
	Move move;
	float R;
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


enum JobType {
	SELECT,
	BACKPROP
};


class Job {
public:
	float R;
	JobType type;
};


// one of these shared by all threads
class UCT {
public:
	UCT(int workers) {
		this->workers = workers;
		this->nodes = std::vector<std::unordered_map<int, UCTNode>>(workers);
		this->rng = std::vector<RNG>(workers);
	}

	int workers = 4;

	std::vector<std::unordered_map<int, UCTNode>> nodes;
	std::vector<RNG> rng;

	bool nodeExists(int nodeID);

	UCTNode getNode(int nodeID);

	void insertNode(int nodeID, UCTNode node);
};
