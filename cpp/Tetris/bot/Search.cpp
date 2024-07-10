#include "Search.hpp"

#include <execution>
#include <numeric>
#include <ranges>

#include "Distribution.hpp"
#include "Eval.hpp"
#include <iostream>
#include <thread>

std::atomic_bool Search::searching = false;



int Search::core_count = 0;

int Search::monte_carlo_depth = 1;

UCT Search::uct;

EmulationGame Search::root_state;

std::vector<std::unique_ptr<zib::wait_mpsc_queue<Job>>> Search::queues;
std::vector<int> core_indices;
std::vector<std::jthread> worker_threads;

std::chrono::steady_clock::time_point search_start_time;

constexpr int LOAD_FACTOR = 6;

void Search::startSearch(const EmulationGame &state, int core_count) {

    //std::cout << "started searching, root hash is: " <<  (int) (state.hash() % 1000) << std::endl;

    search_start_time = std::chrono::steady_clock::now();

    searching = true;

    Search::core_count = core_count;

    root_state = state;

    // Create new search tree
    uct = UCT(core_count);

    // Create root node
    uct.insertNode(UCTNode(root_state));

    // Initialise worker queues

    queues.clear();
    queues.reserve(core_count);

    for (int i = 0; i < core_count; i++) {
        queues.push_back(std::make_unique<zib::wait_mpsc_queue<Job>>(core_count + 1));
    }

    // Thread indices
    core_indices = std::vector<int>(core_count);
    worker_threads = std::vector<std::jthread>(core_count);

    std::iota(core_indices.begin(), core_indices.end(), 0);

    int rootOwnerIdx = uct.getOwner(state.hash());

    // Spawn jobs
    for (int i = 0; i < LOAD_FACTOR * core_count; i++) {
        root_state.chance.reset_rng();
        queues[rootOwnerIdx]->enqueue(Job(root_state, SELECT), core_count);
    }

    // Spawn worker threads
    for (auto& idx : core_indices) {
        worker_threads[idx] = std::jthread(search, idx);
    }
};

void Search::continueSearch(EmulationGame state) {
    //std::cout << "started searching, root hash is: " << (int)(state.hash() % 1000) << std::endl;

    search_start_time = std::chrono::steady_clock::now();

    searching = true;

    root_state = state;

    state.attack = state.app() * 100;
    state.true_attack = state.true_app() * 100;
    state.pieces = 100;

    if (!uct.nodeExists(state.hash())) {
        uct.insertNode(UCTNode(state));
    }

    for (WorkerStatistics& stat : uct.stats) {
        stat = {};
    }
    queues.clear();
    queues.reserve(core_count);
    // Initialise worker queues
    for (int i = 0; i < core_count; i++) {
        queues.emplace_back(std::make_unique<zib::wait_mpsc_queue<Job>>(core_count + 1));
    }

    core_indices = std::vector<int>(core_count);

    std::iota(core_indices.begin(), core_indices.end(), 0);

    int rootOwnerIdx = uct.getOwner(state.hash());

    for (int i = 0; i < LOAD_FACTOR * core_count; i++) {
        queues[rootOwnerIdx]->enqueue(Job(root_state, SELECT), core_count);
    }

    for (auto& idx : core_indices) {
        worker_threads[idx] = std::jthread(search, idx);
    }
}

void Search::endSearch() {
    searching = false;

    for (int i = 0; i < core_count; i++) {
        queues[i]->enqueue(Job(), core_count);
    }

    // join threads
    for (auto& thread : worker_threads) {
		thread.join();
	}

    uct.collect();
    queues.clear();

    //std::cout << "stopped searching" << std::endl;
 };

void Search::printStatistics() {

    std::chrono::steady_clock::time_point search_end_time = std::chrono::steady_clock::now();

    float ms = std::chrono::duration_cast<std::chrono::microseconds>(search_end_time - search_start_time).count();

    int nodes = 0;
    int depth = 0;
    int backprops = 0;

    for (WorkerStatistics stat : uct.stats) {
        nodes += stat.nodes;
        backprops += stat.backprop_messages;
        depth = std::max(stat.deepest_node, depth);
    }

    std::cout << "nodes / second: " << nodes / (ms/ 1000000) << std::endl;
    std::cout << "backprops / second: " << backprops / (ms / 1000000) << std::endl;
    std::cout << "tree depth: " << depth << std::endl;
}

void Search::search(const int threadIdx) {
    while (true) {
        if (!searching) {
            return;
        }

        // Thread waits here until something is in the queue
        // Master thread is required to spawn SELECT and STOP jobs from the root
        // otherwise we have deadlock
        Job job = queues[threadIdx]->dequeue();

        if (job.type == STOP) {
            return;
        }

        if (job.type == SELECT) {

            uct.stats[threadIdx].nodes++;

            EmulationGame& state = job.state;

            uint32_t hash = state.hash();

            if (state.game_over) {

                float reward = rollout(state, threadIdx);

                Job backprop_job(reward, state, BACKPROP, job.path);

                if (job.path.empty()) {
                    continue;
                }

                uint32_t parent_hash = job.path.back().hash;

                uint32_t parentIdx = uct.getOwner(parent_hash);

                // send this one back to our parent

                queues[parentIdx]->enqueue(backprop_job, threadIdx);

                continue;
            }

            if (uct.nodeExists(hash)) {

                UCTNode& node = uct.getNode(hash, threadIdx);
                Action* action = &node.actions[0];

                if constexpr (search_style == NANA) {

                    action = &node.select();
                }
                if constexpr (search_style == CC) {

                    //action = &node.select_r_max();
                    action = &node.select_SOR(uct.rng[threadIdx]);
                }

                // Virtual Loss by setting N := N+1
                node.N += 1;

                action->N += 1;


                state.set_move(action->move);

                state.play_moves();

                uint32_t new_hash = state.hash();

                // Get the owner of the updated state based on the hash
                uint32_t ownerIdx = uct.getOwner(new_hash);

                Job select_job(state, SELECT, job.path);

                select_job.path.push_back(HashActionPair(hash, action->id));

                uct.stats[threadIdx].deepest_node = std::max(uct.stats[threadIdx].deepest_node, (int) select_job.path.size());

                queues[ownerIdx]->enqueue(select_job, threadIdx);

            } 
            else {

                UCTNode node(state);

                uct.insertNode(node);

                Action* action = &node.actions[0];
                if constexpr (search_style == NANA) {

                    action = &node.select();
                }
                if constexpr (search_style == CC) {

                    action = &node.select_SOR(uct.rng[threadIdx]);
                }

                node.N += 1;
                action->N += 1;

                state.set_move(action->move);
                state.play_moves();

                float reward = rollout(state, threadIdx);

                uint32_t parent_hash = job.path.back().hash;

                uint32_t parentIdx = uct.getOwner(parent_hash);

                Job backprop_job(reward, state, BACKPROP, job.path);

                // send rollout reward to parent, who also owns the arm that got here

                queues[parentIdx]->enqueue(backprop_job, threadIdx);
            }
        } 
        else if (job.type == BACKPROP) {

            uct.stats[threadIdx].backprop_messages++;
            UCTNode& node = uct.getNode(job.path.back().hash, threadIdx);

            float reward = job.R;

            // Undo Virtual Loss by adding R
            if constexpr (search_style == NANA) {
                float& R = node.actions[job.path.back().actionID].R;
                int& N = node.actions[job.path.back().actionID].N;
                R = R + reward;
            }
            if constexpr (search_style == CC) {
                if (reward > node.actions[job.path.back().actionID].R) {
                    node.actions[job.path.back().actionID].R = reward;
                }
            }

            job.path.pop_back();

            if (job.path.empty()) {
                Job select_job(root_state, SELECT);

                queues[threadIdx]->enqueue(select_job, threadIdx);

                continue;
            }

            bool should_backprop = true;

            // check if backpropagating would change the outcome at higher nodes
            /*
            for (HashActionPair ha : job.path) {

                if ((ha.hash % core_count) != threadIdx) {
                    continue;
                }

                UCTNode& higher_node = uct.getNode(ha.hash, threadIdx);
                Action* action = &node.actions[0];

                if (search_style == NANA) {

                    action = &node.select();
                }
                if (search_style == CC) {

                    action = &node.select_r_max();
                }

                if (action->id != ha.actionID) {
                    // a sibling is better right now, so backpropagating may change the outcome
                    should_backprop = true;
                    break;
                }
            }
            */

            if (should_backprop) {

                uint32_t parent_hash = job.path.back().hash;

                uint32_t parentIdx = uct.getOwner(parent_hash);

                // drain stashed rewards

                reward += node.R_buffer;
                node.R_buffer = 0;

                Job backprop_job(reward, job.state, BACKPROP, job.path);

                queues[parentIdx]->enqueue(backprop_job, threadIdx);
            }
            else {

                // stash reward and start a new rollout

                node.R_buffer += reward;

                Job select_job(root_state, SELECT);

                queues[threadIdx]->enqueue(select_job, threadIdx);
            }

        }
    }
}


float Search::rollout(EmulationGame& state, int threadIdx) {
    // Rollout using a square-of-rank policy distribution.

    float reward = 0;
    for (int i = 0; i < monte_carlo_depth; i++) {
        
        uct.stats[threadIdx].nodes++;

        if (state.game_over) {
            return 0.0;
        }

        float max_eval = -1;
        Move move;

        if (threadIdx == uct.getOwner(state.hash())) {

            UCTNode node = UCTNode(state);

            uct.insertNode(node);

            move = node.select_SOR(uct.rng[threadIdx]).move;

            for (auto& action : node.actions) {
                max_eval = std::max(max_eval, action.eval);
            }
        }
        else {
            std::vector<Move> moves = state.legal_moves();

            std::vector<Stochastic<Move>> policy;
            std::vector<Stochastic<Move>> SoR_policy;
            std::vector<Stochastic<float>> cc_dist;

            policy.reserve(moves.size());

            for (auto& move : moves) {
                // raw scores
                policy.push_back(Stochastic<Move>(move, Eval::eval_CC(state.game, move)));
            }

            // sort in descending order
            std::ranges::sort(policy, [](const Stochastic<Move>& a, const Stochastic<Move>& b) {
                return a.probability > b.probability;
                });

            // rank

            for (int rank = 1; rank <= policy.size(); rank++) {
                float prob = 1.0 / (rank * rank);
                SoR_policy.push_back(Stochastic<Move>(policy[rank - 1].value, prob));
                cc_dist.push_back(Stochastic<float>(policy[rank - 1].probability, prob));
            }

            max_eval = Distribution::max_value(cc_dist);
        }

        if constexpr (search_style == NANA) {
            //float r = max_eval;
            //float r = Distribution::expectation(cc_dist);
            float r = state.true_app() + max_eval / 2 + std::min(state.b2b(), (float) 2.0) / 10;
            reward = std::max(reward, r);
        }
        if constexpr (search_style == CC) {
            //float r = max_eval;
            float r = state.true_app() + max_eval / 2 + std::min(state.b2b(), (float) 2.0) / 10;
            reward = std::max(reward, r);
        }

        state.set_move(move);
        state.play_moves();
    }

    //reward = (Distribution::sigmoid(reward) + 1)/2;

    return reward;
}

Move Search::bestMove() {

    int biggest_N = 0;
    float biggest_R = 0;
    Move best_move;

    for (Action& action : uct.getNode(root_state.hash()).actions) {
        if constexpr (search_style == NANA) {
            if (action.N == biggest_N) {
                if (action.R > biggest_R) {
                    biggest_R = action.R;
                    biggest_N = action.N;
                    best_move = action.move;
                }
            }
            if (action.N > biggest_N) {
                biggest_R = action.R;
                biggest_N = action.N;
                best_move = action.move;
            }

            // std::cout << "N:" << action.N << " R_avg:" << action.R / action.N << std::endl;
        }
        if constexpr (search_style == CC) {
            if (action.R > biggest_R) {
                biggest_R = action.R;
                biggest_N = action.N;
                best_move = action.move;
            }
            
            // std::cout << "N:" << action.N << " R_max:" << action.R << std::endl;
        }
        

    }
    if constexpr (search_style == NANA) {
        // std::cout << "best move was visited " << biggest_N << " times, with R_avg " << biggest_R / biggest_N << std::endl;
    }
    if constexpr (search_style == CC) {
        // std::cout << "best move was visited " << biggest_N << " times, with R_max " << biggest_R << std::endl;
    }

    return best_move;
}