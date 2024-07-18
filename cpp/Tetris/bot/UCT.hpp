#pragma once
#include <algorithm>
#include <stack>
#include <unordered_map>
#include <vector>

#include "EmulationGame.hpp"
#include "Util/MPSC.hpp"
#include "Move.hpp"
#include "util/rng.hpp"

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
    UCTNode() {};
    UCTNode(const EmulationGame &state);
    UCTNode(const std::vector<Action>& p_actions, int id, int N) {
        this->actions = p_actions;
        for (int i = 0; i < actions.size(); i++) {
            actions[i].id = i;
        }
        this->id = id;
        this->N = N;
    }

    uint32_t id;
    int N;
    float R_buffer = 0.0;

    std::vector<Action> actions;

    Action& select();
    Action& select_r_max();
    Action& select_SOR(RNG& rng);
};

class HashActionPair {
   public:
    HashActionPair(uint32_t hash, int actionID) {
        this->hash = hash;
        this->actionID = actionID;
    }
    uint32_t hash;
    int actionID;
};

enum JobType {
    SELECT,
    BACKPROP,
    STOP,
    PUT
};

class Job {
   public:
    Job()
        : R(0.0), state(EmulationGame()), type(STOP){};

    Job(const EmulationGame& state, JobType type)
        : R(0.0), state(state), type(type){};

    Job(const EmulationGame& state, JobType type, const std::vector<HashActionPair> &path)
        : R(0.0), state(state), type(type), path(path){};

    Job(float R, const EmulationGame& state, JobType type, const std::vector<HashActionPair> &path)
        : R(R), state(state), type(type), path(path){};

    float R;
    EmulationGame state;
    JobType type;

    // for going backwards

    std::vector<HashActionPair> path;
};

class WorkerStatistics {
public:
    int nodes = 0;
    int deepest_node = 0;
    int backprop_messages = 0;
};

// one of these shared by all threads
class UCT {
   public:
    UCT() = default;
    UCT(int workers) {
        this->workers = workers;
        this->nodes_left = std::vector<std::unordered_map<int, UCTNode>>(workers);
        this->nodes_right = std::vector<std::unordered_map<int, UCTNode>>(workers);
        this->rng = std::vector<RNG>(workers);
        this->stats = std::vector<WorkerStatistics>(workers);

        auto devrng = std::random_device();
        for (auto& thread_rng : this->rng) {
            thread_rng.PPTRNG = devrng();
        }
    }
    ~UCT() = default;

    int workers = 0;
    int size = 0;

    std::vector<std::unordered_map<int, UCTNode>> nodes_left;
    std::vector<std::unordered_map<int, UCTNode>> nodes_right;

    std::vector<RNG> rng;
    std::vector<WorkerStatistics> stats;

    bool nodeExists(uint32_t nodeID);

    UCTNode& getNode(uint32_t nodeID, int threadIdx);

    UCTNode& getNode(uint32_t nodeID);

    void insertNode(const UCTNode &node);

    uint32_t getOwner(uint32_t hash);

    void collect();
};
