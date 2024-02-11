#pragma once
#include "Game.hpp"
#include "Piece.hpp"
#include "Move.hpp"

#include <optional>
#include <vector>

class Tetris;

class VersusGame
{
public:
	Game p1_game;
	Game p2_game;
	

	int p1_meter = 0;
	int p2_meter = 0;

	std::pair<Piece, bool> p1_move = std::make_pair(Piece(PieceType::Empty), false);
	std::pair<Piece, bool> p2_move = std::make_pair(Piece(PieceType::Empty), false);

	void play_moves();
	std::vector<Move> get_moves(int id) const;
	std::vector<Move> get_N_moves(int id, int N) const;

	int get_winner() const;

	friend class Tetris;
};