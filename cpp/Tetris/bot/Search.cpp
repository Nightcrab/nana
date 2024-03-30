#include "Search.hpp"

#include <execution>
#include <numeric>
#include <ranges>

#include "Distribution.hpp"
#include "Eval.hpp"

void Search::startSearch(EmulationGame state, int core_count) {
    searching = true;

    Search::core_count = core_count;

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

    // Spawn worker threads
    std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), search);

    int rootOwnerIdx = uct.getOwner(state.hash());

    // Spawn 3*N jobs
    for (int i = 0; i < 3 * core_count; i++) {
        Job select_job = Job(root_state, SELECT);

        queues[rootOwnerIdx]->enqueue(select_job, core_count);
    }
};

void Search::endSearch() {
    searching = false;
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
            EmulationGame state = job.state;

            int hash = state.hash();

            if (uct.nodeExists(hash)) {
                UCTNode node = uct.getNode(hash);
                Action action = node.select();

                // Virtual Loss by setting N := N+1
                node.N += 1;

                // todo: pass actions by reference so we don't have to do this
                node.actions[action.id].N += 1;

                state.set_move(action.move);

                int new_hash = state.hash();

                // Get the owner of the updated state based on the hash
                int ownerIdx = uct.getOwner(new_hash);

                job.path.push(HashActionPair(hash, action));

                Job select_job = Job(0.0, state, SELECT, job.path);

                queues[ownerIdx]->enqueue(select_job, threadIdx);
            } else {
                // Virtual Loss handled automatically by constructor
                UCTNode node = UCTNode(job.state);

                uct.insertNode(node);

                float reward = rollout(state, threadIdx);

                int parent_hash = job.path.top().hash;

                int parentIdx = uct.getOwner(parent_hash);

                job.path.pop();

                Job backprop_job = Job(reward, state, BACKPROP, job.path);

                queues[parentIdx]->enqueue(backprop_job, threadIdx);
            }
        } else if (job.type == BACKPROP) {
            if (job.path.empty()) {
                // start a new search iteration
                Job select_job = Job(root_state, SELECT, job.path);

                queues[threadIdx]->enqueue(select_job, threadIdx);

                continue;
            }

            UCTNode node = uct.getNode(job.state.hash());

            float reward = job.R;

            // Undo Virtual Loss by adding R
            node.actions[job.path.top().action.id].R += reward;

            int parent_hash = job.path.top().hash;

            int parentIdx = uct.getOwner(parent_hash);

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
        // todo: create a UCTNode and take legal moves from that
        std::vector<Move> moves = state.legal_moves();

        std::vector<Stochastic<Move>> policy;

        policy.reserve(moves.size());

        for (auto& move : moves) {
            // raw scores
            policy.push_back(Stochastic<Move>(move, Eval::eval_CC(state.game, move)));
        }

        std::ranges::sort(policy, [](const Stochastic<Move>& a, const Stochastic<Move>& b) {
            return a.probability > b.probability;
        });

        // square of rank

        int r = 1;

        for (int i = policy.size() - 1; i >= 0; i--) {
            policy[i].probability = r * r;
            r++;
        }

        Distribution::normalise(policy);

        Move sample = Distribution::sample(policy, uct.rng[threadIdx]);

        state.set_move(sample);
        state.chance_move();
        state.play_moves();

        reward += Eval::eval_CC(state.game.board);
    }

    return reward;
}

Move Search::bestMove() {
    return Move();
}