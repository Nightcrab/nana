#pragma once
#include "Game.hpp"
#include "Piece.hpp"
#include "Move.hpp"

#include <optional>
#include <vector>

class Tetris;

enum Outcomes {
	P1_WIN,
	P2_WIN,
	DRAW,
	NONE
};

class VersusGame
{
public:
	Game p1_game;
	Game p2_game;

	int turn = 0;
	bool game_over = false;

	int p1_meter = 0;
	int p2_meter = 0;

	double p1_atk = 0;
	double p2_atk = 0;

	double get_app(int id) {
		return id == 0 ? p1_atk / turn : p2_atk / turn;
	}

	double get_b2b(int id) {
		return id == 0 ? p1_game.b2b : p2_game.b2b;
	}

	std::pair<Piece, bool> p1_move = std::make_pair(Piece(PieceType::Empty), false);
	std::pair<Piece, bool> p2_move = std::make_pair(Piece(PieceType::Empty), false);

	void set_move(int id, Move move);

	void play_moves();

	std::vector<Move> get_moves(int id) const;
	std::vector<Move> get_N_moves(int id, int N) const;

	Outcomes get_winner() const;


	Move get_bestish_move(int id) const; 
	Move get_best_move(int id) const;

	friend class Tetris;
};