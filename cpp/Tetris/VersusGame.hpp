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


	std::pair<Piece, bool> p1_move = std::make_pair(Piece(PieceType::Empty), false);
	std::pair<Piece, bool> p2_move = std::make_pair(Piece(PieceType::Empty), false);

	double p1_atk = 0;
	double p2_atk = 0;
	
	int p1_meter = 0;
	int p2_meter = 0;

	int turn = 0;
	bool game_over = false;

	// get attack per piece
	double get_app(int id) const {
		return id == 0 ? p1_atk / turn : p2_atk / turn;
	}

	double get_b2b(int id) const {
		return id == 0 ? p1_game.b2b : p2_game.b2b;
	}

	const Game& get_game(int id) const {
		return id == 0 ? p1_game : p2_game;
	}


	void set_move(int id, Move move);

	std::vector<VersusGame> play_moves()const;

	std::vector<Move> get_moves(int id) const;
	std::vector<Move> get_N_moves(int id, int N) const;

	Outcomes get_winner() const;


	Move get_bestish_move(int id) const; 
	Move get_best_move(int id) const;
	std::vector<Move> get_sorted_moves(int id) const;
	Move something(int N, int id) const;
	friend class Tetris;
};