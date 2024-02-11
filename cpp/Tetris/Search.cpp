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

				Move p1_move;
				Move p2_move;

				// randomly select moves
				p1_move = sim_game.get_N_moves(id, 1)[0];
				p2_move = sim_game.get_N_moves(o_id, 1)[0];

				if (id == 1) {
					std::swap(p1_move, p2_move);
				}


				if (depth == 0) {
					// p1 is us
					root_move = p1_move;
				}

				sim_game.p1_move = std::make_pair(p1_move.piece, p1_move.null_move);
				sim_game.p2_move = std::make_pair(p1_move.piece, p1_move.null_move);
				sim_game.play_moves();

				outcome = sim_game.get_winner();

				if (outcome != NONE) {
					break;
				}

				depth++;
			}

			auto& avg = state[root_move];

			// update rewards table

			//avg.first++;
			//avg.second += id == 0 ? Eval::eval(sim_game.p1_game.board) : Eval::eval(sim_game.p2_game.board);
			//continue;

			if ((outcome == P1_WIN && id == 0) || (outcome == P2_WIN && id == 1)) {
				avg.first++;
				avg.second += 1;
			}

			// game didn't end
			if (outcome == NONE) {
				avg.first++;

				double e1 = Eval::eval(sim_game.p1_game.board);
				double e2 = Eval::eval(sim_game.p2_game.board);

				if (id != 0) {
					std::swap(e1, e2);
				}

				double diff = e1 / (e1 + e2);


				avg.second += diff;
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

	std::vector<std::map<Move, std::pair<int, double>>> indices(samples);

	// this is a bandit model, so there exists a determinstic optimal policy. 
	std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), run_this);


	for (auto const& state : indices) {
		for (auto const& [key, val] : state) {
			auto& avg = action_rewards[key];
			avg.first += val.first;
			avg.second += val.second;
		}
	}


	Move best_move;

	double best_score = 0;

	for (auto const& [key, val] : action_rewards)
	{
		int n = val.first;
		double r = val.second;


		if (n < 0) {
			// sample amount too low
			continue;
		}

		double v = r / n;

		if (v >= best_score) {
			best_move = key;
			best_score = v;
		}
	}


	return best_move;

}