#include "Search.hpp"

#include <execution>
#include <numeric>
#include <ranges>

#include "Distribution.hpp"
#include "Eval.hpp"
#include <iostream>

std::atomic_bool Search::searching = false;

SearchType Search::search_style = NANA;

int Search::core_count = 0;

int Search::monte_carlo_depth = 2;

UCT Search::uct = UCT(4);

EmulationGame Search::root_state;

zib::wait_mpsc_queue<Job>* Search::queues[256];
std::vector<int> core_indices;
std::vector<std::jthread> worker_threads;

const int LOAD_FACTOR = 100;

void Search::startSearch(const EmulationGame &state, int core_count) {

    std::cout << "started searching, root hash is: " <<  (int) (state.hash() % 1000) << std::endl;

    searching = true;

    Search::core_count = core_count;

    root_state = state;

    // Create new search tree
    uct = UCT(core_count);

    // Create root node
    uct.insertNode(state);

    // Initialise worker queues
    for (int i = 0; i < core_count; i++) {
        queues[i] = new zib::wait_mpsc_queue<Job>(core_count + 1);
    }

    // Thread indices
    core_indices = std::vector<int>(core_count);
    worker_threads = std::vector<std::jthread>(core_count);

    std::iota(core_indices.begin(), core_indices.end(), 0);

    int rootOwnerIdx = uct.getOwner(state.hash());

    // Spawn jobs
    for (int i = 0; i < LOAD_FACTOR * core_count; i++) {
        queues[rootOwnerIdx]->enqueue(Job(root_state, SELECT), core_count);
    }

    // Spawn worker threads
    for (auto& idx : core_indices) {
        worker_threads[idx] = std::jthread(search, idx);
    }
};

void Search::continueSearch(EmulationGame state) {
    std::cout << "started searching, root hash is: " << (int)(state.hash() % 1000) << std::endl;

    searching = true;

    root_state = state;

    if (!uct.nodeExists(state.hash())) {
        uct.insertNode(UCTNode(state));
    }

    // Initialise worker queues
    for (int i = 0; i < core_count; i++) {
        queues[i] = new zib::wait_mpsc_queue<Job>(core_count + 1);
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

    std::cout << "stopped searching" << std::endl;

    std::cout << "nodes created: " << uct.size << std::endl;

};

void Search::search(int threadIdx) {
    while (true) {
        if (!searching) {
            return;
        }

        // Thread waits here until something is in the queue
        // Master thread is required to spawn SELECT jobs from the root
        // otherwise we risk deadlock
        Job job = queues[threadIdx]->dequeue();

        if (job.type == STOP) {
            return;
        }

        if (job.type == SELECT) {

            EmulationGame& state = job.state;

            uint32_t hash = state.hash();

            if (state.game_over) {

                float reward = rollout(state, threadIdx);

                Job backprop_job = Job(reward, state, BACKPROP, job.path);

                if (job.path.empty()) {
                    continue;
                }

                uint32_t parent_hash = job.path.top().hash;

                uint32_t parentIdx = uct.getOwner(parent_hash);

                // send this one back to our parent

                queues[parentIdx]->enqueue(backprop_job, threadIdx);

                continue;

            } 
            if (uct.nodeExists(hash)) {

                UCTNode& node = uct.getNode(hash);
                Action* action = &node.actions[0];

                if (search_style == NANA) {

                    action = &node.select();
                }
                if (search_style == CC) {

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

                Job select_job = Job(0.0, state, SELECT, job.path);

                select_job.path.push(HashActionPair(hash, action->id));

                queues[ownerIdx]->enqueue(select_job, threadIdx);
            } else {
                UCTNode node = UCTNode(state);

                uct.insertNode(node);

                Action* action = &node.actions[0];
                if (search_style == NANA) {

                    action = &node.select();
                }
                if (search_style == CC) {

                    action = &node.select_SOR(uct.rng[threadIdx]);
                }

                node.N += 1;
                action->N += 1;

                state.set_move(action->move);
                state.play_moves();

                float reward = rollout(state, threadIdx);

                uint32_t parent_hash = job.path.top().hash;

                uint32_t parentIdx = uct.getOwner(parent_hash);

                Job backprop_job = Job(reward, state, BACKPROP, job.path);

                // send rollout reward to parent, who also owns the arm that got here

                queues[parentIdx]->enqueue(backprop_job, threadIdx);
            }
        } else if (job.type == BACKPROP) {
            UCTNode& node = uct.getNode(job.path.top().hash);

            float reward = job.R;

            // Undo Virtual Loss by adding R
            if (search_style == NANA) {
                node.actions[job.path.top().actionID].R += reward;
            }
            if (search_style == CC) {
                if (reward > node.actions[job.path.top().actionID].R) {
                    node.actions[job.path.top().actionID].R = reward;
                }
            }

            job.path.pop();

            if (job.path.empty()) {
                Job select_job = Job(root_state, SELECT, job.path);

                queues[threadIdx]->enqueue(select_job, threadIdx);

                continue;
            }

            uint32_t parent_hash = job.path.top().hash;

            uint32_t parentIdx = uct.getOwner(parent_hash);

            Job backprop_job = Job(reward, job.state, BACKPROP, job.path);

            queues[parentIdx]->enqueue(backprop_job, threadIdx);
        }
    }
}

float Search::rollout(EmulationGame state, int threadIdx) {
    // Rollout using a square-of-rank policy distribution.

    float reward = 0;
    for (int i = 0; i < monte_carlo_depth; i++) {

        if (state.game_over) {
            return 0.0;
        }

        // todo: create a UCTNode and take legal moves from that
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

        // square of rank

        for (int rank = 1; rank <= policy.size(); rank++) {
            float prob = 1.0 / (rank * rank);
            SoR_policy.push_back(Stochastic<Move>(policy[rank - 1].value, prob));
            cc_dist.push_back(Stochastic<float>(policy[rank - 1].probability, prob));
        }

        if (search_style == NANA) {
            float r = Distribution::max_value(cc_dist) + state.app() / 10 + state.b2b() / 50;;
            //float r = Distribution::expectation(cc_dist);
            //float r = state.app();
            reward = std::max(reward, r);
        }
        if (search_style == CC) {
            float r = Distribution::max_value(cc_dist) + state.app() / 10 + state.b2b() / 50;
            //float r = state.app();
            reward = std::max(reward, r);
        }

        SoR_policy = Distribution::normalise(SoR_policy);

        Move sample = Distribution::sample(SoR_policy, uct.rng[threadIdx]);

        state.set_move(sample);
        state.play_moves();
    }

    reward = (Distribution::sigmoid(reward) + 1)/2;

    return reward;
}

Move Search::bestMove() {

    int biggest_N = 0;
    float biggest_R = 0;
    Move best_move;

    for (Action& action : uct.getNode(root_state.hash()).actions) {
        if (search_style == NANA) {
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
            std::cout << "N:" << action.N << " R_avg:" << action.R / action.N << std::endl;
        }
        if (search_style == CC) {
            if (action.R > biggest_R) {
                biggest_R = action.R;
                biggest_N = action.N;
                best_move = action.move;
            }std::cout << "N:" << action.N << " R_max:" << action.R << std::endl;
        }
        

    }
    if (search_style == NANA) {
        std::cout << "best move was visited " << biggest_N << " times, with R_avg " << biggest_R / biggest_N << std::endl;
    }
    if (search_style == CC) {
        std::cout << "best move was visited " << biggest_N << " times, with R_max " << biggest_R << std::endl;
    }

    return best_move;
}