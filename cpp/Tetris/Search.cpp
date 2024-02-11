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

Move Search::monte_carlo_best_move(const VersusGame& game, int threads, int samples, int N, int id) {
	std::map<Move, std::pair<int, double>> action_rewards;

	int samples_per_thread = samples / threads;

	auto run_this = [game, id, N, samples_per_thread](std::map<Move, std::pair<int, double>>& state) {
		for (int i = 0; i < samples_per_thread; i++) {

			int o_id = id == 0 ? 1 : 0;
			VersusGame sim_game = game;
			int depth = 0;

			Outcomes outcome = NONE;

			Move root_move;

			while (depth < N) {

				Move our_move;
				Move opp_move;

				// randomly select moves
				our_move = sim_game.get_bestish_move(id);

				opp_move = sim_game.get_bestish_move(o_id);

				if (depth == 0) {
					// p1 is us
					root_move = our_move;
				}

				sim_game.set_move(id, our_move);
				sim_game.set_move(o_id, opp_move);

				sim_game.play_moves();

				outcome = sim_game.get_winner();

				if (outcome != NONE) {
					break;
				}

				depth++;
			}

			auto& avg = state[root_move];

			// update rewards table

			// we win
			if ((outcome == P1_WIN && id == 0) || (outcome == P2_WIN && id == 1)) {
				avg.first++;
				avg.second += 1;
			}

			// game didn't end
			if (outcome == NONE) {
				avg.first++;

				double e1 = Eval::eval(sim_game.get_game(id).board);
				double e2 = Eval::eval(sim_game.get_game(o_id).board);

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

	std::vector<std::map<Move, std::pair<int, double>>> indices(threads);

	// gag console during simulations
	std::cout.setstate(std::ios_base::failbit);

	std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), run_this);

	std::cout.clear();


	for (auto const& state : indices) {
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