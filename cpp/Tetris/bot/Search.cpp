#include "Search.hpp"

#include <execution>
#include <numeric>
#include <ranges>

#include "Util/Distribution.hpp"
#include "Eval.hpp"
#include <iostream>
#include <thread>

std::atomic_bool Search::searching = false;

int Search::core_count = 0;

int Search::monte_carlo_depth = 1;

UCT Search::uct;

EmulationGame Search::root_state;

std::vector<std::unique_ptr<mpsc<Job>>> Search::queues;
std::vector<int> core_indices;
std::vector<std::jthread> worker_threads;

std::chrono::steady_clock::time_point search_start_time;

bool Search::initialised = false;

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
        queues.push_back(std::make_unique<mpsc<Job>>(core_count + 1));
    }

    // Thread indices
    core_indices = std::vector<int>(core_count);
    worker_threads = std::vector<std::jthread>(core_count);

    std::iota(core_indices.begin(), core_indices.end(), 0);

    int rootOwnerIdx = uct.getOwner(state.hash());

    // Spawn jobs
    for (int i = 0; i < LOAD_FACTOR * core_count; i++) {
        root_state.chance.reset_rng();
        root_state.opponent.reset_rng();
        queues[rootOwnerIdx]->enqueue(Job(root_state, SELECT), core_count);
    }

    // Spawn worker threads
    for (auto& idx : core_indices) {
        worker_threads[idx] = std::jthread(search, idx);
    }

    initialised = true;
};

void Search::continueSearch(EmulationGame state) {
    //std::cout << "started searching, root hash is: " << (int)(state.hash() % 1000) << std::endl;

    search_start_time = std::chrono::steady_clock::now();

    searching = true;

    root_state = state;

    root_state.attack = state.app() * 10;
    root_state.true_attack = state.true_app() * 10;
    root_state.pieces = 10;

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
        queues.emplace_back(std::make_unique<mpsc<Job>>(core_count + 1));
    }

    core_indices = std::vector<int>(core_count);

    std::iota(core_indices.begin(), core_indices.end(), 0);

    int rootOwnerIdx = uct.getOwner(state.hash());
    for (int j = 0; j < LOAD_FACTOR; j++) {
        root_state.chance.reset_rng();
        root_state.opponent.reset_rng();
        for (int i = 0; i < core_count; i++) {
            queues[rootOwnerIdx]->enqueue(Job(root_state, SELECT), core_count);
        }

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

void Search::maybeSteal(int threadIdx, int targetThread, Job job) {
    bool is_empty = true;
    for (int i = 0; i < queues[threadIdx]->size; i++) {
        Job* job = queues[threadIdx]->peek();
        if (job != nullptr) {
            if ((*job).type != STOP) {
                is_empty = false;
                break;
            }
        }
    }
    if (is_empty) {
        // steal
        queues[threadIdx]->enqueue(job, threadIdx);
    }
    else {
        // don't steal
        queues[targetThread]->enqueue(job, threadIdx);
    }
}

void Search::processJob(const int threadIdx, Job job) {

    if (job.type == PUT) {
        uct.insertNode(job.node);
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
                return;
            }

            uint32_t parent_hash = job.path.back().hash;

            uint32_t parentIdx = uct.getOwner(parent_hash);

            // send this one back to our parent

            maybeSteal(threadIdx, parentIdx, backprop_job);

            return;
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
            state.chance_move();

            uint32_t new_hash = state.hash();

            // Get the owner of the updated state based on the hash
            uint32_t ownerIdx = uct.getOwner(new_hash);

            Job select_job(state, SELECT, job.path);

            select_job.path.push_back(HashActionPair(hash, action->id));

            uct.stats[threadIdx].deepest_node = std::max(uct.stats[threadIdx].deepest_node, (int)select_job.path.size());

            maybeSteal(threadIdx, ownerIdx, select_job);
        }
        else {

            UCTNode node(state);
            maybeInsertNode(node, threadIdx);

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
            state.chance_move();

            float reward = rollout(state, threadIdx);

            uint32_t parent_hash = job.path.back().hash;

            uint32_t parentIdx = uct.getOwner(parent_hash);

            Job backprop_job(reward, state, BACKPROP, job.path);


            // send rollout reward to parent, who also owns the arm that got here

            maybeSteal(threadIdx, parentIdx, backprop_job);
        }
    }
    else if (job.type == BACKPROP) {

        uct.stats[threadIdx].backprop_messages++;
        UCTNode& node = uct.getNode(job.path.back().hash, threadIdx);

        float reward = job.R;

        // Undo Virtual Loss by adding R
        if constexpr (search_style == NANA) {
            float& R = node.actions[job.path.back().actionID].R;
            //int& N = node.actions[job.path.back().actionID].N;
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

            // give ourself this job
            queues[threadIdx]->enqueue(select_job, threadIdx);

            return;
        }

        bool should_backprop = true;

        if (should_backprop) {

            uint32_t parent_hash = job.path.back().hash;

            uint32_t parentIdx = uct.getOwner(parent_hash);

            // drain stashed rewards

            reward += node.R_buffer;
            node.R_buffer = 0;

            Job backprop_job(reward, job.state, BACKPROP, job.path);

            maybeSteal(threadIdx, parentIdx, backprop_job);
        }
        else {

            // stash reward and start a new rollout

            node.R_buffer += reward;

            Job select_job(root_state, SELECT);

            // give ourself this job
            queues[threadIdx]->enqueue(select_job, threadIdx);
        }
    }
}

void Search::maybeInsertNode(UCTNode node, const int threadIdx) {
    int owner = uct.getOwner(node.id);
    if (threadIdx == owner) {
        uct.insertNode(node);
    }
    else {
        Job put_job(node, PUT);
        queues[owner]->enqueue(put_job, threadIdx);
    }
}

void Search::search(const int threadIdx) {
#ifdef BENCH
    std::vector<u64> times;
#endif
    while (true) {
        if (!searching) {
            return;
        }

        // Thread waits here until something is in the queue
        Job job = queues[threadIdx]->dequeue();

#if BENCH
        struct bench{
            std::vector<u64> &times;
        
            bench(std::vector<u64>& times) : times(times) {
                times.push_back(std::chrono::steady_clock::now().time_since_epoch().count());
            }
            ~bench() {
                auto end = std::chrono::steady_clock::now();
                times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end.time_since_epoch()).count());
            }
			
        } b(times);
#endif
        if (job.type == STOP) {
            return;
        }

        processJob(threadIdx, job);
    }

    // write times to file with thread index in the name
#ifdef BENCH
    std::ofstream file("bench_" + std::to_string(threadIdx) + ".txt");
    for (int i = 0; i < times.size(); i += 2) {
		file << times[i] << " " << times[i + 1] << std::endl;
	}
#endif
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
        UCTNode node = UCTNode(state);
        move = node.select_SOR(uct.rng[threadIdx]).move;

        for (auto& action : node.actions) {
            max_eval = std::max(max_eval, action.eval);
        }

        maybeInsertNode(node, threadIdx);

        if constexpr (search_style == NANA) {
            float r = state.true_app() + max_eval / 2 + std::min(state.b2b(), (float) 2.0) / 10;
            reward = std::max(reward, r);
        }
        if constexpr (search_style == CC) {
            float r = state.true_app() + max_eval / 2 + std::min(state.b2b(), (float) 2.0) / 10;
            reward = std::max(reward, r);
        }

        if (state.opponent.garbage_height() > 15) {
            reward += state.opponent.garbage_height() / 20;
        }

        if (state.opponent.is_dead()) {
            // gottem
            reward = 1;
        }

        state.set_move(move);
        state.play_moves();
        state.chance_move();
    }

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
#ifndef TBP
            //std::cout << "N:" << action.N << " R_avg:" << action.R / action.N << std::endl;
#endif
        }
        if constexpr (search_style == CC) {
            if (action.R > biggest_R) {
                biggest_R = action.R;
                biggest_N = action.N;
                best_move = action.move;
            }

#ifndef TBP
            //std::cout << "N:" << action.N << " R_max:" << action.R << std::endl;
#endif
        }
        

    }
    if constexpr (search_style == NANA) {
#ifndef TBP
         //std::cout << "best move was visited " << biggest_N << " times, with R_avg " << biggest_R / biggest_N << std::endl;
#endif
    }
    if constexpr (search_style == CC) {
#ifndef TBP
         //std::cout << "best move was visited " << biggest_N << " times, with R_max " << biggest_R << std::endl;
#endif
    }

    return best_move;
}