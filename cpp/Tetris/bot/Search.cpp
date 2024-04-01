#include "Search.hpp"

#include <execution>
#include <numeric>
#include <ranges>

#include "Distribution.hpp"
#include "Eval.hpp"
#include <iostream>

std::atomic_bool Search::searching = false;

SearchType Search::search_style = NANA;

int Search::core_count = 4;

int Search::monte_carlo_depth = 1;

UCT Search::uct = UCT(4);

EmulationGame Search::root_state;

zib::wait_mpsc_queue<Job>* Search::queues[256];

void Search::startSearch(EmulationGame state, int core_count) {

    std::cout << "started searching, root hash is: " <<  (int) (state.hash() % 1000) << std::endl;

    searching = true;

    Search::core_count = core_count;

    root_state = state;

    // Create new search tree
    uct = UCT(core_count);

    // Create root node
    uct.insertNode(UCTNode(state));

    // Initialise worker queues
    for (int i = 0; i < core_count; i++) {
        queues[i] = new zib::wait_mpsc_queue<Job>(core_count + 1);
    }

    // Thread indices
    std::vector<int> indices(core_count);

    std::iota(indices.begin(), indices.end(), 0);

    int rootOwnerIdx = uct.getOwner(state.hash());

    // Spawn jobs
    for (int i = 0; i < 6 * core_count; i++) {
        Job select_job = Job(EmulationGame(root_state), SELECT);

        queues[rootOwnerIdx]->enqueue(select_job, core_count);
    }

    // Spawn worker threads
    for (auto& idx : indices) {
        std::thread t(search, idx);
        t.detach();
    }
};

void Search::endSearch() {
    searching = false;

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
        if (job.type == SELECT) {

            EmulationGame& state = job.state;

            uint32_t hash = state.hash();

            //std::cout << "SELECT" <<  std::endl;

            //std::cout << "state hash: " << hash % 1000 << std::endl;


            if (state.game_over) {
                //std::cout << "Game Over" << std::endl;
                float reward = rollout(state, threadIdx);

                Job backprop_job = Job(reward, state, BACKPROP, job.path);

                uint32_t parent_hash = job.path.top().hash;

                uint32_t parentIdx = uct.getOwner(parent_hash);

                // send this one back to our parent

                queues[parentIdx]->enqueue(backprop_job, threadIdx);

                continue;

            } 
            if (uct.nodeExists(hash)) {
                //std::cout << "Node Exists" << std::endl;

                UCTNode& node = uct.getNode(hash);
                Action& action = node.actions[0];
                if (search_style == NANA) {

                    action = node.select();
                }
                if (search_style == CC) {

                    action = node.select_SOR(uct.rng[threadIdx]);
                }

                // Virtual Loss by setting N := N+1
                node.N += 1;

                action.N += 1;

                //std::cout << "selected action #" << action.id << std::endl;

                //std::cout << "action.N: " << action.N << std::endl;

                state.set_move(action.move);

                state.play_moves();

                uint32_t new_hash = state.hash();

                //std::cout << "newstate hash: " << new_hash % 1000 << std::endl;

                // Get the owner of the updated state based on the hash
                uint32_t ownerIdx = uct.getOwner(new_hash);

                job.path.push(HashActionPair(hash, action.id));

                Job select_job = Job(0.0, state, SELECT, job.path);

                queues[ownerIdx]->enqueue(select_job, threadIdx);
            } else {
                //std::cout << "Creating Node" << std::endl;
                UCTNode node = UCTNode(job.state);

                uct.insertNode(node);

                Action& action = node.select();

                node.N += 1;
                action.N += 1;

                state.set_move(action.move);
                state.play_moves();

                float reward = rollout(state, threadIdx);

                uint32_t parent_hash = job.path.top().hash;

                uint32_t parentIdx = uct.getOwner(parent_hash);

                Job backprop_job = Job(reward, state, BACKPROP, job.path);

                job.path.push(HashActionPair(hash, action.id));

                // send rollout reward to parent, who also owns the arm that got here

                queues[parentIdx]->enqueue(backprop_job, threadIdx);
            }
        } else if (job.type == BACKPROP) {
            //std::cout << "BACKPROP" << std::endl;
            if (job.path.empty()) {
                // start a new search iteration
                Job select_job = Job(root_state, SELECT, job.path);

                queues[threadIdx]->enqueue(select_job, threadIdx);

                continue;
            }

            UCTNode& node = uct.getNode(job.path.top().hash);

            float reward = job.R;

            // Undo Virtual Loss by adding R
            if (search_style == NANA) {
                node.actions[job.path.top().actionID].R += reward;
            }
            if (search_style == CC) {
                node.actions[job.path.top().actionID].R = std::max(reward, node.actions[job.path.top().actionID].R);
            }

            uint32_t parent_hash = job.path.top().hash;

            uint32_t parentIdx = uct.getOwner(parent_hash);

            job.path.pop();

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

        // rather than eval, the expectation of eval is more stable and basically free,
        // since we already computed eval for all possible boards
        if (search_style == NANA) {
            reward += Distribution::expectation(cc_dist);
        }
        if (search_style == CC) {
            reward = std::max(reward, Distribution::max_value(cc_dist));
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
    int biggest_R = 0;
    Move best_move;

    for (Action& action : uct.getNode(root_state.hash()).actions) {
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

    }

    std::cout << "best move was visited " << biggest_N << " times" << std::endl;

    return best_move;
}