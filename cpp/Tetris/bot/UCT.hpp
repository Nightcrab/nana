#pragma once
#include <vector>
#include <stack>
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
	uint32_t id;
};

// state
class UCTNode {
public:
	UCTNode(EmulationGame state);
	UCTNode(std::vector<Action> actions, int ID, int N) {
		for (int i = 0; i < actions.size(); i++) {
			actions[i].id = i;
		}
		this->actions = actions;
		this->ID = ID;
		this->N = N;
	}
	uint32_t ID;
	int N;

	std::vector<Action> actions;

	Action select();
};

class HashActionPair {
public:
	HashActionPair(int hash, Action action) {
		this->hash = hash;
		this->action = action;
	}
	uint32_t hash;
	Action action;
};


enum JobType {
	SELECT,
	BACKPROP
};


class Job {
public:
	Job() {
		this->R = 0.0;
		this->state = EmulationGame();
		this->type = SELECT;
		this->path = std::stack<HashActionPair>();
	};
	Job(EmulationGame state, JobType type) {
		this->R = 0.0;
		this->state = state;
		this->type = type;
		this->path = std::stack<HashActionPair>();
	};
	Job(EmulationGame state, JobType type, std::stack<HashActionPair> path) {
		this->R = 0.0;
		this->state = state;
		this->type = type;
		this->path = path;
	};
	Job(float R, EmulationGame state, JobType type, std::stack<HashActionPair> path) {
		this->R = R;
		this->state = state;
		this->type = type;
		// for going backwards
		this->path = path;
	};
	float R;
	EmulationGame state;
	JobType type;
	std::stack<HashActionPair> path;
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

	bool nodeExists(uint32_t nodeID);

	UCTNode getNode(uint32_t nodeID);

	void insertNode(UCTNode node);

	uint32_t getOwner(uint32_t hash);
};
