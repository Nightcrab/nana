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
	Action(Move move, uint32_t id) {
		this->move = move;
		this->N = 0;
		this->R = 0;
		this->prior = 1;
		this->eval = 0.0;
		this->id = id;
	}
	Action(Move move, uint32_t id, float eval) {
		this->move = move;
		this->N = 0;
		this->R = 0;
		this->prior = 1;
		this->eval = eval;
		this->id = id;
	}
	Move move;
	float R;
	float prior;
	float eval;
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

	Action& select();
	Action& select_r_max();
	Action& select_SOR(RNG &rng);
};

class HashActionPair {
public:
	HashActionPair(int hash, int actionID) {
		this->hash = hash;
		this->actionID = actionID;
	}
	uint32_t hash;
	int actionID;
};


enum JobType {
	SELECT,
	BACKPROP,
	STOP
};


class Job {
public:
	Job() {
		this->R = 0.0;
		this->state = EmulationGame();
		this->type = STOP;
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
		auto devrng = std::random_device();
		for (auto& thread_rng : this->rng) {
			thread_rng.PPTRNG = devrng();
		}
	}

	int workers = 4;
	int size = 0;

	std::vector<std::unordered_map<int, UCTNode>> nodes;
	std::vector<RNG> rng;

	bool nodeExists(uint32_t nodeID);

	UCTNode& getNode(uint32_t nodeID);

	void insertNode(UCTNode node);

	uint32_t getOwner(uint32_t hash);
};

