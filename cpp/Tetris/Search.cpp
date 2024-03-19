#include "Search.hpp"
#include "Move.hpp"
#include "Eval.hpp"
#include <map>
#include <optional>
#include <iostream>
#include <algorithm>
#include <execution>

double sigmoid(double x) {
	return 1 / (1 + exp(-x));
}

Move Search::monte_carlo_best_move(const VersusGame& game, int threads, int samples, int max_depth, int id) {

	using action_map_t = std::map<Move, std::pair<int, double>>;
	using state_t = std::pair<action_map_t, Move>;
	action_map_t action_rewards;

	int samples_per_thread = samples / threads;

	auto run_this = [game, id, max_depth, samples_per_thread](state_t& state) {
		
		for (int i = 0; i < samples_per_thread; i++) {

			int o_id = id == 0 ? 1 : 0;
			VersusGame sim_game = game;
			int cur_depth = 0;

			Outcomes outcome = NONE;

			Move root_move = state.second;

			while (cur_depth < max_depth) {

				// randomly select moves
				Move our_move = sim_game.get_bestish_move(id);
				Move opp_move = sim_game.get_bestish_move(o_id);

				if (cur_depth == 0) {
					// p1 is us
					our_move = root_move;
				}

				sim_game.set_move(id, our_move);
				sim_game.set_move(o_id, opp_move);

				sim_game.play_moves();

				outcome = sim_game.get_winner();

				if (outcome != NONE) {
					break;
				}

				cur_depth++;
			}

			auto& avg = state.first[root_move];

			// update rewards table

			// we win
			if ((outcome == P1_WIN && id == 0) || (outcome == P2_WIN && id == 1)) {
				avg.first++;
				avg.second += 1;
			}

			// game didn't end
			if (outcome == NONE) {
				avg.first++;

				double e1 = Eval::eval_LUT(sim_game.get_game(id).board);
				double e2 = Eval::eval_LUT(sim_game.get_game(o_id).board);

				double app = sim_game.get_app(id);

				double b2b = std::min(sim_game.get_b2b(id), 2.0) / 8.0;

				double diff = e1 / (e1 + e2);

				avg.second += diff * 0.4 + app * 0.575 + b2b * 0.025;
			}

			if (outcome == DRAW) {
				// draw
				avg.first++;
				avg.second += 0.5;
			}

			if ((outcome == P1_WIN && id == 1) || (outcome == P2_WIN && id == 0)) {
				// reward of loss is 0
				avg.first++;
			}
		}
	};
	
	std::vector<state_t> indices(threads);
	const Game &player = game.get_game(id);
	std::vector<Move> moves = player.get_sorted_moves();

	for (int i = 0; i < indices.size(); ++i)
	{
		if (!(i < moves.size()))
		{
			indices.resize(i);
			break;
		}
		indices[i].second = moves[i];

	}

	// gag console during simulations
	std::cout.setstate(std::ios_base::failbit);



	std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), run_this);

	std::cout.clear();


	for (auto const& [state, move] : indices) {
		for (auto const& [key, val] : state) {
			auto& avg = action_rewards[key];

			avg.first += val.first;
			avg.second += val.second;
		}
	}

	// this is a bandit model, so there exists a determinstic optimal policy. 
	Move best_move;

	double best_score = 0;

	for (auto const& [key, val] : action_rewards)
	{
		int n = val.first;
		double r = val.second;


		if (n < 2) {
			// sample amount too low
			continue;
		}

		double v = r / n;

		if (v >= best_score) {
			best_move = key;
			best_score = v;
		}
	}

	//std::cout << "number of moves tried " << action_rewards.size() << std::endl;

	return best_move;

}