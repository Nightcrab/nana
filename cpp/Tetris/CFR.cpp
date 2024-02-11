#include "CFR.hpp"


void cfr()
{
	int n = 10;

	int iterations = 0;

	for (int i = 0; i < n; i++)
	{
		// play game according to regrets
		// get counterfactual regrets at every visited infostate
		// add these regrets


		// sample values
		// update regrets
		// update strategy
		iterations++;
	}

	// return strategy
}

void CFR::sample_values(VersusGame game)
{
	// estimate counterfactual values in the tree

	const int n_games = 5; // depth of the tree
	int games_played = 0;

	std::vector<InfoState> info_states;

	while (true) {
		// play moves acocrding to regrets
		game.p1_move = game.p1_game.get_best_piece();
		game.p2_move = game.p2_game.get_best_piece();

		game.play_moves();

		if (game.p1_game.collides(game.p1_game.board, game.p1_game.current_piece) ||
			game.p2_game.collides(game.p2_game.board, game.p2_game.current_piece))
		{
			break;
		}

		if (games_played == n_games)
			break;
	}

}
