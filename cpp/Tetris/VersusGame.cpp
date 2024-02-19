#include "VersusGame.hpp"
#include "Move.hpp"
#include "Eval.hpp"
#include "rng.hpp"

#include <algorithm>
#include <set>

std::vector<VersusGame>  VersusGame::play_moves() const
{
	VersusGame copy;

	// game over conditions

	if (copy.game_over) {
		return { copy };
	}

	if (copy.p1_game.collides(copy.p1_game.board, copy.p1_game.current_piece)) {
		//std::cout << "game lasted: " << turn << std::endl;
		copy.game_over = true;
		return { copy };
	}

	if (copy.p2_game.collides(copy.p2_game.board, copy.p2_game.current_piece)) {
		//std::cout << "game lasted: " << turn << std::endl;
		copy.game_over = true;
		return  { copy };
	}



	if (copy.p1_move.second && copy.p2_move.second) {
		copy.p1_move.second = false;
		copy.p2_move.second = false;
	}

	copy.turn += 1;

	int p1_cleared_lines = 0;
	// player 1 move
	if (!copy.p1_move.second) {
		if (copy.p1_move.first.type != copy.p1_game.current_piece.type)
		{
			if (copy.p1_game.hold) {
				std::swap(copy.p1_game.hold.value(), copy.p1_game.current_piece);
			}
			else {
				copy.p1_game.hold = copy.p1_game.current_piece;
				// shift queue
				copy.p1_game.current_piece = copy.p1_game.queue.front();

				std::ranges::shift_left(copy.p1_game.queue, 1);

				copy.p1_game.queue.back() = PieceType::Empty;
			}
		}

		copy.p1_game.current_piece = copy.p1_move.first;
		spinType spin = copy.p1_game.current_piece.spin;

		copy.p1_game.place_piece();
		p1_cleared_lines = copy.p1_game.board.clearLines();

		bool pc = true;
		for (int i = 0; i < BOARD_WIDTH; i++) {
			if (copy.p1_game.board.get_column(i) != 0) {
				pc = false;
				break;
			}
		}

		if (spin == spinType::normal && p1_cleared_lines > 0) {
			//std::cout << "p1 did T-spin " << p1_cleared_lines << std::endl;
		}

		if (pc) {
			//std::cout << "p1 did perfect clear" << std::endl;
		}

		int dmg = copy.p1_game.damage_sent(p1_cleared_lines, spin, pc);

		copy.p1_atk += dmg;

		copy.p2_meter += dmg;
	}
	int p2_cleared_lines = 0;
	// player 2 move
	if (!copy.p2_move.second) {
		if (copy.p2_move.first.type != copy.p2_game.current_piece.type) {
			if (copy.p2_game.hold) {
				std::swap(copy.p2_game.hold.value(), copy.p2_game.current_piece);
			}
			else {
				copy.p2_game.hold = copy.p2_game.current_piece;
				// shift queue
				copy.p2_game.current_piece = copy.p2_game.queue.front();

				std::ranges::shift_left(copy.p2_game.queue, 1);

				copy.p2_game.queue.back() = PieceType::Empty;
			}
		}


		copy.p2_game.current_piece = copy.p2_move.first;
		spinType spin = copy.p2_game.current_piece.spin;

		copy.p2_game.place_piece();
		p2_cleared_lines = copy.p2_game.board.clearLines();

		bool pc = true;
		for (int i = 0; i < BOARD_WIDTH; i++) {
			if (copy.p2_game.board.get_column(i) != 0) {
				pc = false;
				break;
			}
		}

		if (spin == spinType::normal && p2_cleared_lines > 0) {
			//std::cout << "p2 did T-spin " << p2_cleared_lines << std::endl;
		}

		if (pc) {
			//std::cout << "p2 did perfect clear" << std::endl;
		}

		int dmg = copy.p2_game.damage_sent(p2_cleared_lines, spin, pc);

		copy.p2_atk += dmg;

		copy.p1_meter += dmg;
	}

	int min_meter = std::min(copy.p1_meter, copy.p2_meter);

	copy.p1_meter -= min_meter;
	copy.p2_meter -= min_meter;

	std::vector<VersusGame> out = {};


	// if possible for both players to have damage, update here
	if (!copy.p1_move.second && p1_cleared_lines == 0)
	{
		if (copy.p1_meter > 0)
		{
			out.reserve(10);
			auto games = copy.p1_game.add_garbage(copy.p1_meter);
			copy.p1_meter = 0;

			for (auto& game : games)
			{
				copy.p1_game = game;
				out.emplace_back(copy);
			}
		}
	}

	if (!copy.p2_move.second && p2_cleared_lines == 0)
	{
		if (copy.p2_meter > 0)
		{
			out.reserve(10);
			auto games = copy.p2_game.add_garbage(copy.p2_meter);
			copy.p2_meter = 0;

			for (auto& game : games)
			{
				copy.p2_game = game;
				out.emplace_back(copy);
			}
		}
	}


	if (out.size() == 0)
	{
		out.emplace_back(copy);
	}
	return out;
}

std::vector<Move> VersusGame::get_moves(int id) const
{
	std::vector<Move> moves;

	const Game& player = id == 0 ? p1_game : p2_game;

	auto movegen_pieces = player.movegen(player.current_piece.type);

	PieceType hold = player.hold.has_value() ? player.hold.value().type : player.queue.front();

	std::vector<Piece> hold_pieces;
	if(hold != PieceType::Empty)
		hold_pieces = player.movegen(hold);

	moves.reserve((movegen_pieces.size() + hold_pieces.size()) * 2);

	for (auto& piece : movegen_pieces) {
		moves.emplace_back(piece, false);
		moves.emplace_back(piece, true);
	}

	for (auto& piece : hold_pieces) {
		moves.emplace_back(piece, false);
		moves.emplace_back(piece, true);
	}

	return moves;
}

std::vector<Move> VersusGame::get_N_moves(int id, int N) const
{
	const Game& player = id == 0 ? p1_game : p2_game;
	std::vector<Piece> valid_pieces = player.movegen(player.current_piece.type);
	PieceType holdType = player.hold.has_value() ? player.hold->type : player.queue.front();

	std::vector<Piece> hold_pieces = player.movegen(holdType);
	valid_pieces.reserve(valid_pieces.size() + hold_pieces.size());
	for (auto& piece : hold_pieces)
	{
		valid_pieces.emplace_back(piece);
	}

	std::vector<std::optional<Piece>> moves;

	for (auto& piece : valid_pieces)
	{
		Board temp_board = player.board;
		temp_board.set(piece);
		moves.emplace_back(piece);
	}

	// try the null move
	{
		//moves.emplace_back(std::nullopt);
	}

	std::vector<Move> out;
	out.reserve(N);

	// make sure not to sample the same move twice
	pptRNG rng;

	std::set<int> sampled_indices;

	for (int i = 0; i < N; i++)
	{
		int index = rng.GetRand(moves.size());
		while (sampled_indices.find(index) != sampled_indices.end())
		{
			index = rng.GetRand(moves.size());
		}

		sampled_indices.insert(index);

		out.emplace_back(moves[index]);
	}

	return out;
}


Outcomes VersusGame::get_winner() const
{

	Outcomes out = Outcomes::NONE;

	if (p1_game.collides(p1_game.board, p1_game.current_piece)) {
		out = Outcomes::P2_WIN;
	}

	if (p2_game.collides(p2_game.board, p2_game.current_piece)) {
		if (out == Outcomes::P2_WIN) {
			out = Outcomes::DRAW;
		}
		else {
			out = Outcomes::P1_WIN;
		}
	}

	return out;
}

void VersusGame::set_move(int id, Move move)
{
	if (id == 0) {
		p1_move = move.as_pair();
	}
	else {
		p2_move = move.as_pair();
	}
}



Move VersusGame::get_bestish_move(int id) const
{
	if (id == 0) {
		return p1_game.get_bestish_piece();
	}
	return p2_game.get_bestish_piece();
}


Move VersusGame::get_best_move(int id) const
{
	if (id == 0) {
		return p1_game.get_best_piece();
	}
	return p2_game.get_best_piece();
}

std::vector<Move> VersusGame::get_sorted_moves(int id) const
{
	if (id == 0) {
		return p1_game.get_sorted_moves();
	}
	return p2_game.get_sorted_moves();
}

Move VersusGame::something(int N, int id) const
{
	int other_id = id == 0 ? 1 : 0;

	const Game& player = id == 0 ? p1_game : p2_game;
	const Game& opponent = id == 0 ? p2_game : p1_game;

	const std::vector<Move> moves = this->get_moves(id);
	const std::vector<Move> moves2 = this->get_moves(other_id);

	std::vector<std::vector<VersusGame>> games;
	std::vector<std::vector<VersusGame>> next_games;

	games.resize(moves.size());
	next_games.resize(moves.size());

	for (int i = 0; i < moves.size(); ++i) {
		games[i].reserve(moves2.size());
		for (int j = 0; j < moves2.size(); ++j)
		{
			VersusGame temp = *this;
			temp.set_move(other_id, moves2[j]);
			temp.set_move(id, moves[i]);
			auto played_games = temp.play_moves();
			for (const auto& played_game : played_games)
			{
				games[i].emplace_back(played_game);
			}
		}
	}

	for (int n = 0; n < N; n++)
	{
		for (int i = 0; i < games.size(); ++i)
		{
			const auto& bucket = games[i];
			for (const auto& game : bucket)
			{
				if (game.game_over)
				{
					next_games[i].emplace_back(game);
					continue;
				}
				const std::vector<Move> our_moves = game.get_moves(id);
				const std::vector<Move> opponent_moves = game.get_moves(other_id);
				for (const auto& our_move : our_moves)
				{
					for (const auto& opponent_move : opponent_moves)
					{
						VersusGame temp = game;
						temp.set_move(id, our_move);
						temp.set_move(other_id, opponent_move);
						auto played_games = temp.play_moves();
						for (const auto& played_game : played_games)
						{
							next_games[i].emplace_back(played_game);
						}
					}
				}
			}
		}

		games = next_games;
		next_games.clear();
	}

	std::vector<int> move_scores;

	for (int i = 0; i < moves.size(); i++)
	{
		int score = 0;
		for (const auto& game : games[i])
		{
			const Outcomes WIN = id == 0 ? Outcomes::P1_WIN : Outcomes::P2_WIN;
			const Outcomes LOSE = id == 0 ? Outcomes::P2_WIN : Outcomes::P1_WIN;
			const Outcomes outcome = game.get_winner();
			if (outcome == WIN)
			{
				score += 1;
			}
			else if (outcome == LOSE)
			{
				score -= 1;
			}
			else {
				Board board = id == 0 ? game.p1_game.board : game.p2_game.board;
				Board other_board = id == 0 ? game.p2_game.board : game.p1_game.board;
				double our_eval = Eval::eval(board);
				double their_eval = Eval::eval(other_board);

				score += (our_eval - their_eval) / (our_eval + their_eval);
			}
		}
		move_scores.emplace_back(score);
	}

	int max_index = 0;
	int max_score = move_scores[0];

	for (int i = 1; i < move_scores.size(); i++)
	{
		if (move_scores[i] > max_score)
		{
			max_score = move_scores[i];
			max_index = i;
		}
	}

	return moves[max_index];
}
