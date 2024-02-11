#pragma once

#include "Search.hpp"
#include "Move.hpp"
#include <map>
#include <optional>

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

			std::mt19937 gen;
			gen.seed(std::random_device()());
			std::uniform_int_distribution<int> dis1(0, p1_moves.size());
			std::uniform_int_distribution<int> dis2(0, p1_moves.size());

			// randomly select moves
			Move p1_move = p1_moves[dis1(gen)];
			Move p2_move = p1_moves[dis2(gen)];

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
		
		if (outcome == -1) {
			// todo: substitute with eval
			avg.first++;
			avg.second += 0.5;
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

		if (n < 3) {
			// sample amount too low
			continue;
		}

		double v = r / n;
		
		if (v > best_score) {
			best_move = key;
		}
	}

	return best_move;

}