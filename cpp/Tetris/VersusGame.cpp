#include "VersusGame.hpp"

#include <iostream>

void VersusGame::play_moves()
{

	// game over condition
	if (p1_game.collides(p1_game.board, p1_game.current_piece)) {
		return;
	}

	if (p2_game.collides(p2_game.board, p2_game.current_piece)) {
		return;
	}

	if (p1_move.second && p2_move.second) {
		p1_move.second = false; 
		p2_move.second = false;
	}

	int p1_cleared_lines = 0;
	// player 1 move
	if (!p1_move.second) {
		if (p1_move.first.type != p1_game.current_piece.type)
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

		p2_meter += p1_game.damage_sent(p1_cleared_lines, spin, pc);
	}
	int p2_cleared_lines = 0;
	// player 2 move
	if (!p2_move.second) {
		if (p2_move.first.type != p2_game.current_piece.type)
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

		p1_meter += p2_game.damage_sent(p2_cleared_lines, spin, pc);
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

std::vector<std::optional<Piece>> VersusGame::get_moves(int id)
{
	std::vector<std::optional<Piece>> moves = { std::nullopt };

	Game& player = id == 0 ? p1_game : p2_game;

	auto movegen_moves = player.movegen(player.current_piece.type);

	PieceType hold = player.hold.has_value() ? player.hold.value().type : player.queue.front();
	std::vector<Piece> hold_moves = player.movegen(hold);

	moves.reserve(movegen_moves.size() + hold_moves.size() + 1);

	for (auto& move : movegen_moves) {
		moves.emplace_back(move);
	}

	for (auto& move : hold_moves) {
		moves.emplace_back(move);
	}

	return moves;
}
