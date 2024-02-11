#pragma once

#include "Search.hpp"
#include "Move.hpp"
#include "Eval.hpp"
#include <map>
#include <optional>
#include <iostream>

double sigmoid(double x) {
	return 1 / (1 + exp(-x));
}

Move Search::monte_carlo_best_move(const VersusGame& game, int samples, int id) {
	std::map<Move, std::pair<int, double>> action_rewards;

	int o_id = (id + 1) % 2;

	for (int i = 0; i < samples; i++) {
		VersusGame sim_game = game;
		int N = 5;
		int depth = 0;

		int outcome = -1;

		Move root_move;

		while (depth < N) {

			std::vector<Move> p1_moves = game.get_moves(id);
			std::vector<Move> p2_moves = game.get_moves(o_id);

			Move p1_move;
			Move p2_move;

			// randomly select moves

			p1_move = sim_game.p1_game.get_best_piece();
			p2_move = sim_game.p2_game.get_best_piece();

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

			if (outcome != -1) {
				break;
			}

			depth++;
		}

		auto& avg = action_rewards[root_move];

		// update rewards table

		if (outcome == id) {
			avg.first++;
			avg.second += 1;
		}
		
		// game didn't end
		if (outcome == -1) {
			avg.first++;

			double e1 = Eval::eval(sim_game.p1_game.board);
			double e2 = Eval::eval(sim_game.p2_game.board);

			if (id != 0) {
				std::swap(e1, e2);
			}

			double diff = e1 / (e1 + e2);

			std::cout << diff << std::endl;

			avg.second += diff;
		}

		if (outcome == 2) {
			// draw
			avg.first++;
			avg.second += 0.5;
		}

		if (outcome == o_id) {
			// reward of loss is 0
			avg.first++;
		}
	}

	// this is a bandit model, so there exists a determinstic optimal policy. 

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

	std::cout << best_score << std::endl;

	return best_move;

}