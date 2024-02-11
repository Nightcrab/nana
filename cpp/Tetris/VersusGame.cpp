#include "VersusGame.hpp"
#include "Move.hpp"
#include "Eval.hpp"

#include <iostream>
#include <ranges>
#include <set>

void VersusGame::play_moves()
{
	// game over conditions

	if (game_over) {
		return;
	}

	if (p1_game.collides(p1_game.board, p1_game.current_piece)) {
		//std::cout << "game lasted: " << turn << std::endl;
		game_over = true;
		return;
	}

	if (p2_game.collides(p2_game.board, p2_game.current_piece)) {
		//std::cout << "game lasted: " << turn << std::endl;
		game_over = true;
		return;
	}



	if (p1_move.second && p2_move.second) {
		p1_move.second = false;
		p2_move.second = false;
	}

	turn += 1;

	int p1_cleared_lines = 0;
	// player 1 move
	if (!p1_move.second) {
		if (p1_move.first.type != p1_game.current_piece.type)
		{
			if (p1_game.hold) {
				std::swap(p1_game.hold.value(), p1_game.current_piece);
			}
			else {
				p1_game.hold = p1_game.current_piece;
				// shift queue
				p1_game.current_piece = p1_game.queue.front();

				std::ranges::shift_left(p1_game.queue, 1);

				p1_game.queue.back() = p1_game.rng.getPiece();
			}
		}

		p1_game.current_piece = p1_move.first;
		spinType spin = p1_game.current_piece.spin;

		p1_game.place_piece();
		p1_cleared_lines = p1_game.board.clearLines();

		bool pc = true;
		for (int i = 0; i < BOARD_WIDTH; i++) {
			if (p1_game.board.get_column(i) != 0) {
				pc = false;
				break;
			}
		}

		if (spin == spinType::normal && p1_cleared_lines > 0) {
			std::cout << "p1 did T-spin " << p1_cleared_lines << std::endl;
		}

		if (pc) {
			std::cout << "p1 did perfect clear" << std::endl;
		}

		int dmg = p1_game.damage_sent(p1_cleared_lines, spin, pc);

		p1_atk += dmg;

		p2_meter += dmg;
	}
	int p2_cleared_lines = 0;
	// player 2 move
	if (!p2_move.second) {
		if (p2_move.first.type != p2_game.current_piece.type) {
			if (p2_game.hold) {
				std::swap(p2_game.hold.value(), p2_game.current_piece);
			}
			else {
				p2_game.hold = p2_game.current_piece;
				// shift queue
				p2_game.current_piece = p2_game.queue.front();

				std::ranges::shift_left(p2_game.queue, 1);

				p2_game.queue.back() = p2_game.rng.getPiece();
			}
		}


		p2_game.current_piece = p2_move.first;
		spinType spin = p2_game.current_piece.spin;

		p2_game.place_piece();
		p2_cleared_lines = p2_game.board.clearLines();

		bool pc = true;
		for (int i = 0; i < BOARD_WIDTH; i++) {
			if (p2_game.board.get_column(i) != 0) {
				pc = false;
				break;
			}
		}

		if (spin == spinType::normal && p2_cleared_lines > 0) {
			std::cout << "p2 did T-spin " << p2_cleared_lines << std::endl;
		}

		if (pc) {
			std::cout << "p2 did perfect clear" << std::endl;
		}

		int dmg = p2_game.damage_sent(p2_cleared_lines, spin, pc);

		p2_atk += dmg;

		p1_meter += dmg;
	}
	pptRNG rng;

	int min_meter = std::min(p1_meter, p2_meter);

	p1_meter -= min_meter;
	p2_meter -= min_meter;

	if (!p1_move.second && p1_cleared_lines == 0)
	{
		p1_game.add_garbage(p1_meter, rng.GetRand(BOARD_WIDTH));
		p1_meter = 0;
	}

	if (!p2_move.second && p2_cleared_lines == 0)
	{
		p2_game.add_garbage(p2_meter, rng.GetRand(BOARD_WIDTH));
		p2_meter = 0;
	}

}

std::vector<Move> VersusGame::get_moves(int id) const
{
	std::vector<Move> moves;

	const Game& player = id == 0 ? p1_game : p2_game;

	auto movegen_pieces = player.movegen(player.current_piece.type);

	PieceType hold = player.hold.has_value() ? player.hold.value().type : player.queue.front();
	std::vector<Piece> hold_pieces = player.movegen(hold);

	moves.reserve((movegen_pieces.size() + hold_pieces.size()) * 2);

	for (auto& piece : movegen_pieces) {
		moves.emplace_back(Move(piece, false));
		moves.emplace_back(Move(piece, true));
	}

	for (auto& piece : hold_pieces) {
		moves.emplace_back(Move(piece, false));
		moves.emplace_back(Move(piece, true));
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

