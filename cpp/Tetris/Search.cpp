#include "Search.hpp"
#include <numeric>
#include <execution>


void Search::startSearch(EmulationGame state, int core_count) {
	searching = true;

	Search::core_count = core_count;

	// Create new search tree
	uct = UCT(core_count);

	// Initialise worker queues
	for (int i = 0; i < core_count; i++) {
		queues[i] = new zib::wait_mpsc_queue<Job>(core_count);
	}

	// Thread indices
	std::vector<int> indices(core_count);

	std::iota(indices.begin(), indices.end(), 0);

	// Spawn worker threads
	std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), search);
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
			}
			else {
				// We handle movegen to create the new node
				std::vector<Piece> raw_actions = state.game.get_possible_piece_placements();

				std::vector<Action> actions;
				actions.reserve(raw_actions.size());

				for (auto& raw_action : raw_actions) {
					actions.push_back(Action(Move(raw_action, true)));
					actions.push_back(Action(Move(raw_action, false)));
				}

				// Virtual Loss by setting node_N := 1
				UCTNode node = UCTNode(actions, hash, 1);

				uct.insertNode(hash, node);

				float reward = rollout(state);

				int parent_hash = job.path.top().hash;

				int parentIdx = uct.getOwner(parent_hash);

				job.path.pop();

				Job backprop_job = Job(reward, state, BACKPROP, job.path);

				queues[parentIdx]->enqueue(backprop_job, threadIdx);
			}
		}
		else if (job.type == BACKPROP) {
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


float Search::rollout(EmulationGame state) {
	return 0.0;
}


Move Search::bestMove() {
	return Move();
}